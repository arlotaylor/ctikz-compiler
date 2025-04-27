#include "Transpiler.h"
#include <iostream>

Diagram& Diagram::operator+=(const CanvasPrimitive &p)
{
    m_Primitives.push_back(p);
    return *this;
}

CanvasPrimitive::CanvasPrimitive(CanvasPrimitiveTypes t, const std::vector<Vec2>& d)
    : type(t), data(d)
{
    if (
        (type == CanvasPrimitiveTypes::Point && d.size() != 1)
        || (type == CanvasPrimitiveTypes::Line && d.size() != 2)
    )
    {
        std::cout << "Error: CanvasPrimitive initialized with the wrong amount of data." << std::endl;
    }
}
