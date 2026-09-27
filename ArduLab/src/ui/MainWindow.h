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

namespace ardulab::ui {

class ComponentGraphicsItem;

struct MainWindowDependencies final
{
    std::shared_ptr<core::EventBus> eventBus;
    project::ProjectService* projectService = nullptr;      ///< Owned by bootstrap.
    components::IComponentManager* componentManager = nullptr; ///< Owned by bootstrap.
    QString applicationVersion;
};

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(MainWindowDependencies deps, QWidget* parent = nullptr);
    ~MainWindow() override;

    [[nodiscard]] canvas::A3CanvasView* canvasView() const noexcept { return m_view; }

    /// Show a startup/operation problem in the status bar and a dialog.
    void reportError(const core::Error& error);

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onCloseProject();
    void onFitSheet();
    void onToggleGrid(bool checked);
    void onRefreshCatalog();
    void onPlaceSelectedComponent();
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

    MainWindowDependencies m_deps;

    canvas::ViewportController* m_viewport = nullptr;
    canvas::A3CanvasScene* m_scene = nullptr;
    canvas::A3CanvasView* m_view = nullptr;

    QListWidget* m_catalogList = nullptr;
    QLabel* m_statusCursor = nullptr;
    QLabel* m_statusZoom = nullptr;
    QLabel* m_statusCatalog = nullptr;

    QAction* m_actSave = nullptr;
    QAction* m_actSaveAs = nullptr;
    QAction* m_actClose = nullptr;
    QAction* m_actPlace = nullptr;

    std::vector<ComponentGraphicsItem*> m_items; // owned by the scene
    std::vector<core::Subscription> m_subscriptions;
};

} // namespace ardulab::ui
