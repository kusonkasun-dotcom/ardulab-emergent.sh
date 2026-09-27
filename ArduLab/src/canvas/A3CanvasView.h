#pragma once

// A3CanvasView — presents A3CanvasScene and forwards pan/zoom input to
// ViewportController (Plan §4.4). Contains no catalog or persistence logic.
//
// Input model:
//   - Ctrl + wheel   → zoom around cursor
//   - Wheel          → vertical pan; Shift + wheel → horizontal pan
//   - Middle-drag / Space + left-drag → pan
//   - Ctrl+0 / "fit" → fit sheet

#include "canvas/A3CanvasScene.h"
#include "canvas/ViewportController.h"
#include "core/Units.h"

#include <QGraphicsView>
#include <QPoint>

namespace ardulab::canvas {

class A3CanvasView final : public QGraphicsView
{
    Q_OBJECT

public:
    explicit A3CanvasView(A3CanvasScene* scene, ViewportController* viewport, QWidget* parent = nullptr);

    [[nodiscard]] ViewportController* viewportController() const noexcept { return m_viewport; }
    [[nodiscard]] A3CanvasScene* canvasScene() const noexcept { return m_scene; }

    /// Fit the whole sheet into the current widget size.
    void fitSheet();

    /// Millimeter position currently under the given widget-local point.
    [[nodiscard]] core::PointMm mmAt(QPoint viewportPos) const;

signals:
    /// Cursor moved over the sheet (mm). For status-bar presentation only.
    void cursorMovedMm(double xMm, double yMm);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyViewportTransform();

    A3CanvasScene* m_scene;
    ViewportController* m_viewport;
    bool m_panning = false;
    bool m_spaceHeld = false;
    bool m_initialFitDone = false;
    QPoint m_lastPanPos;
};

} // namespace ardulab::canvas
