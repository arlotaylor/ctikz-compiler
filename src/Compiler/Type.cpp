#include "Type.h"
#include "Lexer.h"
#include "Assertion.h"
#include <memory>
#include <variant>

// This language's types fall into five categories
// Atomic Types, which are simple data (sometimes in TrackedPointers). This category also includes bookkeeping types like Error, Void and EndSum, as well as Template.
// Unions, which are tagged unions, with one int beforehand to store the tag. These do not have to have different types, as the tag is just an index. These types can be cast to larger unions, or split, with the split operator.
// Sums, which are record types. A sum can be cast to another sum if all elements cast accordingly. A sum type can be indexed into as well.
// Lambdas, which are functions. Multiple arguments are represented as a sum. Can cast to another lambda where the arguments and return type cast accordingly. Can also curry???
// Overloads, which are essentially sums, where each type is unique (up to casts) and can cast to any of its constituents. Can cast to a subset overload.

// Basically the rules are the following, where sums are <a,b>, unions are <a|b>, lambdas are <a->b> and overloads are <a&b>:
// if a casts to A:

bool operator==(const Type& a, const Type& b)
{
    if (std::holds_alternative<AtomicType>(a) && std::holds_alternative<AtomicType>(b))
    {
        return std::get<AtomicType>(a) == std::get<AtomicType>(b);
    }
    else if (std::holds_alternative<UnionType>(a) && std::holds_alternative<UnionType>(b))
    {
        auto av = std::get<UnionType>(a).values;
        auto bv = std::get<UnionType>(b).values;
        if (av.size() != bv.size()) return false;
        for (int i = 0; i < av.size(); i++) if (av[i].Get() != bv[i].Get()) return false;
        return true;
    }
    else if (std::holds_alternative<OverloadType>(a) && std::holds_alternative<OverloadType>(b))
    {
        auto av = std::get<OverloadType>(a).values;
        auto bv = std::get<OverloadType>(b).values;
        if (av.size() != bv.size()) return false;
        for (int i = 0; i < av.size(); i++) if (av[i].Get() != bv[i].Get()) return false;
        return true;
    }
    else if (std::holds_alternative<RecordType>(a) && std::holds_alternative<RecordType>(b))
    {
        auto av = std::get<RecordType>(a).values;
        auto bv = std::get<RecordType>(b).values;
        if (av.size() != bv.size()) return false;
        for (int i = 0; i < av.size(); i++) if (av[i].Get() != bv[i].Get()) return false;
        return true;
    }
    else if (std::holds_alternative<LambdaType>(a) && std::holds_alternative<LambdaType>(b))
    {
        return std::get<LambdaType>(a).arg.Get() == std::get<LambdaType>(b).arg.Get() && std::get<LambdaType>(a).ret.Get() == std::get<LambdaType>(b).ret.Get();
    }
    else
    {
        return false;
    }
}

bool operator!=(const Type& a, const Type& b)
{
    return !(a == b);
}


bool ParseType(VectorView<Token> tokens, ParsingContext& ctx, Type& outType, int& tokensConsumed, TypeParsingPrecedence tp)
{
    if (tp == TypeParsingPrecedence::Atomic)
    {
        if (tokens[0].type == TokenType::Text)
        {
            if (ctx.typedefs.count(tokens[0].value))
            {
                outType = ctx.typedefs[tokens[0].value];
                tokensConsumed = 1;
                return true;
            }
            else
            {
                ctx.errors.push_back({ "Unknown atomic type: " + tokens[0].value + ".", tokens[0].line, tokens[0].column });
                outType = AtomicType::Error;
                tokensConsumed = 1;
                return true;
            }
        }
        else
        {
            return false;
        }
    }
    else if (tp == TypeParsingPrecedence::Bracket && tokens[0].type == TokenType::Symbol && tokens[0].value == "(")
    {
        if (!ParseType(tokens.SubView(1), ctx, outType, tokensConsumed)) return false;
        if (tokens[tokensConsumed + 1].type != TokenType::Symbol || tokens[tokensConsumed + 1].value != ")")
        {
            ctx.errors.push_back({ "No closing bracket.", tokens[0].line, tokens[0].column });
            return false;
        }
        else
        {
            tokensConsumed += 2;
            return true;
        }
    }

    if (!ParseType(tokens, ctx, outType, tokensConsumed, (TypeParsingPrecedence)((int)tp + 1))) return false;

    if (tp == TypeParsingPrecedence::Record)
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == ",")
        {
            std::vector<HeapAlloc<Type>> recordVec = { { outType } };
            while (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == ",")
            {
                tokensConsumed += 1;
                int consumed = 0;
                if (!ParseType(tokens.SubView(tokensConsumed), ctx, outType, consumed, TypeParsingPrecedence::Union))
                {
                    if (recordVec.size() > 1) outType = RecordType{ recordVec };
                    else outType = recordVec[0].Get();
                    tokensConsumed -= 1;
                    return true;
                }
                tokensConsumed += consumed;
                recordVec.push_back({ outType });
            }
            outType = RecordType{ recordVec };
            return true;
        }
        else
        {
            return true;
        }
    }
    else if (tp == TypeParsingPrecedence::Union)
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "|")
        {
            std::vector<HeapAlloc<Type>> unionVec = { { outType } };
            while (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "|")
            {
                tokensConsumed += 1;
                int consumed = 0;
                if (!ParseType(tokens.SubView(tokensConsumed), ctx, outType, consumed, TypeParsingPrecedence::Overload))
                {
                    if (unionVec.size() > 1) outType = UnionType{ unionVec };
                    else outType = unionVec[0].Get();
                    tokensConsumed -= 1;
                    return true;
                }
                tokensConsumed += consumed;
                unionVec.push_back({ outType });
            }
            outType = UnionType{ unionVec };
            return true;
        }
        else
        {
            return true;
        }
    }
    else if (tp == TypeParsingPrecedence::Overload)
    {
        if (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "&")
        {
            std::vector<HeapAlloc<Type>> overloadVec = { { outType } };
            while (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "&")
            {
                tokensConsumed += 1;
                int consumed = 0;
                if (!ParseType(tokens.SubView(tokensConsumed), ctx, outType, consumed, TypeParsingPrecedence::Lambda))
                {
                    if (overloadVec.size() > 1) outType = OverloadType{ overloadVec };
                    else outType = overloadVec[0].Get();
                    tokensConsumed -= 1;
                    return true;
                }
                tokensConsumed += consumed;
                overloadVec.push_back({ outType });
            }
            outType = OverloadType{ overloadVec };
            return true;
        }
        else
        {
            return true;
        }
    }
    else if (tp == TypeParsingPrecedence::Lambda)
    {
        while (tokens[tokensConsumed].type == TokenType::Symbol && tokens[tokensConsumed].value == "-" && tokens[tokensConsumed + 1].type == TokenType::Symbol && tokens[tokensConsumed + 1].value == ">")
        {
            tokensConsumed += 2;
            int consumed = 0;
            Type t;
            if (!ParseType(tokens.SubView(tokensConsumed), ctx, t, consumed, TypeParsingPrecedence::Bracket))
            {
                tokensConsumed -= 2;
                break;
            }
            tokensConsumed += consumed;
            outType = LambdaType{ { outType }, { t } };
        }
        return true;
    }
    else if (tp == TypeParsingPrecedence::Bracket)
    {
        return true;
    }
    else
    {
        Assert(false, "How did we get here (atomic type)");
        return false;
    }
}

std::string TypeToString(Type t)
{
    if (std::holds_alternative<AtomicType>(t)) return "Atom{" + std::to_string((int)std::get<AtomicType>(t)) + "}";
    if (std::holds_alternative<UnionType>(t))
    {
        std::string out = "Union{";
        for (HeapAlloc<Type>& i : std::get<UnionType>(t).values)
        {
            out += TypeToString(i.Get()) + ",";
        }
        if (out[out.size() - 1] == ',') out = out.substr(0, out.size() - 1);
        out += "}";
        return out;
    }
    if (std::holds_alternative<RecordType>(t))
    {
        std::string out = "Record{";
        for (HeapAlloc<Type>& i : std::get<RecordType>(t).values)
        {
            out += TypeToString(i.Get()) + ",";
        }
        if (out[out.size() - 1] == ',') out = out.substr(0, out.size() - 1);
        out += "}";
        return out;
    }
    if (std::holds_alternative<OverloadType>(t))
    {
        std::string out = "Overload{";
        for (HeapAlloc<Type>& i : std::get<OverloadType>(t).values)
        {
            out += TypeToString(i.Get()) + ",";
        }
        if (out[out.size() - 1] == ',') out = out.substr(0, out.size() - 1);
        out += "}";
        return out;
    }
    if (std::holds_alternative<LambdaType>(t))
    {
        return "Lambda{" + TypeToString(std::get<LambdaType>(t).arg.Get()) + "," + TypeToString(std::get<LambdaType>(t).ret.Get()) + "}";
    }
    return "huh?";
}

int main()
{
    while (true)
    {
        std::cout << "Enter possible type: ";
        std::string temp; std::getline(std::cin, temp);

        std::vector<Token> tokens = Tokenize(temp);
        for (Token& i : tokens)
        {
             std::cout << i.value << std::endl;
        }
        ParsingContext pc; Type t; int n = 0;
        std::cout << (ParseType({tokens, 0}, pc, t, n) ? "Parsing worked!" : "Parsing failed.") << std::endl;
        std::cout << TypeToString(t) << std::endl;
        std::cout << n << " tokens parsed." << std::endl;
    }
}

