//
// Created by jcen on 4.04.2026.
//

#include "ast.h"

#include <iostream>
#include <stack>

#include "llvm/IR/Verifier.h"

// From kaleidoscope tutorial:
// TheContext is an opaque object that owns a lot of core LLVM data structures, such as the type and constant value tables
std::unique_ptr<llvm::LLVMContext> Context;

// The Builder object is a helper object that makes it easy to generate LLVM instructions. Instances of the IRBuilder class template keep track of the current place to insert instructions and has methods to create new instructions.
std::unique_ptr<llvm::IRBuilder<>> Builder;

// TheModule is an LLVM construct that contains functions and global variables. In many ways, it is the top-level structure that the LLVM IR uses to contain code. It will own the memory for all of the IR that we generate, which is why the codegen() method returns a raw Value*, rather than a unique_ptr<Value>.
std::unique_ptr<llvm::Module> TheModule;

// The NamedValues map keeps track of which values are defined in the current scope and what their LLVM representation is. (In other words, it is a symbol table for the code).
std::map<std::string, llvm::Value *> NamedValues;

void InitializeCodeGen() {
    Context = std::make_unique<llvm::LLVMContext>();
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    TheModule = std::make_unique<llvm::Module>("main module", *Context);
}

std::unique_ptr<ExpressionAST> parse_expression(std::vector<Token> &tokens) {
    // if tokens.length() == 1 -> identifier or literal
    if (tokens.empty()) throw std::runtime_error("Tried to parse an empty expression");

    // variable / literal references
    if (tokens.size() == 1) {
        // TODO: differetiate between identifiers and literals
        bool is_literal = true;
        for (const char c : tokens.at(0).value.value()) {
            if (std::isalpha(c)) {
                is_literal = false;
                break;
            }
        }
        std::unique_ptr<ExpressionAST> expr;
        if (is_literal) {
            expr = std::make_unique<LiteralExpressionAST>(tokens);
        }else {
            expr = std::make_unique<VariableExpressionAST>(tokens);
        }
        expr->resolve();
        return expr;
    }
    // only parse first 3 tokens for expressions like 1 + 3
    if (tokens.size() == 3) {
        auto expr = std::make_unique<BinaryExpressionAST>(tokens);
        expr->resolve();
        return expr;
    }

    throw std::runtime_error("Tried to parse an empty expression");
}

std::unique_ptr<FunctionAST> parse_function(std::vector<Token> &tokens) {
    // find closing brace
    // create AST Node
    // return node and number of consumed tokens (reference parameter?) or just consume them from reference
    // tokens = [fn, identifier, (, args..., ), '-> type ?', {, ..., }]
    auto const& identifier_token = tokens.at(1);

    if (! identifier_token.value) {
        throw std::logic_error("FunctionAST::parse_function(): empty identifier");
    }

    std::unique_ptr<FunctionAST> functionAST = nullptr;

    size_t open_brace_i = 0;
    size_t close_brace_i = 0;
    // find the opening brace
    while (tokens[open_brace_i].type != TokenType::BRACE_L) open_brace_i++;

    // find the matching closing brace
    std::stack<TokenType> stack{};
    stack.push(TokenType::BRACE_L);
    close_brace_i = open_brace_i + 1;
    while (!stack.empty()) {
        if (tokens[close_brace_i].type == TokenType::BRACE_L) {
            stack.push(TokenType::BRACE_L);
            close_brace_i++;
            continue;
        }

        if (tokens[close_brace_i].type == TokenType::BRACE_R && stack.top() == TokenType::BRACE_L) {
            stack.pop();
            close_brace_i++;
            continue;
        }

        if (tokens[close_brace_i].type == TokenType::BRACE_R && stack.top() == TokenType::BRACE_R) {
            throw std::logic_error("Braces do not match");
        }

        // skip tokens that we don't care about here
        close_brace_i++;
    }

    // tokens [open_brace_i : close_brace_i] contains function body + braces

    // Empty function
    if (open_brace_i + 1 == close_brace_i) {
        // empty token vector
        functionAST = std::make_unique<FunctionAST>(std::vector<Token>{}, identifier_token.value.value(), std::vector<std::string>{});
    }else {
        auto body_tokens = std::vector<Token>(tokens.begin() + open_brace_i + 1, tokens.begin() + close_brace_i - 1);
        functionAST = std::make_unique<FunctionAST>(body_tokens, identifier_token.value.value(), std::vector<std::string>{});
        functionAST->resolve();
        tokens.erase(tokens.begin(), tokens.begin() + close_brace_i);
    }

    return functionAST;
}

llvm::Function * PrototypeAST::codegen() {

    std::vector<llvm::Type*> arg_types{};
    for (auto& arg : this->arg_identifiers) {
        arg_types.push_back(llvm::Type::getInt32Ty(*Context));
    }
    llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(*Context), arg_types, false);
    llvm::Function* fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, identifier, TheModule.get());

    size_t idx = 0;
    // sets names for arguments. Mostly used for debugging IR
    for (auto& arg : fn->args()) {
        arg.setName(this->arg_identifiers[idx++]);
    }
    return fn;
}

void FunctionAST::resolve() {

    // iterate over tokens, split them into statements and call resolve on them
    std::cout << "Trying to resolve function " << this->prototype->identifier << std::endl;
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

        switch (statement_tokens.front().type) {
            case TokenType::LET: {
                auto statement = std::make_unique<LetStatementAST>(statement_tokens);
                statement->resolve();
                this->statements.push_back(std::move(statement));
                break;
            }
            case TokenType::RETURN: {
                auto statement = std::make_unique<ReturnStatementAST>(statement_tokens);
                statement->resolve();
                this->statements.push_back(std::move(statement));
                break;
            }
            default:
                throw std::runtime_error("Statement not recognized");

        }
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
    // function didn't have a return statement
    if (dynamic_cast<ReturnStatementAST*>(statements.back().get()) == nullptr) {
        Builder->CreateRetVoid();
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

llvm::Value * LetStatementAST::codegen() {
    return nullptr;
}

void ReturnStatementAST::resolve() {
    // tokens = [return, <expression>]
    if (tokens.front().type != TokenType::RETURN) {
        throw std::logic_error("Incorrect tokens for a return statement");
    }
    auto expr_tokens = std::vector<Token>(tokens.begin() + 1, tokens.end());
    this->expression = parse_expression(expr_tokens);
}

llvm::Value * ReturnStatementAST::codegen() {
    auto ret =  this->expression->codegen();
    Builder->CreateRet(ret);
    return ret;
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
                std::cout << "Parsed a function: " << node->prototype->identifier << std::endl;
                this->declarations.push_back(std::move(node));
                break;
            }
            default:
                throw std::logic_error("ModuleAST::resolve(): unknown token type");
        }
    }
}

void ModuleAST::codegen() {
    for (auto& declaration : this->declarations) {
        if (auto fn = dynamic_cast<FunctionAST*>(declaration.get())) {
            fn->codegen();
        }
    }
}


void BinaryExpressionAST::resolve() {
    // TODO: handle expressions with more than 3 tokens
    auto lhs_tokens = std::vector<Token>(tokens.begin(), tokens.begin() + 1);
    this->lhs = parse_expression(lhs_tokens);

    // this->op
    switch (tokens.at(1).type) {
        case TokenType::PLUS: {
            this->operator_ = BinaryOperator::Add;
            std::cout << "Parsed a plus operator" << std::endl;
            break;
        }
        case TokenType::MINUS: {
            this->operator_ = BinaryOperator::Subtract;
            break;
        }
        default:
            throw std::logic_error("BinaryExpressionAST::resolve(): unknown token type");
    }

    auto rhs_tokens = std::vector<Token>(tokens.end() - 1, tokens.end());
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
    // TODO: get the real value
    int num = 0;
    try {
        num = std::stoi(this->value_str);
        std::cout << "Integer: " << num << "\n";
    } catch (const std::invalid_argument& e) {
        // String contained no digits
        throw std::logic_error("Tried to parse " + this->value_str + " as a number");
    } catch (const std::out_of_range& e) {
        throw std::logic_error("Literal " + this->value_str + " is too large to fit in i32");
    }
    return llvm::ConstantInt::get(*Context, llvm::APInt(32, num, true));
}
