//
// Created by jcen on 7.04.2026.
//

#include "TestRunner.h"

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "Lexer.h"
#include "Token.h"

bool LexerTestRunner::compare_tokens(std::vector<Token>const & expected, std::vector<Token>const & given) {
    if (expected.size() != given.size()) {
        std::cout << "Expected " << expected.size() << " tokens but got " << given.size() << std::endl;
        return false;
    }
    bool passed = true;
    for (size_t i{}; i < expected.size(); i++) {
        const auto& expected_tok = expected.at(i);
        const auto& given_tok = given.at(i);
        if (expected_tok.type != given_tok.type) {
            passed = false;
            std::cout << "Token " << i << " (" << token_to_string(expected_tok) << ") expected token " << token_to_string(expected_tok)<< " but got " << token_to_string(given_tok) << std::endl;
        }else if (expected_tok.value) {
            if (!given_tok.value) {
                passed = false;
                std::cout << "Token " << i << " (" << token_to_string(expected_tok) << ") expected value " << expected_tok.value.value() << " but got nothing" << std::endl;
            }else if (expected_tok.value.value() != given_tok.value.value()) {
                passed = false;
                std::cout << "Token " << i << " (" << token_to_string(expected_tok) << ") expected value " << expected_tok.value.value() << " but got " << given_tok.value.value() << std::endl;
            }
        }
    }
    return passed;
}
LexerTestRunner::LexerTestRunner() {
    tests.emplace_back(
        "main",
        [this]() {
            std::string source =
                "fn main() {"
                "   return 0;"
                "}" ;
            std::vector<Token> expectedTokens{
                Token(TokenType::FN),
                Token(TokenType::IDENTIFIER, "main"),
                Token(TokenType::BRACKET_L),
                Token(TokenType::BRACKET_R),
                Token(TokenType::BRACE_L),
                Token(TokenType::RETURN),
                Token(TokenType::LITERAL, "0"),
                Token(TokenType::SEMICOLON),
                Token(TokenType::BRACE_R),
            };
            auto lexer = std::make_unique<Lexer>();
            auto actualTokens = lexer->parse(source);

            return compare_tokens(expectedTokens, actualTokens);
        }
    );
    tests.emplace_back(
        "basic expressions",
        [this]() {
            const std::string source =
                "fn main() {"
                "   return 1 + 2 - 3;"
                "}" ;
            const std::vector<Token> expectedTokens{
                // Function declaration
                Token(TokenType::FN),
                Token(TokenType::IDENTIFIER, "main"),
                Token(TokenType::BRACKET_L),
                Token(TokenType::BRACKET_R),
                Token(TokenType::BRACE_L),
                // Function body
                Token(TokenType::RETURN),
                Token(TokenType::LITERAL, "1"),
                Token(TokenType::PLUS),
                Token(TokenType::LITERAL, "2"),
                Token(TokenType::MINUS),
                Token(TokenType::LITERAL, "3"),
                Token(TokenType::SEMICOLON),

                Token(TokenType::BRACE_R),
            };
            const auto lexer = std::make_unique<Lexer>();
            const auto actualTokens = lexer->parse(source);

            return compare_tokens(expectedTokens, actualTokens);
        }
    );
}

void LexerTestRunner::runLexerTestCases() {
    std::cout << "Running tests for Lexer..." << std::endl;
    for (const auto& test : tests) {
        if (test.second()) {
            std::cout << "\x1B[32m" << "Passed test" << "\033[0m " << test.first << std::endl;
        }else {
            std::cout << "\x1B[31m" << "Failed test" << "\033[0m " << test.first << std::endl;
            break;
        }
    }
}