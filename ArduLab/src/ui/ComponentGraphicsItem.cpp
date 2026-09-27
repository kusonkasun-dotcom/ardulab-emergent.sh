#include "ui/ComponentGraphicsItem.h"

#include <QFont>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#include <algorithm>

namespace ardulab::ui {

namespace {
const QColor kBodyFill(0xff, 0xf6, 0xd5);
const QColor kBodyStroke(0x8a, 0x2b, 0x06);
const QColor kPinStroke(0x1f, 0x3a, 0x5f);
const QColor kAnchorFill(0xd6, 0x28, 0x28);
const QColor kLabel(0x22, 0x22, 0x22);
constexpr double kAnchorRadiusMm = 0.35;
constexpr double kFallbackBodyMm = 5.0;
} // namespace

ComponentGraphicsItem::ComponentGraphicsItem(components::ComponentSnapshotPtr snapshot,
                                             core::InstanceId instanceId,
                                             const canvas::CoordinateSystem& coordinates,
                                             QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_snapshot(std::move(snapshot))
    , m_instanceId(std::move(instanceId))
    , m_coordinates(coordinates)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false); // editing is a Schematic Engine concern
    setToolTip(m_snapshot->component().name + QStringLiteral("\n") + m_snapshot->version().versionId.value());
}

void ComponentGraphicsItem::setPositionMm(core::PointMm positionMm)
{
    setPos(m_coordinates.toScene(positionMm));
}

core::PointMm ComponentGraphicsItem::positionMm() const
{
    return m_coordinates.toMm(pos());
}

QPointF ComponentGraphicsItem::anchorScenePos(const components::PinAnchor& anchor) const
{
    return mapToScene(m_coordinates.toScene(anchor.position()));
}

QRectF ComponentGraphicsItem::bodyRect() const
{
    const components::Package& pkg = m_snapshot->package();
    core::SizeMm size = pkg.hasGeometry() ? pkg.bodySize : core::SizeMm(kFallbackBodyMm, kFallbackBodyMm);
    const QSizeF sceneSize = m_coordinates.toScene(size);
    // Body centred on the package origin (ADR §3.6: origin = package body origin).
    return QRectF(-sceneSize.width() / 2.0, -sceneSize.height() / 2.0, sceneSize.width(), sceneSize.height());
}

QRectF ComponentGraphicsItem::boundingRect() const
{
    QRectF rect = bodyRect();
    const double anchorPad = m_coordinates.toScene(core::Millimeters(kAnchorRadiusMm * 2.0));
    for (const components::Pin& pin : m_snapshot->pins()) {
        const QPointF p = m_coordinates.toScene(pin.position);
        const QPointF a = m_coordinates.toScene(pin.anchorPosition);
        rect |= QRectF(p, QSizeF(0, 0)).adjusted(-anchorPad, -anchorPad, anchorPad, anchorPad);
        rect |= QRectF(a, QSizeF(0, 0)).adjusted(-anchorPad, -anchorPad, anchorPad, anchorPad);
    }
    // Room for the label above the body.
    const double labelPad = m_coordinates.toScene(core::Millimeters(4.0));
    return rect.adjusted(-labelPad, -labelPad, labelPad, labelPad);
}

void ComponentGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /*widget*/)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF body = bodyRect();

    // Body
    QPen bodyPen(kBodyStroke);
    bodyPen.setCosmetic(true);
    bodyPen.setWidth(option->state.testFlag(QStyle::State_Selected) ? 3 : 2);
    painter->setPen(bodyPen);
    painter->setBrush(kBodyFill);
    painter->drawRect(body);

    // Pins: stub from pin position to anchor, plus anchor marker.
    QPen pinPen(kPinStroke);
    pinPen.setCosmetic(true);
    pinPen.setWidth(2);
    const double anchorRadius = m_coordinates.toScene(core::Millimeters(kAnchorRadiusMm));
    for (const components::Pin& pin : m_snapshot->pins()) {
        const QPointF p = m_coordinates.toScene(pin.position);
        const QPointF a = m_coordinates.toScene(pin.anchorPosition);
        painter->setPen(pinPen);
        if (p != a) {
            painter->drawLine(p, a);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(kAnchorFill);
        painter->drawEllipse(a, anchorRadius, anchorRadius);
    }

    // Label: instance ID and component name above the body.
    QFont font = painter->font();
    font.setPointSizeF(std::max(6.0, m_coordinates.toScene(core::Millimeters(1.6))));
    painter->setFont(font);
    painter->setPen(kLabel);
    const QString label = m_instanceId.isValid()
        ? m_instanceId.value() + QStringLiteral("  ") + m_snapshot->component().name
        : m_snapshot->component().name;
    const QRectF labelRect(body.left(), body.top() - m_coordinates.toScene(core::Millimeters(3.5)),
                           std::max(body.width(), m_coordinates.toScene(core::Millimeters(30.0))),
                           m_coordinates.toScene(core::Millimeters(3.0)));
    painter->drawText(labelRect, Qt::AlignLeft | Qt::AlignBottom, label);

    painter->restore();
}

} // namespace ardulab::ui
