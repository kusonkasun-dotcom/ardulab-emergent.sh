#pragma once

// CoordinateSystem — millimeter ↔ scene-unit conversion (Plan §4.4).
//
// The A3 sheet origin (0 mm, 0 mm) is the top-left corner of the sheet and
// maps to scene (0, 0). +X mm → +X scene (right), +Y mm → +Y scene (down),
// matching the ADR §3.6 engineering-view convention and Qt's scene axes, so
// no axis flip is required.
//
// `sceneUnitsPerMm` is a fixed *resolution* constant of the scene — NOT the
// user zoom. Zoom/pan live in ViewportController and are applied by the view
// transform, never by rescaling engineering values.
//
// Deterministic and unit-testable without any QWidget.

#include "core/Units.h"

#include <QPointF>
#include <QRectF>
#include <QSizeF>

namespace ardulab::canvas {

class CoordinateSystem final
{
public:
    /// Default scene resolution: 10 scene units per millimeter (0.1 mm precision
    /// at integer scene coordinates; A3 becomes 4200 × 2970 scene units).
    static constexpr double kDefaultSceneUnitsPerMm = 10.0;

    explicit CoordinateSystem(double sceneUnitsPerMm = kDefaultSceneUnitsPerMm,
                              core::SizeMm sheetSize = core::sheet::a3Landscape()) noexcept;

    [[nodiscard]] double sceneUnitsPerMm() const noexcept { return m_sceneUnitsPerMm; }
    [[nodiscard]] core::SizeMm sheetSizeMm() const noexcept { return m_sheetSize; }
    [[nodiscard]] core::RectMm sheetRectMm() const noexcept { return core::RectMm(core::PointMm(0.0, 0.0), m_sheetSize); }

    // ---- mm → scene -------------------------------------------------------
    [[nodiscard]] double toScene(core::Millimeters length) const noexcept { return length.value * m_sceneUnitsPerMm; }
    [[nodiscard]] QPointF toScene(core::PointMm point) const noexcept;
    [[nodiscard]] QSizeF toScene(core::SizeMm size) const noexcept;
    [[nodiscard]] QRectF toScene(core::RectMm rect) const noexcept;

    // ---- scene → mm -------------------------------------------------------
    [[nodiscard]] core::Millimeters toMm(double sceneLength) const noexcept { return core::Millimeters(sceneLength / m_sceneUnitsPerMm); }
    [[nodiscard]] core::PointMm toMm(QPointF scenePoint) const noexcept;
    [[nodiscard]] core::SizeMm toMm(QSizeF sceneSize) const noexcept;

    /// Scene rectangle of the whole sheet.
    [[nodiscard]] QRectF sheetSceneRect() const noexcept { return toScene(sheetRectMm()); }

    /// Snap a millimeter point to the nearest grid intersection.
    [[nodiscard]] static core::PointMm snapToGrid(core::PointMm point, double gridMm) noexcept;

private:
    double m_sceneUnitsPerMm;
    core::SizeMm m_sheetSize;
};

} // namespace ardulab::canvas
