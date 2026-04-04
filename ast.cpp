//
// Created by jcen on 4.04.2026.
//

#include "ast.h"


#include <iostream>
#include <stack>


std::unique_ptr<ExpressionAST> parse_expression(std::vector<Token> &tokens) {
    // if tokens.length() == 1 -> identifier or literal
    if (tokens.empty()) throw std::runtime_error("Tried to parse an empty expression");

    // variable / literal references
    if (tokens.size() == 1) {
        // TODO: differetiate between identifiers and literals
        auto expr = std::make_unique<VariableExpressionAST>(tokens);
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
        functionAST = std::make_unique<FunctionAST>(std::vector<Token>{}, identifier_token.value.value());
    }else {
        auto body_tokens = std::vector<Token>(tokens.begin() + open_brace_i + 1, tokens.begin() + close_brace_i - 1);
        functionAST = std::make_unique<FunctionAST>(body_tokens, identifier_token.value.value());
        functionAST->resolve();
        tokens.erase(tokens.begin(), tokens.begin() + close_brace_i);
    }

    return functionAST;
}

void FunctionAST::resolve() {
    // iterate over tokens, split them into statements and call resolve on them
    std::cout << "Trying to resolve function " << this->identifier << std::endl;
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

void ReturnStatementAST::resolve() {
    // tokens = [return, <expression>]
    if (tokens.front().type != TokenType::RETURN) {
        throw std::logic_error("Incorrect tokens for a return statement");
    }
    auto expr_tokens = std::vector<Token>(tokens.begin() + 1, tokens.end());
    this->expression = parse_expression(expr_tokens);
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
                std::cout << "Parsed a function: " << node->identifier << std::endl;
                this->declarations.push_back(std::move(node));
                break;
            }
            default:
                throw std::logic_error("ModuleAST::resolve(): unknown token type");
        }
    }
}

void BinaryExpressionAST::resolve() {
    // TODO: handle expressions with more than 3 tokens
    auto lhs_tokens = std::vector<Token>(tokens.begin(), tokens.begin() + 1);
    this->lhs = parse_expression(lhs_tokens);

    // this->op

    auto rhs_tokens = std::vector<Token>(tokens.end() - 1, tokens.end());
    this->rhs = parse_expression(rhs_tokens);
}

void VariableExpressionAST::resolve() {
    if (this->tokens.size() != 1) throw std::logic_error("Created variable expression with multiple tokens");
    this->identifier = tokens.front().value.value();
}

void LiteralExpressionAST::resolve() {
    if (this->tokens.size() != 1) throw std::logic_error("Created literal expression with multiple tokens");
    this->value_str = tokens.front().value.value();
}
