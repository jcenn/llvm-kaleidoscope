//
// Created by jcen on 4.04.2026.
//

#include "ast.h"

#include "Parser.h"
#include "llvm/IR/Verifier.h"

#include <iostream>

llvm::Function * PrototypeAST::codegen() {

    std::vector<llvm::Type*> arg_types{};
    for (auto& arg : this->args) {
        arg_types.push_back(llvm::Type::getInt32Ty(*Context));
    }
    llvm::Type* return_type = nullptr;
    switch (this->ret_type) {
        case TypeIdentifier::VOID:
            return_type = llvm::Type::getVoidTy(*Context);
            break;
        case TypeIdentifier::I32:
            return_type = llvm::Type::getInt32Ty(*Context);
            break;
        default:
            throw std::logic_error("Invalid return type");
    }
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
    }

    throw std::logic_error("Codegen error couldn't parse operator");
    return nullptr;
}

llvm::Value * VariableExpressionAST::codegen() {
    llvm::Value* v = NamedValues[identifier];

    if (!v) throw std::runtime_error("Variable expression with unknown identifier: " + identifier);

    return v;
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
