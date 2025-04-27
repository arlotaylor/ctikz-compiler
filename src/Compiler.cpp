#include "Compiler.h"
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

struct ParseContext
{
    std::map<std::string, size_t> drawVars;
    std::map<std::string, float> mathVars;
    std::map<std::string, std::pair<size_t, std::string>> funcVars;
    std::map<size_t, size_t> drawArgs;
};

float CalculateMath(StringStream& code, ParseContext& vars)
{
    code.SkipSpaces();
    if (code.Done()) return 0;

    // std::cout << "CM: " << code.str.substr(code.place) << "\n";

    if (code.str[code.place] == '*') { code.place += 1; float a = CalculateMath(code, vars); float b = CalculateMath(code, vars); return a * b; }
    if (code.str[code.place] == '/') { code.place += 1; float a = CalculateMath(code, vars); float b = CalculateMath(code, vars); return a / b; }
    if (code.str[code.place] == '+') { code.place += 1; float a = CalculateMath(code, vars); float b = CalculateMath(code, vars); return a + b; }
    if (code.str[code.place] == '-') { code.place += 1; float a = CalculateMath(code, vars); return -a; }  // unary negation
    if (code.str[code.place] == '%') { code.place += 1; float a = CalculateMath(code, vars); float b = CalculateMath(code, vars); return std::fmod(a, b); }
    if (code.str[code.place] == '^') { code.place += 1; float a = CalculateMath(code, vars); float b = CalculateMath(code, vars); return std::pow(a, b); }
    if (code.str[code.place] == 's') { code.place += 1; float a = CalculateMath(code, vars); return std::sin(a); }
    if (code.str[code.place] == 'c') { code.place += 1; float a = CalculateMath(code, vars); return std::cos(a); }
    if (code.str[code.place] == 't') { code.place += 1; float a = CalculateMath(code, vars); return std::tan(a); }
    if (code.str[code.place] == 'p') { code.place += 1; return M_PI; }
    if (code.str[code.place] == 'e') { code.place += 1; return M_E; }
    if (code.str[code.place] == 'a')
    {
        code.place += 1; if (code.Done()) return 0.f;
        if (code.str[code.place] == 's') { code.place += 1; float a = CalculateMath(code, vars); return std::asin(a); }
        if (code.str[code.place] == 'c') { code.place += 1; float a = CalculateMath(code, vars); return std::acos(a); }
        if (code.str[code.place] == 't') { code.place += 1; float a = CalculateMath(code, vars); return std::atan(a); }
        return 0.f;
    }

    float ret = 0;
    size_t next = 0;
    if (code.str[code.place] == '$')
    {
        code.place += 1;
        next = code.str.find_first_of("*/+%^ ,)", code.place);
        // todo: add error handling here
        ret = vars.mathVars[code.str.substr(code.place, next - code.place)];
        // std::cout << "var used " << code.str.substr(code.place, next - code.place) << " = " << ret << ".\n";
    }
    else
    {
        next = code.str.find_first_of("*/+%^sctpea ,)", code.place);
        ret = std::stoi(code.str.substr(code.place, next - code.place));
    }

    code.place = next;
    code.SkipSpaces();
    if (!code.Done() && code.str[code.place] == ',') code.place += 1;

    // std::cout << "Calc math returning " << ret << ", also next is " << code.place << ".\n";
    return ret;
}



size_t LoadDrawables(StringStream& code, std::vector<std::pair<DrawableBase*,bool>>& vec, ParseContext& vars)
{
    code.SkipSpaces();

    if (code.Done()) return 0;

    // std::cout << "LD: " << code.str.substr(code.place) << "\n";

    bool isHidden = false;
    if (code.str[code.place] == '_')
    {
        code.place += 1;
        isHidden = true;
    }

    if (code.str[code.place] == '$')
    {
        if (code.place == 0)
        {
            std::string setVarName = code.str.substr(1, code.str.find_first_of(" =") - 1);
            code.place = code.str.find('=') + 1;
            if (!code.Done() && code.str[code.place] == ':')
            {
                code.place += 1;
                vars.mathVars.insert({ setVarName, CalculateMath(code, vars) });
                return 0;
            }
            else if (setVarName.find('(') != std::string::npos)
            {
                vars.funcVars.insert({ setVarName.substr(0, setVarName.find('(')), { std::stoul(setVarName.substr(setVarName.find('(') + 1)), code.str.substr(code.place) } });
                return 0;
            }
            else
            {
                vars.drawVars.insert({ setVarName, LoadDrawables(code, vec, vars) });
                return 0;
            }
        }
        else
        {
            std::string name = code.str.substr(code.place + 1, code.str.find_first_of(",() ", code.place + 1) - code.place - 1);
            if (!name.empty() && std::isdigit(name[0]))
            {
                size_t ret = vars.drawArgs[std::stoul(name)];
                code.place += 1 + name.size();
                return ret;
            }
            else if (vars.drawVars.count(name))
            {
                size_t ret = vars.drawVars[name];
                code.place += 1 + name.size();
                return ret;
            }
            else if (vars.funcVars.count(name))
            {
                code.place += 1 + name.size();
                code.SkipSpaces();
                code.place += 1; // assume '('
                std::map<size_t, size_t> newDrawArgs;

                for (int i = 0; i < vars.funcVars[name].first; i++)
                {
                    newDrawArgs.insert({ i, LoadDrawables(code, vec, vars) });
                    code.SkipSpaces(); code.place += 1; // assume ',' or at the end ')'
                }

                // save draw args and restore them once done in the new func
                std::map<size_t, size_t> oldDrawArgs = vars.drawArgs;
                vars.drawArgs = newDrawArgs;
                size_t ret = LoadDrawables(StringStream{ vars.funcVars[name].second, 0 }, vec, vars);
                vars.drawArgs = oldDrawArgs;
                return ret;
            }
        }
    }

    if (code.str[code.place] == '(')  // point case
    {
        code.place += 1;
        float x = CalculateMath(code, vars);
        float y = CalculateMath(code, vars);
        vec.push_back({new PointLiteral{ vec.size(), { x, y } }, isHidden});
        // std::cout << "Just created a point literal (" + std::to_string(x) + "," + std::to_string(y) + ").\n";
    }
    else if (code.str.substr(code.place, 4) == "pil(")  // point on intersection of lines
    {
        code.place += 4;
        size_t a = LoadDrawables(code, vec, vars);
        code.SkipSpaces(); code.place += 1;  // assume ','
        size_t b = LoadDrawables(code, vec, vars);
        vec.push_back({new PointIntersectionOfLines{ vec.size(), *(LineBase*)(vec[a].first), *(LineBase*)vec[b].first }, isHidden});
    }
    else if (code.str.substr(code.place, 4) == "ltp(")  // line through points
    {
        code.place += 4;
        size_t a = LoadDrawables(code, vec, vars);
        code.SkipSpaces(); code.place += 1;  // assume ','
        size_t b = LoadDrawables(code, vec, vars);
        vec.push_back({new LineThroughPoints{ vec.size(), *(PointBase*)(vec[a].first), *(PointBase*)vec[b].first }, isHidden});
    }
    else
    {
        return 0;
    }

    code.SkipSpaces();
    code.place += 1;  // assume ')'
    code.SkipSpaces();
    if (!code.Done() && code.str[code.place] == ';')
    {
        code.place += 1;
        return LoadDrawables(code, vec, vars);
    }
    return vec.size() - 1;
}

std::vector<std::pair<DrawableBase*,bool>> LoadDrawables(std::filesystem::path path)
{
    std::vector<std::pair<DrawableBase*,bool>> ret;
    ParseContext vars;

    std::ifstream file(path);
    std::string temp;
    int line = 0;
    while (std::getline(file, temp))
    {
        // std::cout << "\nLine " << line + 1 << ":\n";
        LoadDrawables(StringStream{ temp, 0 }, ret, vars);
        line++;
    }

    return ret;
}
