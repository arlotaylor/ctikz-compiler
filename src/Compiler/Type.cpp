#include "Type.h"
#include "Lexer.h"
#include "Assertion.h"
#include "Parser.h"
#include <variant>

// This language's types fall into five categories
// Atomic Types, which are simple data (sometimes in TrackedPointers). This category also includes bookkeeping types like Error, Void and EndSum, as well as Template.
// Unions, which are tagged unions, with one int beforehand to store the tag. These do not have to have different types, as the tag is just an index. These types can be cast to larger unions, or split, with the split operator.
// Sums, which are record types. A sum can be cast to another sum if all elements cast accordingly. A sum type can be indexed into as well.
// Lambdas, which are functions. Multiple arguments are represented as a sum. Can cast to another lambda where the arguments and return type cast accordingly. Can also curry???
// Overloads, which are essentially sums, where each type is unique (up to casts) and can cast to any of its constituents. Can cast to a subset overload.
//
// We do not allow unions, overloads and records to contain only one item. As it is (probably) impossible to define such a type, we do not provide a nice error, instead asserting that there be more than one value. In the case of unions and overloads, we allow redundant copies of types, but these are not retained after parsing.

// Basically the rules are the following, where sums are <a,b>, unions are <a|b>, lambdas are <a->b> and overloads are <a&b>:

UnionType::UnionType(std::vector<HeapAlloc<Type>> v)
    : values(v)
{
    Assert(values.size() > 1, "How did you construct a union with less than two elements?");

    for (int i = 0; i < values.size(); i++)
    {
        for (int j = i + 1; j < values.size(); j++)
        {
            if (values[i].Get() == values[j].Get())
            {
                values.erase(values.begin() + j);
                j--;
            }
        }
    }
}

OverloadType::OverloadType(std::vector<HeapAlloc<Type>> v)
    : values(v)
{
    Assert(values.size() > 1, "How did you construct an overload with less than two elements?");

    for (int i = 0; i < values.size(); i++)
    {
        for (int j = i + 1; j < values.size(); j++)
        {
            if (values[i].Get() == values[j].Get())
            {
                values.erase(values.begin() + j);
                j--;
            }
        }
    }
}

RecordType::RecordType(std::vector<HeapAlloc<Type>> v)
    : values(v)
{
    Assert(values.size() > 1, "How did you construct a record with less than two elements?");
}

LambdaType::LambdaType(HeapAlloc<Type> a, HeapAlloc<Type> r) : arg(a), ret(r), isTemplate(false)
{
    if (a.Get() == AtomicType::Template)
    {
        isTemplate = true;
    }
    else
    {
        Assert(std::holds_alternative<OverloadType>(a.Get()));
        for (HeapAlloc<Type>& t : std::get<OverloadType>(a.Get()).values)
        {
            if (t.Get() == AtomicType::Template)
            {
                isTemplate = true;
                break;
            }
        }
    }
}


void LambdaType::CheckArgDef(int instArg, int definition, std::vector<ErrorOutput>& errors)
{
    VectorView<Token>& vec = definitions[definition];
    Type& a = instantiatedArgs[instArg].Get();

    Assert(vec[0].type == TokenType::Symbol && vec[0].value == "(");
    Expression expr = LiteralExpression{ AtomicType::Error, vec };
    int consumed = 0;
    ParsingContext pc;  // TODO: figure out how to get the right parsing context
    if (!ParseExpression<ExpressionParsingPrecedence::MultiVarDef>(vec.SubView(1), pc, expr, consumed)) Assert(false);
    Assert(vec[1+consumed].type == TokenType::Symbol && vec[1+consumed].value == ")");

    if (std::holds_alternative<VariableExpression>(expr))
    {
        Assert(GetExpressionType(expr) == AtomicType::Template);
        pc.varStack[std::get<VariableExpression>(expr).stackIndex].second = a;
    }
    else
    {
        Assert(std::holds_alternative<MultiExpression>(expr));
        Assert(std::holds_alternative<RecordType>(a) && std::get<RecordType>(a).values.size() == std::get<MultiExpression>(expr).elements.size());

        for (int i = 0; i < std::get<MultiExpression>(expr).elements.size(); i++)
        {
            if (GetExpressionType(std::get<MultiExpression>(expr).elements[i].Get()) == AtomicType::Template)
            {
                pc.varStack[std::get<VariableExpression>(std::get<MultiExpression>(expr).elements[i].Get()).stackIndex].second = std::get<RecordType>(a).values[i].Get();
            }
        }
    }

    // Parsing context is ready, time to parse
    Statement oStat = SingleStatement{ { LiteralExpression{ AtomicType::Error, vec } } };
    if (!ParseStatement(vec.SubView(consumed + 2), pc, oStat, consumed)) Assert(false);  // this should be true, because the statement should have already been checked, if not type checked. The important part here is that we add the relevant errors to the parsing context.
    for (ErrorOutput& err : pc.errors)
    {
        errors.push_back(err);
    }
}

void LambdaType::AddInstArgs(Type a, std::vector<ErrorOutput>& errors)
{
    Assert(isTemplate);

    instantiatedArgs.push_back({ a });
    for (int i = 0; i < definitions.size(); i++)
    {
        CheckArgDef(instantiatedArgs.size() - 1, i, errors);
    }
}

void LambdaType::AddDefinition(VectorView<Token> tokens, std::vector<ErrorOutput>& errors)
{
    Assert(isTemplate);

    definitions.push_back(tokens);
    for (int i = 0; i < instantiatedArgs.size(); i++)
    {
        CheckArgDef(i, definitions.size() - 1, errors);
    }
}



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


// Remember, Unions cast up, Overloads cast down.
bool CheckCast(Type from, Type to)
{
    if (from == to) return true;

    if (std::holds_alternative<OverloadType>(from))  // overload from case
    {
        for (HeapAlloc<Type>& i : std::get<OverloadType>(from).values)
        {
            if (CheckCast(i.Get(), to)) return true;  // can cast an overload to one of its elements
        }

        if (std::holds_alternative<OverloadType>(to))  // casting an overload down
        {
            for (HeapAlloc<Type>& i : std::get<OverloadType>(to).values)
            {
                bool didMatch = false;
                for (HeapAlloc<Type>& j : std::get<OverloadType>(from).values)
                {
                    if (i.Get() == j.Get())
                    {
                        didMatch = true;
                        break;
                    }
                }
                if (!didMatch) return false;  // new overload had a type that was not in the old overload
            }
            return true;  // new overload is a subset of the old overload
        }
        else
        {
            return false;  // nothing worked
        }
    }

    if (std::holds_alternative<UnionType>(to))  // to union case
    {
        for (HeapAlloc<Type>& i : std::get<UnionType>(to).values)
        {
            if (CheckCast(from, i.Get())) return true;  // can cast to a union containing you
        }

        if (std::holds_alternative<UnionType>(from))  // casting a union up
        {
            for (HeapAlloc<Type>& i : std::get<UnionType>(from).values)
            {
                bool didMatch = false;
                for (HeapAlloc<Type>& j : std::get<UnionType>(to).values)
                {
                    if (i.Get() == j.Get())
                    {
                        didMatch = true;
                        break;
                    }
                }
                if (!didMatch) return false;  // old union had a type that was not in the new overload
            }
            return true;  // old union is a subset of the new union
        }
        else
        {
            return false;  // nothing worked
        }
    }

    if (std::holds_alternative<RecordType>(from))
    {
        if (std::holds_alternative<RecordType>(to))
        {
            if (std::get<RecordType>(from).values.size() == std::get<RecordType>(to).values.size())
            {
                for (int i = 0; i < std::get<RecordType>(from).values.size(); i++)
                {
                    if (!CheckCast(std::get<RecordType>(from).values[i].Get(), std::get<RecordType>(to).values[i].Get()))
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }

    if (std::holds_alternative<AtomicType>(from))
    {
        AtomicType fromVal = std::get<AtomicType>(from);

        if (std::holds_alternative<AtomicType>(to))
        {
            AtomicType toVal = std::get<AtomicType>(to);

            if ((fromVal == AtomicType::Integer || fromVal == AtomicType::Double || fromVal == AtomicType::Boolean)
             && (toVal == AtomicType::Integer || toVal == AtomicType::Double || toVal == AtomicType::Boolean)) return true;
        }

        return false;
    }

    return false;
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
                ctx.errors.push_back({ "Unknown atomic type: " + tokens[0].value + ".", tokens[0].pos });
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
        if (!ParseType(tokens.SubView(1), ctx, outType, tokensConsumed, TypeParsingPrecedence::Record)) return false;
        if (tokens[tokensConsumed + 1].type != TokenType::Symbol || tokens[tokensConsumed + 1].value != ")")
        {
            ctx.errors.push_back({ "No closing bracket.", tokens[0].pos });
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

