//
// Created by jcen on 4.04.2026.
//

#include "ast.h"

#include <iostream>
#include <stack>

#include "Parser.h"
#include "llvm/IR/Verifier.h"

std::unique_ptr<ExpressionAST> parse_expression(std::vector<Token> &tokens) {
    // if tokens.length() == 1 -> identifier or literal
    if (tokens.empty()) throw std::runtime_error("Tried to parse an empty expression");

    // variable / literal references
    if (tokens.size() == 1) {
        std::unique_ptr<ExpressionAST> expr;
        if (tokens.front().type == TokenType::LITERAL) {
            expr = std::make_unique<LiteralExpressionAST>(tokens);
        }else if (tokens.front().type == TokenType::IDENTIFIER) {
            expr = std::make_unique<VariableExpressionAST>(tokens);
        }else {
            throw std::runtime_error("Tried to parse an expression with invalid identifier");
        }
        expr->resolve();
        return expr;
    }

    // Function calls
    // TODO: remove literal after fixing differentiating between them
    if ((tokens.front().type == TokenType::IDENTIFIER) && tokens.at(1).type == TokenType::BRACKET_L && tokens.back().type == TokenType::BRACKET_R) {
        // TODO: check if we're parsing a single paren expression like `foo(a + b) and not  foo() + foo()
        size_t end_paren_i = Parser::find_matching_paren_index(tokens, 1, TokenType::BRACKET_L, TokenType::BRACKET_R);
        if (end_paren_i == tokens.size()-1) {
            auto expr = std::make_unique<CallExpressionAST>(tokens);
            expr->resolve();
            return expr;
        }
    }

    // TODO: modify to handle more complex expressions
    // only parse first 3 tokens for expressions like 1 + 3
    bool binary_exp = false;
    for (size_t i{}; i < tokens.size(); i++) {
        // TODO: check other binary expression tokens
        if (Parser::is_binary_operator(tokens.at(i))) {
            binary_exp = true;
        }
    }
    if (binary_exp) {
        auto expr = std::make_unique<BinaryExpressionAST>(tokens);
        expr->resolve();
        return expr;
    }

    throw std::runtime_error("Tried to parse an invalid expression");
}

// Parsing function *definitions*
std::unique_ptr<FunctionAST> parse_function(std::vector<Token> &tokens) {
    // find closing brace
    // create AST Node
    // return node and number of consumed tokens (reference parameter?) or just consume them from reference
    // tokens = [fn, identifier, (, args..., ), '->', 'type_ident', {, ..., }]

    std::unique_ptr<FunctionAST> functionAST = nullptr;

    size_t open_brace_i = 0;
    // find the opening and closing brace
    while (tokens[open_brace_i].type != TokenType::BRACE_L) open_brace_i++;
    size_t close_brace_i =  Parser::find_matching_paren_index(tokens, open_brace_i, TokenType::BRACE_L, TokenType::BRACE_R);

    auto proto_tokens = std::vector<Token>(tokens.begin() + 1, tokens.begin() + open_brace_i);
    auto prototype = Parser::parse_prototype( proto_tokens );


    // tokens [open_brace_i : close_brace_i] contains function body + braces
    // Empty function
    if (open_brace_i + 1 == close_brace_i) {
        functionAST = std::make_unique<FunctionAST>(std::vector<Token>{}, std::move(prototype));
    }else {
        auto body_tokens = std::vector<Token>(tokens.begin() + open_brace_i + 1, tokens.begin() + close_brace_i);
        functionAST = std::make_unique<FunctionAST>(body_tokens, std::move(prototype));
        functionAST->resolve();
        tokens.erase(tokens.begin(), tokens.begin() + close_brace_i + 1);
    }

    return functionAST;
}

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

void FunctionAST::resolve() {
    // iterate over tokens, split them into statements and call resolve on them
    // std::cout << "Trying to resolve function " << this->prototype->identifier << std::endl;
    size_t token_count = 0;
    while (!this->tokens.empty()) {
        auto const& tok = this->tokens.at(token_count);
        if (tok.type != TokenType::SEMICOLON) {
            token_count++;
            continue;
        };
        // when we hit a semicolon we consume all tokens until that semicolon
        auto const statement_tokens = std::vector<Token>(this->tokens.begin(), this->tokens.begin() + token_count);

        // + 1 to include the semicolon
        this->tokens.erase(this->tokens.begin(), this->tokens.begin() + token_count + 1);

        this->statements.push_back(Parser::parse_statement(statement_tokens));
        token_count = 0;

    }
    return;
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

void LetStatementAST::resolve() {
    // tokens = [let, x, =,  (...)]
    auto const& ident_token = this->tokens.at(1);
    if (tokens.front().type != TokenType::LET) {
        throw std::logic_error("Incorrect tokens for a let statement");
    }
    if (!ident_token.value) {
        throw std::runtime_error("No identifier found for the let statement");
    }
    this->LHS_identifier = ident_token.value.value();

    // resolve expression on the right hand side
    auto expression_tokens = std::vector<Token>(tokens.begin() + 3, tokens.end());
    this->expression = parse_expression(expression_tokens);
}

void CallStatementAST::resolve() {
    // tokens = [ identifier, (, ident/literal, ident/literal, ..., ) ]
    auto const& fn_identifier = this->tokens.front();
    if (tokens.front().type != TokenType::IDENTIFIER) {
        throw std::logic_error("Incorrect tokens for a call statement");
    }
    if (!fn_identifier.value) {
        throw std::runtime_error("No function identifier found for the call statement");
    }

    // resolve call expression
    auto expression_tokens = std::vector<Token>(tokens.begin(), tokens.end());
    this->call_expression = parse_expression(expression_tokens);
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

void ReturnStatementAST::resolve() {
    // tokens = [return, <expression>]
    // `return;` inferred to return void
    if (tokens.size() == 1) {
        return;
    }
    //`return expr;`
    if (tokens.front().type != TokenType::RETURN) {
        throw std::logic_error("Incorrect tokens for a return statement");
    }
    auto expr_tokens = std::vector<Token>(tokens.begin() + 1, tokens.end());
    this->expression = parse_expression(expr_tokens);
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



void ModuleAST::resolve() {
    if (this->tokens.empty()) {
        throw std::logic_error("ModuleAST::resolve(): empty tokens list");
    }

    // Consume tokens in a loop converting them to AST nodes
    while (!this->tokens.empty()) {
        auto token = this->tokens.front();
        // At this point we expect the module to only have the main function, but it could be later extended
        // to include different functions and global variables
        switch (token.type) {
            case TokenType::FN: {
                auto node = parse_function(tokens);
                // std::cout << "Parsed a function: " << node->prototype->identifier << std::endl;
                this->declarations.push_back(std::move(node));
                break;
            }
            case TokenType::EXTERN: {
                auto semi_colon_i = 0;
                while (semi_colon_i < tokens.size() && tokens.at(semi_colon_i).type != TokenType::SEMICOLON) semi_colon_i++;
                auto proto_tokens = std::vector<Token>(tokens.begin() + 1, tokens.begin() + semi_colon_i);
                auto proto = Parser::parse_prototype(proto_tokens);
                tokens.erase(tokens.begin(), tokens.begin() + semi_colon_i + 1);
                // std::cout << "Found external function " << proto->identifier << std::endl;
                this->declarations.push_back(std::move(proto));
                break;
            }
            default:
                throw std::logic_error("ModuleAST::resolve(): unknown token type");
        }
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


void BinaryExpressionAST::resolve() {
    // TODO: handle expressions with more than 3 tokens

    // Find the operator for current expression
    size_t operator_index = 0;
    for (size_t i{}; i < tokens.size(); i++) {
        if (Parser::is_binary_operator(tokens.at(i))) {
            operator_index = i;
            // Currently stopping at first operator, but we should handle precedence priority
            break;
        }
    }
    auto lhs_tokens = std::vector<Token>(tokens.begin(), tokens.begin() + operator_index);
    this->lhs = parse_expression(lhs_tokens);

    // this->op
    switch (tokens.at(operator_index).type) {
        case TokenType::PLUS: {
            this->operator_ = BinaryOperator::Add;
            // std::cout << "Parsed a plus operator" << std::endl;
            break;
        }
        case TokenType::MINUS: {
            this->operator_ = BinaryOperator::Subtract;
            break;
        }
        default:
            throw std::logic_error("BinaryExpressionAST::resolve(): unknown token type");
    }

    auto rhs_tokens = std::vector<Token>(tokens.begin() + operator_index + 1, tokens.end());
    this->rhs = parse_expression(rhs_tokens);
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


void VariableExpressionAST::resolve() {
    if (this->tokens.size() != 1) throw std::logic_error("Created variable expression with multiple tokens");
    this->identifier = tokens.front().value.value();
}

llvm::Value * VariableExpressionAST::codegen() {
    llvm::Value* v = NamedValues[identifier];

    if (!v) throw std::runtime_error("Variable expression with unknown identifier: " + identifier);

    return v;
}

void LiteralExpressionAST::resolve() {
    if (this->tokens.size() != 1) throw std::logic_error("Created literal expression with multiple tokens");
    this->value_str = tokens.front().value.value();
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

void CallExpressionAST::resolve() {
    // expects tokens like [ foo(bar, baz) ]
    if (this->tokens.size() < 3) {
        throw std::logic_error("Tried to create a call expression with less than 3 tokens");
    }
    if (this->tokens.front().type != TokenType::IDENTIFIER) {
        throw std::logic_error("Tried to create a call expression with no identifier");
    }

    this->calee_identifier = this->tokens.front().value.value();
    // Function with no arguments ex. foo();
    if (this->tokens.size() == 3) {
        return;
    }

    size_t arg_start = 2;
    size_t i = arg_start;

    // iterate over tokens until we hit closing parentheses and pass any found arguments to an expression AST node
    // TODO: could cause problems when passing other function calls ex. foo(1, bar(2, 3)) as it could consider the , inside bar to be a terminator
    // omit function identifier, open paren and close paren
    auto arg_source_tokens = std::vector<Token>(tokens.begin() + 2, tokens.end() - 1);
    auto arg_tokens = Parser::get_function_arg_tokens(arg_source_tokens);

    for (auto& toks : arg_tokens) {
        auto expression = parse_expression(toks);
        this->arg_expressions.push_back(std::move(expression));
    }

}

llvm::Value * CallExpressionAST::codegen() {
    llvm::Function* function = TheModule->getFunction(this->calee_identifier);
    if (!function) {
        throw std::runtime_error("Tried to call a non-existent function " + this->calee_identifier);
    }

    if (function->arg_size() != this->arg_expressions.size()) {
        throw std::runtime_error("Mismatch in argument count for function " + this->calee_identifier + "\n expected " + std::to_string(function->arg_size()) + " arguments, " + "got " + std::to_string(this->arg_expressions.size()) + ".");
    }

    std::vector<llvm::Value*> arg_values{};
    for (auto& arg : this->arg_expressions) {
        auto val = arg->codegen();
        if (!val) {
            throw std::runtime_error("Codegen error couldn't parse argument value when calling " + this->calee_identifier);
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
