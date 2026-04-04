//
// Created by jcen on 4.04.2026.
//

#include "Parser.h"

#include <iostream>
#include <stack>

ModuleAST* Parser::parse_tokens(const std::vector<Token> &tokens) {
    auto moduleNode = new ModuleAST(tokens);
    moduleNode->resolve();

    return moduleNode;
}
FunctionAST* parse_function(std::vector<Token> &tokens) {
    // find closing brace
    // create AST Node
    // return node and number of consumed tokens (reference parameter?) or just consume them from reference
    // tokens = [fn, identifier, (, args..., ), '-> type ?', {, ..., }]
    auto const& identifier_token = tokens.at(1);

    if (! identifier_token.value) {
        throw std::logic_error("FunctionAST::parse_function(): empty identifier");
    }

    FunctionAST* functionAST = nullptr;

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
        functionAST = new FunctionAST(std::vector<Token>{}, identifier_token.value.value());
    }else {
        auto body_tokens = std::vector<Token>(tokens.begin() + open_brace_i + 1, tokens.begin() + close_brace_i - 1);
        functionAST = new FunctionAST(body_tokens, identifier_token.value.value());
        functionAST->resolve();
        tokens.erase(tokens.begin(), tokens.begin() + close_brace_i);
    }

    return functionAST;
}

FunctionAST::~FunctionAST() noexcept {
    for (auto const statement : this->statements) {
        delete statement;
    }
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

        StatementAST* statement = new StatementAST(statement_tokens);
        this->statements.push_back(statement);
        token_count = 0;

    }
    return;
}

void StatementAST::resolve() {
}

void ModuleAST::resolve() {
    if (this->tokens.empty()) {
        throw std::logic_error("ModuleAST::resolve(): empty tokens list");
    }

    // Consume tokens in a loop converting them to AST nodes
    while (!this->tokens.empty()) {
        auto token = this->tokens.front();
        AST_Node* node;
        switch (token.type) {
            case TokenType::FN:
                node = parse_function(tokens);
                std::printf("Parsed a function\n");
                break;
            default:
                throw std::logic_error("ModuleAST::resolve(): unknown token type");
        }
        this->declarations.push_back(node);
        // index++;
    }
}

