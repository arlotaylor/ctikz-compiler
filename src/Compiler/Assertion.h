#pragma once
#include <string>
#include <iostream>

struct TextPosition
{
    int line; int column;
};

inline void Assert(bool b, std::string msg = "No message given.", TextPosition pos = { -1, -1 })
{
    if (!b)
    {
        std::string out = "Assertion Failed";
        if (pos.line != -1)
        {
            out += " (line " + std::to_string(pos.line);
            if (pos.column != -1)
            {
                out += ", column " + std::to_string(pos.column);
            }
            out += ")";
        }
        out += ": " + msg;
        std::cout << out << std::endl;
        std::exit(1);
    }
}

