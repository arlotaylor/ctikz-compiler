#pragma once
#include "Type.h"
#include <variant>
#include <vector>

struct SingleStatement; struct ScopeStatement; struct ForStatement; struct WhileStatement; struct IfStatement; struct ReturnStatement;
typedef std::variant<SingleStatement, ScopeStatement, ForStatement, WhileStatement, IfStatement, ReturnStatement> Statement;

struct LiteralExpression; struct VariableExpression; struct LambdaExpression; struct MultiExpression; struct BinaryExpression; struct UnaryExpression;
typedef std::variant<LiteralExpression, VariableExpression, LambdaExpression, MultiExpression, BinaryExpression, UnaryExpression> Expression;


struct LiteralExpression
{
    Type type;
    VectorView<Token> vec;
};

struct VariableExpression
{
    Type type;
    VectorView<Token> vec;
    int stackIndex;
};

struct LambdaExpression
{
    Type type;
    VectorView<Token> vec;
    HeapAlloc<Expression> args;
    HeapAlloc<Statement> body;
};

enum class MultiExpressionType
{
    Variables, Record, Overload,
};

struct MultiExpression
{
    Type type;  // must be a record
    VectorView<Token> vec;
    MultiExpressionType exprType;
    std::vector<HeapAlloc<Expression>> elements;
};

enum class BinaryExpressionType
{
    Assignment, BooleanOr, BooleanAnd, NotEquals, Equals,
    Less, Greater, LEq, GEq,
    Add, Subtract, Multiply, Divide, Modulus, Exponentiate,
    FunctionCall,
};

struct BinaryExpression
{
    Type type;
    VectorView<Token> vec;
    BinaryExpressionType exprType;
    HeapAlloc<Expression> a;
    HeapAlloc<Expression> b;
};

enum class UnaryExpressionType
{
    Cast, Not, Minus, Plus,  // Cast is a UnaryExpressionType, but it is parsed when ExpressionParsingPrecedence is Cast, not Unary
};

struct UnaryExpression
{
    Type type;
    VectorView<Token> vec;
    UnaryExpressionType exprType;
    HeapAlloc<Expression> a;
};

inline const Type& GetExpressionType(const Expression& expr)
{
    if (std::holds_alternative<LiteralExpression>(expr)) return std::get<LiteralExpression>(expr).type;
    if (std::holds_alternative<VariableExpression>(expr)) return std::get<VariableExpression>(expr).type;
    if (std::holds_alternative<LambdaExpression>(expr)) return std::get<LambdaExpression>(expr).type;
    if (std::holds_alternative<MultiExpression>(expr)) return std::get<MultiExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    return std::get<UnaryExpression>(expr).type;
}

inline Type& GetExpressionType(Expression& expr)
{
    if (std::holds_alternative<LiteralExpression>(expr)) return std::get<LiteralExpression>(expr).type;
    if (std::holds_alternative<VariableExpression>(expr)) return std::get<VariableExpression>(expr).type;
    if (std::holds_alternative<LambdaExpression>(expr)) return std::get<LambdaExpression>(expr).type;
    if (std::holds_alternative<MultiExpression>(expr)) return std::get<MultiExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    return std::get<UnaryExpression>(expr).type;
}

enum class ExpressionParsingPrecedence
{
    // TODO: change the multivardef to just a multi expression. Also need to figure out how to use the same syntax for records and arguments. eg num: int, string, foo = 4, "5", 3.3; is nonsense. Might need to change the record syntax, and/or add {} around record r-values. So it could look like num: int + string, foo = { 4, "5" }, 3.3;
    VarDef, MultiVarDef,
    Assignment, Record, Overload, Booleans, Equals, Less, Add, Multiply, Exponentiate, Cast, FunctionCall, Unary, Brackets, Literal, Lambda, Variable,
};

template<ExpressionParsingPrecedence T = ExpressionParsingPrecedence::Assignment>
bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

template<> bool ParseExpression<ExpressionParsingPrecedence::Variable>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Lambda>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Literal>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Brackets>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Unary>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::FunctionCall>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Cast>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Exponentiate>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Overload>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Record>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Assignment>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

template<> bool ParseExpression<ExpressionParsingPrecedence::VarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

std::string ExpressionToString(Expression e);


struct ReturnTypeSet
{
    std::vector<Type> types;
    bool isOptional = true;

    Type ToType();
};

struct SingleStatement
{
    HeapAlloc<Expression> expr;
    ReturnTypeSet type;
};

struct ScopeStatement
{
    std::vector<HeapAlloc<Statement>> vec;
    ReturnTypeSet type = {};
    std::vector<std::pair<std::string, Type>> ctx;
};

struct ForStatement
{
    HeapAlloc<Expression> cond1;
    HeapAlloc<Expression> cond2;
    HeapAlloc<Expression> cond3;
    HeapAlloc<Statement> contents;
    ReturnTypeSet type = {};
};

struct WhileStatement
{
    HeapAlloc<Expression> condition;
    HeapAlloc<Statement> contents;
    ReturnTypeSet type = {};
};

struct IfStatement
{
    HeapAlloc<Expression> condition;
    HeapAlloc<Statement> contents;
    ReturnTypeSet type = {};
};

struct ReturnStatement
{
    HeapAlloc<Expression> expr;
    ReturnTypeSet type = {};
};


inline const ReturnTypeSet& GetStatementType(const Statement& expr)
{
    if (std::holds_alternative<SingleStatement>(expr)) return std::get<SingleStatement>(expr).type;
    if (std::holds_alternative<ScopeStatement>(expr)) return std::get<ScopeStatement>(expr).type;
    if (std::holds_alternative<ForStatement>(expr)) return std::get<ForStatement>(expr).type;
    if (std::holds_alternative<WhileStatement>(expr)) return std::get<WhileStatement>(expr).type;
    if (std::holds_alternative<IfStatement>(expr)) return std::get<IfStatement>(expr).type;
    return std::get<ReturnStatement>(expr).type;
}

inline ReturnTypeSet& GetStatementType(Statement& expr)
{
    if (std::holds_alternative<SingleStatement>(expr)) return std::get<SingleStatement>(expr).type;
    if (std::holds_alternative<ScopeStatement>(expr)) return std::get<ScopeStatement>(expr).type;
    if (std::holds_alternative<ForStatement>(expr)) return std::get<ForStatement>(expr).type;
    if (std::holds_alternative<WhileStatement>(expr)) return std::get<WhileStatement>(expr).type;
    if (std::holds_alternative<IfStatement>(expr)) return std::get<IfStatement>(expr).type;
    return std::get<ReturnStatement>(expr).type;
}


enum class StatementParsingType
{
    Single, If, For, While, Return, Scope,
};

template<StatementParsingType T = StatementParsingType::Single> bool ParseStatement(VectorView<Token> tokens, ParsingContext& ctx, Statement& outStatement, int& tokensConsumed) = delete;

template<> bool ParseStatement<StatementParsingType::Single>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed);
template<> bool ParseStatement<StatementParsingType::If>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed);
template<> bool ParseStatement<StatementParsingType::For>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed);
template<> bool ParseStatement<StatementParsingType::While>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed);
template<> bool ParseStatement<StatementParsingType::Return>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed);
template<> bool ParseStatement<StatementParsingType::Scope>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed);

std::string StatementToString(Statement s);
std::string ExpressionToString(Expression e);
std::string TypeToString(Type t);

