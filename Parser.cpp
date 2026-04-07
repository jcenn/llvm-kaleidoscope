//
// Created by jcen on 4.04.2026.
//

#include "Parser.h"

#include <stack>


void Parser::InitializeCodeGen() {
    Context = std::make_unique<llvm::LLVMContext>();
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    TheModule = std::make_unique<llvm::Module>("main module", *Context);
}

bool Parser::is_binary_operator(const Token& tok) {
    switch (tok.type) {
        case TokenType::PLUS: [[fallthrough]];
        case TokenType::MINUS:
            return true;
        default: return false;
    }
    return false;
}

size_t Parser::find_matching_paren_index(std::vector<Token> &tokens, size_t open_paren_i, TokenType open_tok, TokenType close_tok) {
    auto stack = std::stack<TokenType>();
    stack.push(open_tok);
    size_t i = open_paren_i + 1;
    while (i < tokens.size()) {
        if (stack.top() == open_tok && tokens.at(i).type == close_tok) {
            stack.pop();
        }else if (tokens.at(i).type == open_tok) {
            stack.push(open_tok);
        }
        if (stack.empty()) {
            return i;
        }
        i++;
    }
    throw std::runtime_error("Tried to parse an expression with mismatched parentheses");

}

std::unique_ptr<ModuleAST> Parser::parse_tokens(const std::vector<Token> &tokens) {
    auto moduleNode = std::make_unique<ModuleAST>(tokens);
    moduleNode->resolve();

    return moduleNode;
}

std::unique_ptr<StatementAST> Parser::parse_statement(const std::vector<Token> &statement_tokens) {
    std::unique_ptr<StatementAST> statement;
    switch (statement_tokens.front().type) {
        case TokenType::LET: {
            statement = std::make_unique<LetStatementAST>(statement_tokens);
            break;
        }
        case TokenType::RETURN: {
            statement = std::make_unique<ReturnStatementAST>(statement_tokens);
            break;
        }
        // Call statement like InitWindow();
        case TokenType::IDENTIFIER: {
            statement = std::make_unique<CallStatementAST>(statement_tokens);
            break;
        }
        default:
            throw std::runtime_error("Statement not recognized");
    }
    statement->resolve();
    return std::move(statement);
}

// expects tokens to only contain the content between function parentheses ex. for `foo(a, b) tokens = [a, ',', b]
std::vector<std::pair<std::string, TypeIdentifier>> Parser::parse_function_parameters(const std::vector<Token> &tokens) {
    // Parse function arguments
    auto arg_identifiers = std::vector<std::pair<std::string, TypeIdentifier>>();
    size_t arg_index = 0;

    while (arg_index < tokens.size()) {
        auto tok = tokens.at(arg_index);
        if (tok.type == TokenType::IDENTIFIER) {
            // TODO: handle different types
            arg_identifiers.emplace_back(tok.value.value(), TypeIdentifier::I32);
        }
        arg_index++;
    }
    return arg_identifiers;
}

std::unique_ptr<PrototypeAST> Parser::parse_prototype(std::vector<Token> &tokens) {
    auto const& identifier_token = tokens.at(0);
    auto ret_type = TypeIdentifier::VOID;

    if (! identifier_token.value) {
        throw std::logic_error("FunctionAST::parse_function(): empty identifier");
    }

    auto arg_index = 2;
    // open bracket is always on the 3rd token
    // fn foo(...)
    // extern foo(...)
    size_t open_paren_i = 1;
    auto close_paren_i = Parser::find_matching_paren_index(tokens, open_paren_i, TokenType::BRACKET_L, TokenType::BRACKET_R);
    // find return type identifier
    // TODO: will not work for more complex types like -> (i32, i32)
    // TODO: something like `Parser::parse_type_expression(tokens, close_paren_i + 1)`
    if (tokens.size() > close_paren_i + 1 &&  tokens.at(close_paren_i + 1).type == TokenType::ARROW && tokens.at(close_paren_i + 2).type == TokenType::IDENTIFIER) {
        auto type_ident = tokens.at(close_paren_i + 2).value.value();
        if (!Parser::type_identifiers.contains(type_ident)) {
            throw std::runtime_error("Type identifier " + type_ident + " is not recognized by parser");
        }
        ret_type = Parser::type_identifiers.at(type_ident);
    }

    // main has inferred type
    if (identifier_token.value.value() == "main") {
        ret_type = TypeIdentifier::I32;
    }
    auto arg_tokens = std::vector(tokens.begin() + open_paren_i + 1, tokens.begin() + close_paren_i);
    auto arg_identifiers = Parser::parse_function_parameters( arg_tokens );
    return std::move(std::make_unique<PrototypeAST>(identifier_token.value.value(), arg_identifiers, ret_type));
}