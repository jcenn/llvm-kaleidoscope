//
// Created by jcen on 3.04.2026.
//

#include "Lexer.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>


bool match_pattern(const std::string& source, const std::string&& pattern) {
    if (source.substr(0, pattern.length()) == pattern) {
        // printf("Found pattern %s \n", pattern.c_str());
        return true;
    }
    return false;
}

std::vector<Token> Lexer::parse(const std::string& input) {
    std::unordered_map<std::string, TokenType> keyword_map;
    keyword_map["fn "] = TokenType::FN;
    keyword_map["let "] = TokenType::LET;
    keyword_map["return "] = TokenType::RETURN;

    this->source = input;
    this->index = 0;
    size_t source_length = input.length();
    tokens.clear();
    // std::vector<Token> tokens{};

    while(index < source_length) {
        while(isspace(source[this->index])) {
            index++;
        }

        if (index >= source_length) break;

        // Check all possible token types

        // Single character operator
        index++;
        switch (source[this->index-1]) {
            case ';':
                tokens.emplace_back(TokenType::SEMICOLON);
                continue;
            case '+':
                tokens.emplace_back(TokenType::PLUS);
                continue;
            case '-':
                tokens.emplace_back(TokenType::MINUS);
                continue;
            case '=':
                tokens.emplace_back(TokenType::EQUALS);
                continue;
            case '(':
                tokens.emplace_back(TokenType::BRACKET_L);
                continue;
            case ')':
                tokens.emplace_back(TokenType::BRACKET_R);
                continue;
            case '{':
                tokens.emplace_back(TokenType::BRACE_L);
                continue;
            case '}':
                tokens.emplace_back(TokenType::BRACE_R);
                continue;
            default:
                index--;
        }

        // Iterate over known keyword patterns
        bool found_keyword = false;
        for (const auto& key_val : keyword_map) {
            if (match_pattern(source.substr(index), std::move(key_val.first))) {
                tokens.emplace_back(key_val.second);
                index += key_val.first.length();
                found_keyword = true;
                break;
            }
        }
        if (found_keyword) continue;

        //Handling identifiers
        // Identifier
        // Could use regex here to get the name from allowed character combination
        // For now we just allow all alphabet characters and '_'
        size_t start_index = this->index;
        size_t count = 0;

        // Characters that can terminate the identifier
        // let x = 5 -> space
        // let x=5 -> '='
        // fn main() -> '('
        constexpr char terminator_characters[] = {' ', ';', '=', '(', '+', '-', '*', '/', '%', 0};
        std::string_view terminators = terminator_characters;

        char c = source[start_index + count];
        while ( !terminators.contains(c)) {
            count++;
            c = source[start_index + count];
        }

        if (count <= 0) {
            throw std::runtime_error("Couldn't parse identifier");
        }

        std::string identifier = source.substr(start_index, count);
        tokens.emplace_back(TokenType::IDENTIFIER, identifier);
        index += identifier.length();
        continue;
    }

    return tokens;
}


std::vector<Token> Lexer::parse_file(const std::string& file_path) {
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Couldn't open provided input file: " + file_path);
    }
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    return this->parse(buffer.str());
}
