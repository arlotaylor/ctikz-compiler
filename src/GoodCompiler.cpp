#include <cctype>
#include <string>
#include <iostream>
#include <optional>


enum class GrammarTypes
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
    AddExpr,
    EqExpr,
    SetVarExpr,
    Expression,
    Statement,
    Block,
    Scope,
    IfStatement,
    WhileStatement,
    ForStatement,
    FunctionDefinition,
};

struct IntRange
{
    int begin, end;
};

struct ASTGNode
{
    GrammarTypes type;
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

ASTGNode* GenerateParentNode(GrammarTypes t, IntRange r)
{
    return new ASTGNode{ t, r };
}

template<typename... Args>
ASTGNode* GenerateParentNode(GrammarTypes t, IntRange r, ASTGNode* child1, Args... args)
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
}


template<GrammarTypes T> ASTGNode* Parse(const std::string& str, int pos) = delete;

template<> ASTGNode* Parse<GrammarTypes::Integer>(const std::string& str, int pos)
{
    if (std::isdigit(str[pos]))
    {
        int newpos = pos + 1;
        while (std::isdigit(str[newpos])) newpos++;

        return GenerateParentNode(GrammarTypes::Integer, {pos, newpos});
    }
    else
    {
        return nullptr;
    }
}

template<> ASTGNode* Parse<GrammarTypes::Decimal>(const std::string& str, int pos)
{
    ASTGNode* v = Parse<GrammarTypes::Integer>(str, pos);
    if (v != nullptr)
    {
        if (str[v->range.end] == '.')
        {
            ASTGNode* v2 = Parse<GrammarTypes::Integer>(str, v->range.end + 1);
            if (v2 != nullptr)
            {
                return GenerateParentNode(GrammarTypes::Decimal, {pos, v2->range.end}, v, v2);
            }
        }
        delete v;
    }
    return nullptr;
}

template<> ASTGNode* Parse<GrammarTypes::StringLit>(const std::string& str, int pos)
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

    return GenerateParentNode(GrammarTypes::StringLit, {pos, newpos + 1});
}

template<> ASTGNode* Parse<GrammarTypes::Identifier>(const std::string& str, int pos)
{
    if (!std::isalpha(str[pos])) return nullptr;

    int newpos = pos + 1;
    while (std::isalnum(str[newpos]) || str[newpos] == '_') newpos++;

    return GenerateParentNode(GrammarTypes::Identifier, {pos, newpos});
}

template<> ASTGNode* Parse<GrammarTypes::Arguments>(const std::string& str, int pos)
{
    // this function doesn't fail, just returns empty arg string
    ASTGNode* ret = GenerateParentNode(GrammarTypes::Arguments, {pos, pos - 1});
    ASTGNode* prev = nullptr;

    do
    {
        ASTGNode* next = Parse<GrammarTypes::Expression>(str, ret->range.end + 1);
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

    return ret;
}


template<> ASTGNode* Parse<GrammarTypes::FunctionCall>(const std::string& str, int pos)
{
    ASTGNode* name = Parse<GrammarTypes::Identifier>(str, pos);
    if (name != nullptr)
    {
        if (str[name->range.end] == '(')
        {
            ASTGNode* args = Parse<GrammarTypes::Arguments>(str, name->range.end + 1);
            if (args != nullptr)  // should always be true
            {
                if (str[args->range.end] == ')')
                {
                    return GenerateParentNode(GrammarTypes::FunctionCall, {pos, args->range.end+1}, name, args);
                }
                delete args;
            }
        }
        delete name;
    }
}

template<> ASTGNode* Parse<GrammarTypes::ParenExpr>(const std::string& str, int pos)
{
    if (str[pos] == '(')
    {
        
    }
}


int main()
{
    std::cout << "Enter mathematical expression: ";
    std::string expr; std::getline(std::cin, expr);

}


