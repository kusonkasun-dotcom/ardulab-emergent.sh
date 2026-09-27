#include "canvas/ViewportController.h"

#include <QSizeF>

#include <algorithm>
#include <cmath>

namespace ardulab::canvas {

ViewportController::ViewportController(QObject* parent)
    : QObject(parent)
{
}

QTransform ViewportController::transform() const
{
    QTransform t;
    t.scale(m_zoom, m_zoom);
    t.translate(m_pan.x(), m_pan.y());
    return t;
}

double ViewportController::clampZoom(double z) const noexcept
{
    if (!std::isfinite(z)) {
        return m_zoom;
    }
    return std::clamp(z, m_minZoom, m_maxZoom);
}

double ViewportController::setZoom(double factor)
{
    const double clamped = clampZoom(factor);
    if (clamped != m_zoom) {
        m_zoom = clamped;
        emit zoomChanged(m_zoom);
        emit viewportChanged();
    }
    return m_zoom;
}

double ViewportController::zoomBy(int steps)
{
    return setZoom(m_zoom * std::pow(kDefaultZoomStep, steps));
}

void ViewportController::zoomAround(double newZoom, QPointF anchorScene, QPointF anchorViewport)
{
    const double clamped = clampZoom(newZoom);
    if (clamped == m_zoom) {
        return;
    }
    // viewport = (scene + pan) * zoom  ⇒  pan = viewport / zoom − scene
    m_zoom = clamped;
    m_pan = QPointF(anchorViewport.x() / m_zoom - anchorScene.x(), anchorViewport.y() / m_zoom - anchorScene.y());
    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
    emit viewportChanged();
}

void ViewportController::setPanOffset(QPointF sceneOffset)
{
    if (sceneOffset != m_pan) {
        m_pan = sceneOffset;
        emit panChanged(m_pan);
        emit viewportChanged();
    }
}

void ViewportController::panBy(QPointF sceneDelta)
{
    setPanOffset(m_pan + sceneDelta);
}

void ViewportController::setZoomRange(double minZoom, double maxZoom)
{
    if (minZoom > 0.0 && maxZoom >= minZoom) {
        m_minZoom = minZoom;
        m_maxZoom = maxZoom;
        setZoom(m_zoom); // re-clamp
    }
}

void ViewportController::reset()
{
    const bool changed = m_zoom != 1.0 || !m_pan.isNull();
    m_zoom = 1.0;
    m_pan = QPointF();
    if (changed) {
        emit zoomChanged(m_zoom);
        emit panChanged(m_pan);
        emit viewportChanged();
    }
}

double ViewportController::fitZoom(QSizeF sceneSize, QSizeF viewportSize, double marginFraction) noexcept
{
    if (sceneSize.width() <= 0.0 || sceneSize.height() <= 0.0 || viewportSize.width() <= 0.0 || viewportSize.height() <= 0.0) {
        return 1.0;
    }
    const double usable = std::clamp(1.0 - marginFraction, 0.1, 1.0);
    const double zx = viewportSize.width() * usable / sceneSize.width();
    const double zy = viewportSize.height() * usable / sceneSize.height();
    return std::min(zx, zy);
}

} // namespace ardulab::canvas
