#include "Parser.h"
#include "Lexer.h"
#include "Type.h"
#include <variant>
#include <iostream>
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
template<> Type GetBinaryReturnType<ExpressionParsingPrecedence::Less>(BinaryExpressionType exp, Type t1, Type t2) { TEMPCHECK; if (t1 == AtomicType::Integer && t2 == AtomicType::Integer) { return AtomicType::Boolean; } else if (t1 == AtomicType::Double && t2 == AtomicType::Double) { return AtomicType::Boolean; } else { return AtomicType::Error; }; };
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

const std::vector<std::string> EXPRESSION_PREC_NAMES = { "VarDef", "MultiVarDef", "Assignment", "Record", "Overload", "Booleans", "Equals", "Less", "Add", "Multiply", "Exponentiate", "Cast", "FunctionCall", "Unary", "Brackets", "Literal", "Lambda", "Variable" };

struct Instrumentation
{
    static int depth;

    ExpressionParsingPrecedence prec;
    Instrumentation(ExpressionParsingPrecedence ep)
        : prec(ep)
    {
        for (int i = 0; i < depth; i++) std::cout << "  ";
        std::cout << EXPRESSION_PREC_NAMES[(int)prec] << '\n';
        depth++;
    }
    ~Instrumentation()
    {
        depth--;
    }
};

int Instrumentation::depth = 0;

template<ExpressionParsingPrecedence T>
bool ParseExpression(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(T);

    if (!ParseExpression<(ExpressionParsingPrecedence)((int)T + 1)>(tokens, ctx, outExpr, tokensConsumed)) return false;

    std::string val = tokens[tokensConsumed].value;
    while (tokens[tokensConsumed].type == TokenType::Symbol && IsSymbolValid<T>(val))
    {
        int consumed = 0;
        Expression expr = LiteralExpression{ AtomicType::Error, tokens };
        if (!ParseExpression<(ExpressionParsingPrecedence)((int)T + 1)>(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed))
        {
            ctx.errors.push_back({ "Failed to parse expression.", tokens[tokensConsumed + 1].pos });
            return false;
        }
        else
        {
            BinaryExpressionType ty = GetBinaryType<T>(val);
            Type ot = GetBinaryReturnType<T>(ty, GetExpressionType(outExpr), GetExpressionType(expr));
            if (ot == AtomicType::Error)
            {
                ctx.errors.push_back({ "Wrong types for '" + val + "' operation.", tokens[0].pos });
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
    Instrumentation inst(ExpressionParsingPrecedence::Variable);

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
        ctx.errors.push_back({ "Unrecognized identifier: " + tokens[0].value + ".", tokens[0].pos });
        tokensConsumed = 1;
        outExpr = VariableExpression{ AtomicType::Error, tokens, -1 };
        return true;
    }
    else
    {
        return false;
    }
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Lambda>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Lambda);

    if (tokens[0].type == TokenType::Text && tokens[0].value == "lambda")
    {
        if (tokens[1].type != TokenType::Symbol || tokens[1].value != "(")
        {
            ctx.errors.push_back({ "Lambda arguments must be enclosed by parentheses.", tokens[1].pos });
            return false;
        }
        int varStackSize = ctx.varStack.size();
        if (!ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(tokens.SubView(2), ctx, outExpr, tokensConsumed))
        {
            ctx.errors.push_back({ "Error while parsing lambda arguments.", tokens[2].pos });
            return false;
        }
        tokensConsumed += 2;
        if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ")")
        {
            ctx.errors.push_back({ "Lambda arguments must be enclosed by parentheses.", tokens[tokensConsumed].pos });
            return false;
        }
        tokensConsumed += 1;

        Statement stat = SingleStatement{ { LiteralExpression{ AtomicType::Error, tokens } } };
        int consumed = 0;
        if (!ParseStatement(tokens.SubView(tokensConsumed), ctx, stat, consumed))
        {
            ctx.errors.push_back({ "Error in lambda body.", tokens[tokensConsumed].pos });
            return false;
        }
        tokensConsumed += consumed;
        Type retType;
        if (GetStatementType(stat).types.size() == 0)
        {
            retType = AtomicType::Void;
        }
        else if (GetStatementType(stat).types.size() == 1 && !GetStatementType(stat).isOptional)
        {
            retType = GetStatementType(stat).types.front();
        }
        else
        {
            std::vector<HeapAlloc<Type>> retTypes;
            for (Type& t : GetStatementType(stat).types) retTypes.push_back({ t });
            if (GetStatementType(stat).isOptional) retTypes.push_back({ AtomicType::Void });
            retType = UnionType{ retTypes };
        }
        outExpr = LambdaExpression{ LambdaType{ { GetExpressionType(outExpr) }, { retType } }, tokens, { outExpr }, { stat } };
        return true;
    }
    else
    {
        return ParseExpression<ExpressionParsingPrecedence::Variable>(tokens, ctx, outExpr, tokensConsumed);
    }
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Literal>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Literal);

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
    else if (tokens[0].type == TokenType::Boolean)
    {
        outExpr = LiteralExpression{ AtomicType::Boolean, tokens };
    }
    else
    {
        return ParseExpression<ExpressionParsingPrecedence::Lambda>(tokens, ctx, outExpr, tokensConsumed);
    }
    tokensConsumed = 1;
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Brackets>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Brackets);

    if (tokens[0].type == TokenType::Symbol && tokens[0].value == "(")
    {
        if (!ParseExpression(tokens.SubView(1), ctx, outExpr, tokensConsumed)) return false;
        if (tokens[tokensConsumed + 1].type != TokenType::Symbol || tokens[tokensConsumed + 1].value != ")")
        {
            ctx.errors.push_back({ "No matching ).", tokens[0].pos });
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
    Instrumentation inst(ExpressionParsingPrecedence::Unary);

    std::string val = tokens[0].value;

    if (tokens[0].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Unary>(val))
    {
        if (!ParseExpression<ExpressionParsingPrecedence::Unary>(tokens.SubView(1), ctx, outExpr, tokensConsumed)) return false;
        tokensConsumed += 1;

        UnaryExpressionType ty = GetUnaryType<ExpressionParsingPrecedence::Unary>(val);
        Type ot = GetUnaryReturnType<ExpressionParsingPrecedence::Unary>(ty, GetExpressionType(outExpr));
        if (ot == AtomicType::Error)
        {
            ctx.errors.push_back({ "Wrong types for unary '" + val + "' operation.", tokens[0].pos });
        }

        outExpr = UnaryExpression{ ot, tokens, ty, { outExpr } };
        return true;

    }
    else
    {
        return ParseExpression<ExpressionParsingPrecedence::Brackets>(tokens, ctx, outExpr, tokensConsumed);
    }
}

template<> bool ParseExpression<ExpressionParsingPrecedence::FunctionCall>(VectorView<Token> tokens, ParsingContext &ctx, Expression &outExpr, int &tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::FunctionCall);

    if (!ParseExpression<ExpressionParsingPrecedence::Unary>(tokens, ctx, outExpr, tokensConsumed)) return false;

    if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "(")
    {
        int consumed = 0;
        Expression expr = LiteralExpression{ AtomicType::Error, tokens };
        if (!ParseExpression(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed))
        {
            return false;
        }
        else if (tokens[tokensConsumed + 1 + consumed].type != TokenType::Symbol || tokens[tokensConsumed + 1 + consumed].value != ")")
        {
            return false;
        }
        else if (!std::holds_alternative<LambdaType>(GetExpressionType(outExpr)))
        {
            ctx.errors.push_back({ "Cannot call a non-lambda type.", tokens[tokensConsumed].pos });
            GetExpressionType(outExpr) = AtomicType::Error;
        }
        else if (std::get<LambdaType>(GetExpressionType(outExpr)).arg.Get() != GetExpressionType(expr))
        {
            ctx.errors.push_back({ "Invalid function arguments.", tokens[tokensConsumed].pos });
            GetExpressionType(outExpr) = AtomicType::Error;
        }
        else
        {
            outExpr = BinaryExpression{ std::get<LambdaType>(GetExpressionType(outExpr)).ret.Get(), tokens, BinaryExpressionType::FunctionCall, { outExpr }, { expr } };
        }
        tokensConsumed += 2 + consumed;
        return true;
    }
    else
    {
        return true;
    }
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Cast>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Cast);

    if (!ParseExpression<ExpressionParsingPrecedence::FunctionCall>(tokens, ctx, outExpr, tokensConsumed)) return false;

    std::string val = tokens[tokensConsumed].value;
    if (tokens[tokensConsumed].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Cast>(val))
    {
        if (tokens[tokensConsumed + 1].type == TokenType::Symbol && IsSymbolValid<ExpressionParsingPrecedence::Cast>(tokens[tokensConsumed + 1].value))  // type check
        {
            int consumed = 0;
            Type ot;
            if (!ParseType(tokens.SubView(tokensConsumed + 2), ctx, ot, consumed))
            {
                ctx.errors.push_back({ "Failed to parse type check.", tokens[tokensConsumed + 2].pos });
                return false;
            }
            else
            {
                if (GetExpressionType(outExpr) != ot)
                {
                    ctx.errors.push_back({ "Type check failed.", tokens[tokensConsumed + 2].pos });
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
                    ctx.errors.push_back({ "Invalid type cast.", tokens[tokensConsumed].pos });
                }

                tokensConsumed += 1 + consumed;
                outExpr = UnaryExpression{ ot, tokens, UnaryExpressionType::Cast, { outExpr } };
                return true;
            }
            else
            {
                ctx.errors.push_back({ "Failed to parse type cast.", tokens[tokensConsumed + 1].pos });
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
    Instrumentation inst(ExpressionParsingPrecedence::Exponentiate);

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
            ctx.errors.push_back({ "Wrong types for '" + val + "' operation.", tokens[0].pos });
        }

        tokensConsumed += 1 + consumed;
        outExpr = BinaryExpression{ ot, tokens, ty, { outExpr }, { expr } };
    }

    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Overload>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Overload);

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
        outExpr = MultiExpression{ OverloadType{ types }, tokens, MultiExpressionType::Overload, exprs };
    }
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Record>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Record);

    std::vector<HeapAlloc<Expression>> exprs;
    std::vector<HeapAlloc<Type>> types;
    tokensConsumed = 0;

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
    if (exprs.size() != 1)
    {
        outExpr = MultiExpression{ RecordType{ types }, tokens, MultiExpressionType::Record, exprs };
    }
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::Assignment>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::Assignment);

    ParsingContext oldctx = ctx;  // vardef will mess with ctx, so we need to revert upon failure

    if (ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(tokens, ctx, outExpr, tokensConsumed))
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "=")
        {
            Expression expr = LiteralExpression{ AtomicType::Error, tokens };
            int consumed = 0;
            if (!ParseExpression<ExpressionParsingPrecedence::Assignment>(tokens.SubView(tokensConsumed + 1), ctx, expr, consumed))
            {
                ctx.errors.push_back({ "Error while parsing assignment.", tokens[tokensConsumed].pos });
                return false;
            }
            else
            {
                if (std::holds_alternative<VariableExpression>(outExpr))
                {
                    if (std::get<VariableExpression>(outExpr).stackIndex == -1)
                    {
                        ctx.errors.push_back({ "Cannot use '_' notation for single variable assignment.", tokens[0].pos });
                    }
                    else
                    {
                        Type& ot2 = GetExpressionType(outExpr);
                        if (ot2 == AtomicType::Template)
                        {
                            ctx.varStack[std::get<VariableExpression>(outExpr).stackIndex].second = GetExpressionType(expr);
                            ot2 = GetExpressionType(expr);
                        }
                        else if (ot2 != GetExpressionType(expr))
                        {
                            ctx.errors.push_back({ "Cannot set variable to an expression of a different type.", tokens[tokensConsumed].pos });
                        }
                    }
                }
                else  // outExpr must be a MultiVar
                {
                    if (!std::holds_alternative<RecordType>(GetExpressionType(expr)))
                    {
                        ctx.errors.push_back({ "Attempted to set multiple variables with a single (non-record) expression.", tokens[tokensConsumed].pos });
                    }
                    else
                    {
                        Type ot2 = GetExpressionType(outExpr);
                        if (std::get<RecordType>(ot2).values.size() != std::get<RecordType>(GetExpressionType(expr)).values.size())
                        {
                            ctx.errors.push_back({ "Type mismatch during assignment (different number of components).", tokens[tokensConsumed].pos });
                        }
                        else
                        {
                            bool isSetting = false;
                            for (int i = 0; i < std::get<RecordType>(ot2).values.size(); i++)
                            {
                                if (std::get<RecordType>(ot2).values[i].Get() == AtomicType::Template)
                                {
                                    int stackIndex = std::get<VariableExpression>(std::get<MultiExpression>(outExpr).elements[i].Get()).stackIndex;
                                    if (stackIndex != -1)
                                    {
                                        ctx.varStack[stackIndex].second = std::get<RecordType>(GetExpressionType(expr)).values[i].Get();
                                        std::get<RecordType>(ot2).values[i] = HeapAlloc<Type>{ ctx.varStack[stackIndex].second };
                                        isSetting = true;
                                    }
                                }
                                else
                                {
                                    isSetting = true;
                                    if (std::get<RecordType>(ot2).values[i].Get() != std::get<RecordType>(GetExpressionType(expr)).values[i].Get())
                                    {
                                        ctx.errors.push_back({ "Type mismatch during assignment.", tokens[tokensConsumed].pos });
                                    }
                                }
                            }
                            if (!isSetting)
                            {
                                ctx.errors.push_back({ "Cannot use '_' for every variable in assignment.", tokens[0].pos });
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
    ctx = oldctx;

    return ParseExpression<ExpressionParsingPrecedence::Record>(tokens, ctx, outExpr, tokensConsumed);
}


template<> bool ParseExpression<ExpressionParsingPrecedence::VarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::VarDef);

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
            ctx.errors.push_back({ "Cannot redefine variable to a different type.", tokens[1].pos });
        }

        tokensConsumed += 1 + consumed;
    }

    outExpr = VariableExpression{ ctx.varStack[stackPos].second, tokens, stackPos };
    return true;
}

template<> bool ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(VectorView<Token> tokens, ParsingContext& ctx, Expression& outExpr, int& tokensConsumed)
{
    Instrumentation inst(ExpressionParsingPrecedence::MultiVarDef);

    auto oldVS = ctx.varStack;

    std::vector<HeapAlloc<Type>> types;
    std::vector<HeapAlloc<Expression>> locs;

    if (!ParseExpression<ExpressionParsingPrecedence::VarDef>(tokens, ctx, outExpr, tokensConsumed)) return false;

    types.push_back({ std::get<VariableExpression>(outExpr).type });
    locs.push_back(outExpr);

    while (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == ",")
    {
        int consumed = 0;
        if (!ParseExpression<ExpressionParsingPrecedence::VarDef>(tokens.SubView(tokensConsumed + 1), ctx, outExpr, consumed))
        {
            ctx.varStack = oldVS;
            return false;
        }
        types.push_back({ std::get<VariableExpression>(outExpr).type });
        locs.push_back(outExpr);
        tokensConsumed += 1 + consumed;
    }

    if (types.size() == 1)
    {
        outExpr = locs.front().Get();
    }
    else
    {
        outExpr = MultiExpression{ RecordType{ types }, tokens, MultiExpressionType::Variables, locs };
    }

    return true;
}


// Statement types primer:
//   A statement's type is its return type. This is empty for almost all statements.
//   Return statements return a type.
//   If, for and while statements return unions of NoReturn and whatever the inner scope returns.
//   Scopes are weird. They loop through, adding each return type (with NoReturns stripped) of their constituents to a list. If one of these returns has no NoReturns, and is not the last substatement, an error is raised. If it is the end, the final return type will not have a NoReturns. Otherwise, the final return type is the list we collected, plus NoReturns, in a union.

template<> bool ParseStatement<StatementParsingType::Single>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed)
{
    if (ParseStatement<StatementParsingType::Scope>(tokens, ctx, outStatement, tokensConsumed)) return true;
    if (ParseStatement<StatementParsingType::If>(tokens, ctx, outStatement, tokensConsumed)) return true;
    if (ParseStatement<StatementParsingType::For>(tokens, ctx, outStatement, tokensConsumed)) return true;
    if (ParseStatement<StatementParsingType::While>(tokens, ctx, outStatement, tokensConsumed)) return true;
    if (ParseStatement<StatementParsingType::Return>(tokens, ctx, outStatement, tokensConsumed)) return true;

    Expression expr = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens, ctx, expr, tokensConsumed)) return false;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ";")
    {
        ctx.errors.push_back({ "Missing semicolon in statement." + tokens[tokensConsumed].value, tokens[tokensConsumed].pos });
        return false;
    }

    tokensConsumed += 1;
    outStatement = SingleStatement{ { expr } };  // single statements always output NoReturn, even when the underlying expression does not.
    return true;
}

template<> bool ParseStatement<StatementParsingType::If>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed)
{
    if (tokens[0].type != TokenType::Text || tokens[0].value != "if") return false;
    if (tokens[1].type != TokenType::Symbol || tokens[1].value != "(")
    {
        ctx.errors.push_back({ "If statement must have parentheses around the condition.", tokens[1].pos });
        return false;
    }

    Expression expr = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens.SubView(2), ctx, expr, tokensConsumed)) return false;
    tokensConsumed += 2;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ")")
    {
        ctx.errors.push_back({ "If statement must have parentheses around the condition.", tokens[tokensConsumed].pos });
        return false;
    }
    tokensConsumed += 1;

    int consumed = 0;
    if (!ParseStatement(tokens.SubView(tokensConsumed), ctx, outStatement, consumed)) return false;
    tokensConsumed += consumed;

    if (GetExpressionType(expr) != AtomicType::Boolean)
    {
        ctx.errors.push_back({ "If statement conditional must be a boolean.", tokens[2].pos });
    }

    outStatement = IfStatement{ { expr }, { outStatement }, { GetStatementType(outStatement).types, true } };
    return true;
}

template<> bool ParseStatement<StatementParsingType::For>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed)
{
    if (tokens[0].type != TokenType::Text || tokens[0].value != "for") return false;
    if (tokens[1].type != TokenType::Symbol || tokens[1].value != "(")
    {
        ctx.errors.push_back({ "For statement must have parentheses around the arguments.", tokens[1].pos });
        return false;
    }

    Expression expr1 = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens.SubView(2), ctx, expr1, tokensConsumed)) return false;
    tokensConsumed += 2;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ";")
    {
        ctx.errors.push_back({ "For statement must have 2 semicolons to separate the arguments.", tokens[tokensConsumed].pos });
        return false;
    }
    tokensConsumed += 1;

    int consumed = 0;
    Expression expr2 = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens.SubView(tokensConsumed), ctx, expr2, consumed)) return false;
    tokensConsumed += consumed;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ";")
    {
        ctx.errors.push_back({ "For statement must have 2 semicolons to separate the arguments.", tokens[tokensConsumed].pos });
        return false;
    }
    tokensConsumed += 1;

    Expression expr3 = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens.SubView(tokensConsumed), ctx, expr3, consumed)) return false;
    tokensConsumed += consumed;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ")")
    {
        ctx.errors.push_back({ "For statement must have parentheses around the arguments.", tokens[tokensConsumed].pos });
        return false;
    }
    tokensConsumed += 1;

    if (!ParseStatement(tokens.SubView(tokensConsumed), ctx, outStatement, consumed)) return false;
    tokensConsumed += consumed;

    if (GetExpressionType(expr2) != AtomicType::Boolean)
    {
        ctx.errors.push_back({ "For statement conditional must be a boolean.", tokens[2].pos });
    }

    outStatement = ForStatement{ { expr1 }, { expr2 }, { expr3 }, { outStatement }, { GetStatementType(outStatement).types, true } };
    return true;
}

template<> bool ParseStatement<StatementParsingType::While>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed)
{
    if (tokens[0].type != TokenType::Text || tokens[0].value != "while") return false;
    if (tokens[1].type != TokenType::Symbol || tokens[1].value != "(")
    {
        ctx.errors.push_back({ "While statement must have parentheses around the condition.", tokens[1].pos });
        return false;
    }

    Expression expr = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens.SubView(2), ctx, expr, tokensConsumed)) return false;
    tokensConsumed += 2;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ")")
    {
        ctx.errors.push_back({ "While statement must have parentheses around the condition.", tokens[tokensConsumed].pos });
        return false;
    }
    tokensConsumed += 1;

    int consumed = 0;
    if (!ParseStatement(tokens.SubView(tokensConsumed), ctx, outStatement, consumed)) return false;
    tokensConsumed += consumed;

    if (GetExpressionType(expr) != AtomicType::Boolean)
    {
        ctx.errors.push_back({ "While statement conditional must be a boolean.", tokens[2].pos });
    }

    outStatement = WhileStatement{ { expr }, { outStatement }, { GetStatementType(outStatement).types, true } };
    return true;
}

template<> bool ParseStatement<StatementParsingType::Return>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed)
{
    if (tokens[0].type != TokenType::Text || tokens[0].value != "return") return false;
    Expression expr = LiteralExpression{ AtomicType::Error, tokens };
    if (!ParseExpression(tokens.SubView(1), ctx, expr, tokensConsumed)) return false;
    tokensConsumed += 1;
    if (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != ";")
    {
        ctx.errors.push_back({ "Missing semicolon in return statement.", tokens[tokensConsumed].pos });
        return false;
    }
    tokensConsumed += 1;
    outStatement = ReturnStatement{ { expr }, { { GetExpressionType(expr) }, false } };
    return true;
}

template<> bool ParseStatement<StatementParsingType::Scope>(VectorView<Token> tokens, ParsingContext &ctx, Statement &outStatement, int &tokensConsumed)
{
    if (tokens[0].type != TokenType::Symbol || tokens[0].value != "{") return false;
    tokensConsumed = 1;
    int stackSize = ctx.varStack.size();

    std::vector<HeapAlloc<Statement>> statements;
    std::vector<Type> returnTypes;
    int definiteReturnTypes = 0;

    while (tokens[tokensConsumed].type != TokenType::Symbol || tokens[tokensConsumed].value != "}")
    {
        if (definiteReturnTypes == 1)
        {
            ctx.errors.push_back({ "Unreachable code (already returned).", tokens[tokensConsumed].pos });
        }

        int consumed = 0;
        if (!ParseStatement(tokens.SubView(tokensConsumed), ctx, outStatement, consumed)) return false;
        statements.push_back({ outStatement });
        tokensConsumed += consumed;

        returnTypes.insert(returnTypes.end(), GetStatementType(outStatement).types.begin(), GetStatementType(outStatement).types.end());
        if (!GetStatementType(outStatement).isOptional) definiteReturnTypes++;
    }
    tokensConsumed += 1;
    outStatement = ScopeStatement{ statements, { returnTypes, definiteReturnTypes == 0 } };
    ctx.varStack.erase(ctx.varStack.begin() + stackSize, ctx.varStack.end());
    return true;
}


std::string StatementToString(Statement s);

std::string ExpressionToString(Expression e)
{
    std::string type = TypeToString(GetExpressionType(e));

    if (std::holds_alternative<LiteralExpression>(e))
    {
        if (!std::holds_alternative<AtomicType>(std::get<LiteralExpression>(e).type)) return "ERROR";

        return "LiteralExp{" + std::get<LiteralExpression>(e).vec[0].value + ",type:" + type + "}";
    }
    else if (std::holds_alternative<VariableExpression>(e))
    {
        return "VariableExp{index:" + std::to_string(std::get<VariableExpression>(e).stackIndex) + ",type:" + type + "}";
    }
    else if (std::holds_alternative<LambdaExpression>(e))
    {
        return "LambdaExpression{" + ExpressionToString(std::get<LambdaExpression>(e).args.Get()) + "," + StatementToString(std::get<LambdaExpression>(e).body.Get()) + ",type:" + type + "}";
    }
    else if (std::holds_alternative<MultiExpression>(e))
    {
        std::string ret = "MultiExp{" + std::to_string((int)std::get<MultiExpression>(e).exprType);
        for (HeapAlloc<Expression>& i : std::get<MultiExpression>(e).elements)
        {
            ret += ExpressionToString(i.Get()) + ",";
        }
        ret = ret.substr(0, ret.size() - 1);
        ret += ",type:" + type + "}";
        return ret;
    }
    else if (std::holds_alternative<BinaryExpression>(e))
    {
        return "BinaryExp{" + std::to_string((int)std::get<BinaryExpression>(e).exprType) + "," + ExpressionToString(std::get<BinaryExpression>(e).a.Get()) + "," + ExpressionToString(std::get<BinaryExpression>(e).b.Get()) + ",type:" + type + "}";
    }
    else
    {
        return "UnaryExp{" + std::to_string((int)std::get<UnaryExpression>(e).exprType) + "," + ExpressionToString(std::get<UnaryExpression>(e).a.Get()) + ",type:" + type + "}";
    }
}

std::string StatementToString(Statement s)
{
    if (std::holds_alternative<SingleStatement>(s))
    {
        return ExpressionToString(std::get<SingleStatement>(s).expr.Get()) + ";\n";
    }
    else if (std::holds_alternative<IfStatement>(s))
    {
        return "if (" + ExpressionToString(std::get<IfStatement>(s).condition.Get()) + ")\n" + StatementToString(std::get<IfStatement>(s).contents.Get());
    }
    else if (std::holds_alternative<ForStatement>(s))
    {
        return "for (" + ExpressionToString(std::get<ForStatement>(s).cond1.Get()) + ";" + ExpressionToString(std::get<ForStatement>(s).cond2.Get()) + ";" + ExpressionToString(std::get<ForStatement>(s).cond3.Get()) + ")\n" + StatementToString(std::get<ForStatement>(s).contents.Get());
    }
    else if (std::holds_alternative<WhileStatement>(s))
    {
        return "while (" + ExpressionToString(std::get<WhileStatement>(s).condition.Get()) + ")\n" + StatementToString(std::get<WhileStatement>(s).contents.Get());
    }
    else if (std::holds_alternative<ScopeStatement>(s))
    {
        std::string ret = "{\n";
        for (HeapAlloc<Statement>& i : std::get<ScopeStatement>(s).vec)
        {
            ret += StatementToString(i.Get());
        }
        return ret + "}\n";
    }
    else
    {
        return "return " + ExpressionToString(std::get<ReturnStatement>(s).expr.Get()) + ";\n";
    }
}

int main()
{
    while (true)
    {
        std::cout << "Enter possible statement: ";
        std::string temp; std::getline(std::cin, temp);

        std::vector<Token> tokens = Tokenize(temp);
        for (Token& i : tokens)
        {
             std::cout << i.value << std::endl;
        }
        ParsingContext pc = { { { "func1", LambdaType{ { AtomicType::Integer }, { AtomicType::String } } } } };
        Statement s = SingleStatement{ { LiteralExpression{ AtomicType::Error, { tokens, 0 } } } }; int n = 0;
        std::cout << (ParseStatement({tokens, 0}, pc, s, n) ? "Parsing worked!" : "Parsing failed.") << std::endl;
        std::cout << StatementToString(s) << std::endl;
        std::cout << n << " tokens parsed." << std::endl;
        std::cout << "There were " << pc.errors.size() << " errors." << std::endl;
        for (auto& i : pc.errors)
        {
            std::cout << "Error (" << i.pos.line << "," << i.pos.column << "): " << i.msg << std::endl;
            int pos = 0;
            for (int j = 1; j < i.pos.line; j++) j = 1 + temp.find('\n', pos);
            for (int j = 1; j < i.pos.column; j++) std::cout << ' ';
            std::cout << "v\n" << temp.substr(pos, temp.find('\n', pos) - pos) << "\n" << std::endl;
        }
    }
}

