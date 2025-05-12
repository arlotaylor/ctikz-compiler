#pragma once
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
    int line; int column;
};

std::vector<Token> Tokenize(std::string str);

