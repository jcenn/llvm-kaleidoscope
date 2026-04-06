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

// TheModule is an LLVM construct that contains functions and global variables. In many ways, it is the top-level structure that the LLVM IR uses to contain code. It will own the memory for all the IR that we generate, which is why the codegen() method returns a raw Value*, rather than a unique_ptr<Value>.
std::unique_ptr<llvm::Module> TheModule;

// The NamedValues map keeps track of which values are defined in the current scope and what their LLVM representation is. (In other words, it is a symbol table for the code).
std::map<std::string, llvm::Value *> NamedValues;

void InitializeCodeGen() {
    Context = std::make_unique<llvm::LLVMContext>();
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    TheModule = std::make_unique<llvm::Module>("main module", *Context);
}

bool is_binary_operator(const Token& tok) {
    switch (tok.type) {
        case TokenType::PLUS: [[fallthrough]];
        case TokenType::MINUS:
            return true;
        default: return false;
    }
    return false;
}

size_t find_matching_paren_index(std::vector<Token> &tokens, size_t open_paren_i) {
    auto stack = std::stack<TokenType>();
    stack.push(TokenType::BRACKET_L);
    size_t i = open_paren_i + 1;
    while (i < tokens.size()) {
        if (stack.top() == TokenType::BRACKET_L && tokens.at(i).type == TokenType::BRACKET_R) {
            stack.pop();
        }else if (tokens.at(i).type == TokenType::BRACKET_L) {
            stack.push(TokenType::BRACKET_L);
        }
        if (stack.empty()) {
            return i;
        }
        i++;
    }
    throw std::runtime_error("Tried to parse an expression with mismatched parentheses");

}

std::unique_ptr<ExpressionAST> parse_expression(std::vector<Token> &tokens) {
    // if tokens.length() == 1 -> identifier or literal
    if (tokens.empty()) throw std::runtime_error("Tried to parse an empty expression");

    // variable / literal references
    if (tokens.size() == 1) {
        // TODO: differentiate between identifiers and literals in a different pass
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

    // Function calls
    // TODO: remove literal after fixing differentiating between them
    if ((tokens.front().type == TokenType::IDENTIFIER || tokens.front().type == TokenType::LITERAL) && tokens.at(1).type == TokenType::BRACKET_L && tokens.back().type == TokenType::BRACKET_R) {
        // TODO: check if we're parsing a single paren expression like `foo(a + b) and not  foo() + foo()
        size_t end_paren_i = find_matching_paren_index(tokens, 1);
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
        if (is_binary_operator(tokens.at(i))) {
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
    std::vector<std::string> arg_identifiers{};

    auto arg_index = 2;
    while (tokens.at(arg_index).type != TokenType::BRACKET_R) {
        // TODO: remove literal after adding proper identifier parsing
        if (tokens.at(arg_index).type == TokenType::IDENTIFIER || tokens.at(arg_index).type == TokenType::LITERAL) {
            arg_identifiers.push_back(tokens.at(arg_index).value.value());
        }
        arg_index++;
    }

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
        functionAST = std::make_unique<FunctionAST>(body_tokens, identifier_token.value.value(), arg_identifiers);
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

std::unique_ptr<PrototypeAST> parse_prototype(std::vector<Token>& tokens) {
    if (tokens.front().type != TokenType::EXTERN) {
        throw std::logic_error("Incorrect tokens for a prototype");
    }

    // find identifier and arguments
    std::string identifier = tokens.at(1).value.value();
    // opening bracket
    auto i = 2;
    std::vector<std::string> arg_identifiers{};
    while (i < tokens.size()) {
        // we don't expect () inside extern declarations
        if (tokens.at(i).type == TokenType::BRACKET_R) {
            break;
        }

        if (tokens.at(i).type == TokenType::IDENTIFIER || tokens.at(i).type == TokenType::LITERAL) {
            arg_identifiers.push_back(tokens.at(i).value.value());
        }
        i++;
    }
    tokens.erase(tokens.begin(), tokens.begin() + i + 2); // +2 => closing ')' and semicolon
    return std::make_unique<PrototypeAST>(identifier, arg_identifiers);
}

void ModuleAST::resolve() {
    if (this->tokens.empty()) {
        throw std::logic_error("ModuleAST::resolve(): empty tokens list");
    }

    // Consume tokens in a loop converting them to AST nodes
    size_t i{};
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
            case TokenType::EXTERN: {
                auto proto = parse_prototype(tokens);
                std::cout << "Found external function " << proto->identifier << std::endl;
                this->declarations.push_back(std::move(proto));
                break;
            }
            default:
                throw std::logic_error("ModuleAST::resolve(): unknown token type");
        }
        i++;
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
        if (is_binary_operator(tokens.at(i))) {
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
        std::cout << "Integer: " << num << "\n";
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
        throw std::logic_error("Created call expression with less than 3 tokens");
    }
    if (this->tokens.front().type != TokenType::IDENTIFIER) {
        throw std::logic_error("Created call expression with no identifier");
    }

    this->calee_identifier = this->tokens.front().value.value();
    // Function with no arguments ex. foo();
    if (this->tokens.size() == 3) {
        return;
    }
    size_t arg_start = 2;
    size_t i = arg_start;

    // TODO: consider i < tokens.size() ?
    while (this->tokens.at(i).type != TokenType::BRACKET_R) {
        // iterate over tokens until we hit closing parentheses and pass any found arguments to an expression AST node
        // TODO: could cause problems when passing other function calls ex. foo(1, bar(2, 3)) as it could consider the , inside bar to be a terminator

        // if we hit a '(' in arguments, we skip to the matching ')' to handle calls like foo(1, foo(2, 3))
        if (tokens.at(i).type == TokenType::BRACKET_L) {
            i = find_matching_paren_index(tokens, i) + 1;
            continue;
        }

        // We hit the end of the first argument
        if (tokens.at(i).type == TokenType::COMMA) {
            auto expr_tokens = std::vector<Token>(tokens.begin() + arg_start, tokens.begin() + i);
            auto expression = parse_expression(expr_tokens);
            this->arg_expressions.push_back(std::move(expression));
            arg_start = i + 1;
        }

        i++;
    }
    // we hit the closing paren
    // if function has arguments we consume the last one
    // i == 3 would indicate a function without arguments -> Token[foo, (, )]
    if (i != 2) {
        auto expr_tokens = std::vector<Token>(tokens.begin() + arg_start, tokens.end() - 1);
        auto expression = parse_expression(expr_tokens);
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

    return Builder->CreateCall(function, arg_values, "call_tmp");

}
