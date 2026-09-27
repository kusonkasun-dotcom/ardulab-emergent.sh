#pragma once

// MainWindow — thin Qt shell (Plan §4.9).
//
// Composes menus, the central A3 canvas view, a catalog dock, and the status
// bar. Forwards user intent to ProjectService and IComponentManager. Contains
// no SQL, JSON parsing, .FAL parsing, or component validation logic. Reacts to
// domain changes through EventBus subscriptions, not through direct calls
// from services.

#include "canvas/A3CanvasScene.h"
#include "canvas/A3CanvasView.h"
#include "canvas/ViewportController.h"
#include "components/IComponentImporter.h"
#include "components/IComponentManager.h"
#include "core/EventBus.h"
#include "project/ProjectService.h"

#include <QMainWindow>

#include <memory>
#include <vector>

class QAction;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QUndoStack;

namespace ardulab::ui {

class ComponentGraphicsItem;

struct MainWindowDependencies final
{
    std::shared_ptr<core::EventBus> eventBus;
    project::ProjectService* projectService = nullptr;      ///< Owned by bootstrap.
    components::IComponentManager* componentManager = nullptr; ///< Owned by bootstrap.
    components::IComponentImporter* componentImporter = nullptr; ///< Owned by bootstrap; may be null.
    QString applicationVersion;
};

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(MainWindowDependencies deps, QWidget* parent = nullptr);
    ~MainWindow() override;

    [[nodiscard]] canvas::A3CanvasView* canvasView() const noexcept { return m_view; }

    /// Undo history (read-only; exposed for tests and future history views).
    [[nodiscard]] const QUndoStack* undoStack() const noexcept { return m_undoStack; }

    /// Show a startup/operation problem in the status bar and a dialog.
    void reportError(const core::Error& error);

    // ---- undoable edit commands (dialog-free; the slots above collect input) --
    void changeInstanceValue(const core::InstanceId& id, const QString& value);
    void changeInstanceReference(const core::InstanceId& from, const core::InstanceId& to);

    // ---- edit primitives invoked by undo commands (model + scene together) ----
    void placeInstance(const project::ComponentInstance& instance);
    void removeInstanceById(const core::InstanceId& id);
    void moveInstanceTo(const core::InstanceId& id, core::PointMm positionMm);
    void rotateInstanceTo(const core::InstanceId& id, double degrees);
    void renameInstanceTo(const core::InstanceId& id, const QString& name);
    void setInstanceValueTo(const core::InstanceId& id, const QString& value);
    void setInstanceReferenceTo(const core::InstanceId& from, const core::InstanceId& to);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onCloseProject();
    void onFitSheet();
    void onToggleGrid(bool checked);
    void onRefreshCatalog();
    void onImportComponent();
    void onPlaceSelectedComponent();
    void onRotateSelected();
    void onDeleteSelected();
    void onRenameSelected();
    void onChangeValueSelected();
    void onChangeReferenceSelected();
    void onCursorMoved(double xMm, double yMm);
    void onZoomChanged(double zoom);

private:
    void buildMenus();
    void buildDocks();
    void buildStatusBar();
    void subscribeToEvents();
    void rebuildSceneFromProject();
    void clearSceneItems();
    void updateWindowTitle();
    void updateActionStates();
    [[nodiscard]] bool confirmDiscardChanges();

    // Edit helpers.
    [[nodiscard]] ComponentGraphicsItem* itemFor(const core::InstanceId& id) const;
    [[nodiscard]] ComponentGraphicsItem* selectedComponentItem() const;
    void renderInstance(const project::ComponentInstance& instance);
    [[nodiscard]] double gridMm() const;
    void updateSnapReadout(const QPoint& viewportPos);

    MainWindowDependencies m_deps;

    canvas::ViewportController* m_viewport = nullptr;
    canvas::A3CanvasScene* m_scene = nullptr;
    canvas::A3CanvasView* m_view = nullptr;
    QUndoStack* m_undoStack = nullptr;

    QListWidget* m_catalogList = nullptr;
    QLabel* m_statusCursor = nullptr;
    QLabel* m_statusSnap = nullptr;
    QLabel* m_statusAnchor = nullptr;
    QLabel* m_statusZoom = nullptr;
    QLabel* m_statusCatalog = nullptr;

    QAction* m_actSave = nullptr;
    QAction* m_actSaveAs = nullptr;
    QAction* m_actClose = nullptr;
    QAction* m_actPlace = nullptr;
    QAction* m_actRotate = nullptr;
    QAction* m_actDelete = nullptr;
    QAction* m_actRename = nullptr;
    QAction* m_actValue = nullptr;
    QAction* m_actReference = nullptr;

    // Interactive drag state (one drag == one undo command).
    bool m_dragActive = false;
    core::InstanceId m_dragId;
    core::PointMm m_dragStartMm;

    std::vector<ComponentGraphicsItem*> m_items; // owned by the scene
    std::vector<core::Subscription> m_subscriptions;
};

} // namespace ardulab::ui
