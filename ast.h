//
// Created by jcen on 4.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_AST_H
#define LLVM_KALEIDOSCOPE_AST_H
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"

#include "Token.h"
#include "Types.h"


// Provided by Parser.cpp
extern std::unique_ptr<llvm::LLVMContext> Context;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::map<std::string, llvm::Value *> NamedValues;


class AST_Node {
protected:
    //const std::vector<Token> tokens;

public:
    virtual ~AST_Node() noexcept = default;

    explicit AST_Node() { }
    /// Consumes some of its tokens and passes them forward to new child AST nodes
    // virtual void resolve() = 0;
};

/// Top level class for translation units
class ModuleAST : public AST_Node {
public:
    std::vector<std::unique_ptr<AST_Node>> declarations;
    explicit ModuleAST(const std::vector<Token>& tokens) : AST_Node() { }
    ~ModuleAST() noexcept override = default;
    // void resolve() override;
    void codegen();
};

// For function declarations (name + args + return type) used either for module functions or imported ones
class PrototypeAST : public AST_Node {
public:
    // Assume function returns void unless specified otherwise
    TypeIdentifier ret_type = TypeIdentifier::VOID;
    std::string identifier;
    std::vector<std::pair<std::string, TypeIdentifier>> args{};
    PrototypeAST(const std::string& identifier, std::vector<std::pair<std::string, TypeIdentifier>> args, TypeIdentifier return_type) : AST_Node() {
        this->identifier = identifier;
        this->args = std::move(args);
        this->ret_type = return_type;
    };
    ~PrototypeAST() noexcept override = default;
    // void resolve() override {};
    llvm::Function* codegen();
};

// assignment operations, if statements, return statements
class StatementAST : public AST_Node {
public:
    explicit StatementAST(const std::vector<Token>& tokens) : AST_Node() { };
    ~StatementAST() noexcept override = default;
    // void resolve() override = 0;
    virtual llvm::Value* codegen() = 0;
};


class FunctionAST : public AST_Node {
public:
    std::vector<std::unique_ptr<StatementAST>> statements;
    std::unique_ptr<PrototypeAST> prototype;

    FunctionAST(std::vector<std::unique_ptr<StatementAST>>&& statements, std::unique_ptr<PrototypeAST> proto) : AST_Node() {
        this->prototype = std::move(proto);
        this->statements = std::move(statements);
    };
    ~FunctionAST() noexcept override = default;
    llvm::Value* codegen();
};

// Abstract class that acts as a common parent for other Expression types
class ExpressionAST : public AST_Node {
public:
    explicit ExpressionAST() : AST_Node() { }
    ~ExpressionAST() override = default;
    // void resolve() override = 0;
    virtual llvm::Value* codegen() = 0;
};

class BinaryExpressionAST : public ExpressionAST {
public:
    BinaryOperator operator_;
    std::unique_ptr<ExpressionAST> lhs;
    std::unique_ptr<ExpressionAST> rhs;
    explicit BinaryExpressionAST(
        BinaryOperator operator_,
        std::unique_ptr<ExpressionAST>&& lhs,
        std::unique_ptr<ExpressionAST>&& rhs
    ) : operator_(operator_)
    {
        this->lhs = std::move(lhs);
        this->rhs = std::move(rhs);
    }

    ~BinaryExpressionAST() override = default;
    // void resolve() override;

    llvm::Value * codegen() override;
};

// Leaf node for variable references inside expressions
class VariableExpressionAST : public ExpressionAST {
public:
    std::string identifier;
    explicit VariableExpressionAST(const Token& tok) : ExpressionAST(), identifier(tok.value.value()) { }
    ~VariableExpressionAST() override = default;

    // void resolve() override;
    
    llvm::Value* codegen() override;
};

// Leaf node for literals inside expressions
class LiteralExpressionAST : public ExpressionAST {
public:
    std::string value_str;
    explicit LiteralExpressionAST(const Token& tok) : ExpressionAST(), value_str(tok.value.value()) { }
    ~LiteralExpressionAST() override = default;

    // void resolve() override;
    llvm::Value* codegen() override;
};

// Leaf node for literals inside expressions
class CallExpressionAST : public ExpressionAST {
public:
    std::string calee_identifier;
    std::vector<std::unique_ptr<ExpressionAST>> arg_expressions{};
    explicit CallExpressionAST() : ExpressionAST() { }
    ~CallExpressionAST() override = default;

    // void resolve() override;
    llvm::Value* codegen() override;
};

// ex. let x = 2 + 3;
class LetStatementAST : public StatementAST {
public:
    std::string LHS_identifier;
    std::unique_ptr<ExpressionAST> expression;

    explicit LetStatementAST(const std::vector<Token>& tokens) : StatementAST(tokens) { };
    // void resolve() override;

    llvm::Value* codegen() override;
};

// call statement (functions returning void)
class CallStatementAST : public StatementAST {
public:
    std::unique_ptr<ExpressionAST> call_expression;

    explicit CallStatementAST(const std::vector<Token>& tokens) : StatementAST(tokens) { };
    // void resolve() override;

    llvm::Value* codegen() override;
};

class ReturnStatementAST : public StatementAST {
public:
    std::unique_ptr<ExpressionAST> expression = nullptr;

    explicit ReturnStatementAST(std::unique_ptr<ExpressionAST>&& expression) : StatementAST({})
    {
        this->expression = std::move(expression);
    };
    // void resolve() override;

    llvm::Value* codegen() override;
};

#endif //LLVM_KALEIDOSCOPE_AST_H
