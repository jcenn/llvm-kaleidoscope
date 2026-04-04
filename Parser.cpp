//
// Created by jcen on 4.04.2026.
//

#include "Parser.h"



ModuleAST* Parser::parse_tokens(const std::vector<Token> &tokens) {
    auto moduleNode = new ModuleAST(tokens);
    moduleNode->resolve();

    return moduleNode;
}
