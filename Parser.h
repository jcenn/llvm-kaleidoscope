//
// Created by jcen on 4.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_PARSER_H
#define LLVM_KALEIDOSCOPE_PARSER_H
#include <vector>

#include "Token.h"

class AST_Node {
protected:
    std::vector<Token> tokens;

public:
    virtual ~AST_Node() = default;

    explicit AST_Node(const std::vector<Token> &tokens) : tokens(tokens) { }
    /// Consumes some of its tokens and passes them forward to new child AST nodes
    virtual void resolve() = 0;
};

/// Top level class for translation units
class ModuleAST : public AST_Node {
    std::vector<AST_Node*> declarations;
public:
    ModuleAST(const std::vector<Token>& tokens) : AST_Node(tokens) { }
    ~ModuleAST() override = default;
    void resolve() override;
};

class FunctionAST : public AST_Node {
public:
    std::string identifier;
    FunctionAST(const std::vector<Token>& tokens, const std::string& identifier) : AST_Node(tokens) {
        this->identifier = identifier;
    };
    ~FunctionAST() noexcept override;
    void resolve() override;
private:
    // std::string params;
    std::vector<AST_Node*> statements;
};

// function calls, assignments, if statements
class StatementAST : public AST_Node {
public:
    explicit StatementAST(const std::vector<Token>& tokens) : AST_Node(tokens) { };
    ~StatementAST() override = default;
    void resolve() override;
};

class Parser {
public:
    ModuleAST* parse_tokens(const std::vector<Token>& tokens);
};


#endif //LLVM_KALEIDOSCOPE_PARSER_H
