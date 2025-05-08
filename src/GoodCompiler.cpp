// #include <cctype>
#include <string>
#include <iostream>  // for logging
#include <vector>  // for reflections
#include <map>
#include <variant>

#pragma region Lexer

std::string RemoveStrings(std::string str)
{
    std::string ret = "";
    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t' || str[i] == '\r') continue;
        ret += str[i];
    }
    return ret + " ";  // add one to the end as a buffer
}

#pragma endregion

#pragma region Types

enum class AtomicType
{
    Error = -1,
    Void = 0,
    EndSum,
    Template,
    Int,
    Double,
    String,
    Drawable,
};

struct UnionType; struct SumType; struct LambdaType;

typedef std::variant<AtomicType, UnionType, SumType, LambdaType> LanguageType;

struct UnionType
{
    LanguageType* a;
    LanguageType* b;

    UnionType(const LanguageType& aa, const LanguageType& bb);

    ~UnionType();
};

struct SumType
{
    LanguageType* a;
    LanguageType* b;

    SumType(const LanguageType& aa, const LanguageType& bb);

    ~SumType();
};

struct LambdaType
{
    LanguageType* argumentType;
    LanguageType* returnType;

    LambdaType(const LanguageType& aa, const LanguageType& bb);

    ~LambdaType();
};




UnionType::UnionType(const LanguageType& aa, const LanguageType& bb)
{
    if (std::holds_alternative<UnionType>(aa)) std::cout << "Warning: Union Type should not contain Union type as first child.\n";

    a = new LanguageType{aa};
    b = new LanguageType{bb};
}

UnionType::~UnionType()
{
    delete a;
    delete b;
}

SumType::SumType(const LanguageType& aa, const LanguageType& bb)
{
    a = new LanguageType{aa};
    b = new LanguageType{bb};
    // if (std::holds_alternative<SumType*>(aa)) std::cout << "Warning: Sum Type should not contain Sum type as first child.\n";
    // you have to be careful here
}

SumType::~SumType()
{
    delete a;
    delete b;
}

LambdaType::LambdaType(const LanguageType& aa, const LanguageType& bb)
{
    argumentType = new LanguageType{aa};
    returnType = new LanguageType{bb};
}

LambdaType::~LambdaType()
{
    delete argumentType;
    delete returnType;
}


#pragma endregion

#pragma region Compiler

enum class GrammarType
{
    Integer,
    Decimal,
    StringLit,
    Identifier,
    Arguments,
    FunctionCall,
    ParenExpr,
    ExpExpr,
    MulExpr,
    DivExpr,
    ModExpr,
    AddExpr,
    SubExpr,
    EqExpr,
    NeqExpr,  // no bitwise operations
    AndExpr,
    OrExpr,
    VarAssignment,
    Expression,  // set var first
    Statement,
    Block,
    Scope,
    IfStatement,
    WhileStatement,
    ForStatement,
    Lambda,
};

const std::vector<std::string> GRAMMAR_TYPE_NAMES = {
    "Integer",
    "Decimal",
    "StringLit",
    "Identifier",
    "Arguments",
    "FunctionCall",
    "ParenExpr",
    "ExpExpr",
    "MulExpr",
    "DivExpr",
    "ModExpr",
    "AddExpr",
    "SubExpr",
    "EqExpr",
    "NeqExpr",  // no bitwise operations
    "AndExpr",
    "OrExpr",
    "VarAssignment",
    "Expression",  // set var first
    "Statement",
    "Block",
    "Scope",
    "IfStatement",
    "WhileStatement",
    "ForStatement",
    "Lambda",
};


struct IntRange
{
    int begin, end;
};

struct ASTGNode
{
    GrammarType type;
    IntRange range;
    ASTGNode* parent = nullptr;
    ASTGNode* left = nullptr;
    ASTGNode* right = nullptr;
    ASTGNode* child = nullptr;
    int functionIndex = -1;

    ~ASTGNode() // it is the responsibility of parent or left to call this
    {
        if (child != nullptr) delete child;
        if (right != nullptr) delete right;
    }
};

std::string ASTGNodeToString(ASTGNode* node, const std::string& str)
{
    std::string ret = GRAMMAR_TYPE_NAMES[(int)node->type] + "{";
    if (node->child != nullptr)
    {
        ret += ASTGNodeToString(node->child, str);
    }
    else
    {
        ret += str.substr(node->range.begin, node->range.end - node->range.begin);
    }
    ret += "}";
    if (node->right != nullptr)
    {
        ret += ", ";
        ret += ASTGNodeToString(node->right, str);
    }
    return ret;
}

ASTGNode* PrintNode(ASTGNode* node, const std::string& str, GrammarType caller, bool force = false)
{
    if (node == nullptr) return node;
    if (force || false)
    {
        std::cout << GRAMMAR_TYPE_NAMES[(int)caller] << ": " << ASTGNodeToString(node, str) << std::endl;
    }
    return node;
}


ASTGNode* GenerateParentNode(GrammarType t, IntRange r)
{
    return new ASTGNode{ t, r };
}

template<typename... Args>
ASTGNode* GenerateParentNode(GrammarType t, IntRange r, ASTGNode* child1, Args... args)
{
    if (child1->left != nullptr || child1->right != nullptr || child1->parent != nullptr) std::cout << "Adding a node with siblings or a parent to new parent may fail...\n";

    ASTGNode* parent = GenerateParentNode(t, r, args...);
    child1->parent = parent;

    if (parent->child == nullptr)
    {
        parent->child = child1;
    }
    else
    {
        child1->right = parent->child;
        parent->child->left = child1;
        parent->child = child1;
    }

    return parent;
}

template<GrammarType T> ASTGNode* Parse(const std::string& str, int pos) = delete;

template<> ASTGNode* Parse<GrammarType::Integer>(const std::string& str, int pos)
{
    if (std::isdigit(str[pos]))
    {
        int newpos = pos + 1;
        while (std::isdigit(str[newpos])) newpos++;

        return PrintNode(GenerateParentNode(GrammarType::Integer, {pos, newpos}), str, GrammarType::Integer);
    }
    else
    {
        return nullptr;
    }
}

template<> ASTGNode* Parse<GrammarType::Decimal>(const std::string& str, int pos)
{
    ASTGNode* v = Parse<GrammarType::Integer>(str, pos);
    if (v != nullptr)
    {
        if (str[v->range.end] == '.')
        {
            ASTGNode* v2 = Parse<GrammarType::Integer>(str, v->range.end + 1);
            if (v2 != nullptr)
            {
                return PrintNode(GenerateParentNode(GrammarType::Decimal, {pos, v2->range.end}, v, v2), str, GrammarType::Decimal);
            }
        }
        delete v;
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::StringLit>(const std::string& str, int pos)
{
    if (str[pos] != '"') return nullptr;

    int newpos = pos + 1;
    bool isEscaped = false;
    while (str[newpos] != '"' || isEscaped)
    {
        if (isEscaped) isEscaped = false;
        else if (str[newpos] == '\\') isEscaped = true;
        newpos += 1;
    }

    if (str[newpos] != '"') return nullptr;

    return PrintNode(GenerateParentNode(GrammarType::StringLit, {pos, newpos + 1}), str, GrammarType::StringLit);
}

template<> ASTGNode* Parse<GrammarType::Identifier>(const std::string& str, int pos)
{
    if (!std::isalpha(str[pos])) return nullptr;

    int newpos = pos + 1;
    while (std::isalnum(str[newpos]) || str[newpos] == '_') newpos++;

    return PrintNode(GenerateParentNode(GrammarType::Identifier, {pos, newpos}), str, GrammarType::Identifier);
}


template<> ASTGNode* Parse<GrammarType::Expression>(const std::string& str, int pos);


template<> ASTGNode* Parse<GrammarType::Arguments>(const std::string& str, int pos)
{
    // this function doesn't fail, just returns empty arg string
    ASTGNode* ret = GenerateParentNode(GrammarType::Arguments, {pos, pos - 1});
    ASTGNode* prev = nullptr;

    do
    {
        ASTGNode* next = Parse<GrammarType::Expression>(str, ret->range.end + 1);
        if (next == nullptr)
        {
            break;
        }

        next->parent = ret;
        if (prev == nullptr)  // first arg
        {
            ret->child = next;
        }
        else  // other arg
        {
            prev->right = next;
            next->left = prev;
        }
        prev = next;
        ret->range.end = next->range.end;
    }
    while (str[ret->range.end] == ',');

    if (ret->range.begin > ret->range.end) ret->range.end = ret->range.begin;
    return PrintNode(ret, str, GrammarType::Arguments);
}


template<> ASTGNode* Parse<GrammarType::FunctionCall>(const std::string& str, int pos)
{
    ASTGNode* name = Parse<GrammarType::Identifier>(str, pos);
    if (name != nullptr)
    {
        if (str[name->range.end] == '(')
        {
            ASTGNode* args = Parse<GrammarType::Arguments>(str, name->range.end + 1);
            if (args != nullptr)  // should always be true
            {
                if (str[args->range.end] == ')')
                {
                    return PrintNode(GenerateParentNode(GrammarType::FunctionCall, {pos, args->range.end+1}, name, args), str, GrammarType::FunctionCall);
                }
                delete args;
            }
        }
        delete name;
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::Lambda>(const std::string& str, int pos);

template<> ASTGNode* Parse<GrammarType::ParenExpr>(const std::string& str, int pos)
{
    if (str[pos] == '(')
    {
        ASTGNode* expr = Parse<GrammarType::Expression>(str, pos + 1);
        if (expr != nullptr)
        {
            if (str[expr->range.end] == ')')
            {
                return PrintNode(GenerateParentNode(GrammarType::ParenExpr, {pos, expr->range.end+1}, expr), str, GrammarType::ParenExpr);
            }

            delete expr;
        }
        return nullptr;
    }

    ASTGNode* dec = Parse<GrammarType::Decimal>(str, pos);  // this returns a different type, but maybe that's ok...
    if (dec != nullptr)
    {
        return PrintNode(dec, str, GrammarType::ParenExpr);
    }
    ASTGNode* num = Parse<GrammarType::Integer>(str, pos);  // this returns a different type, but maybe that's ok...
    if (num != nullptr)
    {
        return PrintNode(num, str, GrammarType::ParenExpr);
    }
    ASTGNode* strlit = Parse<GrammarType::StringLit>(str, pos);  // this returns a different type, but maybe that's ok...
    if (strlit != nullptr)
    {
        return PrintNode(strlit, str, GrammarType::ParenExpr);
    }
    ASTGNode* lambda = Parse<GrammarType::Lambda>(str, pos);  // this returns a different type, but maybe that's ok...
    if (lambda != nullptr)
    {
        return PrintNode(lambda, str, GrammarType::ParenExpr);
    }
    ASTGNode* funcCall = Parse<GrammarType::FunctionCall>(str, pos);  // this returns a different type, but maybe that's ok...
    if (funcCall != nullptr)
    {
        return PrintNode(funcCall, str, GrammarType::ParenExpr);
    }
    ASTGNode* varRef = Parse<GrammarType::Identifier>(str, pos);
    if (varRef != nullptr)
    {
        return PrintNode(varRef, str, GrammarType::ParenExpr);
    }

    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::ExpExpr>(const std::string& str, int pos)
{
    ASTGNode* lhs = Parse<GrammarType::ParenExpr>(str, pos);
    if (lhs != nullptr)
    {
        if (str[lhs->range.end] == '^')
        {
            ASTGNode* rhs = Parse<GrammarType::ExpExpr>(str, lhs->range.end+1);
            if (rhs != nullptr)
            {
                return PrintNode(GenerateParentNode(GrammarType::ExpExpr, {pos, rhs->range.end}, lhs, rhs), str, GrammarType::ExpExpr);
            }
        }
        else
        {
            return PrintNode(lhs, str, GrammarType::ExpExpr);
        }
        delete lhs;
    }

    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::MulExpr>(const std::string& str, int pos)
{
    ASTGNode* expr = Parse<GrammarType::ExpExpr>(str, pos);
    if (expr == nullptr) return nullptr;

    while (true)
    {
        if (str[expr->range.end] == '*')
        {
            ASTGNode* nexpr = Parse<GrammarType::ExpExpr>(str, expr->range.end + 1);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::MulExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else if (str[expr->range.end] == '/')
        {
            ASTGNode* nexpr = Parse<GrammarType::ExpExpr>(str, expr->range.end + 1);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::DivExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else if (str[expr->range.end] == '%')
        {
            ASTGNode* nexpr = Parse<GrammarType::ExpExpr>(str, expr->range.end + 1);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::ModExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else
        {
            return PrintNode(expr, str, GrammarType::MulExpr);
        }
    }

    delete expr;
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::AddExpr>(const std::string& str, int pos)
{
    ASTGNode* expr = Parse<GrammarType::MulExpr>(str, pos);
    if (expr == nullptr) return nullptr;

    while (true)
    {
        if (str[expr->range.end] == '+')
        {
            ASTGNode* nexpr = Parse<GrammarType::MulExpr>(str, expr->range.end + 1);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::AddExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else if (str[expr->range.end] == '-')
        {
            ASTGNode* nexpr = Parse<GrammarType::MulExpr>(str, expr->range.end + 1);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::SubExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else
        {
            return PrintNode(expr, str, GrammarType::AddExpr);
        }
    }

    delete expr;
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::EqExpr>(const std::string& str, int pos)
{
    ASTGNode* expr = Parse<GrammarType::AddExpr>(str, pos);
    if (expr == nullptr) return nullptr;

    while (true)
    {
        if (str.substr(expr->range.end, 2) == "==")
        {
            ASTGNode* nexpr = Parse<GrammarType::AddExpr>(str, expr->range.end + 2);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::EqExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        if (str.substr(expr->range.end, 2) == "!=")
        {
            ASTGNode* nexpr = Parse<GrammarType::AddExpr>(str, expr->range.end + 2);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::NeqExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else
        {
            return PrintNode(expr, str, GrammarType::EqExpr);
        }
    }

    delete expr;
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::AndExpr>(const std::string& str, int pos)
{
    ASTGNode* expr = Parse<GrammarType::EqExpr>(str, pos);
    if (expr == nullptr) return nullptr;

    while (true)
    {
        if (str.substr(expr->range.end, 2) == "&&")
        {
            ASTGNode* nexpr = Parse<GrammarType::EqExpr>(str, expr->range.end + 2);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::AndExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        if (str.substr(expr->range.end, 2) == "||")
        {
            ASTGNode* nexpr = Parse<GrammarType::EqExpr>(str, expr->range.end + 2);
            if (nexpr == nullptr)
            {
                break;
            }
            expr = GenerateParentNode(GrammarType::OrExpr, {pos, nexpr->range.end}, expr, nexpr);
        }
        else
        {
            return PrintNode(expr, str, GrammarType::AndExpr);
        }
    }

    delete expr;
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::VarAssignment>(const std::string& str, int pos)
{
    ASTGNode* name = Parse<GrammarType::Identifier>(str, pos);
    if (name != nullptr)
    {
        if (str[name->range.end] == '=' && str.substr(name->range.end, 2) != "==")
        {
            ASTGNode* val = Parse<GrammarType::AndExpr>(str, name->range.end + 1);

            if (val != nullptr)
            {
                return PrintNode(GenerateParentNode(GrammarType::VarAssignment, {pos, val->range.end}, name, val), str, GrammarType::VarAssignment);
            }
        }
        delete name;
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::Expression>(const std::string& str, int pos)
{
    ASTGNode* assign = Parse<GrammarType::VarAssignment>(str, pos);
    if (assign != nullptr)
    {
        return PrintNode(assign, str, GrammarType::Expression);
    }
    return Parse<GrammarType::AndExpr>(str, pos);
}

template<> ASTGNode* Parse<GrammarType::Statement>(const std::string& str, int pos);

template<> ASTGNode* Parse<GrammarType::Block>(const std::string& str, int pos)
{
    // this function doesn't fail, just returns empty arg string
    ASTGNode* ret = GenerateParentNode(GrammarType::Block, {pos, pos});
    ASTGNode* prev = nullptr;

    while (true)
    {
        ASTGNode* next = Parse<GrammarType::Statement>(str, ret->range.end);
        if (next == nullptr)
        {
            break;
        }

        next->parent = ret;
        if (prev == nullptr)  // first arg
        {
            ret->child = next;
        }
        else  // other arg
        {
            prev->right = next;
            next->left = prev;
        }
        prev = next;
        ret->range.end = next->range.end;
    }

    return PrintNode(ret, str, GrammarType::Block);

}

template<> ASTGNode* Parse<GrammarType::Scope>(const std::string& str, int pos)
{
    if (str[pos] == '{')
    {
        ASTGNode* block = Parse<GrammarType::Block>(str, pos + 1);
        if (block != nullptr)  // always true
        {
            if (str[block->range.end] == '}')
            {
                return PrintNode(GenerateParentNode(GrammarType::Scope, {pos, block->range.end + 1}, block), str, GrammarType::Scope);
            }
            delete block;
        }
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::IfStatement>(const std::string& str, int pos)
{
    if (str.substr(pos,3) == "if(")
    {
        ASTGNode* check = Parse<GrammarType::Expression>(str, pos + 3);
        if (check != nullptr)
        {
            if (str[check->range.end] == ')')
            {
                ASTGNode* code = Parse<GrammarType::Scope>(str, check->range.end + 1);
                if (code != nullptr)  // should always be true
                {
                    return PrintNode(GenerateParentNode(GrammarType::IfStatement, {pos, code->range.end}, check, code), str, GrammarType::IfStatement);
                }
            }
            delete check;
        }
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::WhileStatement>(const std::string& str, int pos)
{
    if (str.substr(pos,6) == "while(")
    {
        ASTGNode* check = Parse<GrammarType::Expression>(str, pos + 6);
        if (check != nullptr)
        {
            if (str[check->range.end] == ')')
            {
                ASTGNode* code = Parse<GrammarType::Scope>(str, check->range.end + 1);
                if (code != nullptr)  // should always be true
                {
                    return PrintNode(GenerateParentNode(GrammarType::WhileStatement, {pos, code->range.end}, check, code), str, GrammarType::WhileStatement);
                }
            }
            delete check;
        }
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::ForStatement>(const std::string& str, int pos)
{
    if (str.substr(pos,4) == "for(")
    {
        ASTGNode* st1 = Parse<GrammarType::Expression>(str, pos + 4);
        if (st1 != nullptr)
        {
            if (str[st1->range.end] == ';')
            {
                ASTGNode* st2 = Parse<GrammarType::Expression>(str, st1->range.end + 1);
                if (st2 != nullptr)
                {
                    if (str[st2->range.end] == ';')
                    {
                        ASTGNode* st3 = Parse<GrammarType::Expression>(str, st2->range.end + 1);
                        if (st3 != nullptr)
                        {
                            if (str[st3->range.end] == ')')
                            {
                                ASTGNode* code = Parse<GrammarType::Scope>(str, st3->range.end + 1);
                                if (code != nullptr)  // should always be true
                                {
                                    return PrintNode(GenerateParentNode(GrammarType::ForStatement, {pos, code->range.end}, st1, st2, st3, code), str, GrammarType::ForStatement);
                                }
                            }
                            delete st3;
                        }
                    }
                    delete st2;
                }
            }
            delete st1;
        }
    }

    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::Statement>(const std::string& str, int pos)
{
    ASTGNode* scst = Parse<GrammarType::Scope>(str, pos);
    if (scst != nullptr)
    {
        return PrintNode(scst, str, GrammarType::Statement);
    }
    ASTGNode* ifst = Parse<GrammarType::IfStatement>(str, pos);
    if (ifst != nullptr)
    {
        return PrintNode(ifst, str, GrammarType::Statement);
    }
    ASTGNode* whst = Parse<GrammarType::WhileStatement>(str, pos);
    if (whst != nullptr)
    {
        return PrintNode(whst, str, GrammarType::Statement);
    }
    ASTGNode* frst = Parse<GrammarType::ForStatement>(str, pos);
    if (frst != nullptr)
    {
        return PrintNode(frst, str, GrammarType::Statement);
    }

    ASTGNode* expr = Parse<GrammarType::Expression>(str, pos);
    if (expr != nullptr)
    {
        if (str[expr->range.end] == ';')
        {
            return PrintNode(GenerateParentNode(GrammarType::Statement, {pos, expr->range.end + 1}, expr), str, GrammarType::Statement);
        }
        delete expr;
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarType::Lambda>(const std::string& str, int pos)
{
    if (str.substr(pos, 7) == "lambda(")
    {
        ASTGNode* args = Parse<GrammarType::Arguments>(str, pos + 7);
        if (args != nullptr)
        {
            if (str[args->range.end] == ')')
            {
                ASTGNode* scope = Parse<GrammarType::Scope>(str, args->range.end + 1);
                if (scope != nullptr)
                {
                    return PrintNode(GenerateParentNode(GrammarType::Lambda, {pos, scope->range.end}), str, GrammarType::Lambda);
                }
            }
            delete args;
        }
    }
    return nullptr;
}


#pragma endregion

#pragma region TypeChecker

struct VarSignature
{
    std::string name;
    LanguageType type = AtomicType::Template;
};

const std::vector<VarSignature> s_BuiltinSignatures = {
    { "__toInt", AtomicType::Double },
    { "__toDouble", AtomicType::Int },
    { "__operator+",  LambdaType{ SumType{ AtomicType::Int, AtomicType::Int }, AtomicType::Int } },
    { "__operator+", LambdaType{ SumType{ AtomicType::Double, AtomicType::Double }, AtomicType::Double } },
    { "__operator+", LambdaType{ SumType{ AtomicType::String, AtomicType::String }, AtomicType::String } },
};

enum CheckCastOut
{
    Failure = 0,
    Template = 1,
    Cast = 2,
    Success = 3,
};

CheckCastOut CheckCast(const LanguageType& a, const LanguageType& b)
{
    if (std::holds_alternative<AtomicType>(a) && std::get<AtomicType>(a) == AtomicType::Template) return CheckCastOut::Template;
    if (std::holds_alternative<AtomicType>(b) && std::get<AtomicType>(b) == AtomicType::Template) return CheckCastOut::Template;

    if (std::holds_alternative<AtomicType>(a) && std::holds_alternative<AtomicType>(b))
    {
        if (std::get<AtomicType>(a) == std::get<AtomicType>(b)) return CheckCastOut::Success;
        if (std::get<AtomicType>(a) == AtomicType::Int && std::get<AtomicType>(b) == AtomicType::Double) return CheckCastOut::Cast;
        if (std::get<AtomicType>(a) == AtomicType::Double && std::get<AtomicType>(b) == AtomicType::Int) return CheckCastOut::Cast;
    }

    if (std::holds_alternative<UnionType>(a))
    {
        if (std::holds_alternative<UnionType>(b))
        {
            return std::max(CheckCast(a, *std::get<UnionType>(b).a), CheckCast(a, *std::get<UnionType>(b).b));
        }
        else
        {
            return std::max(CheckCast(*std::get<UnionType>(a).a, b), CheckCast(*std::get<UnionType>(a).b, b));
        }
    }

    if (std::holds_alternative<SumType>(a) && std::holds_alternative<SumType>(b))
    {
        return std::min(CheckCast(*std::get<SumType>(a).a, *std::get<SumType>(b).a), CheckCast(*std::get<SumType>(a).b, *std::get<SumType>(b).b));
    }

    if (std::holds_alternative<LambdaType>(a) && std::holds_alternative<LambdaType>(b))
    {
        return std::min(CheckCast(*std::get<LambdaType>(a).argumentType, *std::get<LambdaType>(b).argumentType), CheckCast(*std::get<LambdaType>(a).returnType, *std::get<LambdaType>(b).returnType));
    }

    return CheckCastOut::Failure;
}


template<typename T>
struct LinkedListNode  // for some reason felt the need to implement a linked list...
{
    T elem;
    int prev = -1;
    int next = -1;
};

template<typename T>
class LinkedList
{
    std::vector<LinkedListNode<T>>& container;
    int headIndex = -1;
    int tailIndex = -1;
    int size = 0;

public:
    LinkedList(std::vector<LinkedListNode<T>>& vec, int head, int tail, int count)
        : container(vec), headIndex(head), tailIndex(tail), size(count)
    {
    }

    template<typename... Args>
    void AddItems(const T& t, Args... args)
    {
        container.push_back({t});
        if (headIndex == -1)
        {
            headIndex = container.size() - 1;
            tailIndex = headIndex;
        }
        else
        {
            container.back().prev = tailIndex;
            container[tailIndex].next = container.size() - 1;
            tailIndex = container.size() - 1;
        }
        size++;
        if constexpr(sizeof...(args) != 0)
        {
            AddItems(args...);
        }
    }

    template<typename... Args>
    LinkedList(std::vector<LinkedListNode<T>>& vec, const T& t, Args... args)
        : container(vec)
    {
        AddItems(t, args...);
    }

    bool IsValid()
    {
        if (container[headIndex].prev != -1 || container[tailIndex].next != -1) return false;
        return true;
    }

    int GetSize()
    {
        return size;
    }

    T& operator[](int index)
    {
        if (!IsValid()) std::cout << "Warning: a linked list should not be used after it has been added to another.\n";

        LinkedListNode<T>& ll = container[headIndex];
        for (int i = 0; i < index; i++)
        {
            ll = container[ll.next];
        }
        return ll.elem;
    }

    LinkedList<T> operator+(const LinkedList<T>& other)
    {
        if (!IsValid()) std::cout << "Warning: a linked list should not be used after it has been added to another.\n";

        if (!other.IsValid()) std::cout << "Warning: a linked list should not be used after it has been added to another.\n";

        // stitch together
        container[tailIndex].next = other.headIndex;
        container[other.headIndex].prev = tailIndex;

        return { container, headIndex, other.tailIndex, GetSize() + other.GetSize() };
    }

    LinkedList<T> Copy()
    {
        if (!IsValid()) std::cout << "Warning: a linked list should not be used after it has been added to another.\n";

        LinkedList<T> ret = { container, -1, -1, 0 };
        LinkedListNode<T>& ll = container[headIndex];
        while (true)
        {
            ret.AddItems(ll.elem);
            if (ll.next == -1) break;
            ll = container[ll.next];
        }
        return ret;
    }
};


enum class InstructionType
{
    LoadValue,
    RunFunction,
    PopStack,
    GotoIf,
};

struct Instruction
{
    InstructionType type;
    int value;  // can be the value to load, the function to read, or the place to go to
};


struct ProgramContext
{
    std::vector<char> varStack;
    std::vector<int> stackPtrs;

    std::vector<char> exprStack;
};


void RunProgram(LinkedList<Instruction> inst, std::vector<char> initials)
{
    ProgramContext ctx = { initials, { initials.size() }, {} };

    for (int instPtr = 0; instPtr < inst.size(); inst++)
    {
        switch (inst[instPtr].type)
        {
        case InstructionType::LoadValue:
            // todo: keep going
        break;
        case InstructionType::RunFunction:
        break;
        case InstructionType::PopStack:
            if (ctx.stackPtrs.size() <= 1) return;
            while (ctx.varStack.size() > ctx.stackPtrs.back()) ctx.varStack.pop_back();
            ctx.stackPtrs.pop_back();
        break;
        case InstructionType::GotoIf:
            if (exprStack.back())
            {
                instPtr = inst[instPtr].value;
            }
            exprStack.pop_back();
        break;
        default: break;
        }
    }
}




int SearchVars(const std::vector<VarSignature>& vars, const VarSignature& elem, CheckCastOut* out = nullptr)
{
    for (int i = 0; i < vars.size(); i++)
    {
        if (vars[i].name != elem.name) continue;
        CheckCastOut o = CheckCast(vars[i].type, elem.type);
        if (o != CheckCastOut::Failure)
        {
            if (out != nullptr) *out = o;
            return i;
        }
    }

    if (out != nullptr) *out = CheckCastOut::Failure;
    return -1;
}

LanguageType TypeCheck(const std::string& str, ASTGNode* node, std::vector<VarSignature>& vars)
{
    if (node->type == GrammarType::Integer)
    {
        return AtomicType::Int;
    }
    else if (node->type == GrammarType::Decimal)
    {
        return AtomicType::Double;
    }
    else if (node->type == GrammarType::StringLit)
    {
        return AtomicType::String;
    }
    else if (node->type == GrammarType::Identifier)
    {
        int loc = SearchVars(vars, {str.substr(node->range.begin, node->range.end - node->range.begin)});
        if (loc == -1)
        {
            return AtomicType::Error;
        }
        else
        {
            return vars[loc].type;
        }
    }
    else if (node->type == GrammarType::FunctionCall)
    {
        if (node->left->type != GrammarType::Identifier) std::cout << "Error: Function call with invalid identifier???\n";
        LanguageType lt = TypeCheck(str, node->right, vars);

        CheckCastOut locType = CheckCastOut::Failure;
        int loc = SearchVars(vars, {str.substr(node->left->range.begin, node->left->range.end - node->left->range.begin), LambdaType{ lt, AtomicType::Template } }, &locType);
        if (loc == -1)
        {
            return AtomicType::Error;
        }

        LanguageType lt2 = vars[loc].type;
        if (!std::holds_alternative<LambdaType>(lt2))
        {
            return AtomicType::Error;  // shouldn't ever run
        }
        else
        {
            const LambdaType& lambdaType = std::get<LambdaType>(lt2);
            LanguageType argT = TypeCheck(str, node->right, vars);
            if (!CheckCast(*lambdaType.argumentType, argT))
            {
                return AtomicType::Error;
            }
        }
    }

}

#pragma endregion



int main()
{
    std::cout << "Enter mathematical expression: ";
    std::string expr; std::getline(std::cin, expr);
    std::cout << "Recieved " << expr << ".\n";

    ASTGNode* program = Parse<GrammarType::Block>(RemoveStrings(expr), 0);
    if (program != nullptr)
    {
        ASTGNode* ptr = program;

        std::cout << ASTGNodeToString(ptr, expr) << std::endl;
    }
    else
    {
        std::cout << "Syntax Error!\n";
    }
}


