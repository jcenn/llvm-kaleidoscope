//
// Created by jcen on 3.04.2026.
//

#include "Lexer.h"

#include <fstream>
#include <iostream>
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
    keyword_map["extern "] = TokenType::EXTERN;
    keyword_map["for "] = TokenType::FOR;

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

        // comments
        if (source[index] == '/' && source[index + 1] == '/') {
            // skip all characters for line that starts with //
            while (index < source_length && source[index] != '\n') {
                index++;
            }
            continue;
        }
        // Check all possible token types

        if (source[index] == '-' && source[index + 1] == '>') {
            tokens.emplace_back(TokenType::ARROW);
            index += 2;
            continue;
        }

        if (source[index] == '=' && source[index + 1] == '=') {
            tokens.emplace_back(TokenType::EQUALS);
            index += 2;
            continue;
        }

        // "if" followed by a space or (
        if (source.substr(index, 2) == "if" && source[index + 2] == ' ' ){
            tokens.emplace_back(TokenType::IF);
            index += 2;
            continue;
        }

        if (source.substr(index, 4) == "else" && (source[index + 4] == ' ' || source[index + 4] == '{')) {
            tokens.emplace_back(TokenType::ELSE);
            index += 4;
            continue;
        }

        // Single character operator
        index++;
        switch (source[this->index-1]) {
            case ';':
                tokens.emplace_back(TokenType::SEMICOLON);
                continue;
            case ',':
                tokens.emplace_back(TokenType::COMMA);
                continue;
            case '+':
                tokens.emplace_back(TokenType::PLUS);
                continue;
            case '-':
                tokens.emplace_back(TokenType::MINUS);
                continue;
            case '*':
                tokens.emplace_back(TokenType::ASTERISK);
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
        constexpr char terminator_characters[] = {' ', ';', ',', '=', '(', ')', '{', '}', '+', '-', '*', '/', '%', 0};
        std::string_view terminators = terminator_characters;

        char c = source[start_index + count];
        while ( !terminators.contains(c)) {
            count++;
            c = source[start_index + count];
        }

        if (count <= 0) {
            throw std::runtime_error("Couldn't parse identifier");
        }

        std::string value = source.substr(start_index, count);

        // TODO: differentiate between identifiers and literals in a different pass
        bool is_literal = true;
        for (const char c : value) {
            if (std::isalpha(c)) {
                is_literal = false;
                break;
            }
        }
        if (is_literal) {
            tokens.emplace_back(TokenType::LITERAL, value);
        }else {
            // TODO: might need more checks or a switch statement to go through keywords
            if (value == "return") {
                tokens.emplace_back(TokenType::RETURN);
            }else {
                tokens.emplace_back(TokenType::IDENTIFIER, value);
            }
        }
        index += value.length();
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
