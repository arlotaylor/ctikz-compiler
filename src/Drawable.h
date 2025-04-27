#pragma once
#include "Transpiler.h"
#include <string>
#include <optional>


class DrawableBase
{
public:
    size_t m_ID;
    virtual std::optional<CanvasPrimitive> ToPrimitive() = 0;

protected:
    inline DrawableBase(size_t id) : m_ID(id) {}
};


class PointBase : public DrawableBase
{
public:
    virtual std::optional<Vec2> GetPosition() const = 0;
    std::optional<CanvasPrimitive> ToPrimitive() override;

protected:
    inline PointBase(size_t id) : DrawableBase(id) {}
};

class LineBase : public DrawableBase
{
public:
    virtual std::optional<Vec2> GetPositionA() const = 0;
    virtual std::optional<Vec2> GetPositionB() const = 0;
    std::optional<CanvasPrimitive> ToPrimitive() override;

protected:
    inline LineBase(size_t id) : DrawableBase(id) {}
};


class PointLiteral : public PointBase
{
public:
    PointLiteral(size_t, Vec2);
    std::optional<Vec2> GetPosition() const override;

private:
    Vec2 m_Position;
};

class PointIntersectionOfLines : public PointBase
{
public:
    PointIntersectionOfLines(size_t, const LineBase&, const LineBase&);
    std::optional<Vec2> GetPosition() const override;

private:
    const LineBase& m_LineA;
    const LineBase& m_LineB;
};

class LineThroughPoints : public LineBase
{
public:
    LineThroughPoints(size_t, const PointBase&, const PointBase&);
    std::optional<Vec2> GetPositionA() const override;
    std::optional<Vec2> GetPositionB() const override;

private:
    const PointBase& m_PointA;
    const PointBase& m_PointB;
};
