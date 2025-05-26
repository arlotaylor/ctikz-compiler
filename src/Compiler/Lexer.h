#pragma once
#include "Assertion.h"
#include <string>
#include <vector>

enum class TokenType
{
    Text,
    Symbol,
    StringLiteral,
    Integer,
    Decimal,
    Boolean,
    EndOfFile,
};

struct Token
{
    TokenType type;
    std::string value;
    TextPosition pos;
};

std::vector<Token> Tokenize(std::string str);

