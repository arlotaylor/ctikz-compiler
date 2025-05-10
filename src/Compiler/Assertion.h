#pragma once
#include <string>
#include <iostream>

inline void Assert(bool b, std::string msg = "No message given.", int line = -1, int column = -1)
{
    if (!b)
    {
        std::string out = "Assertion Failed";
        if (line != -1)
        {
            out += " (line " + std::to_string(line);
            if (column != -1)
            {
                out += ", column " + std::to_string(column);
            }
            out += ")";
        }
        out += ": " + msg;
        std::cout << out << std::endl;
        std::exit(1);
    }
}

