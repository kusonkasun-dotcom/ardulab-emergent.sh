#include "canvas/A3CanvasScene.h"

#include <QPainter>
#include <QPen>

#include <cmath>

namespace ardulab::canvas {

namespace {
const QColor kBackdropColor(0x2b, 0x2f, 0x36);
const QColor kSheetColor(0xfb, 0xfb, 0xf7);
const QColor kSheetShadow(0, 0, 0, 90);
const QColor kMinorGridColor(0xd9, 0xdc, 0xe0);
const QColor kMajorGridColor(0xb5, 0xba, 0xc2);
const QColor kBorderColor(0x3a, 0x3f, 0x47);
constexpr double kBorderMarginMm = 10.0;
} // namespace

A3CanvasScene::A3CanvasScene(CoordinateSystem coordinateSystem, QObject* parent)
    : QGraphicsScene(parent)
    , m_coordinates(coordinateSystem)
{
    // Scene rect slightly larger than the sheet so the sheet edge is visible
    // when panned to the border.
    const QRectF sheet = sheetRect();
    const double marginScene = m_coordinates.toScene(core::Millimeters(40.0));
    setSceneRect(sheet.adjusted(-marginScene, -marginScene, marginScene, marginScene));
    setBackgroundBrush(kBackdropColor);
}

void A3CanvasScene::setGridMm(double minorMm, double majorMm)
{
    if (minorMm > 0.0) m_minorGridMm = minorMm;
    if (majorMm > 0.0) m_majorGridMm = majorMm;
    update();
}

void A3CanvasScene::setGridVisible(bool visible)
{
    if (m_gridVisible != visible) {
        m_gridVisible = visible;
        update();
    }
}

void A3CanvasScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    const QRectF sheet = sheetRect();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    // Sheet with shadow.
    const double shadowOffset = m_coordinates.toScene(core::Millimeters(1.5));
    painter->fillRect(sheet.translated(shadowOffset, shadowOffset), kSheetShadow);
    painter->fillRect(sheet, kSheetColor);

    // Grid, clipped to the sheet and to the exposed rect.
    if (m_gridVisible) {
        const QRectF visible = rect.intersected(sheet);
        if (!visible.isEmpty()) {
            const double minorStep = m_coordinates.toScene(core::Millimeters(m_minorGridMm));
            const double majorStep = m_coordinates.toScene(core::Millimeters(m_majorGridMm));

            // Level-of-detail: skip minor grid when it would be denser than ~4 px.
            const double lod = painter->worldTransform().m11();
            const bool drawMinor = minorStep * lod >= 4.0;

            auto drawLines = [&](double step, const QColor& color) {
                QPen pen(color);
                pen.setCosmetic(true);
                pen.setWidth(1);
                painter->setPen(pen);
                const double startX = sheet.left() + std::floor((visible.left() - sheet.left()) / step) * step;
                const double startY = sheet.top() + std::floor((visible.top() - sheet.top()) / step) * step;
                for (double x = startX; x <= visible.right(); x += step) {
                    painter->drawLine(QPointF(x, visible.top()), QPointF(x, visible.bottom()));
                }
                for (double y = startY; y <= visible.bottom(); y += step) {
                    painter->drawLine(QPointF(visible.left(), y), QPointF(visible.right(), y));
                }
            };

            if (drawMinor) drawLines(minorStep, kMinorGridColor);
            drawLines(majorStep, kMajorGridColor);
        }
    }

    // Sheet border and inner drawing-frame.
    QPen border(kBorderColor);
    border.setCosmetic(true);
    border.setWidth(2);
    painter->setPen(border);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sheet);

    const double frameMargin = m_coordinates.toScene(core::Millimeters(kBorderMarginMm));
    border.setWidth(1);
    painter->setPen(border);
    painter->drawRect(sheet.adjusted(frameMargin, frameMargin, -frameMargin, -frameMargin));

    painter->restore();
}

} // namespace ardulab::canvas
