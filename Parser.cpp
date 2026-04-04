//
// Created by jcen on 4.04.2026.
//

#include "Parser.h"



std::unique_ptr<ModuleAST> Parser::parse_tokens(const std::vector<Token> &tokens) {
    auto moduleNode = std::make_unique<ModuleAST>(tokens);
    moduleNode->resolve();

    return moduleNode;
}
