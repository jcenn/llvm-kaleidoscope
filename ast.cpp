//
// Created by jcen on 4.04.2026.
//

#include "ast.h"

#include "Parser.h"
#include "llvm/IR/Verifier.h"

#include <iostream>

// The NamedValues map keeps track of which values are defined in the current scope and what their LLVM representation is. (In other words, it is a symbol table for the code).
std::map<std::string, llvm::Value *> NamedValues;

llvm::Type* get_llvm_type(const TypeIdentifier type_ident)
{
    switch (type_ident)
    {
        case TypeIdentifier::I32:
            return llvm::Type::getInt32Ty(*Context);
        case TypeIdentifier::VOID:
            return llvm::Type::getVoidTy(*Context);
        case TypeIdentifier::Double:
            return llvm::Type::getDoubleTy(*Context);
        default:
            throw std::logic_error("Invalid type identifier");
    }
}

llvm::Function * PrototypeAST::codegen() {

    std::vector<llvm::Type*> arg_types{};
    for (auto& arg : this->args) {
        arg_types.push_back(get_llvm_type(arg.second));
    }
    llvm::Type* return_type = get_llvm_type(this->ret_type);
    llvm::FunctionType* ft = llvm::FunctionType::get(return_type, arg_types, false);
    llvm::Function* fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, identifier, TheModule.get());

    size_t idx = 0;
    // sets names for arguments. Mostly used for debugging IR
    for (auto& arg : fn->args()) {
        arg.setName(this->args[idx++].first);
    }
    return fn;
}


llvm::Value * FunctionAST::codegen() {

    // handle function prototype
    this->prototype->codegen();

    // We check if function already exists (extern)
    llvm::Function* fn = TheModule->getFunction(this->prototype->identifier);

    // If function wasn't imported via extern we generate the prototype
    if (!fn) {
        fn = this->prototype->codegen();
    }

    if (!fn) throw std::runtime_error("Something went wrong while generating function " + prototype->identifier);

    // function already has a body -> we're trying to redefine it
    if (!fn->empty()) throw std::runtime_error("Tried to redefine function " + prototype->identifier);

    // Create actual function body block
    llvm::BasicBlock* block = llvm::BasicBlock::Create(*Context, "entry", fn);
    Builder->SetInsertPoint(block);
    NamedValues.clear();
    for (auto& arg : fn->args()) {
        NamedValues[std::string(arg.getName())] = &arg;
    }

    // TODO:
    //codegen function body
    // get return value
    for (auto& statement : statements) {
        statement->codegen();
    }
    auto error = llvm::verifyFunction(*fn, &llvm::errs());
    if (error) {
        throw std::runtime_error("Failed to verify function " + prototype->identifier);
    }

    return fn;

    // fn->eraseFromParent();
    // throw std::runtime_error("Error while generating IR for body of function " + this->prototype->identifier);
    // return nullptr;

}

llvm::Value * CallStatementAST::codegen() {
    auto expr = this->call_expression->codegen();
    // if (!expr) {
    //     throw std::runtime_error("Failed to generate call statement");
    // }
    // Call statemetns are not expected to return values so we return nullptr here instead of expression result
    return nullptr;
}

llvm::Value * LetStatementAST::codegen() {
    // Add an alloca statement to the current block/function
    // Add pointer from that alloca variable symbol table
    // TODO: handle pointers in variableExpressions

    // store provided value under var pointer

    // might be useful if we want to implement an optimization with placing all allocas at the beginning of the function's block
    // auto fn = Builder->GetInsertBlock()->getParent();
    llvm::AllocaInst* alloca_ptr = Builder->CreateAlloca(
        get_llvm_type(this->type_hint),
        nullptr, // array size, not used here
        this->variable_identifier  // name for easier debugging
    );

    // TODO: this can override previous values (shadowing) should handle that
    // maybe implement function scoped symbol tables
    NamedValues[this->variable_identifier] = alloca_ptr;

    auto expr_value = this->expression->codegen();
    Builder->CreateStore(expr_value, alloca_ptr);

    return nullptr;
}

llvm::Value* ForStatementAST::codegen()
{
    auto fn = Builder->GetInsertBlock()->getParent();

    auto header_block = llvm::BasicBlock::Create(*Context, "for_header", fn);
    auto loop_block = llvm::BasicBlock::Create(*Context, "loop");
    auto after_block = llvm::BasicBlock::Create(*Context, "after");

    // jump from original block to the start of our for loop
    Builder->CreateBr(header_block);
    Builder->SetInsertPoint(header_block);

    auto condition = this->cond_expression->codegen();
    Builder->CreateCondBr(condition, loop_block, after_block);

    fn->insert(fn->end(), loop_block);
    Builder->SetInsertPoint(loop_block);
    // TODO: handle termination inside loop block
    for (auto& statement : this->statements)
    {
        statement->codegen();
    }

    Builder->CreateBr(header_block);

    fn->insert(fn->end(), after_block);
    Builder->SetInsertPoint(after_block);

    return nullptr;
}


llvm::Value * ReturnStatementAST::codegen() {
    if (this->expression != nullptr) {
        auto ret =  this->expression->codegen();
        Builder->CreateRet(ret);
        return ret;
    }else {
        return Builder->CreateRetVoid();
        return nullptr;
    }
}

llvm::Value* IfStatementAST::codegen()
{
    llvm::Value* cond_val = this->condition_expression->codegen();
    if (!cond_val)
    {
        throw std::logic_error("Condition expression is null");
    }

    llvm::Function* fn = Builder->GetInsertBlock()->getParent();
    // // Create blocks for then, else and resolution
    // cond_val = Builder->CreateICmpEQ(cond_val, llvm::ConstantInt::get(*Context, llvm::APInt(32, 0)), "ifcond");

    // Create blocks and set up conditional jumps
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(*Context, "then", fn);
    llvm::BasicBlock* else_block = nullptr;
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(*Context, "merge");
    if (this->else_statements.empty())
    {
        // no else statement, jump straight to the final block
        Builder->CreateCondBr(cond_val, then_block, merge_block);
    }else
    {
        else_block = llvm::BasicBlock::Create(*Context, "else");
        Builder->CreateCondBr(cond_val, then_block, else_block);

    }

    // Generate 'Then' block
    Builder->SetInsertPoint(then_block);
    for (auto& statement : this->then_statements)
    {
        statement->codegen();
    }

    // if block wasn't terminated with a `return` we jump to the final block
    if (!then_block->getTerminator())
    {
        Builder->CreateBr(merge_block);
    }

    // If there's an else statement we generate the IR for it
    if (!else_statements.empty())
    {
        fn->insert(fn->end(), else_block);
        Builder->SetInsertPoint(else_block);

        for (auto& statement : this->else_statements)
        {
            statement->codegen();
        }


        // if block wasn't terminated with a `return` we jump to the final block
        if (!else_block->getTerminator())
        {
            Builder->CreateBr(merge_block);
        }

    }
    // both code paths terminate so there's no need for merge
    // TODO: check if blocks are terminated
    
    // Check if anyone actually needs the merge block
    // A block only needs the merge block if it didn't have its own terminator
    bool then_needs_merge = !then_block->getTerminator();
    bool else_needs_merge = else_block && !else_block->getTerminator();
    
    // If there is no else statement, the 'false' condition branches straight to merge
    bool false_needs_merge = this->else_statements.empty(); 

    if (then_needs_merge || else_needs_merge || false_needs_merge) {
        // Someone branches here, so we must add it and set the insert point
        fn->insert(fn->end(), merge_block);
        Builder->SetInsertPoint(merge_block);
    } else {
        // No one branches here! Both paths returned early.
        // We delete the block to prevent the "missing terminator" error.
        delete merge_block; 
        
        // Note: Do NOT set the insert point to a deleted block. 
        // Any statements parsed after this IF statement in the same scope 
        // will be unreachable (dead code), but you might want to handle that 
        // cleanly in your AST loop.
    }
    return nullptr;
}

llvm::Value* AssignmentStatementAST::codegen()
{
    if (!NamedValues.contains(this->var_identifier))
    {
        throw std::logic_error("Variable identifier not recognized");
    }
    llvm::Value* var_ptr = NamedValues.at(this->var_identifier);

    llvm::Value* val = this->rhs_expression->codegen();
    if (!val)
    {
        throw std::logic_error("Codegen error couldn't parse rhs value");
    }

    Builder->CreateStore(val, var_ptr);
    // might be useful to return val for something like a = b = 3;
    return nullptr;
}

void ModuleAST::codegen() {
    for (auto& declaration : this->declarations) {
        if (auto proto = dynamic_cast<PrototypeAST*>(declaration.get())) {
            proto->codegen();
            continue;
        }
        if (auto fn = dynamic_cast<FunctionAST*>(declaration.get())) {
            fn->codegen();
            continue;
        }
    }
}


llvm::Value * BinaryExpressionAST::codegen() {
    auto lhs_val = this->lhs->codegen();
    auto rhs_val = this->rhs->codegen();

    if (!lhs_val || !rhs_val) {
        throw std::logic_error("Codegen error couldn't parse lhs or rhs value");
    }

    switch (this->operator_) {
        case BinaryOperator::Add:
            return Builder->CreateAdd(lhs_val, rhs_val);
            break;
        case BinaryOperator::Subtract:
            return Builder->CreateSub(lhs_val, rhs_val);
            break;
        case BinaryOperator::Multiply:
            return Builder->CreateMul(lhs_val, rhs_val);
            break;
        case BinaryOperator::CompareEQ:
            {
                const auto val = Builder->CreateICmpEQ(lhs_val, rhs_val);
                return val;
            }
            break;
    }

    throw std::logic_error("Codegen error couldn't parse operator");
    return nullptr;
}

llvm::Value * VariableExpressionAST::codegen() {
    llvm::Value* v = NamedValues[identifier];

    if (!v) throw std::runtime_error("Variable expression with unknown identifier: " + identifier);

    // ptr value from alloca, we want to create an instruction to load it
    if (v->getType()->isPointerTy())
    {
        return Builder->CreateLoad( llvm::Type::getInt32Ty(*Context), v, this->identifier);
    }else
    {
        return v;
    }
}

llvm::Value * LiteralExpressionAST::codegen() {
    int num = 0;
    try {
        num = std::stoi(this->value_str);
    } catch (const std::invalid_argument& e) {
        // String contained no digits
        throw std::logic_error("Tried to parse " + this->value_str + " as a number");
    } catch (const std::out_of_range& e) {
        throw std::logic_error("Literal " + this->value_str + " is too large to fit in i32");
    }
    return llvm::ConstantInt::get(*Context, llvm::APInt(32, num, true));
}

llvm::Value* BooleanExpressionAST::codegen()
{
    const auto val = this->inner_expression->codegen();
    if (!val)
    {
        throw std::runtime_error("Codegen error couldn't generate boolean expression");
    }
    // if we received an i1 value we can pass it further, if not we have to cast it to i1
    if (val->getType()->isIntegerTy(1))
    {
        return val;
    }else
    {
        // only works if val is i32
        return Builder->CreateICmpNE(
            val,
            llvm::ConstantInt::get(val->getType(), 0),
            "is_not_zero"
        );
    }

}


llvm::Value * CallExpressionAST::codegen() {
    llvm::Function* function = TheModule->getFunction(this->callee_identifier);
    if (!function) {
        throw std::runtime_error("Tried to call a non-existent function " + this->callee_identifier);
    }

    if (function->arg_size() != this->arg_expressions.size()) {
        throw std::runtime_error("Mismatch in argument count for function " + this->callee_identifier + "\n expected " + std::to_string(function->arg_size()) + " arguments, " + "got " + std::to_string(this->arg_expressions.size()) + ".");
    }

    std::vector<llvm::Value*> arg_values{};
    for (auto& arg : this->arg_expressions) {
        auto val = arg->codegen();
        if (!val) {
            throw std::runtime_error("Codegen error couldn't parse argument value when calling " + this->callee_identifier);
        }
        arg_values.push_back(val);
    }
    if (function->getReturnType()->isVoidTy()) {
        Builder->CreateCall(function, arg_values, "");
        return nullptr;
    }else {
        return Builder->CreateCall(function, arg_values, "tmp_call");
    }

}
