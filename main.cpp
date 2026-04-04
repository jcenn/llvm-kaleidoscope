#include <iostream>

#include "Lexer.h"
#include "Parser.h"

// TODO: currently identifier tokens include literals, iterate over them and turn identifiers into proper literals

int main() {
    const auto input = "fn main(){}";
    Lexer lexer = Lexer();
    // auto tokens = lexer.parse(input);
    auto tokens = lexer.parse_file("programs/main.asn");
    // for (Token token : tokens) {
    //     std::cout << token_to_string(token.type) << std::endl;
    // }

    Parser parser = Parser();
    ModuleAST* AST = parser.parse_tokens(tokens);
    return 0;
}
