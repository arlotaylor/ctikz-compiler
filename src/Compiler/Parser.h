#pragma once
#include "Type.h"
#include <variant>
#include <vector>


struct LiteralExpression; struct VariableExpression; struct MultiVariableExpression; struct BinaryExpression; struct UnaryExpression;
typedef std::variant<LiteralExpression, VariableExpression, MultiVariableExpression, BinaryExpression, UnaryExpression> Expression;

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

struct MultiVariableExpression
{
    Type type;  // must be a record
    VectorView<Token> vec;
    std::vector<int> stackIndices;
};

enum class BinaryExpressionType
{
    Assignment, BooleanOr, BooleanAnd, NotEquals, Equals,
    Less, Greater, LEq, GEq,
    Add, Subtract, Multiply, Divide, Modulus, Exponentiate,
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
    if (std::holds_alternative<MultiVariableExpression>(expr)) return std::get<MultiVariableExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    return std::get<UnaryExpression>(expr).type;
}

inline Type& GetExpressionType(Expression& expr)
{
    if (std::holds_alternative<LiteralExpression>(expr)) return std::get<LiteralExpression>(expr).type;
    if (std::holds_alternative<VariableExpression>(expr)) return std::get<VariableExpression>(expr).type;
    if (std::holds_alternative<MultiVariableExpression>(expr)) return std::get<MultiVariableExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    return std::get<UnaryExpression>(expr).type;
}

enum class ExpressionParsingPrecedence
{
    VarDef, MultiVarDef,
    Assignment, Booleans, Equals, Less, Add, Multiply, Exponentiate, Cast, Unary, Brackets, Literal, Variable,
};

template<ExpressionParsingPrecedence T = ExpressionParsingPrecedence::Assignment>
bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

template<> bool ParseExpression<ExpressionParsingPrecedence::Variable>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Literal>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Brackets>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Unary>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Cast>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Exponentiate>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::Assignment>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

template<> bool ParseExpression<ExpressionParsingPrecedence::VarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
template<> bool ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

