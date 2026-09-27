#pragma once

// Shared fixture for the headless UI editing tests (undo/redo + snap readout).
// Builds a two-pin resistor snapshot and helpers to inspect the scene.

#include "components/ComponentSnapshot.h"
#include "ui/ComponentGraphicsItem.h"
#include "ui/MainWindow.h"

#include <QGraphicsItem>

namespace ardulab::testing {

inline components::ComponentSnapshot seedResistor()
{
    using namespace components;
    Component c;
    c.componentId = core::ComponentId(QStringLiteral("R1-A-0805-10K"));
    c.name = QStringLiteral("Resistor 10k 0805");
    c.categoryId = core::CategoryId(QStringLiteral("PASSIVE"));
    c.version = QStringLiteral("1.0.0");
    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = c.version;
    v.versionId = core::makeComponentVersionId(c.componentId, c.version);
    v.contentHash = QStringLiteral("sha256:edit");
    Package p;
    p.packageId = core::PackageId(QStringLiteral("0805"));
    p.bodySize = core::SizeMm(2.0, 1.25);
    p.pinCount = 2;
    Pin p1;
    p1.pinNumber = QStringLiteral("1");
    p1.position = core::PointMm(-1.0, 0.0);
    p1.anchorPosition = core::PointMm(-1.0, 0.0);
    Pin p2 = p1;
    p2.pinNumber = QStringLiteral("2");
    p2.position = core::PointMm(1.0, 0.0);
    p2.anchorPosition = core::PointMm(1.0, 0.0);
    return ComponentSnapshot(c, v, p, {p1, p2}, {});
}

inline int renderedItemCount(ui::MainWindow& window)
{
    int n = 0;
    for (QGraphicsItem* gi : window.canvasView()->canvasScene()->items()) {
        if (qgraphicsitem_cast<ui::ComponentGraphicsItem*>(gi)) {
            ++n;
        }
    }
    return n;
}

inline ui::ComponentGraphicsItem* firstItem(ui::MainWindow& window)
{
    for (QGraphicsItem* gi : window.canvasView()->canvasScene()->items()) {
        if (auto* item = qgraphicsitem_cast<ui::ComponentGraphicsItem*>(gi)) {
            return item;
        }
    }
    return nullptr;
}

} // namespace ardulab::testing
