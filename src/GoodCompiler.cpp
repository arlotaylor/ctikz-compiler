// #include <cctype>
#include <string>
#include <iostream>  // for logging
#include <vector>  // for reflections

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

    ~ASTGNode() // it is the responsibility of parent or left to call this
    {
        if (child != nullptr) delete child;
        if (right != nullptr) delete right;
    }
};

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

        return GenerateParentNode(GrammarType::Integer, {pos, newpos});
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
                return GenerateParentNode(GrammarType::Decimal, {pos, v2->range.end}, v, v2);
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

    return GenerateParentNode(GrammarType::StringLit, {pos, newpos + 1});
}

template<> ASTGNode* Parse<GrammarType::Identifier>(const std::string& str, int pos)
{
    if (!std::isalpha(str[pos])) return nullptr;

    int newpos = pos + 1;
    while (std::isalnum(str[newpos]) || str[newpos] == '_') newpos++;

    return GenerateParentNode(GrammarType::Identifier, {pos, newpos});
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
    return ret;
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
                    return GenerateParentNode(GrammarType::FunctionCall, {pos, args->range.end+1}, name, args);
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
                return GenerateParentNode(GrammarType::ParenExpr, {pos, expr->range.end+1}, expr);
            }

            delete expr;
        }
        return nullptr;
    }

    ASTGNode* lambda = Parse<GrammarType::Lambda>(str, pos);  // this returns a different type, but maybe that's ok...
    if (lambda != nullptr)
    {
        return lambda;
    }
    ASTGNode* funcCall = Parse<GrammarType::FunctionCall>(str, pos);  // this returns a different type, but maybe that's ok...
    if (funcCall != nullptr)
    {
        return funcCall;
    }
    ASTGNode* varRef = Parse<GrammarType::Identifier>(str, pos);
    if (varRef != nullptr)
    {
        return varRef;
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
                return GenerateParentNode(GrammarType::ExpExpr, {pos, rhs->range.end}, lhs, rhs);
            }
        }
        else
        {
            return lhs;
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
            return expr;
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
            return expr;
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
            return expr;
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
            return expr;
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
                return GenerateParentNode(GrammarType::VarAssignment, {pos, val->range.end}, name, val);
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
        return assign;
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

    return ret;

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
                return GenerateParentNode(GrammarType::Scope, {pos, block->range.end + 1});
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
                    return GenerateParentNode(GrammarType::IfStatement, {pos, code->range.end}, check, code);
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
                    return GenerateParentNode(GrammarType::WhileStatement, {pos, code->range.end}, check, code);
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
                                    return GenerateParentNode(GrammarType::ForStatement, {pos, code->range.end}, st1, st2, st3, code);
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
        return scst;
    }
    ASTGNode* ifst = Parse<GrammarType::IfStatement>(str, pos);
    if (ifst != nullptr)
    {
        return ifst;
    }
    ASTGNode* whst = Parse<GrammarType::WhileStatement>(str, pos);
    if (whst != nullptr)
    {
        return whst;
    }
    ASTGNode* frst = Parse<GrammarType::ForStatement>(str, pos);
    if (frst != nullptr)
    {
        return frst;
    }

    ASTGNode* expr = Parse<GrammarType::Expression>(str, pos);
    if (expr != nullptr)
    {
        if (str[expr->range.end] == ';')
        {
            return GenerateParentNode(GrammarType::Statement, {pos, expr->range.end + 1});
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
                    return GenerateParentNode(GrammarType::Lambda, {pos, scope->range.end});
                }
            }
            delete args;
        }
    }
    return nullptr;
}





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





int main()
{
    std::cout << "Enter mathematical expression: ";
    std::string expr; std::getline(std::cin, expr);

    ASTGNode* program = Parse<GrammarType::Block>(expr, 0);
    if (program != nullptr)
    {
        ASTGNode* ptr = program;

        while (ptr != nullptr)
        {
            std::cout << GRAMMAR_TYPE_NAMES[(int)ptr->type] << "{";
            if (ptr->child == nullptr)
            {
                std::cout << expr.substr(ptr->range.begin, ptr->range.end - ptr->range.begin);

                while (ptr != nullptr && ptr->right == nullptr) { std::cout << "}"; ptr = ptr->parent; }
                if (ptr != nullptr)
                {
                    std::cout << ", ";
                    ptr = ptr->right;
                }
            }
            else
            {
                ptr = ptr->child;
            }
        }
    }
    else
    {
        std::cout << "Syntax Error!\n";
    }
}


