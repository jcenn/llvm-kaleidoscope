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
    explicit StatementAST() : AST_Node() { };
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
    TypeIdentifier return_type = TypeIdentifier::VOID;
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
        if (lhs->return_type != rhs->return_type) throw std::runtime_error("Binary operand type mismatch");
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
    explicit VariableExpressionAST(const Token& tok, TypeIdentifier var_type) : ExpressionAST(), identifier(tok.value.value())
    {
        this->return_type = var_type;
    }
    ~VariableExpressionAST() override = default;

    // void resolve() override;
    
    llvm::Value* codegen() override;
};

// Leaf node for literals inside expressions
class LiteralExpressionAST : public ExpressionAST {
public:
    std::string value_str;
    explicit LiteralExpressionAST(const Token& tok, const TypeIdentifier ret_type) : ExpressionAST(), value_str(tok.value.value())
    {
        this->return_type = ret_type;
    }
    ~LiteralExpressionAST() override = default;

    // void resolve() override;
    llvm::Value* codegen() override;
};

// Node for conditional statements (if, for)
// codegen should resolve to i1 where 0 -> false, 1 -> true
class BooleanExpressionAST : public ExpressionAST {
public:
    std::unique_ptr<ExpressionAST> inner_expression;

    explicit BooleanExpressionAST( std::unique_ptr<ExpressionAST>&& expr ) : inner_expression(std::move(expr)) { }

    ~BooleanExpressionAST() override = default;

    llvm::Value* codegen() override;
};

// Leaf node for literals inside expressions
class CallExpressionAST : public ExpressionAST {
public:
    std::string callee_identifier;
    std::vector<std::unique_ptr<ExpressionAST>> arg_expressions{};
    explicit CallExpressionAST(const std::string& callee, std::vector<std::unique_ptr<ExpressionAST>> expressions) : callee_identifier(callee)
    {
        this->arg_expressions = std::move(expressions);
    }
    ~CallExpressionAST() override = default;
    llvm::Value* codegen() override;
};

// ex. let x = 2 + 3;
class LetStatementAST : public StatementAST {
public:
    std::string variable_identifier;
    std::unique_ptr<ExpressionAST> expression;
    TypeIdentifier type_hint;

    explicit LetStatementAST(const std::string& ident, std::unique_ptr<ExpressionAST> expr) : variable_identifier(ident), expression(std::move(expr))
    {
        // TODO: figure out var type from expression
        // get_expression_type()? or add to parse_expression'
        type_hint = TypeIdentifier::String;
    };
    explicit LetStatementAST(const std::string& ident, std::unique_ptr<ExpressionAST> expr, TypeIdentifier type_h) : variable_identifier(ident), expression(std::move(expr)), type_hint(type_h) { };
    llvm::Value* codegen() override;
};

class ForStatementAST : public StatementAST {
public:
    std::unique_ptr<BooleanExpressionAST> cond_expression;
    std::vector<std::unique_ptr<StatementAST>> statements;

    explicit ForStatementAST(std::unique_ptr<BooleanExpressionAST> expr, std::vector<std::unique_ptr<StatementAST>> arg_statements) : cond_expression(std::move(expr)), statements(std::move(arg_statements)) { };
    llvm::Value* codegen() override;
};


// call statement (functions returning void)
class CallStatementAST : public StatementAST {
public:
    std::unique_ptr<ExpressionAST> call_expression;

    explicit CallStatementAST(std::unique_ptr<ExpressionAST>&& expression)
    {
        this->call_expression = std::move(expression);
    };
    llvm::Value* codegen() override;
};

class ReturnStatementAST : public StatementAST {
public:
    std::unique_ptr<ExpressionAST> expression = nullptr;

    explicit ReturnStatementAST(std::unique_ptr<ExpressionAST>&& expression)
    {
        this->expression = std::move(expression);
    };
    // void resolve() override;

    llvm::Value* codegen() override;
};


class IfStatementAST : public StatementAST {
public:
    std::unique_ptr<ExpressionAST> condition_expression = nullptr;
    std::vector<std::unique_ptr<StatementAST>> then_statements{};
    std::vector<std::unique_ptr<StatementAST>> else_statements{};

    explicit IfStatementAST(std::unique_ptr<ExpressionAST>&& condition_expr, std::vector<std::unique_ptr<StatementAST>> then)
    {
        this->condition_expression = std::move(condition_expr);
        this->then_statements = std::move(then);
    };
    explicit IfStatementAST(std::unique_ptr<ExpressionAST>&& condition_expr, std::vector<std::unique_ptr<StatementAST>> then, std::vector<std::unique_ptr<StatementAST>> else_stm)
    {
        this->condition_expression = std::move(condition_expr);
        this->then_statements = std::move(then);
        this->else_statements = std::move(else_stm);
    };
    llvm::Value* codegen() override;
};
class AssignmentStatementAST : public StatementAST {
public:
    std::string var_identifier;
    std::unique_ptr<ExpressionAST> rhs_expression{};

    explicit AssignmentStatementAST(const std::string&  ident, std::unique_ptr<ExpressionAST>&& expr) : var_identifier(ident), rhs_expression(std::move(expr)) { };
    llvm::Value* codegen() override;
};

#endif //LLVM_KALEIDOSCOPE_AST_H
