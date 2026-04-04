//
// Created by jcen on 4.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_AST_H
#define LLVM_KALEIDOSCOPE_AST_H
#include <memory>
#include <vector>

#include "Token.h"


class AST_Node {
protected:
    std::vector<Token> tokens;

public:
    virtual ~AST_Node() noexcept = default;

    explicit AST_Node(const std::vector<Token> &tokens) : tokens(tokens) { }
    /// Consumes some of its tokens and passes them forward to new child AST nodes
    virtual void resolve() = 0;
};

/// Top level class for translation units
class ModuleAST : public AST_Node {
    std::vector<std::unique_ptr<AST_Node>> declarations;
public:
    ModuleAST(const std::vector<Token>& tokens) : AST_Node(tokens) { }
    ~ModuleAST() noexcept override = default;
    void resolve() override;
};

class FunctionAST : public AST_Node {
public:
    std::string identifier;
    FunctionAST(const std::vector<Token>& tokens, const std::string& identifier) : AST_Node(tokens) {
        this->identifier = identifier;
    };
    ~FunctionAST() noexcept override = default;
    void resolve() override;
private:
    // std::string params;
    std::vector<std::unique_ptr<AST_Node>> statements;
};

// Abstract class that acts as a common parent for other Expression types
class ExpressionAST : public AST_Node {
public:
    explicit ExpressionAST(const std::vector<Token>& tokens) : AST_Node(tokens) { }
    ~ExpressionAST() override = default;
    // codegen()
};

class BinaryExpressionAST : public ExpressionAST {
private:
    std::unique_ptr<ExpressionAST> lhs;
    std::unique_ptr<ExpressionAST> rhs;

public:
    explicit BinaryExpressionAST(const std::vector<Token>& tokens) : ExpressionAST(tokens) { }
    ~BinaryExpressionAST() override = default;
    void resolve() override;
};

// Leaf node for variable references inside expressions
class VariableExpressionAST : public ExpressionAST {
private:
    std::string identifier;
public:
    explicit VariableExpressionAST(const std::vector<Token> &tokens) : ExpressionAST(tokens) { }
    ~VariableExpressionAST() override = default;

    void resolve() override;
};

// Leaf node for literals inside expressions
class LiteralExpressionAST : public ExpressionAST {
private:
    std::string value_str;
public:
    explicit LiteralExpressionAST(const std::vector<Token> &tokens) : ExpressionAST(tokens) { }
    ~LiteralExpressionAST() override = default;

    void resolve() override;
};
// assignment operations, if statements, return statements
class StatementAST : public AST_Node {
public:
    explicit StatementAST(const std::vector<Token>& tokens) : AST_Node(tokens) { };
    ~StatementAST() noexcept override = default;
    void resolve() override = 0;
};

// ex. let x = 2 + 3;
class LetStatementAST : public StatementAST {
private:
    std::string LHS_identifier;
    std::unique_ptr<ExpressionAST> expression;

public:
    explicit LetStatementAST(const std::vector<Token>& tokens) : StatementAST(tokens) { };
    void resolve() override;
};

class ReturnStatementAST : public StatementAST {
private:
    std::unique_ptr<ExpressionAST> expression;

public:
    explicit ReturnStatementAST(const std::vector<Token>& tokens) : StatementAST(tokens) { };
    void resolve() override;
};



#endif //LLVM_KALEIDOSCOPE_AST_H
