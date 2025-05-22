#include "Parser.h"
#include "Lexer.h"
#include "Type.h"
#include <variant>
#include <vector>

template<ExpressionParsingPrecedence T> bool IsSymbolValid(std::string val) = delete;
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Unary>(std::string val) { return val == "!" || val == "-" || val == "+"; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Cast>(std::string val) { return val == ":"; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Exponentiate>(std::string val) { return val == "^"; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Multiply>(std::string val) { return val == "*" || val == "/" || val == "%"; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Add>(std::string val) { return val == "+" || val == "-"; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Less>(std::string val) { return val == "<" || val == ">" || val == "<=" || val == ">="; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Equals>(std::string val) { return val == "==" || val == "!="; }
template<> bool IsSymbolValid<ExpressionParsingPrecedence::Booleans>(std::string val) { return val == "||" || val == "&&"; }

template<ExpressionParsingPrecedence T> BinaryExpressionType GetBinaryType(std::string val) = delete;
template<> BinaryExpressionType GetBinaryType<ExpressionParsingPrecedence::Exponentiate>(std::string val) { return BinaryExpressionType::Exponentiate; }
template<> BinaryExpressionType GetBinaryType<ExpressionParsingPrecedence::Multiply>(std::string val) { return val == "*" ? BinaryExpressionType::Multiply : val == "/" ? BinaryExpressionType::Divide : BinaryExpressionType::Modulus; }
template<> BinaryExpressionType GetBinaryType<ExpressionParsingPrecedence::Add>(std::string val) { return val == "+" ? BinaryExpressionType::Add : BinaryExpressionType::Subtract; }
template<> BinaryExpressionType GetBinaryType<ExpressionParsingPrecedence::Less>(std::string val) { return val == "<" ? BinaryExpressionType::Less : val == ">" ? BinaryExpressionType::Greater : val == "<=" ? BinaryExpressionType::LEq : BinaryExpressionType::GEq; }
template<> BinaryExpressionType GetBinaryType<ExpressionParsingPrecedence::Equals>(std::string val) { return val == "==" ? BinaryExpressionType::Equals : BinaryExpressionType::NotEquals; }
template<> BinaryExpressionType GetBinaryType<ExpressionParsingPrecedence::Booleans>(std::string val) { return val == "&&" ? BinaryExpressionType::BooleanAnd : BinaryExpressionType::BooleanOr; }

template<ExpressionParsingPrecedence T> UnaryExpressionType GetUnaryType(std::string val) = delete;
template<> UnaryExpressionType GetUnaryType<ExpressionParsingPrecedence::Unary>(std::string val) { return val == "!" ? UnaryExpressionType::Not : val == "-" ? UnaryExpressionType::Minus : UnaryExpressionType::Plus; };
template<> UnaryExpressionType GetUnaryType<ExpressionParsingPrecedence::Cast>(std::string val) { return UnaryExpressionType::Cast; };

#define TEMPCHECK if (t1 == AtomicType::Template || t2 == AtomicType::Template) return AtomicType::Template

template<ExpressionParsingPrecedence T> Type GetBinaryReturnType(BinaryExpressionType exp, Type t1, Type t2) = delete;
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Exponentiate>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Integer && t2 == AtomicType::Integer) { return AtomicType::Integer; } else if (t1 == AtomicType::Double && t2 == AtomicType::Double) { return AtomicType::Double; } else { return AtomicType::Error; }; }
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Multiply>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Integer && t2 == AtomicType::Integer) { return AtomicType::Integer; } else if (t1 == AtomicType::Double && t2 == AtomicType::Double) { return AtomicType::Double; } else { return AtomicType::Error; }; };
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Add>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Integer && t2 == AtomicType::Integer) { return AtomicType::Integer; } else if (t1 == AtomicType::Double && t2 == AtomicType::Double) { return AtomicType::Double; } else if (t1 == AtomicType::String && t2 == AtomicType::String) { return AtomicType::String; } else { return AtomicType::Error; }; };
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Less>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Integer && t2 == AtomicType::Integer) { return AtomicType::Integer; } else if (t1 == AtomicType::Double && t2 == AtomicType::Double) { return AtomicType::Double; } else { return AtomicType::Error; }; };
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Equals>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Error || t2 == AtomicType::Error) { return AtomicType::Error; } else if (t1 == t2) { return AtomicType::Boolean; } else { return AtomicType::Error; }; };
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Booleans>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Boolean && t2 == AtomicType::Boolean) { return AtomicType::Boolean; } else { return AtomicType::Error; }; };

template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Cast>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (CheckCast(t1, t2)) { return t2; } else { return AtomicType::Error; }; };

template<ExpressionParsingPrecedence T> Type GetUnaryReturnType(UnaryExpressionType exp, Type t1) = delete;
template<> Type GetUnaryReturnType<ExpressionParsingPrecedence::Unary>(UnaryExpressionType exp, Type t1)
{
    if (t1 == AtomicType::Template) return t1;

    if (exp == UnaryExpressionType::Not)
    {
        if (t1 == AtomicType::Boolean) return AtomicType::Boolean;
        else return AtomicType::Error;
    }
    else if (exp == UnaryExpressionType::Minus || exp == UnaryExpressionType::Plus)
    {
        if (t1 == AtomicType::Integer || t1 == AtomicType::Double) return t1;
        else return AtomicType::Error;
    }
    else
    {
        return AtomicType::Error;
    }
}


template<ExpressionParsingPrecedence T>
bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    if (!ParseExpression<(ExpressionParsingPrecedence)((int)T + 1)>(tokens, ctx, outExpr, tokensConsumed)) return false;

    std::string val = tokens[tokensConsumed].value;
    while (tokens[tokensConsumed].type == TokenType::Symbol && IsSymbolValid<T>(val))
    {
        int consumed = 0;
        Expression expr = LiteralExpression{ AtomicType::Error, tokens };
        if (!ParseExpression<(ExpressionParsingPrecedence)((int)T + 1)>(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed))
        {
            ctx.errors.push_back({ "Failed to parse expression.", tokens[tokensConsumed + 1].line, tokens[tokensConsumed + 1].column });
            return false;
        }
        else
        {
            BinaryExpressionType ty = GetBinaryType<T>(val);
            Type ot = GetBinaryReturnType<T>(ty, GetExpressionType(outExpr), GetExpressionType(expr));
            if (ot == AtomicType::Error)
            {
                ctx.errors.push_back({ "Wrong types for '" + val + "' operation.", tokens[0].line, tokens[0].column });
            }

            tokensConsumed += 1 + consumed;
            outExpr = BinaryExpression{ ot, tokens, ty, { outExpr }, { expr } };
        }
        val = tokens[tokensConsumed].value;
    }

    return true;
}

//    Assignment, Booleans, Equals, Add, Multiply, Exponentiate, Cast, Unary, Brackets, Literal, Variable,


template<> bool ParseExpression<ExpressionParsingPrecedence::Variable>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
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

template<> bool ParseExpression<ExpressionParsingPrecedence::Literal>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
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
        return ParseExpression<ExpressionParsingPrecedence::Variable>(tokens, ctx, outExpr, tokensConsumed);
    }
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Brackets>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    if (tokens[0].type == TokenType::Symbol && tokens[0].value == "(")
    {
        if (!ParseExpression<ExpressionParsingPrecedence::Literal>(tokens.SubView(1), ctx, outExpr, tokensConsumed)) return false;
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
        return ParseExpression<ExpressionParsingPrecedence::Literal>(tokens, ctx, outExpr, tokensConsumed);
    }
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Unary>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    std::string val = tokens[0].value;

    if (tokens[0].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Unary>(val))
    {
        if (!ParseExpression<ExpressionParsingPrecedence::Unary>(tokens.SubView(1), ctx, outExpr, tokensConsumed)) return false;
        tokensConsumed += 1;

        UnaryExpressionType ty = GetUnaryType<ExpressionParsingPrecedence::Unary>(val);
        Type ot = GetUnaryReturnType<ExpressionParsingPrecedence::Unary>(ty, GetExpressionType(outExpr));
        if (ot == AtomicType::Error)
        {
            ctx.errors.push_back({ "Wrong types for unary '" + val + "' operation.", tokens[0].line, tokens[0].column });
        }

        outExpr = UnaryExpression{ ot, tokens, ty, { outExpr } };
        return true;

    }
    else
    {
        return ParseExpression<ExpressionParsingPrecedence::Brackets>(tokens, ctx, outExpr, tokensConsumed);
    }
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Cast>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    if (!ParseExpression<ExpressionParsingPrecedence::Unary>(tokens, ctx, outExpr, tokensConsumed)) return false;

    std::string val = tokens[tokensConsumed].value;
    if (tokens[tokensConsumed].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Cast>(val))
    {
        if (tokens[tokensConsumed + 1].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Cast>(tokens[tokensConsumed + 1].value))  // type check
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
        else  // type cast
        {
            int consumed = 0;
            Type ot;
            if (ParseType(tokens.SubView(tokensConsumed + 1), ctx, ot, consumed))
            {
                ot = GetBinaryReturnType<ExpressionParsingPrecedence::Cast>(BinaryExpressionType::Add, GetExpressionType(outExpr), ot);
                if (ot == AtomicType::Error)
                {
                    ctx.errors.push_back({ "Invalid type cast.", tokens[tokensConsumed].line, tokens[tokensConsumed].column });
                }

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

template<> bool ParseExpression<ExpressionParsingPrecedence::Exponentiate>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    if (!ParseExpression<ExpressionParsingPrecedence::Cast>(tokens, ctx, outExpr, tokensConsumed)) return false;

    std::string val = tokens[tokensConsumed].value;
    if (tokens[tokensConsumed].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Exponentiate>(val))
    {
        int consumed = 0;
        Expression expr = LiteralExpression{ AtomicType::Error, tokens };
        if (!ParseExpression<ExpressionParsingPrecedence::Exponentiate>(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed)) return false;

        BinaryExpressionType ty = GetBinaryType<ExpressionParsingPrecedence::Exponentiate>(val);
        Type ot = GetBinaryReturnType<ExpressionParsingPrecedence::Exponentiate>(ty, GetExpressionType(outExpr), GetExpressionType(expr));
        if (ot == AtomicType::Error)
        {
            ctx.errors.push_back({ "Wrong types for '" + val + "' operation.", tokens[0].line, tokens[0].column });
        }

        tokensConsumed += 1 + consumed;
        outExpr = BinaryExpression{ ot, tokens, ty, { outExpr }, { expr } };
    }

    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Overload>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    std::vector<HeapAlloc<Expression>> exprs;
    std::vector<HeapAlloc<Type>> types;
    tokensConsumed = 0;

    do
    {
        int consumed = 0;
        if (!ParseExpression<ExpressionParsingPrecedence::Booleans>(tokens.SubView(tokensConsumed), ctx, outExpr, consumed)) return false;
        exprs.push_back(outExpr);
        types.push_back(GetExpressionType(outExpr));
        tokensConsumed += consumed + 1;
    }
    while (tokens[tokensConsumed - 1].type == TokenType::Symbol && tokens[tokensConsumed - 1].value == "&");

    tokensConsumed -= 1;
    if (exprs.size() != 1)
    {
        outExpr = MultiExpression{ OverloadType{ types }, tokens, exprs };
    }
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Record>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    std::vector<HeapAlloc<Expression>> exprs;
    std::vector<HeapAlloc<Type>> types;
    tokensConsumed = 1;

    if (tokens[0].type != TokenType::Symbol || tokens[0].value != "{") return false;

    do
    {
        int consumed = 0;
        if (!ParseExpression<ExpressionParsingPrecedence::Overload>(tokens.SubView(tokensConsumed), ctx, outExpr, consumed)) return false;
        exprs.push_back(outExpr);
        types.push_back(GetExpressionType(outExpr));
        tokensConsumed += consumed + 1;
    }
    while (tokens[tokensConsumed - 1].type == TokenType::Symbol && tokens[tokensConsumed - 1].value == ",");

    tokensConsumed -= 1;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != "}") return false;

    if (exprs.size() != 1)
    {
        outExpr = MultiExpression{ RecordType{ types }, tokens, exprs };
    }
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Assignment>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    if (ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(tokens, ctx, outExpr, tokensConsumed))
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "=")
        {
            Expression expr = LiteralExpression{ AtomicType::Error, tokens };
            int consumed = 0;
            if (!ParseExpression<ExpressionParsingPrecedence::Assignment>(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed))
            {
                ctx.errors.push_back({ "Error while parsing assignment.", tokens[tokensConsumed].line, tokens[tokensConsumed].column });
                return false;
            }
            else
            {
                if (std::holds_alternative<VariableExpression>(outExpr))
                {
                    if (std::get<VariableExpression>(outExpr).stackIndex == -1)
                    {
                        ctx.errors.push_back({ "Cannot use '_' notation for single variable assignment.", tokens[0].line, tokens[0].column });
                    }
                    else
                    {
                        Type ot2 = GetExpressionType(outExpr);
                        if (ot2 == AtomicType::Template)
                        {
                            ctx.varStack[std::get<VariableExpression>(outExpr).stackIndex].second = GetExpressionType(expr);
                        }
                        else if (ot2 != GetExpressionType(expr))
                        {
                            ctx.errors.push_back({ "Cannot set variable to an expression of a different type.", tokens[tokensConsumed].line, tokens[tokensConsumed].column });
                        }
                    }
                }
                else  // outExpr must be a MultiVar
                {
                    if (!std::holds_alternative<RecordType>(GetExpressionType(expr)))
                    {
                        ctx.errors.push_back({ "Attempted to set multiple variables with a single (non-record) expression.", tokens[tokensConsumed].line, tokens[tokensConsumed].column });
                    }
                    else
                    {
                        Type ot2 = GetExpressionType(outExpr);
                        if (std::get<RecordType>(ot2).values.size() != std::get<RecordType>(GetExpressionType(expr)).values.size())
                        {
                            ctx.errors.push_back({ "Type mismatch during assignment (different number of components).", tokens[tokensConsumed].line, tokens[tokensConsumed].column });
                        }
                        else
                        {
                            bool isSetting = false;
                            for (int i = 0; i < std::get<RecordType>(ot2).values.size(); i++)
                            {
                                if (std::get<RecordType>(ot2).values[i].Get() == AtomicType::Template)
                                {
                                    int stackIndex = std::get<MultiVariableExpression>(outExpr).stackIndices[i];
                                    if (stackIndex != -1)
                                    {
                                        ctx.varStack[stackIndex].second = std::get<RecordType>(GetExpressionType(expr)).values[i].Get();
                                        isSetting = true;
                                    }
                                }
                                else
                                {
                                    isSetting = true;
                                    if (std::get<RecordType>(ot2).values[i].Get() != std::get<RecordType>(GetExpressionType(expr)).values[i].Get())
                                    {
                                        ctx.errors.push_back({ "Type mismatch during assignment.", tokens[tokensConsumed].line, tokens[tokensConsumed].column });
                                    }
                                }
                            }
                            if (!isSetting)
                            {
                                ctx.errors.push_back({ "Cannot use '_' for every variable in assignment.", tokens[0].line, tokens[0].column });
                            }
                        }
                    }
                }
                outExpr = BinaryExpression{ GetExpressionType(expr), tokens, BinaryExpressionType::Assignment, { outExpr }, { expr } };
                tokensConsumed += 1 + consumed;
                return true;
            }
        }
    }

    return ParseExpression<ExpressionParsingPrecedence::Booleans>(tokens, ctx, outExpr, tokensConsumed);
}


template<> bool ParseExpression<ExpressionParsingPrecedence::VarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    if (tokens[0].type == TokenType::Symbol && tokens[0].value == "_")
    {
        outExpr = VariableExpression{ AtomicType::Template, tokens, -1 };
        tokensConsumed = 1;
        return true;
    }

    if (tokens[0].type != TokenType::Text) return false;

    int stackPos = -1;
    for (int i = ctx.varStack.size() - 1; i >= 0; i--)
    {
        if (tokens[0].value == ctx.varStack[i].first)
        {
            stackPos = i;
            break;
        }
    }
    if (stackPos == -1)
    {
        stackPos = ctx.varStack.size();
        ctx.varStack.push_back({ tokens[0].value, AtomicType::Template });
    }

    tokensConsumed = 1;

    if (tokens[1].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Cast>(tokens[1].value))
    {
        Type ot = AtomicType::Error;
        int consumed = 0;
        if (!ParseType(tokens.SubView(2), ctx, ot, consumed)) return false;

        if (ctx.varStack[stackPos].second == AtomicType::Template)
        {
            ctx.varStack[stackPos].second = ot;
        }
        else if (ctx.varStack[stackPos].second != ot)
        {
            ctx.errors.push_back({ "Cannot redefine variable to a different type.", tokens[1].line, tokens[1].column });
        }

        tokensConsumed += 1 + consumed;
    }

    outExpr = VariableExpression{ ctx.varStack[stackPos].second, tokens, stackPos };
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    std::vector<HeapAlloc<Type>> types;
    std::vector<int> locs;

    if (!ParseExpression<ExpressionParsingPrecedence::VarDef>(tokens, ctx, outExpr, tokensConsumed)) return false;

    types.push_back({ std::get<VariableExpression>(outExpr).type });
    locs.push_back(std::get<VariableExpression>(outExpr).stackIndex);

    while (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == ",")
    {
        int consumed = 0;
        if (!ParseExpression<ExpressionParsingPrecedence::VarDef>(tokens.SubView(tokensConsumed + 1), ctx, outExpr, consumed)) return false;
        types.push_back({ std::get<VariableExpression>(outExpr).type });
        locs.push_back(std::get<VariableExpression>(outExpr).stackIndex);
        tokensConsumed += 1 + consumed;
    }

    if (types.size() == 1)
    {
        outExpr = VariableExpression{ types.front().Get(), tokens, locs.front() };
    }
    else
    {
        outExpr = MultiVariableExpression{ RecordType{ types }, tokens, locs };
    }

    return true;
}



