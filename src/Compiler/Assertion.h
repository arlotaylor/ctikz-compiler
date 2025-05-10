#pragma once
#include <string>
#include <iostream>

inline void Assert(bool b, std::string msg)
{
    if (!b)
    {
        std::cout << "Assertion Failed: " << msg << std::endl;
        std::exit(0);
    }
}
