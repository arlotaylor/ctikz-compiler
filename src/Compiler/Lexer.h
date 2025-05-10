#pragma once
#include "LinkedList.h"
#include <string>

enum class TokenType
{
    Text,
    Symbol,
    StringLiteral,
    Integer,
    Decimal,
};

struct Token
{
    TokenType type;
    std::string value;
};


