#pragma once

// ComponentGraphicsItem — Renderer adapter (Plan §4.9, §8.1).
//
// Projects an immutable ComponentSnapshot onto the A3 scene:
//   - Reads Package / Pin / PinAnchor values; never mutates them.
//   - Converts physical millimeters through CoordinateSystem.
//   - Never queries the catalog or SQLite; never owns persistence.
//
// Item-local coordinates are scene units of the *component-local* frame
// (package origin), so the item's pos() is the instance placement.

#include "canvas/CoordinateSystem.h"
#include "components/ComponentSnapshot.h"
#include "core/Identifiers.h"

#include <QGraphicsItem>

namespace ardulab::ui {

class ComponentGraphicsItem final : public QGraphicsItem
{
public:
    enum { Type = QGraphicsItem::UserType + 1 };

    ComponentGraphicsItem(components::ComponentSnapshotPtr snapshot,
                          core::InstanceId instanceId,
                          const canvas::CoordinateSystem& coordinates,
                          QGraphicsItem* parent = nullptr);

    [[nodiscard]] int type() const override { return Type; }

    [[nodiscard]] const components::ComponentSnapshot& snapshot() const noexcept { return *m_snapshot; }
    [[nodiscard]] const core::InstanceId& instanceId() const noexcept { return m_instanceId; }

    /// Place the item at a millimeter position on the sheet.
    void setPositionMm(core::PointMm positionMm);
    [[nodiscard]] core::PointMm positionMm() const;

    /// Scene position of a pin anchor (for future Connection Core / UI hit-testing).
    [[nodiscard]] QPointF anchorScenePos(const components::PinAnchor& anchor) const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    [[nodiscard]] QRectF bodyRect() const;

    components::ComponentSnapshotPtr m_snapshot;
    core::InstanceId m_instanceId;
    canvas::CoordinateSystem m_coordinates;
};

} // namespace ardulab::ui
