#include <iostream>

#include <llvm/IR/Verifier.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include "Lexer.h"
#include "Parser.h"
#include "TestRunner.h"

// TODO: currently identifier tokens include literals, iterate over them and turn identifiers into proper literals

void SaveModuleToFile(const std::string &Filename) {
    std::error_code EC;
    // Open the file for writing (text mode)
    llvm::raw_fd_ostream FileStream(Filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    // Print the module to the file stream
    TheModule->print(FileStream, nullptr);

    // Explicitly flush to ensure everything is written
    FileStream.flush();

    llvm::outs() << "Successfully saved IR to " << Filename << "\n";
}

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
        std::cout << "Function " << func->prototype->identifier << std::endl;
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

int main(int argc, char* argv[]) {
    Lexer lexer = Lexer();
    std::string file_path = "";
    const bool run_tests = false;
    if (run_tests) {
        const auto tester = std::make_unique<LexerTestRunner>();
        tester->runLexerTestCases();
        return 0;
    }
    // Running without arguments
    if (argc == 1) {
        file_path = "programs/fn_calls";
    }else {
        file_path = argv[1];
    }
    auto tokens = lexer.parse_file(file_path + ".kld");
    // for (Token token : tokens) {
    //     std::cout << token_to_string(token) << std::endl;
    // }
    std::unique_ptr<ModuleAST> AST = Parser::parse_tokens(tokens);

    print_ast(AST.get());

    Parser::InitializeCodeGen();
    AST->codegen();

    if (llvm::verifyModule(*TheModule, &llvm::errs())) {
        std::cout << "Error: Module is broken!" << std::endl;
    }else {
        std::cout << "Module seems to be okay" << std::endl;
    }
    TheModule->print(llvm::outs(), nullptr);

    SaveModuleToFile(file_path + ".ll");
    return 0;
}
