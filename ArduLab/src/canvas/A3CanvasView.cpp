#include "canvas/A3CanvasView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

namespace ardulab::canvas {

namespace {
/// Pannable region = sheet expanded by this many sheet sizes in each direction.
constexpr double kPannableSheets = 4.0;
} // namespace

A3CanvasView::A3CanvasView(A3CanvasScene* scene, ViewportController* viewport, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
    , m_viewport(viewport)
{
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    // The ViewportController is the single pan authority. QGraphicsView's own
    // scroll offset is used purely as the mechanism to realise that pan, so:
    //  - scrollbars are hidden (never user-driven),
    //  - alignment is top-left (no implicit centring when content is small),
    //  - the view's scene rect is a large pannable region around the sheet so
    //    the scroll range never clamps ordinary navigation.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    const QRectF sheet = m_scene->sheetRect();
    setSceneRect(sheet.adjusted(-kPannableSheets * sheet.width(), -kPannableSheets * sheet.height(),
                                kPannableSheets * sheet.width(), kPannableSheets * sheet.height()));

    connect(m_viewport, &ViewportController::viewportChanged, this, &A3CanvasView::applyViewportTransform);
    applyViewportTransform();
}

void A3CanvasView::applyViewportTransform()
{
    // Contract: viewport = zoom * (scene + pan).
    // QGraphicsView: viewport = zoom * scene - scroll   (scroll value 0 ⇔ scene
    // origin at the viewport's top-left; the scroll range spans the mapped
    // sceneRect). ⇒ scroll = -zoom * pan
    const double zoom = m_viewport->zoom();
    QTransform t;
    t.scale(zoom, zoom);
    setTransform(t);
    const QPointF pan = m_viewport->panOffset();
    horizontalScrollBar()->setValue(qRound(-zoom * pan.x()));
    verticalScrollBar()->setValue(qRound(-zoom * pan.y()));
    QGraphicsView::viewport()->update();
}

void A3CanvasView::fitSheet()
{
    const QRectF sheet = m_scene->sheetRect();
    const QSizeF viewSize = QGraphicsView::viewport()->size();
    const double zoom = ViewportController::fitZoom(sheet.size(), viewSize);
    // Center the sheet: pan so that the sheet centre lands at the viewport centre.
    // viewport = (scene + pan) * zoom  ⇒  pan = viewport/zoom − scene
    const QPointF viewCenter(viewSize.width() / 2.0, viewSize.height() / 2.0);
    const QPointF sheetCenter = sheet.center();
    m_viewport->setZoom(zoom);
    m_viewport->setPanOffset(QPointF(viewCenter.x() / m_viewport->zoom() - sheetCenter.x(),
                                     viewCenter.y() / m_viewport->zoom() - sheetCenter.y()));
}

core::PointMm A3CanvasView::mmAt(QPoint viewportPos) const
{
    return m_scene->coordinateSystem().toMm(mapToScene(viewportPos));
}

void A3CanvasView::wheelEvent(QWheelEvent* event)
{
    const QPoint angle = event->angleDelta();
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const int steps = angle.y() > 0 ? 1 : (angle.y() < 0 ? -1 : 0);
        if (steps != 0) {
            const QPointF anchorViewport = event->position();
            const QPointF anchorScene = mapToScene(anchorViewport.toPoint());
            const double target = m_viewport->zoom() * std::pow(ViewportController::kDefaultZoomStep, steps);
            m_viewport->zoomAround(target, anchorScene, anchorViewport);
        }
        event->accept();
        return;
    }
    // Plain wheel pans. Scene delta = pixel delta / zoom.
    const double pixels = angle.y() / 120.0 * 60.0;
    QPointF delta = event->modifiers().testFlag(Qt::ShiftModifier) ? QPointF(pixels, 0.0) : QPointF(0.0, pixels);
    if (angle.x() != 0) {
        delta.setX(delta.x() + angle.x() / 120.0 * 60.0);
    }
    m_viewport->panBy(delta / m_viewport->zoom());
    event->accept();
}

void A3CanvasView::mousePressEvent(QMouseEvent* event)
{
    const bool panButton = event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && m_spaceHeld);
    if (panButton) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void A3CanvasView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        m_viewport->panBy(QPointF(delta) / m_viewport->zoom());
        event->accept();
        return;
    }
    const core::PointMm mm = mmAt(event->pos());
    emit cursorMovedMm(mm.x, mm.y);
    QGraphicsView::mouseMoveEvent(event);
}

void A3CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_panning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_panning = false;
        setCursor(m_spaceHeld ? Qt::OpenHandCursor : Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void A3CanvasView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::ZoomIn) || (event->key() == Qt::Key_Equal && event->modifiers().testFlag(Qt::ControlModifier))) {
        m_viewport->zoomBy(1);
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::ZoomOut)) {
        m_viewport->zoomBy(-1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_0 && event->modifiers().testFlag(Qt::ControlModifier)) {
        fitSheet();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void A3CanvasView::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = false;
        if (!m_panning) {
            setCursor(Qt::ArrowCursor);
        }
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void A3CanvasView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (!m_initialFitDone && event->size().isValid() && event->size().width() > 0) {
        m_initialFitDone = true;
        fitSheet();
    }
}

} // namespace ardulab::canvas
