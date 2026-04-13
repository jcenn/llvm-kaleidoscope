//

#include "Parser.h"

#include <stack>

// From kaleidoscope tutorial:
// TheContext is an opaque object that owns a lot of core LLVM data structures, such as the type and constant value tables
std::unique_ptr<llvm::LLVMContext> Context;

// The Builder object is a helper object that makes it easy to generate LLVM instructions. Instances of the IRBuilder class template keep track of the current place to insert instructions and has methods to create new instructions.
std::unique_ptr<llvm::IRBuilder<>> Builder;

// TheModule is an LLVM construct that contains functions and global variables. In many ways, it is the top-level structure that the LLVM IR uses to contain code. It will own the memory for all the IR that we generate, which is why the codegen() method returns a raw Value*, rather than a unique_ptr<Value>.
std::unique_ptr<llvm::Module> TheModule;

// The NamedValues map keeps track of which values are defined in the current scope and what their LLVM representation is. (In other words, it is a symbol table for the code).
std::map<std::string, llvm::Value *> NamedValues;

std::map<BinaryOperator, int> bin_op_precedence{
        {BinaryOperator::Add, 1},
        {BinaryOperator::Subtract, 1},
        {BinaryOperator::Multiply, 10},

        // these operators turn the whole expression into a booleanExp
        {BinaryOperator::CompareEQ, 100}
};

void Parser::InitializeCodeGen() {
    Context = std::make_unique<llvm::LLVMContext>();
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    TheModule = std::make_unique<llvm::Module>("main module", *Context);
}

std::unique_ptr<ModuleAST> Parser::parse_tokens(const std::vector<Token> &tokens) {
    auto moduleNode = std::make_unique<ModuleAST>(tokens);
    // iterate over module tokens splitting them into top-level statements like extern statements function declarations, global variables
    // moduleNode->resolve();

    if (tokens.empty()) {
        throw std::logic_error("ModuleAST::resolve(): empty tokens list");
    }

    // Consume tokens in a loop converting them to AST nodes
    size_t i{};
    while (i < tokens.size()) {
        const auto& token = tokens[i];

        if (token.type == TokenType::SEMICOLON)
        {
            i++;
            continue;
        }

        // check the received token, parse its expression and increment the index to point at the next statement's opening token
        if (token.type == TokenType::FN) {
            // find closing brace to know where functions declaration ends
            size_t tmp_i = i;
            while (tokens[tmp_i].type != TokenType::BRACE_L) tmp_i++;
            size_t close_brace_i = find_matching_token_index(tokens, tmp_i, TokenType::BRACE_L, TokenType::BRACE_R);
            auto token_count = close_brace_i - i + 1;
            auto fn_expr = parse_function_def(std::span(tokens).subspan(i, token_count));
            i += token_count;
            moduleNode->declarations.push_back(std::move(fn_expr));
            continue;
        }
        if (token.type == TokenType::EXTERN) {
            auto semi_colon_i = i;
            while (semi_colon_i < tokens.size() && tokens.at(semi_colon_i).type != TokenType::SEMICOLON) semi_colon_i++;
            auto token_slice = std::vector<Token>{tokens.begin() + i + 1, tokens.begin() + semi_colon_i};
            auto proto_tokens = std::span(token_slice);
            auto proto = Parser::parse_prototype(proto_tokens);

            // std::cout << "Found external function " << proto->identifier << std::endl;
            i += proto_tokens.size() + 1;
            moduleNode->declarations.push_back(std::move(proto));
            continue;
        }


        throw std::runtime_error("parsing error for module: found token " + token_to_string(token));
    }
    return moduleNode;
}


// Parsing function definitions => signature + body
std::unique_ptr<FunctionAST> Parser::parse_function_def(std::span<const Token> tokens) {
    // find closing brace -> last token
    // create AST Node
    // return node and number of consumed tokens (reference parameter?) or just consume them from reference
    // tokens = [fn, identifier, (, args..., ), '->', 'type_ident', {, ..., }]

    std::unique_ptr<FunctionAST> functionAST = nullptr;

    size_t open_brace_i = 0;
    // find the opening and closing brace
    while (tokens[open_brace_i].type != TokenType::BRACE_L) open_brace_i++;
    size_t close_brace_i =  find_matching_token_index({tokens.begin(), tokens.end()}, open_brace_i, TokenType::BRACE_L, TokenType::BRACE_R);
    const auto body_token_count = close_brace_i - open_brace_i -1;
    auto proto_tokens = std::span(tokens.begin() + 1, tokens.begin() + open_brace_i);
    // Parses the prototype (identifier + arguments) and
    auto prototype = Parser::parse_prototype( proto_tokens );


    // tokens [open_brace_i : close_brace_i] contains function body + braces
    // Empty function
    if (body_token_count == 0) {
        functionAST = std::make_unique<FunctionAST>(std::vector<std::unique_ptr<StatementAST>>{}, std::move(prototype));
    }else {
        auto body_tokens = tokens.subspan(open_brace_i + 1, body_token_count);
        auto statements = parse_function_body(body_tokens);
        functionAST = std::make_unique<FunctionAST>(std::move(statements), std::move(prototype));
    }

    return std::move(functionAST);
}

// expects tokens to only contain the content between function parentheses ex. for `foo(a, b) tokens = [a, ',', b]
std::vector<std::pair<std::string, TypeIdentifier>> Parser::parse_function_parameters(std::span<const Token> tokens) {
    // Parse function arguments
    auto arg_identifiers = std::vector<std::pair<std::string, TypeIdentifier>>();
    size_t arg_index = 0;

    while (arg_index < tokens.size()) {
        auto tok = tokens.at(arg_index);
        if (tok.type == TokenType::IDENTIFIER) {
            // TODO: handle different types
            arg_identifiers.emplace_back(tok.value.value(), TypeIdentifier::I32);
        }
        arg_index++;
    }
    return arg_identifiers;
}

std::vector<std::unique_ptr<StatementAST>> Parser::parse_function_body(std::span<const Token> tokens)
{
    std::vector<std::unique_ptr<StatementAST>> statements{};
    // iterate over tokens, split them into statements and call resolve on them
    // std::cout << "Trying to resolve function " << this->prototype->identifier << std::endl;
    size_t statement_start = 0;
    size_t token_count = 0;
    while (statement_start + token_count < tokens.size()) {
        auto const& tok = tokens.at(statement_start + token_count);

        // Handle if/else statements
        if (tok.type == TokenType::IF)
        {
            statement_start = statement_start + token_count;
            token_count = 0;
            size_t i = statement_start;
            while (tokens.at(i).type != TokenType::BRACE_L) i++;

            auto if_close_brace_i = find_matching_token_index({tokens.begin(), tokens.end()}, i, TokenType::BRACE_L, TokenType::BRACE_R);
            if (tokens.at(if_close_brace_i+1).type == TokenType::ELSE)
            {
                i = if_close_brace_i + 2;
                if_close_brace_i = find_matching_token_index({tokens.begin(), tokens.end()}, i, TokenType::BRACE_L, TokenType::BRACE_R);
            }
            auto if_else_tokens = std::vector<Token>{tokens.begin() + statement_start, tokens.begin() + if_close_brace_i + 1};
            statements.push_back(parse_statement(std::span<const Token>(if_else_tokens)));
            statement_start += if_else_tokens.size();
            token_count = 0;
            continue;
        }

        // handle for statements
        if (tok.type == TokenType::FOR)
        {
            statement_start = statement_start + token_count;
            token_count = 0;
            size_t i = statement_start;
            while (tokens.at(i).type != TokenType::BRACE_L) i++;

            auto for_close_brace_i = find_matching_token_index({tokens.begin(), tokens.end()}, i, TokenType::BRACE_L, TokenType::BRACE_R);
            if (tokens.at(for_close_brace_i+1).type == TokenType::ELSE)
            {
                i = for_close_brace_i + 2;
                for_close_brace_i = find_matching_token_index({tokens.begin(), tokens.end()}, i, TokenType::BRACE_L, TokenType::BRACE_R);
            }
            auto for_tokens = std::vector<Token>{tokens.begin() + statement_start, tokens.begin() + for_close_brace_i + 1};
            statements.push_back(parse_statement(std::span<const Token>(for_tokens)));
            statement_start += for_tokens.size();
            token_count = 0;
            continue;
        }
        if (tok.type != TokenType::SEMICOLON) {
            token_count++;
            continue;
        };
        // when we hit a semicolon we turn all tokens until that semicolon into a statement
        auto const statement_tokens = tokens.subspan(statement_start, token_count); // already excludes the semicolon

        statements.push_back(parse_statement(statement_tokens));
        if (statement_start + token_count >= tokens.size()) break;
        statement_start += token_count + 1; // skip ; and point at the first token of the next statement
        token_count = 1;
    }
    return statements;
}

std::unique_ptr<PrototypeAST> Parser::parse_prototype(std::span<const Token> tokens) {
    auto const& identifier_token = tokens.at(0);
    auto ret_type = TypeIdentifier::VOID;

    if (! identifier_token.value) {
        throw std::logic_error("FunctionAST::parse_function(): empty identifier");
    }

    auto arg_index = 2;
    // open bracket is always on the 3rd token
    // fn foo(...)
    // extern foo(...)
    size_t open_paren_i = 1;
    auto close_paren_i = Parser::find_matching_token_index({tokens.begin(), tokens.end()}, open_paren_i, TokenType::BRACKET_L, TokenType::BRACKET_R);
    // find return type identifier
    // TODO: will not work for more complex types like -> (i32, i32)
    // TODO: something like `Parser::parse_type_expression(tokens, close_paren_i + 1)`
    if (tokens.size() > close_paren_i + 1 &&  tokens.at(close_paren_i + 1).type == TokenType::ARROW && tokens.at(close_paren_i + 2).type == TokenType::IDENTIFIER) {
        auto type_ident = tokens.at(close_paren_i + 2).value.value();
        if (!Parser::type_identifiers.contains(type_ident)) {
            throw std::runtime_error("Type identifier " + type_ident + " is not recognized by parser");
        }
        ret_type = Parser::type_identifiers.at(type_ident);
    }

    // main has inferred type
    if (identifier_token.value.value() == "main") {
        ret_type = TypeIdentifier::I32;
    }
    auto arg_tokens = std::span(tokens).subspan(open_paren_i + 1, close_paren_i - open_paren_i - 1);
    //auto arg_tokens = std::vector(tokens.begin() + open_paren_i + 1, tokens.begin() + close_paren_i);
    auto arg_identifiers = Parser::parse_function_parameters( arg_tokens );
    return std::move(std::make_unique<PrototypeAST>(identifier_token.value.value(), arg_identifiers, ret_type));
}


std::unique_ptr<StatementAST> Parser::parse_statement(std::span<const Token> tokens, bool top_level) {
    std::unique_ptr<StatementAST> statement;
    switch (tokens.front().type) {
        // TODO: fix this shiet
        case TokenType::LET: {
            statement = parse_let_statement({tokens.begin(), tokens.end()});
            break;
        }
        case TokenType::RETURN:
            statement = parse_return_statement(tokens);
            break;
        case TokenType::IDENTIFIER:
            {
                // Call statement like InitWindow();
                if (tokens.at(1).type == TokenType::BRACKET_L)
                {
                    statement = parse_call_statement(tokens);
                    break;
                }
                // TODO: handle statements like a++, a += 2; a = 3;
                if (tokens.at(1).type == TokenType::ASSIGNMENT)
                {
                    statement = parse_assignment_statement({tokens.begin(), tokens.end()});
                    break;
                }
            }
        case TokenType::IF:
            statement = parse_if_statement(tokens);
            break;
        case TokenType::FOR:
            statement = parse_for_statement({tokens.begin(), tokens.end()});
            break;
        default:
            throw std::runtime_error("Statement not recognized");
    }
    return std::move(statement);
}

std::unique_ptr<ReturnStatementAST> Parser::parse_return_statement(std::span<const Token> tokens)
{
    auto expression_tokens = tokens.subspan(1, tokens.size()-1);

    // empty return (void)
    if (expression_tokens.empty())
    {
        return std::make_unique<ReturnStatementAST>(nullptr);
    }
    auto return_statement = std::make_unique<ReturnStatementAST>(parse_expression(expression_tokens));

    return std::move(return_statement);
}

std::unique_ptr<CallStatementAST> Parser::parse_call_statement(std::span<const Token> tokens)
{
    auto expression_tokens = tokens.subspan(0, tokens.size());
    auto expr = parse_expression(expression_tokens);
    if (!dynamic_cast<CallExpressionAST*>(expr.get()))
    {
        throw std::logic_error("Call Expression expected");
    }
    auto statement = std::make_unique<CallStatementAST>(std::move(expr));
    return std::move(statement);
}

// if <condition expression> {
//      statements...
// }
// (optional) else {
//      statements...
// }
std::unique_ptr<IfStatementAST> Parser::parse_if_statement(std::span<const Token> tokens)
{
    if (tokens.empty() || tokens.front().type != TokenType::IF)
    {
        throw std::logic_error("If Statement expected received");
    }

    size_t open_brace_i = 0;
    while (tokens.at(open_brace_i).type != TokenType::BRACE_L) open_brace_i++;

    auto close_brace_i = find_matching_token_index({tokens.begin(), tokens.end()}, open_brace_i, TokenType::BRACE_L, TokenType::BRACE_R);
    auto inner_exp = parse_expression(std::span<const Token>({tokens.begin() + 1, tokens.begin() + open_brace_i}));
    auto condition_expr = std::make_unique<BooleanExpressionAST>(std::move(inner_exp));
    auto statements = std::vector<std::unique_ptr<StatementAST>>{};
    int i = open_brace_i + 1;
    int statement_start_i = i;
    while (i < close_brace_i)
    {
        if (tokens.at(i).type == TokenType::SEMICOLON)
        {
            statements.push_back(std::move(parse_statement(std::span<const Token>({tokens.begin() + statement_start_i, tokens.begin() + i}))));
            i += 1;
            statement_start_i = i;
            continue;
        }
        i++;
    }
    // no more tokens to check ( else statement not present )
    if (close_brace_i >= tokens.size()-1)
    {
        return std::make_unique<IfStatementAST>(std::move(condition_expr), std::move(statements));
    }

    // more tokens to check ( else statement )
    if (tokens.at(close_brace_i + 1).type != TokenType::ELSE)
    {
        throw std::logic_error("Else statement expected");
    }

    open_brace_i = close_brace_i + 2;
    close_brace_i = find_matching_token_index({tokens.begin(), tokens.end()}, open_brace_i, TokenType::BRACE_L, TokenType::BRACE_R);


    auto else_statements = std::vector<std::unique_ptr<StatementAST>>{};
    i = open_brace_i + 1;
    statement_start_i = i;
    while (i < close_brace_i)
    {
        if (tokens.at(i).type == TokenType::SEMICOLON)
        {
            else_statements.push_back(std::move(parse_statement(std::span<const Token>({tokens.begin() + statement_start_i, tokens.begin() + i}))));
            i += 1;
            statement_start_i = i;
            continue;
        }
        i++;
    }

    return std::make_unique<IfStatementAST>(std::move(condition_expr), std::move(statements), std::move(else_statements));
}

std::unique_ptr<LetStatementAST> Parser::parse_let_statement(const std::vector<Token>& tokens)
{
    if (tokens.front().type != TokenType::LET)
    {
        throw std::logic_error("Let statement expected");
    }

    auto ident_str = tokens.at(1).value.value();
    // +3 -> skip 'let', identifier and '='
    auto expression = parse_expression({tokens.begin() + 3, tokens.end()});
    return std::move(std::make_unique<LetStatementAST>(ident_str, std::move(expression)));
}

std::unique_ptr<ForStatementAST> Parser::parse_for_statement(const std::vector<Token>& tokens)
{
    auto open_brace_i = 0;
    while (tokens.at(open_brace_i).type != TokenType::BRACE_L) open_brace_i++;
    auto close_brace_i = find_matching_token_index(tokens, open_brace_i, TokenType::BRACE_L, TokenType::BRACE_R);

    auto source_expr = parse_expression({tokens.begin() + 1, tokens.begin() + open_brace_i});

    auto condition = std::make_unique<BooleanExpressionAST>(std::move(source_expr));
    auto statements = parse_function_body({tokens.begin() + open_brace_i + 1, tokens.begin() + close_brace_i});
    return std::move(std::make_unique<ForStatementAST>(std::move(condition), std::move(statements)));
}

std::unique_ptr<AssignmentStatementAST> Parser::parse_assignment_statement(const std::vector<Token>& tokens)
{
    // tokens: [ident, =, expression]
    auto ident_str = tokens.at(0).value.value();
    auto expr = parse_expression({tokens.begin() + 2, tokens.end()});
    return std::move(std::make_unique<AssignmentStatementAST>(ident_str, std::move(expr)));
}

std::unique_ptr<ExpressionAST> Parser::parse_expression(std::span<const Token> tokens)
{
    std::unique_ptr<ExpressionAST> expression{};

    // check paren expressions first - [(, expr, )]
    if (tokens.front().type == TokenType::BRACKET_L)
    {
        auto close_paren_i = find_matching_token_index({tokens.begin(), tokens.end()}, 0, TokenType::BRACKET_L, TokenType::BRACKET_R);
        // whole expression is wrapped with ( )
        if (close_paren_i == tokens.size()-1)
        {
            return std::move(parse_expression(tokens.subspan(1, tokens.size()-2))); // -2 to exclude both ( and )
        }
    }

    // if tokens.length() == 1 -> identifier or literal
    if (tokens.empty()) throw std::runtime_error("Tried to parse an empty expression");

    // variable / literal references
    if (tokens.size() == 1) {
        std::unique_ptr<ExpressionAST> expr;
        if (tokens.front().type == TokenType::LITERAL) {

            expr = std::make_unique<LiteralExpressionAST>(tokens.front());
        }else if (tokens.front().type == TokenType::IDENTIFIER) {
            expr = std::make_unique<VariableExpressionAST>(tokens.front());
        }else {
            throw std::runtime_error("Tried to parse an expression with invalid identifier");
        }
        return std::move(expr);
    }

    // [identifier, '(', ...] start of a function call
    if ((tokens.front().type == TokenType::IDENTIFIER) && tokens.at(1).type == TokenType::BRACKET_L) {
        // We need to check if we're parsing a single paren expression like `foo(a + b) and not  foo() + foo()
        size_t end_paren_i = find_matching_token_index({tokens.begin(), tokens.end()}, 1, TokenType::BRACKET_L, TokenType::BRACKET_R);

        // Whole expression is a function call.
        // otherwise we go out of the if statement and continue trying to identify the expression type
        if (end_paren_i == tokens.size()-1) {
            auto expr = parse_call_expression(tokens.subspan(0, tokens.size()));
            return std::move(expr);
        }
    }

    // check if it's a binary expression
    expression = parse_binary_expression(tokens);
    if (expression != nullptr)
    {
        return std::move(expression);
    }

    throw std::runtime_error("Tried to parse an invalid expression");
}

std::unique_ptr<ExpressionAST> Parser::parse_binary_expression(std::span<const Token> tokens)
{
    // NOTE: We should actually start splitting sub-expressions from right to left
    // If we go from left to right an expression like 1 - 2 + 3 will evaluate to
    // 1 -
    //      2 + 3
    // and then to
    // 1 - 5
    // which is the opposite of what we want

    // look for a binary expression operator (+, -, /, etc.)
    bool binary_exp = false;
    size_t operator_i{};
    size_t found_operator_i{};

    // We split by lowest precedence so that the higher precedence operations stay together
    // like 2 + 3 * 3 => (2) + (3 * 3)
    int lowest_precedence = INT_MAX;
    while (operator_i < tokens.size()) {
        const auto& token = tokens.at(operator_i);

        // we hit a function call or paren expression so we want to go around it
        if (token.type == TokenType::BRACKET_L)
        {
            operator_i = find_matching_token_index({tokens.begin(), tokens.end()}, operator_i, TokenType::BRACKET_L, TokenType::BRACKET_R) + 1;
            continue;
        }

        if (binary_operators.contains(token.type)) {
            auto op = binary_operators.at(token.type);
            auto op_precedence = bin_op_precedence.at(op);
            if (op_precedence < lowest_precedence)
            {
                found_operator_i = operator_i;
                lowest_precedence = op_precedence;
            }
            binary_exp = true;
        }
        operator_i++;
    }
    if (binary_exp) {
        auto lhs_tokens = tokens.subspan(0, found_operator_i);
        auto rhs_tokens = tokens.subspan(found_operator_i + 1, tokens.size() - (found_operator_i + 1));
        BinaryOperator bin_operator = binary_operators.at(tokens.at(found_operator_i).type);
        auto expr = std::make_unique<BinaryExpressionAST>(
            bin_operator,
            parse_expression(lhs_tokens),
            parse_expression(rhs_tokens)
        );
        return expr;
    }
    return nullptr;
}

std::unique_ptr<ExpressionAST> Parser::parse_call_expression(std::span<const Token> tokens)
{
    // expects tokens like [ foo(bar, baz) ]
    if (tokens.size() < 3) {
        throw std::logic_error("Tried to create a call expression with less than 3 tokens");
    }
    if (tokens.front().type != TokenType::IDENTIFIER) {
        throw std::logic_error("Tried to create a call expression with no identifier");
    }

    std::unique_ptr<CallExpressionAST> expr;

    auto callee_identifier = tokens.front().value.value();
    auto arg_expressions = std::vector<std::unique_ptr<ExpressionAST>>{};
    // Function with no arguments ex. foo();
    if (tokens.size() == 3) {
        expr = std::make_unique<CallExpressionAST>(callee_identifier, std::vector<std::unique_ptr<ExpressionAST>>{});
        return std::move(expr);
    }

    size_t arg_start = 2;
    size_t i = arg_start;

    // iterate over tokens until we hit closing parentheses and pass any found arguments to an expression AST node
    // TODO: could cause problems when passing other function calls ex. foo(1, bar(2, 3)) as it could consider the , inside bar to be a terminator
    // omit function identifier, open paren and close paren
    auto arg_source_tokens = std::vector<Token>(tokens.begin() + 2, tokens.end() - 1);
    auto arg_tokens = Parser::get_function_arg_tokens(arg_source_tokens);

    for (auto& toks : arg_tokens) {
        auto expression = parse_expression(toks);
        arg_expressions.push_back(std::move(expression));
    }
    expr = std::make_unique<CallExpressionAST>(callee_identifier, std::move(arg_expressions));
    return std::move(expr);
}

size_t Parser::find_matching_token_index(const std::vector<Token> &tokens, size_t open_paren_i, TokenType open_tok, TokenType close_tok) {
    auto stack = std::stack<TokenType>();
    stack.push(open_tok);
    size_t i = open_paren_i + 1;
    while (i < tokens.size()) {
        if (stack.top() == open_tok && tokens.at(i).type == close_tok) {
            stack.pop();
        }else if (tokens.at(i).type == open_tok) {
            stack.push(open_tok);
        }
        if (stack.empty()) {
            return i;
        }
        i++;
    }
    throw std::runtime_error("Tried to parse an expression with mismatched parentheses");

}

std::vector<std::vector<Token>> Parser::get_function_arg_tokens(std::vector<Token> &tokens) {
    std::vector<std::vector<Token>> arg_tokens = std::vector<std::vector<Token>>();
    // skip identifier and opening paren
    size_t arg_start = 0;
    for (size_t i = arg_start; i < tokens.size(); i++) {
        const auto& tok = tokens.at(i);
        if (tok.type == TokenType::BRACKET_L) {
            i = Parser::find_matching_token_index(tokens, i, TokenType::BRACKET_L, TokenType::BRACKET_R);
            continue;
        }

        if (tok.type == TokenType::COMMA) {
            auto new_vec = std::vector(tokens.begin() + arg_start, tokens.begin() + i);
            arg_tokens.push_back(
                new_vec
            );
            arg_start = i+1;
        }
    }
    // add last arg
    arg_tokens.push_back(
        std::vector(tokens.begin() + arg_start, tokens.end())
    );
    return arg_tokens;
}

