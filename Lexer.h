//
// Created by jcen on 3.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_LEXER_H
#define LLVM_KALEIDOSCOPE_LEXER_H
#include <vector>

#include "Token.h"

/// Responsible for turning code in a text form into a stream of tokens
class Lexer {
    std::vector<Token> tokens;
    std::string source;
    size_t index = 0;

public:
    std::vector<Token> parse(const std::string& input);
    std::vector<Token> parse_file(const std::string& file_path);
};



#endif //LLVM_KALEIDOSCOPE_LEXER_H
