#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

struct Vec2
{
    float x, y;
};
inline bool operator==(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }
inline bool operator!=(Vec2 a, Vec2 b) { return !(a == b); }

enum class CanvasPrimitiveTypes
{
    Point, Line,
};

struct CanvasPrimitive
{
    CanvasPrimitiveTypes type;
    std::vector<Vec2> data;
    
    CanvasPrimitive(CanvasPrimitiveTypes t, const std::vector<Vec2>& d);
};

struct Diagram
{
    std::vector<CanvasPrimitive> m_Primitives;
    Diagram& operator+=(const CanvasPrimitive& p);
};

enum class SupportedBackends
{
    TikZ,
};

template<SupportedBackends T>
class Transpiled
{
    std::string m_Val;
    Transpiled(const std::string& v) : m_Val(v) {}

public:
    std::string GetValue() { return m_Val; }
    static Transpiled<T> FromDiagram(const Diagram& d);

    Transpiled<T> operator+(const Transpiled<T>& other) { return { m_Val + other.m_Val }; }
    Transpiled<T>& operator+=(const Transpiled<T>& other) { m_Val += other.m_Val; return *this; }
    void SaveToFile(std::filesystem::path p) { std::ofstream file(p); file << m_Val; }
};
