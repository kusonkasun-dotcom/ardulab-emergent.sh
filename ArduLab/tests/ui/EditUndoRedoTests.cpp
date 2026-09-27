// Interactive editing + undo/redo (MVP completion). Drives the real UI action
// and slot paths headlessly: place, move (drag), rotate, delete, property
// changes (display name, value, reference designator), and the unsaved-changes
// indicator after Save / Undo / Redo.

#include "canvas/CoordinateSystem.h"
#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "core/EventBus.h"
#include "project/ProjectService.h"
#include "ui/ComponentGraphicsItem.h"
#include "ui/EditingFixture.h"
#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QGraphicsItem>
#include <QListWidget>
#include <QMetaObject>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest/QtTest>

#include <memory>

using namespace ardulab;
using ardulab::testing::firstItem;
using ardulab::testing::renderedItemCount;
using ardulab::testing::seedResistor;

namespace {

/// Minimal headless harness: catalog with one resistor + an empty project.
struct Harness final
{
    std::shared_ptr<core::EventBus> bus = core::EventBus::create();
    components::InMemoryComponentCatalog catalog;
    components::ComponentManager manager{catalog, bus};
    std::unique_ptr<project::ProjectService> service;
    std::unique_ptr<ui::MainWindow> window;

    void build()
    {
        QVERIFY(manager.registerSnapshot(seedResistor()).isSuccess());
        service = std::make_unique<project::ProjectService>(&manager, bus);

        ui::MainWindowDependencies deps;
        deps.eventBus = bus;
        deps.projectService = service.get();
        deps.componentManager = &manager;
        deps.applicationVersion = QStringLiteral("0.1.3-edit");
        window = std::make_unique<ui::MainWindow>(std::move(deps));
        window->resize(1200, 800);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window.get()));
        QVERIFY(service->createNew(QStringLiteral("EDIT")).isSuccess());
        QCoreApplication::processEvents();
    }

    /// Place the catalog's only component through the real UI slot.
    void place()
    {
        auto* list = window->findChild<QListWidget*>(QStringLiteral("CatalogList"));
        QVERIFY(list && list->count() == 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(window.get(), "onPlaceSelectedComponent", Qt::DirectConnection));
        QCoreApplication::processEvents();
    }

    [[nodiscard]] QAction* action(const char* name) const
    {
        return window->findChild<QAction*>(QString::fromLatin1(name));
    }
};

void sendMouse(QWidget* target, QEvent::Type type, QPoint pos, Qt::MouseButton button)
{
    QMouseEvent event(type, QPointF(pos), target->mapToGlobal(QPointF(pos)), button, button, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &event);
}

} // namespace

class EditUndoRedoTests final : public QObject
{
    Q_OBJECT

private slots:
    void placeUndoRedoRotateDeleteMoveAndDirtyState()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Harness h;
        h.build();
        project::ProjectService& projectService = *h.service;
        ui::MainWindow& window = *h.window;

        QAction* undoAction = h.action("EditUndoAction");
        QAction* redoAction = h.action("EditRedoAction");
        QVERIFY(undoAction && redoAction);

        // --- Place (through the catalog slot → PlaceInstanceCommand) ---------
        h.place();
        QCOMPARE(projectService.currentProject()->instances().size(), std::size_t{1});
        QCOMPARE(renderedItemCount(window), 1);
        QVERIFY(projectService.currentProject()->isDirty());

        const core::InstanceId id = projectService.currentProject()->instances().front().instanceId;

        // --- Save clears the dirty flag -------------------------------------
        const QString path = dir.filePath(QStringLiteral("EDIT.FAL"));
        QVERIFY(projectService.saveAs(path).isSuccess());
        QCoreApplication::processEvents();
        QVERIFY(!projectService.currentProject()->isDirty());
        QVERIFY(!window.windowTitle().contains(QLatin1Char('*')));

        // --- Undo place → empty + dirty again (below clean index) -----------
        QVERIFY(undoAction->isEnabled());
        undoAction->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(projectService.currentProject()->instances().size(), std::size_t{0});
        QCOMPARE(renderedItemCount(window), 0);
        QVERIFY(projectService.currentProject()->isDirty());
        QVERIFY(window.windowTitle().contains(QLatin1Char('*')));

        // --- Redo place → back to clean (at saved index) --------------------
        QVERIFY(redoAction->isEnabled());
        redoAction->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(projectService.currentProject()->instances().size(), std::size_t{1});
        QCOMPARE(renderedItemCount(window), 1);
        QVERIFY(!projectService.currentProject()->isDirty());
        QVERIFY(!window.windowTitle().contains(QLatin1Char('*')));

        // --- Rotate the selected item ---------------------------------------
        ui::ComponentGraphicsItem* item = firstItem(window);
        QVERIFY(item != nullptr);
        item->setSelected(true);
        QVERIFY(QMetaObject::invokeMethod(&window, "onRotateSelected", Qt::DirectConnection));
        QCoreApplication::processEvents();
        QCOMPARE(projectService.currentProject()->findInstance(id)->rotationDegrees, 90.0);
        QCOMPARE(firstItem(window)->rotationDegrees(), 90.0);
        QVERIFY(projectService.currentProject()->isDirty());

        // Undo rotate → back to 0°.
        undoAction->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(projectService.currentProject()->findInstance(id)->rotationDegrees, 0.0);
        QCOMPARE(firstItem(window)->rotationDegrees(), 0.0);

        // --- Move primitive updates model and item together -----------------
        window.moveInstanceTo(id, core::PointMm(42.0, 24.0));
        QCOMPARE(projectService.currentProject()->findInstance(id)->position, core::PointMm(42.0, 24.0));
        QVERIFY(firstItem(window)->positionMm() == core::PointMm(42.0, 24.0));

        // --- Delete the selected item, then undo restores it ----------------
        firstItem(window)->setSelected(true);
        QVERIFY(QMetaObject::invokeMethod(&window, "onDeleteSelected", Qt::DirectConnection));
        QCoreApplication::processEvents();
        QCOMPARE(projectService.currentProject()->instances().size(), std::size_t{0});
        QCOMPARE(renderedItemCount(window), 0);

        undoAction->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(projectService.currentProject()->instances().size(), std::size_t{1});
        QCOMPARE(projectService.currentProject()->findInstance(id)->instanceId.value(), id.value());
        QCOMPARE(renderedItemCount(window), 1);
    }

    /// A single press → drag → release gesture must produce exactly one
    /// undoable step, not one per mouse-move.
    void oneDragProducesExactlyOneUndoStep()
    {
        Harness h;
        h.build();
        h.place();
        ui::MainWindow& window = *h.window;
        project::Project* proj = h.service->currentProject();
        const core::InstanceId id = proj->instances().front().instanceId;

        window.canvasView()->fitSheet();
        QCoreApplication::processEvents();

        ui::ComponentGraphicsItem* item = firstItem(window);
        QVERIFY(item != nullptr);
        const core::PointMm start = item->positionMm();
        const int stepsBefore = window.undoStack()->count();

        QWidget* vp = window.canvasView()->viewport();
        const QPoint press = window.canvasView()->mapFromScene(item->scenePos());
        QVERIFY2(vp->rect().contains(press), "item must be inside the fitted viewport");

        sendMouse(vp, QEvent::MouseButtonPress, press, Qt::LeftButton);
        // Several intermediate moves, as a real drag produces.
        for (int i = 1; i <= 3; ++i) {
            item->setPositionMm(core::PointMm(start.x + 2.2 * i, start.y + 1.7 * i));
            sendMouse(vp, QEvent::MouseMove, press + QPoint(6 * i, 4 * i), Qt::NoButton);
        }
        sendMouse(vp, QEvent::MouseButtonRelease, press + QPoint(18, 12), Qt::LeftButton);
        QCoreApplication::processEvents();

        QCOMPARE(window.undoStack()->count(), stepsBefore + 1);
        const core::PointMm dropped = proj->findInstance(id)->position;
        QVERIFY(dropped != start);
        // Dropped position is grid-snapped in millimeters.
        QCOMPARE(dropped, canvas::CoordinateSystem::snapToGrid(dropped, proj->canvas().gridMm));

        h.action("EditUndoAction")->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(proj->findInstance(id)->position, start);
        QCOMPARE(firstItem(window)->positionMm(), start);

        h.action("EditRedoAction")->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(proj->findInstance(id)->position, dropped);
        QCOMPARE(firstItem(window)->positionMm(), dropped);
    }

    /// Property changes are separate undoable steps: display name (rename),
    /// engineering value, and the reference designator.
    void valueAndReferenceChangesAreUndoable()
    {
        Harness h;
        h.build();
        h.place();
        ui::MainWindow& window = *h.window;
        project::Project* proj = h.service->currentProject();
        const core::InstanceId id = proj->instances().front().instanceId;
        const core::InstanceId renamed(QStringLiteral("R42"));

        QAction* undo = h.action("EditUndoAction");
        QAction* redo = h.action("EditRedoAction");
        QVERIFY(undo && redo);
        QVERIFY(proj->findInstance(id)->value.isEmpty());

        // --- Value change is its own undoable step --------------------------
        const int stepsBefore = window.undoStack()->count();
        window.changeInstanceValue(id, QStringLiteral("10k 1%"));
        QCOMPARE(window.undoStack()->count(), stepsBefore + 1);
        QCOMPARE(proj->findInstance(id)->value, QStringLiteral("10k 1%"));
        QVERIFY(proj->isDirty());

        undo->trigger();
        QCoreApplication::processEvents();
        QVERIFY(proj->findInstance(id)->value.isEmpty());
        redo->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(proj->findInstance(id)->value, QStringLiteral("10k 1%"));

        // A no-op change pushes nothing.
        window.changeInstanceValue(id, QStringLiteral("10k 1%"));
        QCOMPARE(window.undoStack()->count(), stepsBefore + 1);

        // --- Reference designator change is a distinct undoable step --------
        window.changeInstanceReference(id, renamed);
        QCOMPARE(window.undoStack()->count(), stepsBefore + 2);
        QVERIFY(proj->findInstance(renamed) != nullptr);
        QVERIFY(proj->findInstance(id) == nullptr);
        QCOMPARE(renderedItemCount(window), 1);
        QCOMPARE(firstItem(window)->instanceId().value(), renamed.value());
        // The value travels with the instance, not with the designator.
        QCOMPARE(proj->findInstance(renamed)->value, QStringLiteral("10k 1%"));

        // A designator already in use is rejected without touching the stack.
        window.changeInstanceReference(renamed, renamed);
        QCOMPARE(window.undoStack()->count(), stepsBefore + 2);

        undo->trigger();
        QCoreApplication::processEvents();
        QVERIFY(proj->findInstance(id) != nullptr);
        QCOMPARE(firstItem(window)->instanceId().value(), id.value());
        QCOMPARE(proj->findInstance(id)->value, QStringLiteral("10k 1%"));

        redo->trigger();
        QCoreApplication::processEvents();
        QVERIFY(proj->findInstance(renamed) != nullptr);
        QCOMPARE(firstItem(window)->instanceId().value(), renamed.value());

        // --- Display name rename remains a separate property ---------------
        const QString originalName = proj->findInstance(renamed)->displayName;
        window.renameInstanceTo(renamed, QStringLiteral("Pull-up"));
        QCOMPARE(proj->findInstance(renamed)->displayName, QStringLiteral("Pull-up"));
        QCOMPARE(proj->findInstance(renamed)->value, QStringLiteral("10k 1%")); // untouched
        window.renameInstanceTo(renamed, originalName);
        QCOMPARE(proj->findInstance(renamed)->displayName, originalName);
    }
};

QTEST_MAIN(EditUndoRedoTests)
#include "EditUndoRedoTests.moc"
