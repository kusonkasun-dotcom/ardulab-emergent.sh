#pragma once

// ViewportController — bounded zoom and pan state (Plan §4.4).
//
// Pure presentation state. Holds the zoom factor and the pan offset (scene
// units) and produces a QTransform for the view. It never touches component
// or package engineering dimensions, and it owns no QWidget so it is
// unit-testable without a display.

#include <QObject>
#include <QPointF>
#include <QTransform>

namespace ardulab::canvas {

class ViewportController final : public QObject
{
    Q_OBJECT

public:
    static constexpr double kMinZoom = 0.05;
    static constexpr double kMaxZoom = 40.0;
    static constexpr double kDefaultZoomStep = 1.15;

    explicit ViewportController(QObject* parent = nullptr);

    [[nodiscard]] double zoom() const noexcept { return m_zoom; }
    [[nodiscard]] QPointF panOffset() const noexcept { return m_pan; }
    [[nodiscard]] double minZoom() const noexcept { return m_minZoom; }
    [[nodiscard]] double maxZoom() const noexcept { return m_maxZoom; }

    /// View transform = scale(zoom) ∘ translate(pan). Pan is in scene units.
    [[nodiscard]] QTransform transform() const;

    /// Set zoom, clamped to [minZoom, maxZoom]. Returns the applied value.
    double setZoom(double factor);

    /// Multiply zoom by `steps` powers of kDefaultZoomStep (negative = out).
    double zoomBy(int steps);

    /// Zoom keeping `anchorScene` fixed under the same viewport position.
    /// `anchorViewport` is that position in view (pixel) coordinates.
    void zoomAround(double newZoom, QPointF anchorScene, QPointF anchorViewport);

    void setPanOffset(QPointF sceneOffset);
    void panBy(QPointF sceneDelta);

    /// Configure the bound range (used by fit-to-sheet logic).
    void setZoomRange(double minZoom, double maxZoom);

    /// Reset to zoom 1.0 at origin.
    void reset();

    /// Compute the zoom that fits `sceneSize` into `viewportSize` with margin.
    [[nodiscard]] static double fitZoom(QSizeF sceneSize, QSizeF viewportSize, double marginFraction = 0.04) noexcept;

signals:
    void zoomChanged(double zoom);
    void panChanged(QPointF panOffset);
    void viewportChanged();

private:
    double clampZoom(double z) const noexcept;

    double m_zoom = 1.0;
    QPointF m_pan;
    double m_minZoom = kMinZoom;
    double m_maxZoom = kMaxZoom;
};

} // namespace ardulab::canvas
