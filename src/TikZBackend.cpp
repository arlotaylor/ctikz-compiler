#include "Transpiler.h"

template<>
Transpiled<SupportedBackends::TikZ> Transpiled<SupportedBackends::TikZ>::FromDiagram(const Diagram& d)
{
    std::string ret = "\\begin{tikzpicture}\n";
    for (const CanvasPrimitive& p : d.m_Primitives)
    {
        switch (p.type)
        {
        case CanvasPrimitiveTypes::Point:
            ret += "\\filldraw[black] (" + std::to_string(p.data[0].x) + "," + std::to_string(p.data[0].y) + ") circle (2pt);\n";
            break;
        
        case CanvasPrimitiveTypes::Line:
            ret += "\\draw (" + std::to_string(p.data[0].x) + "," + std::to_string(p.data[0].y)
            + ") -- (" + std::to_string(p.data[1].x) + "," + std::to_string(p.data[1].y) + ");\n";
            break;
        
        default:
            break;
        }
    }
    ret += "\\end{tikzpicture}\n";
    return { ret };
}
