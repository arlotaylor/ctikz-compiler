#include "Drawable.h"
#include <iostream>

PointLiteral::PointLiteral(size_t id, Vec2 v) : PointBase(id), m_Position(v) {}
std::optional<Vec2> PointLiteral::GetPosition() const { return m_Position; }

PointIntersectionOfLines::PointIntersectionOfLines(size_t id, const LineBase& a, const LineBase& b)
    : PointBase(id), m_LineA(a), m_LineB(b)
{
    if (std::max(a.m_ID, b.m_ID) >= id) std::cout << "Error: Possibility of self-referencing definition." << std::endl;
}

std::optional<Vec2> PointIntersectionOfLines::GetPosition() const
{
    std::optional<Vec2> aa = m_LineA.GetPositionA();
    std::optional<Vec2> ab = m_LineA.GetPositionB();
    std::optional<Vec2> ba = m_LineB.GetPositionA();
    std::optional<Vec2> bb = m_LineB.GetPositionB();

    if (aa && ab && ba && bb)
    {
        float d1 = aa.value().x - ab.value().x;
        float d2 = aa.value().y - ab.value().y;
        float d3 = ba.value().x - bb.value().x;
        float d4 = ba.value().y - bb.value().y;
        float denom = d1 * d4 - d2 * d3;

        if (denom == 0) return std::nullopt;

        float n1 = aa.value().x * ab.value().y - aa.value().y * ab.value().x;
        float n2 = ba.value().x * bb.value().y - ba.value().y * bb.value().x;

        return Vec2{ (n1 * d3 - n2 * d1) / denom, (n1 * d4 - n2 * d2) / denom };
    }
    else
    {
        return std::nullopt;
    }
}

LineThroughPoints::LineThroughPoints(size_t id, const PointBase& a, const PointBase& b)
    : LineBase(id), m_PointA(a), m_PointB(b)
{
    if (std::max(a.m_ID, b.m_ID) >= id) std::cout << "Error: Possibility of self-referencing definition." << std::endl;
}

std::optional<Vec2> LineThroughPoints::GetPositionA() const
{
    return m_PointA.GetPosition();
}

std::optional<Vec2> LineThroughPoints::GetPositionB() const
{
    std::optional<Vec2> a = m_PointA.GetPosition();
    std::optional<Vec2> b = m_PointB.GetPosition();

    if (a && b && a.value() != b.value())
    {
        return b;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<CanvasPrimitive> PointBase::ToPrimitive()
{
    std::optional<Vec2> p = GetPosition();
    if (p)
    {
        return CanvasPrimitive{ CanvasPrimitiveTypes::Point, { p.value() } };
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<CanvasPrimitive> LineBase::ToPrimitive()
{
    std::optional<Vec2> a = GetPositionA();
    std::optional<Vec2> b = GetPositionB();
    if (a && b)
    {
        return CanvasPrimitive{ CanvasPrimitiveTypes::Line, { a.value(), b.value() } };
    }
    else
    {
        return std::nullopt;
    }
}
