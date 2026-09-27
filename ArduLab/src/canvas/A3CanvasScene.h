#pragma once

// A3CanvasScene — the 420 mm × 297 mm engineering workspace (Plan §4.4).
//
// Owns presentation items (QGraphicsItems) and draws the sheet, margin, and
// grid. It has no knowledge of Project persistence or the component catalog;
// items are added by the UI adapter from immutable snapshots.

#include "canvas/CoordinateSystem.h"

#include <QGraphicsScene>

namespace ardulab::canvas {

class A3CanvasScene final : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit A3CanvasScene(CoordinateSystem coordinateSystem = CoordinateSystem(), QObject* parent = nullptr);

    [[nodiscard]] const CoordinateSystem& coordinateSystem() const noexcept { return m_coordinates; }

    /// Major/minor grid pitch in millimeters. Presentation only.
    void setGridMm(double minorMm, double majorMm);
    [[nodiscard]] double minorGridMm() const noexcept { return m_minorGridMm; }
    [[nodiscard]] double majorGridMm() const noexcept { return m_majorGridMm; }

    void setGridVisible(bool visible);
    [[nodiscard]] bool isGridVisible() const noexcept { return m_gridVisible; }

    /// Sheet rectangle in scene units.
    [[nodiscard]] QRectF sheetRect() const noexcept { return m_coordinates.sheetSceneRect(); }

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    CoordinateSystem m_coordinates;
    double m_minorGridMm = 1.0;
    double m_majorGridMm = 10.0;
    bool m_gridVisible = true;
};

} // namespace ardulab::canvas
