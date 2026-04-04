#include <iostream>

#include "Lexer.h"
#include "Parser.h"

// TODO: currently identifier tokens include literals, iterate over them and turn identifiers into proper literals

std::string get_node_name(std::unique_ptr<AST_Node>& node) {
    return "unknown AST node";
}

void print_ast(const AST_Node* node, int depth = 0) {
    for (int i = 0; i < depth; ++i) {
        std::cout << '\t';
    }
    if (auto module = dynamic_cast<const ModuleAST *>(node)) {
        std::cout << "Module" << std::endl;
        for (auto& child : module->declarations) {
            print_ast(child.get(), depth + 1);
        }
        return;
    }
    if (auto func = dynamic_cast<const FunctionAST *>(node)) {
        std::cout << "Function " << func->identifier << std::endl;
        for (auto& child : func->statements) {
            print_ast(child.get(), depth + 1);
        }
        return;
    }
    if (auto statement = dynamic_cast<const LetStatementAST *>(node)) {
        std::cout << "Let Statement for variable "  << statement->LHS_identifier << std::endl;
        print_ast(statement->expression.get(), depth + 1);
        return;
    }
    if (auto statement = dynamic_cast<const ReturnStatementAST *>(node)) {
        std::cout << "Return Statement " << std::endl;
        print_ast(statement->expression.get(), depth + 1);
        return;
    }
    if (auto expr = dynamic_cast<const VariableExpressionAST *>(node)) {
        std::cout << "variable " << expr->identifier <<  std::endl;
        return;
    }
    if (auto expr = dynamic_cast<const LiteralExpressionAST *>(node)) {
        std::cout << "literal " << expr->value_str <<  std::endl;
        return;
    }
    if (auto expr = dynamic_cast<const BinaryExpressionAST *>(node)) {
        std::cout << "Binary expression " <<  std::endl;
        print_ast(expr->lhs.get(), depth + 1);
        print_ast(expr->rhs.get(), depth + 1);
        return;
    }
}

int main() {
    const auto input = "fn main(){}";
    Lexer lexer = Lexer();
    // auto tokens = lexer.parse(input);
    auto tokens = lexer.parse_file("programs/main.asn");
    // for (Token token : tokens) {
    //     std::cout << token_to_string(token.type) << std::endl;
    // }

    std::unique_ptr<AST_Node> AST = Parser::parse_tokens(tokens);
    print_ast(AST.get());
    return 0;
}
