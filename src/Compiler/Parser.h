#pragma once
#include "Type.h"
#include <variant>


struct LiteralExpression; struct VariableExpression; struct BinaryExpression; struct UnaryExpression;
typedef std::variant<LiteralExpression, VariableExpression, BinaryExpression, UnaryExpression> Expression;

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

enum class BinaryExpressionType
{
    Assignment, BooleanOr, BooleanAnd, NotEquals, Equals,
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
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    else return std::get<UnaryExpression>(expr).type;
}

inline Type& GetExpressionType(Expression& expr)
{
    if (std::holds_alternative<LiteralExpression>(expr)) return std::get<LiteralExpression>(expr).type;
    if (std::holds_alternative<VariableExpression>(expr)) return std::get<VariableExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    else return std::get<UnaryExpression>(expr).type;
}

enum class ExpressionParsingPrecedence
{
    Assignment, Booleans, Equals, Add, Multiply, Exponentiate, Cast, Unary, Brackets, Literal, Variable,
};

bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outType, int& tokensConsumed, ExpressionParsingPrecedence ep = ExpressionParsingPrecedence::Assignment);

