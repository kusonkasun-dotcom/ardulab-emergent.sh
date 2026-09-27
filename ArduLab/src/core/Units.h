#pragma once

// Engineering units (Plan §4.2, ADR §3.5/§3.6).
//
// All physical geometry in ArduLab is expressed in millimeters. Screen pixels
// and QGraphicsScene units are a Canvas/Renderer concern and are *never*
// stored in domain objects. The types below make the millimeter contract
// explicit at compile time.

#include <cmath>

namespace ardulab::core {

/// A length in millimeters.
struct Millimeters final
{
    double value = 0.0;

    constexpr Millimeters() = default;
    constexpr explicit Millimeters(double mm) noexcept
        : value(mm)
    {
    }

    friend constexpr Millimeters operator+(Millimeters a, Millimeters b) noexcept { return Millimeters(a.value + b.value); }
    friend constexpr Millimeters operator-(Millimeters a, Millimeters b) noexcept { return Millimeters(a.value - b.value); }
    friend constexpr Millimeters operator*(Millimeters a, double k) noexcept { return Millimeters(a.value * k); }
    friend constexpr Millimeters operator/(Millimeters a, double k) noexcept { return Millimeters(a.value / k); }
    friend constexpr bool operator<(Millimeters a, Millimeters b) noexcept { return a.value < b.value; }
    friend constexpr bool operator>(Millimeters a, Millimeters b) noexcept { return a.value > b.value; }
};

/// Absolute-tolerance equality for engineering values (1 nanometre).
constexpr double kMillimeterEpsilon = 1e-6;

inline bool nearlyEqual(double a, double b, double epsilon = kMillimeterEpsilon) noexcept
{
    return std::fabs(a - b) <= epsilon;
}

inline bool operator==(Millimeters a, Millimeters b) noexcept { return nearlyEqual(a.value, b.value); }
inline bool operator!=(Millimeters a, Millimeters b) noexcept { return !(a == b); }

/// A 2D point in millimeters.
/// Convention (ADR §3.6): origin = owning body origin, +X = right, +Y = down.
struct PointMm final
{
    double x = 0.0;
    double y = 0.0;

    constexpr PointMm() = default;
    constexpr PointMm(double xMm, double yMm) noexcept
        : x(xMm)
        , y(yMm)
    {
    }

    friend constexpr PointMm operator+(PointMm a, PointMm b) noexcept { return PointMm(a.x + b.x, a.y + b.y); }
    friend constexpr PointMm operator-(PointMm a, PointMm b) noexcept { return PointMm(a.x - b.x, a.y - b.y); }
};

inline bool operator==(PointMm a, PointMm b) noexcept { return nearlyEqual(a.x, b.x) && nearlyEqual(a.y, b.y); }
inline bool operator!=(PointMm a, PointMm b) noexcept { return !(a == b); }

/// A 2D size in millimeters.
struct SizeMm final
{
    double width = 0.0;
    double height = 0.0;

    constexpr SizeMm() = default;
    constexpr SizeMm(double widthMm, double heightMm) noexcept
        : width(widthMm)
        , height(heightMm)
    {
    }

    [[nodiscard]] constexpr bool isValid() const noexcept { return width > 0.0 && height > 0.0; }
};

inline bool operator==(SizeMm a, SizeMm b) noexcept { return nearlyEqual(a.width, b.width) && nearlyEqual(a.height, b.height); }
inline bool operator!=(SizeMm a, SizeMm b) noexcept { return !(a == b); }

/// Axis-aligned rectangle in millimeters (top-left origin, +Y down).
struct RectMm final
{
    PointMm topLeft;
    SizeMm size;

    constexpr RectMm() = default;
    constexpr RectMm(PointMm origin, SizeMm extent) noexcept
        : topLeft(origin)
        , size(extent)
    {
    }

    [[nodiscard]] constexpr double left() const noexcept { return topLeft.x; }
    [[nodiscard]] constexpr double top() const noexcept { return topLeft.y; }
    [[nodiscard]] constexpr double right() const noexcept { return topLeft.x + size.width; }
    [[nodiscard]] constexpr double bottom() const noexcept { return topLeft.y + size.height; }

    [[nodiscard]] constexpr bool contains(PointMm p) const noexcept
    {
        return p.x >= left() && p.x <= right() && p.y >= top() && p.y <= bottom();
    }
};

/// ISO A3 landscape engineering sheet (Plan §4.4).
namespace sheet {
constexpr double kA3WidthMm = 420.0;
constexpr double kA3HeightMm = 297.0;
constexpr SizeMm a3Landscape() noexcept { return SizeMm(kA3WidthMm, kA3HeightMm); }
} // namespace sheet

} // namespace ardulab::core
