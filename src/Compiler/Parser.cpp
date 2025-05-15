#include "Parser.h"
#include "Lexer.h"
#include "Type.h"

bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed, ExpressionParsingPrecedence ep)
{
    if (ep == ExpressionParsingPrecedence::Variable)
    {
        if (tokens[0].type == TokenType::Text)
        {
            for (int i = ctx.varStack.size() - 1; i >= 0; i--)
            {
                if (tokens[0].value == ctx.varStack[i].first)
                {
                    outExpr = VariableExpression{ ctx.varStack[i].second, tokens, i };
                    tokensConsumed = 1;
                    return true;
                }
            }
            ctx.errors.push_back({ "Unrecognized identifier: " + tokens[0].value + ".", tokens[0].line, tokens[0].column });
            tokensConsumed = 1;
            outExpr = VariableExpression{ AtomicType::Error, tokens, -1 };
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (ep == ExpressionParsingPrecedence::Literal)
    {
        if (tokens[0].type == TokenType::Integer)
        {
            outExpr = LiteralExpression{ AtomicType::Integer, tokens };
        }
        else if (tokens[0].type == TokenType::Decimal)
        {
            outExpr = LiteralExpression{ AtomicType::Double, tokens };
        }
        else if (tokens[0].type == TokenType::StringLiteral)
        {
            outExpr = LiteralExpression{ AtomicType::String, tokens };
        }
        else
        {
            return ParseExpression(tokens, ctx, outExpr, tokensConsumed, ExpressionParsingPrecedence::Variable);
        }
    }
    else if (ep == ExpressionParsingPrecedence::Brackets)
    {
        if (tokens[0].type == TokenType::Symbol && tokens[0].value == "(")
        {
            if (!ParseExpression(tokens.SubView(1), ctx, outExpr, tokensConsumed, ExpressionParsingPrecedence::Literal)) return false;
            if (tokens[tokensConsumed + 1].type != TokenType::Symbol || tokens[tokensConsumed + 1].value != ")")
            {
                ctx.errors.push_back({ "No matching ).", tokens[0].line, tokens[0].column });
                return false;
            }

            tokensConsumed += 2;
            return true;
        }
        else
        {
            return ParseExpression(tokens, ctx, outExpr, tokensConsumed, ExpressionParsingPrecedence::Literal);
        }
    }
    else if (ep == ExpressionParsingPrecedence::Unary)
    {
        if (tokens[0].type == TokenType::Symbol)
        {
            if (tokens[0].value == "!" || tokens[0].value == "-" || tokens[0].value == "+")
            {
                if (!ParseExpression(tokens.SubView(1), ctx, outExpr, tokensConsumed, ExpressionParsingPrecedence::Unary)) return false;
                tokensConsumed += 1;
            }
            if (tokens[0].value == "!")
            {
                if (GetExpressionType(outExpr) == AtomicType::Boolean)
                {
                    outExpr = UnaryExpression{ AtomicType::Boolean, tokens, UnaryExpressionType::Not, { outExpr } };
                }
                else
                {
                    ctx.errors.push_back({ "Argument of !(x) must be a bool.", tokens[0].line, tokens[0].column });
                    outExpr = UnaryExpression{ AtomicType::Error, tokens, UnaryExpressionType::Not, { outExpr } };
                }
                return true;
            }
            else if (tokens[0].value == "-")
            {
                if (GetExpressionType(outExpr) == AtomicType::Integer)
                {
                    outExpr = UnaryExpression{ AtomicType::Integer, tokens, UnaryExpressionType::Minus, { outExpr } };
                }
                else if (GetExpressionType(outExpr) == AtomicType::Double)
                {
                    outExpr = UnaryExpression{ AtomicType::Double, tokens, UnaryExpressionType::Minus, { outExpr } };
                }
                else
                {
                    ctx.errors.push_back({ "Argument of -(x) must be a number.", tokens[0].line, tokens[0].column });
                    outExpr = UnaryExpression{ AtomicType::Error, tokens, UnaryExpressionType::Minus, { outExpr } };
                }
                return true;
            }
            else if (tokens[0].value == "+")
            {
                if (GetExpressionType(outExpr) == AtomicType::Integer)
                {
                    outExpr = UnaryExpression{ AtomicType::Integer, tokens, UnaryExpressionType::Plus, { outExpr } };
                }
                else if (GetExpressionType(outExpr) == AtomicType::Double)
                {
                    outExpr = UnaryExpression{ AtomicType::Double, tokens, UnaryExpressionType::Plus, { outExpr } };
                }
                else
                {
                    ctx.errors.push_back({ "Argument of +(x) must be a number.", tokens[0].line, tokens[0].column });
                    outExpr = UnaryExpression{ AtomicType::Error, tokens, UnaryExpressionType::Plus, { outExpr } };
                }
                return true;
            }
            else 
            {
                return ParseExpression(tokens, ctx, outExpr, tokensConsumed, ExpressionParsingPrecedence::Brackets);
            }
        }
    }

    if (!ParseExpression(tokens, ctx, outExpr, tokensConsumed, (ExpressionParsingPrecedence)((int)ep + 1))) return false;

    if (ep == ExpressionParsingPrecedence::Cast)
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == ":")
        {
            if (tokens[tokensConsumed + 1].type == TokenType::Symbol && tokens[tokensConsumed].value == ":")
            {
                int consumed = 0;
                Type ot;
                if (!ParseType(tokens.SubView(tokensConsumed + 2), ctx, ot, consumed))
                {
                    ctx.errors.push_back({ "Failed to parse type check.", tokens[tokensConsumed + 2].line, tokens[tokensConsumed + 2].column });
                    return false;
                }
                else
                {
                    if (GetExpressionType(outExpr) != ot)
                    {
                        ctx.errors.push_back({ "Type check failed.", tokens[tokensConsumed + 2].line, tokens[tokensConsumed + 2].column });
                        GetExpressionType(outExpr) = AtomicType::Error;
                    }
                    tokensConsumed += 2 + consumed;
                    return true;
                }
            }
            else
            {
                int consumed = 0;
                Type ot;
                if (ParseType(tokens.SubView(tokensConsumed + 1), ctx, ot, consumed))
                {
                    if (!CheckCast(GetExpressionType(outExpr), ot)) return false;

                    tokensConsumed += 1 + consumed;
                    outExpr = UnaryExpression{ ot, tokens, UnaryExpressionType::Cast, { outExpr } };
                    return true;
                }
                else
                {
                    ctx.errors.push_back({ "Failed to parse type cast.", tokens[tokensConsumed + 1].line, tokens[tokensConsumed + 1].column });
                    return false;
                }
            }
        }
        else
        {
            return true;
        }
    }

    if (ep == ExpressionParsingPrecedence::Exponentiate)
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "^")
        {
            int consumed = 0;
            Expression expr = LiteralExpression{ AtomicType::Error, tokens };
            if (!ParseExpression(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed, ExpressionParsingPrecedence::Exponentiate))
            {
                ctx.errors.push_back({ "Failed to parse expression.", tokens[tokensConsumed + 1].line, tokens[tokensConsumed + 1].column });
                return false;
            }
            else
            {
                Type ot = AtomicType::Error;
                if (GetExpressionType(outExpr) == AtomicType::Integer && GetExpressionType(expr) == AtomicType::Integer)
                {
                    ot = AtomicType::Integer;
                }
                else if (GetExpressionType(outExpr) == AtomicType::Double && GetExpressionType(expr) == AtomicType::Double)
                {
                    ot = AtomicType::Double;
                }
                else
                {
                    ctx.errors.push_back({ "Wrong types for '^' operation.", tokens[0].line, tokens[0].column });
                }
                tokensConsumed += 1 + consumed;
                outExpr = BinaryExpression{ ot, tokens, BinaryExpressionType::Exponentiate, { outExpr }, { expr } };
                return true;
            }
        }
    }


    // TODO: FINISH THIS PARSER
    // TODO: IT'S SO CLOSE MAN
}

//    Assignment, Booleans, Equals, Add, Multiply, Exponentiate, Cast, Unary, Brackets, Literal, Variable,


