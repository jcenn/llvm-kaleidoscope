//
// Created by jcen on 3.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_TOKEN_H
#define LLVM_KALEIDOSCOPE_TOKEN_H
#include <optional>
#include <stdexcept>
#include <string>

enum class TokenType {
    // --- KEYWORDS ---
    FN, LET, RETURN, EXTERN,

    // SYMBOLS
    COLON, SEMICOLON, COMMA,

    // --- BINARY OPERATORS ---
    EQUALS,
    PLUS, MINUS,

    EXCLAMATION,

    // --- BRACKETS ---
    BRACKET_L, BRACKET_R,               // ( )
    // SQUARE_BRACKET_L, SQUARE_BRACKET_R, // [ ]
    // SHARP_BRACKET_L, SHARP_BRACKET_R,   // < >
    BRACE_L, BRACE_R,                   // { }

    IDENTIFIER,         // variable and function names
    LITERAL             // static values (numbers / strings / booleans / etc.)
};


struct Token {
    TokenType type;
    std::optional<std::string> value;

    explicit Token(const TokenType type) : type(type) {
        this->value = std::nullopt;
    }

    Token(const TokenType type, const std::optional<std::string> &value) : type(type), value(value) { }

};

inline std::string token_to_string(const Token& token) {
    switch (token.type) {
        // --- KEYWORDS ---
        case TokenType::FN:
            return "FN";
        case TokenType::LET:
            return "LET";
        case TokenType::RETURN:
            return "RETURN";

        case TokenType::COLON:
            return "COLON";
        case TokenType::SEMICOLON:
            return "SEMICOLON";

            // --- BINARY OPERATORS ---
        case TokenType::EQUALS:
            return "EQUALS";
        case TokenType::PLUS:
            return "PLUS";
        case TokenType::MINUS:
            return "MINUS";

        case TokenType::EXCLAMATION:
            return "EXCLAMATION";

            // --- BRACKETS ---
        case TokenType::BRACKET_L:
            return "BRACKET_L";
        case TokenType::BRACKET_R:
            return "BRACKET_R";
        case TokenType::BRACE_L:
            return "BRACE_L";
        case TokenType::BRACE_R:
            return "BRACE_R";

        case TokenType::IDENTIFIER:
            return "IDENTIFIER -> " + token.value.value();
        case TokenType::LITERAL:
            return "LITERAL -> " + token.value.value();
        default:
            throw std::runtime_error("Unknown TokenType");
    }
}
#endif //LLVM_KALEIDOSCOPE_TOKEN_H
