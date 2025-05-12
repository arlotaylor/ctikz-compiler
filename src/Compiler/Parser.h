#pragma once
#include "Type.h"
#include <variant>


struct LiteralExpression; struct VariableExpression; struct BinaryExpression; struct UnaryExpression;
typedef std::variant<LiteralExpression, VariableExpression, BinaryExpression, UnaryExpression> Expression;

struct ExpressionBase
{
    Type type;
    VectorView<Token> vec;
};

struct LiteralExpression : public ExpressionBase
{
};

struct VariableExpression
{
    int stackIndex;
};

enum class BinaryExpressionType
{
    Assignment, BooleanOr, BooleanAnd, NotEquals, Equals,
    Add, Subtract, Multiply, Divide, Modulus, Exponentiate,
};

struct BinaryExpression : public ExpressionBase
{
    BinaryExpressionType type;
    HeapAlloc<LiteralExpression> a;
    HeapAlloc<LiteralExpression> b;
};

enum class UnaryExpressionType
{
    Cast, Not, Minus, Plus,  // Cast is a UnaryExpressionType, but it is parsed when ExpressionParsingPrecedence is Cast, not Unary
};

struct UnaryExpression : public ExpressionBase
{
    UnaryExpressionType type;
    HeapAlloc<LiteralExpression> a;
};


enum class ExpressionParsingPrecedence
{
    Assignment, Booleans, Equals, Add, Multiply, Exponentiate, Cast, Unary, Brackets, Literal, Variable,
};

bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outType, int& tokensConsumed, ExpressionParsingPrecedence ep = ExpressionParsingPrecedence::Assignment);

