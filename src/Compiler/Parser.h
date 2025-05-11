#pragma once
#include "Type.h"
#include <variant>


struct LiteralExpression; struct BinaryExpression; struct UnaryExpression;
typedef std::variant<LiteralExpression, BinaryExpression, UnaryExpression> Expression;

struct ExpressionBase
{
    Type type;
    VectorView<Token> vec;
};

struct LiteralExpression : public ExpressionBase
{
};

enum class BinaryExpressionType
{
    Assignment, BooleanOr, BooleanAnd, NotEquals, Equals,
    Add, Subtract, Multiply, Divide, Modulus, Exponentiate,
};

struct BinaryExpression : public ExpressionBase
{
    HeapAlloc<LiteralExpression> a;
    HeapAlloc<LiteralExpression> b;
};

struct UnaryExpression : public ExpressionBase
{
    HeapAlloc<LiteralExpression> a;
};


enum class ExpressionParsingPrecedence
{

};

bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outType, int& tokensConsumed, ExpressionParsingPrecedence ep);


