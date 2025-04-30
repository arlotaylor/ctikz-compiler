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
    Expression,
    Statement,
    Block,
    Scope,
    SetVarExpr,
    EqExpr,
    AddExpr,
    MulExpr,
    ExpExpr,
    ParenExpr,
    FunctionCall,
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
            delete v2;
        }
    }
    delete v;
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

template<> ASTGNode* Parse<GrammarTypes::FunctionCall>(const std::string& str, int pos)
{
    ASTGNode* name = Parse<GrammarTypes::Identifier>(str, pos);
    if (name != nullptr)
    {
        if (str[name->range.end] == '(')
        {
            ASTGNode* ret = GenerateParentNode(GrammarTypes::FunctionCall, {pos, -1});
            if (str[name->range.end + 1] == ')')
            {
                // no args
                ret->range.end = name->range.end + 2;
                return ret;
            }

            // args
            int newpos = name->range.end + 1;
            ASTGNode* prevSib = nullptr;
            do
            {
                ASTGNode* curSib = Parse<GrammarTypes::Expression>(str, newpos);
                if (curSib == nullptr)
                {
                    prevSib = nullptr;  // this will cause failure
                    break;
                }

                curSib->parent = ret;
                if (prevSib == nullptr)  // first arg
                {
                    ret->child = curSib;
                }
                else  // other arg
                {
                    prevSib->right = curSib;
                    curSib->left = prevSib;
                }
                prevSib = curSib;
            }
            while (str[newpos] == ',');

            if (prevSib != nullptr && str[newpos] == ')')
            {
                ret->range.end = newpos + 1;
                return ret;
            }

            delete ret;
        }
    }

    delete name;
}


int main()
{
    std::cout << "Enter mathematical expression: ";
    std::string expr; std::getline(std::cin, expr);

}


