//
// Created by jcen on 4.04.2026.
//

#ifndef LLVM_KALEIDOSCOPE_AST_H
#define LLVM_KALEIDOSCOPE_AST_H
#include <map>
#include <memory>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"

#include "Token.h"


extern std::unique_ptr<llvm::LLVMContext> Context;

// The Builder object is a helper object that makes it easy to generate LLVM instructions. Instances of the IRBuilder class template keep track of the current place to insert instructions and has methods to create new instructions.
extern std::unique_ptr<llvm::IRBuilder<>> Builder;

// TheModule is an LLVM construct that contains functions and global variables. In many ways, it is the top-level structure that the LLVM IR uses to contain code. It will own the memory for all of the IR that we generate, which is why the codegen() method returns a raw Value*, rather than a unique_ptr<Value>.
extern std::unique_ptr<llvm::Module> TheModule;

// The NamedValues map keeps track of which values are defined in the current scope and what their LLVM representation is. (In other words, it is a symbol table for the code).
extern std::map<std::string, llvm::Value *> NamedValues;
extern void InitializeCodeGen();

enum class BinaryOperator {
    Add,
    Subtract,
    // Multiply,
    // Divide,
    // Modulus,
    // Equals,
    // NotEquals,
    // LessThan,
    // GreaterThan,
    // LessThanOrEquals,
    // GreaterThanOrEquals,
};

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
public:
    std::vector<std::unique_ptr<AST_Node>> declarations;
    explicit ModuleAST(const std::vector<Token>& tokens) : AST_Node(tokens) { }
    ~ModuleAST() noexcept override = default;
    void resolve() override;
    void codegen();
};

// For function declarations (name + args) used either for module functions or imported ones
class PrototypeAST : public AST_Node {
public:
    std::string identifier;
    std::vector<std::string> arg_identifiers;
    PrototypeAST(const std::string& identifier, std::vector<std::string> args) : AST_Node({}) {
        this->identifier = identifier;
        this->arg_identifiers = args;
    };
    ~PrototypeAST() noexcept override = default;
    void resolve() override {};
    llvm::Function* codegen();
};

// assignment operations, if statements, return statements
class StatementAST : public AST_Node {
public:
    explicit StatementAST(const std::vector<Token>& tokens) : AST_Node(tokens) { };
    ~StatementAST() noexcept override = default;
    void resolve() override = 0;
    virtual llvm::Value* codegen() = 0;
};


class FunctionAST : public AST_Node {
public:
    std::vector<std::unique_ptr<StatementAST>> statements;
    std::unique_ptr<PrototypeAST> prototype;
    // std::string params;
    // std::string identifier;

    FunctionAST(const std::vector<Token>& tokens, std::string identifier, std::vector<std::string> args) : AST_Node(tokens) {
        prototype = std::make_unique<PrototypeAST>(identifier, args);
    };
    ~FunctionAST() noexcept override = default;
    void resolve() override;
    llvm::Value* codegen();
};

// Abstract class that acts as a common parent for other Expression types
class ExpressionAST : public AST_Node {
public:
    explicit ExpressionAST(const std::vector<Token>& tokens) : AST_Node(tokens) { }
    ~ExpressionAST() override = default;
    virtual void resolve() override = 0;
    virtual llvm::Value* codegen() = 0;
};

class BinaryExpressionAST : public ExpressionAST {
public:
    BinaryOperator operator_;
    std::unique_ptr<ExpressionAST> lhs;
    std::unique_ptr<ExpressionAST> rhs;
    explicit BinaryExpressionAST(const std::vector<Token>& tokens) : ExpressionAST(tokens) { }
    ~BinaryExpressionAST() override = default;
    void resolve() override;

    llvm::Value * codegen() override;
};

// Leaf node for variable references inside expressions
class VariableExpressionAST : public ExpressionAST {
public:
    std::string identifier;
    explicit VariableExpressionAST(const std::vector<Token> &tokens) : ExpressionAST(tokens) { }
    ~VariableExpressionAST() override = default;

    void resolve() override;
    
    llvm::Value* codegen() override;
};

// Leaf node for literals inside expressions
class LiteralExpressionAST : public ExpressionAST {
public:
    std::string value_str;
    explicit LiteralExpressionAST(const std::vector<Token> &tokens) : ExpressionAST(tokens) { }
    ~LiteralExpressionAST() override = default;

    void resolve() override;
    llvm::Value* codegen() override;
};

// Leaf node for literals inside expressions
class CallExpressionAST : public ExpressionAST {
public:
    std::string calee_identifier;
    std::vector<std::unique_ptr<ExpressionAST>> arg_expressions{};
    explicit CallExpressionAST(const std::vector<Token> &tokens) : ExpressionAST(tokens) { }
    ~CallExpressionAST() override = default;

    void resolve() override;
    llvm::Value* codegen() override;
};

// ex. let x = 2 + 3;
class LetStatementAST : public StatementAST {
public:
    std::string LHS_identifier;
    std::unique_ptr<ExpressionAST> expression;

    explicit LetStatementAST(const std::vector<Token>& tokens) : StatementAST(tokens) { };
    void resolve() override;

    llvm::Value* codegen() override;
};

class ReturnStatementAST : public StatementAST {
public:
    std::unique_ptr<ExpressionAST> expression;

    explicit ReturnStatementAST(const std::vector<Token>& tokens) : StatementAST(tokens) { };
    void resolve() override;

    llvm::Value* codegen() override;
};



#endif //LLVM_KALEIDOSCOPE_AST_H
