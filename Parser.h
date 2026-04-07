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


class Parser {
public:
    static inline const std::map<std::string, TypeIdentifier> type_identifiers = {
        {"void", TypeIdentifier::VOID},
        {"i32", TypeIdentifier::I32},
    };
    static std::unique_ptr<ModuleAST> parse_tokens(const std::vector<Token>& tokens);
};


#endif //LLVM_KALEIDOSCOPE_PARSER_H
