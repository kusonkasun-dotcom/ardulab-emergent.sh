#include "canvas/CoordinateSystem.h"

#include <cmath>

namespace ardulab::canvas {

CoordinateSystem::CoordinateSystem(double sceneUnitsPerMm, core::SizeMm sheetSize) noexcept
    : m_sceneUnitsPerMm(sceneUnitsPerMm > 0.0 ? sceneUnitsPerMm : kDefaultSceneUnitsPerMm)
    , m_sheetSize(sheetSize.isValid() ? sheetSize : core::sheet::a3Landscape())
{
}

QPointF CoordinateSystem::toScene(core::PointMm point) const noexcept
{
    return QPointF(point.x * m_sceneUnitsPerMm, point.y * m_sceneUnitsPerMm);
}

QSizeF CoordinateSystem::toScene(core::SizeMm size) const noexcept
{
    return QSizeF(size.width * m_sceneUnitsPerMm, size.height * m_sceneUnitsPerMm);
}

QRectF CoordinateSystem::toScene(core::RectMm rect) const noexcept
{
    return QRectF(toScene(rect.topLeft), toScene(rect.size));
}

core::PointMm CoordinateSystem::toMm(QPointF scenePoint) const noexcept
{
    return core::PointMm(scenePoint.x() / m_sceneUnitsPerMm, scenePoint.y() / m_sceneUnitsPerMm);
}

core::SizeMm CoordinateSystem::toMm(QSizeF sceneSize) const noexcept
{
    return core::SizeMm(sceneSize.width() / m_sceneUnitsPerMm, sceneSize.height() / m_sceneUnitsPerMm);
}

core::PointMm CoordinateSystem::snapToGrid(core::PointMm point, double gridMm) noexcept
{
    if (gridMm <= 0.0) {
        return point;
    }
    return core::PointMm(std::round(point.x / gridMm) * gridMm, std::round(point.y / gridMm) * gridMm);
}

} // namespace ardulab::canvas
