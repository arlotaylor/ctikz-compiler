#pragma once
#include "Type.h"
#include <variant>
#include <vector>


struct LiteralExpression; struct VariableExpression; struct MultiExpression; struct BinaryExpression; struct UnaryExpression;
typedef std::variant<LiteralExpression, VariableExpression, MultiExpression, BinaryExpression, UnaryExpression> Expression;

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
    if (std::holds_alternative<MultiExpression>(expr)) return std::get<MultiExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    return std::get<UnaryExpression>(expr).type;
}

inline Type& GetExpressionType(Expression& expr)
{
    if (std::holds_alternative<LiteralExpression>(expr)) return std::get<LiteralExpression>(expr).type;
    if (std::holds_alternative<VariableExpression>(expr)) return std::get<VariableExpression>(expr).type;
    if (std::holds_alternative<MultiExpression>(expr)) return std::get<MultiExpression>(expr).type;
    if (std::holds_alternative<BinaryExpression>(expr)) return std::get<BinaryExpression>(expr).type;
    return std::get<UnaryExpression>(expr).type;
}

enum class ExpressionParsingPrecedence
{
    // TODO: change the multivardef to just a multi expression. Also need to figure out how to use the same syntax for records and arguments. eg num: int, string, foo = 4, "5", 3.3; is nonsense. Might need to change the record syntax, and/or add {} around record r-values. So it could look like num: int + string, foo = { 4, "5" }, 3.3;
    VarDef, MultiVarDef,
    Assignment, Record, Overload, Booleans, Equals, Less, Add, Multiply, Exponentiate, Cast, FunctionCall, Unary, Brackets, Literal, Variable,
};

template<ExpressionParsingPrecedence T = ExpressionParsingPrecedence::Assignment>
bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);

template<> bool ParseExpression<ExpressionParsingPrecedence::Variable>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed);
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



struct SingleStatement; struct ScopeStatement; struct ForStatement; struct WhileStatement; struct IfStatement; struct ReturnStatement;
