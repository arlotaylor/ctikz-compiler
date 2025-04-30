#include "Compiler.h"
#include <iostream>
/*
int main()
{
    std::cout << "Enter filename (leave blank for examples):";
    std::string temp; std::getline(std::cin, temp);

    if (temp == "") temp = "../examples/basic-example.txt";

    std::filesystem::path input = std::filesystem::current_path() / temp;
    std::vector<std::pair<DrawableBase*,bool>> stuff = LoadDrawables(input);
    Diagram d;
    for (std::pair<DrawableBase*,bool> i : stuff)
    {
        if (i.second)
        {
            // std::cout << "skipped" << std::endl;
            continue;
        }

        auto p = i.first->ToPrimitive();
        if (p)
        {
            // std::cout << "slay" << std::endl;
            d += p.value();
        }
        else
        {
            // std::cout << "no slay" << std::endl;
        }
    }
    Transpiled<SupportedBackends::TikZ>::FromDiagram(d).SaveToFile(input.parent_path() / (input.stem().string() + ".tikz"));

    return 0;
}
*/