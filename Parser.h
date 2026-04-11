//
// Created by jcen on 4.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_PARSER_H
#define LLVM_KALEIDOSCOPE_PARSER_H
#include <memory>
#include <vector>

#include "ast.h"
#include "Token.h"
#include "Types.h"

namespace Parser {

    void InitializeCodeGen();

    bool is_binary_operator(const Token& tok);

    size_t find_matching_paren_index(const std::vector<Token> &tokens, size_t open_paren_i, TokenType open_tok, TokenType close_tok);

    // Splits list of tokens inside function parentheses and returns a vector where vec[n] corresponds to tokens for n-th argument
    // Must only tokens between function's ( and ) tokens
    std::vector<std::vector<Token>> get_function_arg_tokens(std::vector<Token> &tokens);

    inline const std::map<std::string, TypeIdentifier> type_identifiers = {
        {"void", TypeIdentifier::VOID},
        {"i32", TypeIdentifier::I32},
    };
    std::unique_ptr<ModuleAST> parse_tokens(const std::vector<Token>& tokens);

    std::unique_ptr<FunctionAST> parse_function_def(std::span<const Token> tokens);
    std::unique_ptr<PrototypeAST> parse_prototype(std::span<const Token> tokens);
    std::vector<std::pair<std::string, TypeIdentifier>> parse_function_parameters(std::span<const Token> tokens);
    std::vector<std::unique_ptr<StatementAST>> parse_function_body(std::span<const Token> tokens);

    std::unique_ptr<StatementAST> parse_statement(std::span<const Token> tokens, bool top_level = false);
    std::unique_ptr<ReturnStatementAST> parse_return_statemen(std::span<const Token> tokens);
    std::unique_ptr<CallStatementAST> parse_call_statement(std::span<const Token> tokens);

    std::unique_ptr<ExpressionAST> parse_expression(std::span<const Token> tokens);
    std::unique_ptr<ExpressionAST> parse_binary_expression(std::span<const Token> tokens);
    std::unique_ptr<ExpressionAST> parse_call_expression(std::span<const Token> tokens);
}



#endif //LLVM_KALEIDOSCOPE_PARSER_H
