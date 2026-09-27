#include "ui/MainWindow.h"

#include "components/ComponentManager.h"
#include "ui/ComponentGraphicsItem.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QStatusBar>
#include <QToolBar>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace ardulab::ui {

namespace {
const QString kFalFilter = QStringLiteral("ArduLab Project (*.FAL *.fal);;All files (*)");
constexpr int kRoleComponentId = Qt::UserRole + 1;
constexpr int kRoleVersionId = Qt::UserRole + 2;
constexpr int kRoleScope = Qt::UserRole + 3;
constexpr double kAnchorTolerancePx = 12.0; ///< Screen-pixel snap tolerance for the anchor readout.

// ---- Undo commands: each drives MainWindow's model+scene primitives so a
//      single user gesture is exactly one reversible step. ------------------

class PlaceInstanceCommand final : public QUndoCommand
{
public:
    PlaceInstanceCommand(MainWindow* w, project::ComponentInstance instance)
        : m_window(w), m_instance(std::move(instance))
    {
        setText(QStringLiteral("Place %1").arg(m_instance.instanceId.value()));
    }
    void redo() override { m_window->placeInstance(m_instance); }
    void undo() override { m_window->removeInstanceById(m_instance.instanceId); }

private:
    MainWindow* m_window;
    project::ComponentInstance m_instance;
};

class DeleteInstanceCommand final : public QUndoCommand
{
public:
    DeleteInstanceCommand(MainWindow* w, project::ComponentInstance instance)
        : m_window(w), m_instance(std::move(instance))
    {
        setText(QStringLiteral("Delete %1").arg(m_instance.instanceId.value()));
    }
    void redo() override { m_window->removeInstanceById(m_instance.instanceId); }
    void undo() override { m_window->placeInstance(m_instance); }

private:
    MainWindow* m_window;
    project::ComponentInstance m_instance;
};

class MoveInstanceCommand final : public QUndoCommand
{
public:
    MoveInstanceCommand(MainWindow* w, core::InstanceId id, core::PointMm oldPos, core::PointMm newPos)
        : m_window(w), m_id(std::move(id)), m_old(oldPos), m_new(newPos)
    {
        setText(QStringLiteral("Move %1").arg(m_id.value()));
    }
    void redo() override { m_window->moveInstanceTo(m_id, m_new); }
    void undo() override { m_window->moveInstanceTo(m_id, m_old); }

private:
    MainWindow* m_window;
    core::InstanceId m_id;
    core::PointMm m_old;
    core::PointMm m_new;
};

class RotateInstanceCommand final : public QUndoCommand
{
public:
    RotateInstanceCommand(MainWindow* w, core::InstanceId id, double oldDeg, double newDeg)
        : m_window(w), m_id(std::move(id)), m_old(oldDeg), m_new(newDeg)
    {
        setText(QStringLiteral("Rotate %1").arg(m_id.value()));
    }
    void redo() override { m_window->rotateInstanceTo(m_id, m_new); }
    void undo() override { m_window->rotateInstanceTo(m_id, m_old); }

private:
    MainWindow* m_window;
    core::InstanceId m_id;
    double m_old;
    double m_new;
};

class RenameInstanceCommand final : public QUndoCommand
{
public:
    RenameInstanceCommand(MainWindow* w, core::InstanceId id, QString oldName, QString newName)
        : m_window(w), m_id(std::move(id)), m_old(std::move(oldName)), m_new(std::move(newName))
    {
        setText(QStringLiteral("Rename %1").arg(m_id.value()));
    }
    void redo() override { m_window->renameInstanceTo(m_id, m_new); }
    void undo() override { m_window->renameInstanceTo(m_id, m_old); }

private:
    MainWindow* m_window;
    core::InstanceId m_id;
    QString m_old;
    QString m_new;
};

class ChangeValueCommand final : public QUndoCommand
{
public:
    ChangeValueCommand(MainWindow* w, core::InstanceId id, QString oldValue, QString newValue)
        : m_window(w), m_id(std::move(id)), m_old(std::move(oldValue)), m_new(std::move(newValue))
    {
        setText(QStringLiteral("Change value of %1").arg(m_id.value()));
    }
    void redo() override { m_window->setInstanceValueTo(m_id, m_new); }
    void undo() override { m_window->setInstanceValueTo(m_id, m_old); }

private:
    MainWindow* m_window;
    core::InstanceId m_id;
    QString m_old;
    QString m_new;
};

/// Reference designator change ("R1" → "R7"). Distinct from a display-name
/// rename: the designator is the instance identity recorded in the .FAL file.
class ChangeReferenceCommand final : public QUndoCommand
{
public:
    ChangeReferenceCommand(MainWindow* w, core::InstanceId oldId, core::InstanceId newId)
        : m_window(w), m_old(std::move(oldId)), m_new(std::move(newId))
    {
        setText(QStringLiteral("Reference %1 → %2").arg(m_old.value(), m_new.value()));
    }
    void redo() override { m_window->setInstanceReferenceTo(m_old, m_new); }
    void undo() override { m_window->setInstanceReferenceTo(m_new, m_old); }

private:
    MainWindow* m_window;
    core::InstanceId m_old;
    core::InstanceId m_new;
};

} // namespace

MainWindow::MainWindow(MainWindowDependencies deps, QWidget* parent)
    : QMainWindow(parent)
    , m_deps(std::move(deps))
{
    setObjectName(QStringLiteral("ArduLabMainWindow"));
    resize(1400, 900);

    m_viewport = new canvas::ViewportController(this);
    m_scene = new canvas::A3CanvasScene(canvas::CoordinateSystem(), this);
    m_view = new canvas::A3CanvasView(m_scene, m_viewport, this);
    setCentralWidget(m_view);

    m_undoStack = new QUndoStack(this);

    buildMenus();
    buildDocks();
    buildStatusBar();
    subscribeToEvents();

    connect(m_view, &canvas::A3CanvasView::cursorMovedMm, this, &MainWindow::onCursorMoved);
    connect(m_viewport, &canvas::ViewportController::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &MainWindow::updateActionStates);

    // Dirty state follows the undo stack: clean index ⇔ saved project.
    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool clean) {
        if (project::Project* p = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr) {
            if (clean) {
                p->markClean();
            } else {
                p->markDirty();
            }
        }
        updateWindowTitle();
    });

    // Drag detection + snap readout live on the viewport's event stream so the
    // canvas module stays free of catalog/UI types.
    m_view->viewport()->installEventFilter(this);

    onRefreshCatalog();
    updateWindowTitle();
    updateActionStates();
}

MainWindow::~MainWindow()
{
    // Tear down in a deterministic order: child widgets (scene, undo stack,
    // viewport) are destroyed by QObject *after* this body runs, and their
    // signals must not reach a half-destroyed MainWindow.
    m_subscriptions.clear();
    if (m_view != nullptr && m_view->viewport() != nullptr) {
        m_view->viewport()->removeEventFilter(this);
    }
    if (m_scene != nullptr) {
        disconnect(m_scene, nullptr, this, nullptr);
    }
    if (m_undoStack != nullptr) {
        disconnect(m_undoStack, nullptr, this, nullptr);
        m_undoStack->clear();
    }
    if (m_viewport != nullptr) {
        disconnect(m_viewport, nullptr, this, nullptr);
    }
    if (m_view != nullptr) {
        disconnect(m_view, nullptr, this, nullptr);
    }
    clearSceneItems();
}

// ---------------------------------------------------------------------------
// Composition
// ---------------------------------------------------------------------------

void MainWindow::buildMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* actNew = fileMenu->addAction(tr("&New Project…"), QKeySequence::New, this, &MainWindow::onNewProject);
    QAction* actOpen = fileMenu->addAction(tr("&Open Project…"), QKeySequence::Open, this, &MainWindow::onOpenProject);
    fileMenu->addSeparator();
    m_actSave = fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::onSaveProject);
    m_actSaveAs = fileMenu->addAction(tr("Save &As…"), QKeySequence::SaveAs, this, &MainWindow::onSaveProjectAs);
    fileMenu->addSeparator();
    m_actClose = fileMenu->addAction(tr("&Close Project"), QKeySequence::Close, this, &MainWindow::onCloseProject);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, qApp, &QApplication::closeAllWindows);

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction* actUndo = m_undoStack->createUndoAction(this, tr("&Undo"));
    actUndo->setShortcut(QKeySequence::Undo);
    actUndo->setObjectName(QStringLiteral("EditUndoAction"));
    QAction* actRedo = m_undoStack->createRedoAction(this, tr("&Redo"));
    actRedo->setShortcut(QKeySequence::Redo);
    actRedo->setObjectName(QStringLiteral("EditRedoAction"));
    editMenu->addAction(actUndo);
    editMenu->addAction(actRedo);
    editMenu->addSeparator();
    m_actRotate = editMenu->addAction(tr("Rotate &90°"), QKeySequence(Qt::Key_R), this, &MainWindow::onRotateSelected);
    m_actRotate->setObjectName(QStringLiteral("EditRotateAction"));
    m_actDelete = editMenu->addAction(tr("&Delete"), QKeySequence::Delete, this, &MainWindow::onDeleteSelected);
    m_actDelete->setObjectName(QStringLiteral("EditDeleteAction"));
    m_actRename = editMenu->addAction(tr("Re&name Instance…"), QKeySequence(Qt::Key_F2), this,
                                      &MainWindow::onRenameSelected);
    m_actRename->setObjectName(QStringLiteral("EditRenameAction"));
    m_actValue = editMenu->addAction(tr("Change &Value…"), this, &MainWindow::onChangeValueSelected);
    m_actValue->setObjectName(QStringLiteral("EditValueAction"));
    m_actReference = editMenu->addAction(tr("Change Re&ference…"), this, &MainWindow::onChangeReferenceSelected);
    m_actReference->setObjectName(QStringLiteral("EditReferenceAction"));

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, m_viewport, [this] { m_viewport->zoomBy(1); });
    viewMenu->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, m_viewport, [this] { m_viewport->zoomBy(-1); });
    viewMenu->addAction(tr("&Fit Sheet"), QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::onFitSheet);
    viewMenu->addSeparator();
    QAction* actGrid = viewMenu->addAction(tr("Show &Grid"));
    actGrid->setCheckable(true);
    actGrid->setChecked(true);
    connect(actGrid, &QAction::toggled, this, &MainWindow::onToggleGrid);

    QMenu* catalogMenu = menuBar()->addMenu(tr("&Catalog"));
    catalogMenu->addAction(tr("&Refresh"), QKeySequence::Refresh, this, &MainWindow::onRefreshCatalog);
    QAction* actImport = catalogMenu->addAction(tr("&Import Component…"), this, &MainWindow::onImportComponent);
    actImport->setObjectName(QStringLiteral("CatalogImportAction"));
    actImport->setEnabled(m_deps.componentImporter != nullptr);
    m_actPlace = catalogMenu->addAction(tr("&Place Selected Component"), QKeySequence(Qt::Key_Insert), this,
                                        &MainWindow::onPlaceSelectedComponent);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About ArduLab"), this, [this] {
        QMessageBox::about(this, tr("About ArduLab"),
                           tr("<b>ArduLab %1</b><br/>Offline-first Electronic Engineering Platform.<br/>"
                              "Clean foundation: Core · Project · A3 Canvas · Component Engine · SQLite Catalog.")
                               .arg(m_deps.applicationVersion));
    });

    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName(QStringLiteral("MainToolBar"));
    toolbar->setMovable(false);
    toolbar->addAction(actNew);
    toolbar->addAction(actOpen);
    toolbar->addAction(m_actSave);
    toolbar->addSeparator();
    toolbar->addAction(actUndo);
    toolbar->addAction(actRedo);
    toolbar->addSeparator();
    toolbar->addAction(m_actPlace);
    toolbar->addAction(m_actRotate);
    toolbar->addAction(m_actDelete);
}

void MainWindow::buildDocks()
{
    auto* dock = new QDockWidget(tr("Component Catalog"), this);
    dock->setObjectName(QStringLiteral("CatalogDock"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_catalogList = new QListWidget(dock);
    m_catalogList->setObjectName(QStringLiteral("CatalogList"));
    m_catalogList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_catalogList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { onPlaceSelectedComponent(); });
    connect(m_catalogList, &QListWidget::itemSelectionChanged, this, &MainWindow::updateActionStates);

    dock->setWidget(m_catalogList);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::buildStatusBar()
{
    m_statusCursor = new QLabel(tr("X: —  Y: —"), this);
    m_statusCursor->setObjectName(QStringLiteral("StatusCursor"));
    m_statusCursor->setMinimumWidth(170);
    m_statusSnap = new QLabel(tr("Snap: —"), this);
    m_statusSnap->setObjectName(QStringLiteral("StatusSnap"));
    m_statusSnap->setMinimumWidth(170);
    m_statusAnchor = new QLabel(tr("Pin: —"), this);
    m_statusAnchor->setObjectName(QStringLiteral("StatusAnchor"));
    m_statusAnchor->setMinimumWidth(220);
    m_statusZoom = new QLabel(tr("Zoom: 100%"), this);
    m_statusZoom->setObjectName(QStringLiteral("StatusZoom"));
    m_statusZoom->setMinimumWidth(90);
    m_statusCatalog = new QLabel(this);
    m_statusCatalog->setObjectName(QStringLiteral("StatusCatalog"));
    statusBar()->addPermanentWidget(m_statusCatalog);
    statusBar()->addPermanentWidget(m_statusZoom);
    statusBar()->addPermanentWidget(m_statusAnchor);
    statusBar()->addPermanentWidget(m_statusSnap);
    statusBar()->addPermanentWidget(m_statusCursor);
    statusBar()->showMessage(tr("Ready"), 3000);
}

void MainWindow::subscribeToEvents()
{
    if (!m_deps.eventBus) {
        return;
    }
    m_subscriptions.push_back(m_deps.eventBus->subscribe<project::ProjectOpenedEvent>([this](const project::ProjectOpenedEvent&) {
        m_undoStack->clear();
        rebuildSceneFromProject();
        updateWindowTitle();
        updateActionStates();
    }));
    m_subscriptions.push_back(m_deps.eventBus->subscribe<project::ProjectSavedEvent>([this](const project::ProjectSavedEvent& e) {
        m_undoStack->setClean();
        statusBar()->showMessage(tr("Saved %1").arg(e.filePath), 4000);
        updateWindowTitle();
    }));
    m_subscriptions.push_back(m_deps.eventBus->subscribe<project::ProjectClosedEvent>([this](const project::ProjectClosedEvent&) {
        m_undoStack->clear();
        clearSceneItems();
        updateWindowTitle();
        updateActionStates();
    }));
    m_subscriptions.push_back(m_deps.eventBus->subscribe<components::ComponentRegisteredEvent>(
        [this](const components::ComponentRegisteredEvent&) { onRefreshCatalog(); }));
}

// ---------------------------------------------------------------------------
// Project intent → ProjectService
// ---------------------------------------------------------------------------

bool MainWindow::confirmDiscardChanges()
{
    const project::Project* p = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    if (p == nullptr || !p->isDirty()) {
        return true;
    }
    const auto answer = QMessageBox::question(this, tr("Unsaved changes"),
                                              tr("The project '%1' has unsaved changes. Discard them?").arg(p->metadata().name),
                                              QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

void MainWindow::onNewProject()
{
    if (m_deps.projectService == nullptr || !confirmDiscardChanges()) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Project"), tr("Project name:"), QLineEdit::Normal,
                                               tr("Untitled Project"), &ok);
    if (!ok) {
        return;
    }
    m_deps.projectService->close();
    if (const auto r = m_deps.projectService->createNew(name); !r) {
        reportError(r.error());
    }
}

void MainWindow::onOpenProject()
{
    if (m_deps.projectService == nullptr || !confirmDiscardChanges()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Open ArduLab Project"), QString(), kFalFilter);
    if (path.isEmpty()) {
        return;
    }
    // No pre-close: ProjectService::open is transactional, so a corrupt file
    // fails without discarding the document currently on screen.
    const auto r = m_deps.projectService->open(path);
    if (!r) {
        reportError(r.error());
        return;
    }
    const project::ProjectOpenResult& res = r.value();
    if (!res.report.warnings.isEmpty()) {
        statusBar()->showMessage(tr("Opened with %1 warning(s); %2 unresolved catalog reference(s)")
                                     .arg(res.report.warnings.size())
                                     .arg(res.unresolvedReferences),
                                 6000);
    }
}

void MainWindow::onSaveProject()
{
    if (m_deps.projectService == nullptr) {
        return;
    }
    if (m_deps.projectService->currentFilePath().isEmpty()) {
        onSaveProjectAs();
        return;
    }
    if (const auto r = m_deps.projectService->save(); !r) {
        reportError(r.error());
    }
}

void MainWindow::onSaveProjectAs()
{
    if (m_deps.projectService == nullptr || !m_deps.projectService->hasOpenProject()) {
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Save ArduLab Project"), QString(), kFalFilter);
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".FAL"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".FAL");
    }
    if (const auto r = m_deps.projectService->saveAs(path); !r) {
        reportError(r.error());
    }
}

void MainWindow::onCloseProject()
{
    if (m_deps.projectService == nullptr || !confirmDiscardChanges()) {
        return;
    }
    m_deps.projectService->close();
}

// ---------------------------------------------------------------------------
// Catalog intent → IComponentManager
// ---------------------------------------------------------------------------

void MainWindow::onRefreshCatalog()
{
    m_catalogList->clear();
    if (m_deps.componentManager == nullptr) {
        m_statusCatalog->setText(tr("Catalog: unavailable"));
        return;
    }
    components::ComponentSearchCriteria all;
    const auto results = m_deps.componentManager->search(all);
    if (!results) {
        m_statusCatalog->setText(tr("Catalog: error"));
        reportError(results.error());
        return;
    }
    for (const components::ComponentSummary& s : results.value()) {
        auto* item = new QListWidgetItem(QStringLiteral("%1  —  %2  [%3 · %4 · v%5]")
                                             .arg(s.componentId.value(), s.name,
                                                  QString::fromLatin1(components::catalogScopeName(s.scope)),
                                                  QString::fromLatin1(components::lifecycleStatusName(s.status)), s.version));
        item->setData(kRoleComponentId, s.componentId.value());
        item->setData(kRoleVersionId, s.activeVersionId.value());
        item->setData(kRoleScope, static_cast<int>(s.scope));
        m_catalogList->addItem(item);
    }
    m_statusCatalog->setText(tr("Catalog: %n component(s)", nullptr, static_cast<int>(results.value().size())));
    updateActionStates();
}

void MainWindow::onImportComponent()
{
    if (m_deps.componentImporter == nullptr) {
        statusBar()->showMessage(tr("Import is unavailable"), 3000);
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Component JSON"), QString(),
                                                      tr("Component JSON (*.json);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }
    // The UI forwards a path only; JSON parsing and validation live behind the
    // importer boundary (no SQL/JSON logic in MainWindow).
    const auto result = m_deps.componentImporter->importFromFile(path, components::CatalogScope::User);
    if (!result) {
        reportError(result.error());
        return;
    }
    const components::ImportReport& report = result.value();
    QString detail;
    for (const components::ImportMessage& m : report.messages) {
        detail += QStringLiteral("• [%1] %2%3\n")
                      .arg(QString::fromLatin1(components::importSeverityName(m.severity)),
                           m.ruleId.isEmpty() ? QString() : (m.ruleId + QStringLiteral(": ")), m.message);
    }

    switch (report.outcome) {
    case components::ImportOutcome::Imported:
        statusBar()->showMessage(tr("Imported %1 (%2 warning(s))")
                                     .arg(report.componentId.value())
                                     .arg(report.warningCount()),
                                 5000);
        onRefreshCatalog();
        if (report.warningCount() > 0) {
            QMessageBox::information(this, tr("Component imported"),
                                     tr("Imported %1 as DRAFT with warnings:\n\n%2")
                                         .arg(report.componentId.value(), detail));
        }
        break;
    case components::ImportOutcome::Skipped:
        QMessageBox::information(this, tr("Component already present"),
                                 tr("%1 was not imported:\n\n%2").arg(report.componentId.value(), detail));
        break;
    case components::ImportOutcome::Rejected:
        QMessageBox::warning(this, tr("Import rejected"),
                             tr("%1 was rejected:\n\n%2")
                                 .arg(report.componentId.isEmpty() ? tr("component") : report.componentId.value(),
                                      detail));
        break;
    }
}

void MainWindow::onPlaceSelectedComponent()
{
    if (m_deps.componentManager == nullptr || m_deps.projectService == nullptr) {
        return;
    }
    project::Project* proj = m_deps.projectService->currentProject();
    if (proj == nullptr) {
        statusBar()->showMessage(tr("Create or open a project first"), 3000);
        return;
    }
    const QListWidgetItem* item = m_catalogList->currentItem();
    if (item == nullptr) {
        return;
    }
    const auto scope = static_cast<components::CatalogScope>(item->data(kRoleScope).toInt());
    const core::ComponentId componentId(item->data(kRoleComponentId).toString());
    const core::ComponentVersionId versionId(item->data(kRoleVersionId).toString());

    // Exact version, recorded in the project — never "latest".
    const auto snapshot = m_deps.componentManager->load(scope, componentId, versionId);
    if (!snapshot) {
        reportError(snapshot.error());
        return;
    }

    const QString prefix = snapshot.value()->component().categoryId.value() == QLatin1String("PASSIVE")
        ? QStringLiteral("R") : QStringLiteral("U");
    // Generate a unique instance id (deletes can leave gaps).
    int n = static_cast<int>(proj->instances().size()) + 1;
    core::InstanceId instanceId(prefix + QString::number(n));
    while (proj->findInstance(instanceId) != nullptr) {
        ++n;
        instanceId = core::InstanceId(prefix + QString::number(n));
    }

    project::ComponentInstance instance;
    instance.instanceId = instanceId;
    instance.libraryId = componentId.value();
    instance.displayName = snapshot.value()->component().name;
    const double offset = 15.0 * static_cast<double>(proj->instances().size());
    instance.position = canvas::CoordinateSystem::snapToGrid(core::PointMm(210.0 + offset, 148.5 + offset), gridMm());
    project::CatalogReference ref;
    ref.scope = scope;
    ref.componentId = componentId;
    ref.versionId = versionId;
    ref.contentHash = snapshot.value()->version().contentHash;
    project::CatalogReference::FallbackSnapshot fb;
    fb.name = snapshot.value()->component().name;
    fb.referencePrefix = prefix;
    fb.packageId = snapshot.value()->package().packageId;
    for (const components::Pin& pin : snapshot.value()->pins()) {
        fb.pinNumbers << pin.pinNumber;
    }
    ref.fallback = std::move(fb);
    instance.catalogReference = std::move(ref);
    instance.referenceState = project::ReferenceState::Resolved;

    m_undoStack->push(new PlaceInstanceCommand(this, std::move(instance)));
    statusBar()->showMessage(tr("Placed %1 (%2)").arg(instanceId.value(), versionId.value()), 3000);
}

void MainWindow::onRotateSelected()
{
    ComponentGraphicsItem* item = selectedComponentItem();
    if (item == nullptr || m_deps.projectService == nullptr) {
        return;
    }
    project::Project* proj = m_deps.projectService->currentProject();
    const project::ComponentInstance* inst = proj ? proj->findInstance(item->instanceId()) : nullptr;
    if (inst == nullptr) {
        return;
    }
    const double next = std::fmod(inst->rotationDegrees + 90.0, 360.0);
    m_undoStack->push(new RotateInstanceCommand(this, item->instanceId(), inst->rotationDegrees, next));
}

void MainWindow::onDeleteSelected()
{
    ComponentGraphicsItem* item = selectedComponentItem();
    if (item == nullptr || m_deps.projectService == nullptr) {
        return;
    }
    project::Project* proj = m_deps.projectService->currentProject();
    const project::ComponentInstance* inst = proj ? proj->findInstance(item->instanceId()) : nullptr;
    if (inst == nullptr) {
        return;
    }
    m_undoStack->push(new DeleteInstanceCommand(this, *inst)); // copy keeps full state for undo
}

void MainWindow::onRenameSelected()
{
    ComponentGraphicsItem* item = selectedComponentItem();
    if (item == nullptr || m_deps.projectService == nullptr) {
        return;
    }
    project::Project* proj = m_deps.projectService->currentProject();
    const project::ComponentInstance* inst = proj ? proj->findInstance(item->instanceId()) : nullptr;
    if (inst == nullptr) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Rename Instance"), tr("Display name:"), QLineEdit::Normal,
                                               inst->displayName, &ok);
    if (!ok || name == inst->displayName) {
        return;
    }
    m_undoStack->push(new RenameInstanceCommand(this, item->instanceId(), inst->displayName, name));
}

void MainWindow::onChangeValueSelected()
{
    ComponentGraphicsItem* item = selectedComponentItem();
    if (item == nullptr || m_deps.projectService == nullptr) {
        return;
    }
    project::Project* proj = m_deps.projectService->currentProject();
    const project::ComponentInstance* inst = proj ? proj->findInstance(item->instanceId()) : nullptr;
    if (inst == nullptr) {
        return;
    }
    bool ok = false;
    const QString value = QInputDialog::getText(this, tr("Change Value"), tr("Value:"), QLineEdit::Normal,
                                                inst->value, &ok);
    if (!ok) {
        return;
    }
    changeInstanceValue(item->instanceId(), value);
}

void MainWindow::changeInstanceValue(const core::InstanceId& id, const QString& value)
{
    project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    const project::ComponentInstance* inst = proj ? proj->findInstance(id) : nullptr;
    if (inst == nullptr || inst->value == value) {
        return;
    }
    m_undoStack->push(new ChangeValueCommand(this, id, inst->value, value));
}

void MainWindow::changeInstanceReference(const core::InstanceId& from, const core::InstanceId& to)
{
    project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    if (proj == nullptr || !to.isValid() || to == from || proj->findInstance(from) == nullptr) {
        return;
    }
    if (proj->findInstance(to) != nullptr) {
        statusBar()->showMessage(tr("Reference %1 is already used").arg(to.value()), 4000);
        return;
    }
    m_undoStack->push(new ChangeReferenceCommand(this, from, to));
}

void MainWindow::onChangeReferenceSelected()
{
    ComponentGraphicsItem* item = selectedComponentItem();
    if (item == nullptr || m_deps.projectService == nullptr) {
        return;
    }
    project::Project* proj = m_deps.projectService->currentProject();
    const core::InstanceId current = item->instanceId();
    if (proj == nullptr || proj->findInstance(current) == nullptr) {
        return;
    }
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("Change Reference"), tr("Reference designator:"),
                                               QLineEdit::Normal, current.value(), &ok);
    if (!ok) {
        return;
    }
    changeInstanceReference(current, core::InstanceId(text.trimmed()));
}

// ---------------------------------------------------------------------------
// Edit primitives (model + scene) — called only by undo commands
// ---------------------------------------------------------------------------

void MainWindow::placeInstance(const project::ComponentInstance& instance)
{
    project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    if (proj == nullptr) {
        return;
    }
    if (!proj->addInstance(instance)) {
        return; // duplicate id — should not happen through the command path
    }
    renderInstance(instance);
    updateWindowTitle();
    updateActionStates();
}

void MainWindow::removeInstanceById(const core::InstanceId& id)
{
    if (ComponentGraphicsItem* item = itemFor(id)) {
        m_scene->removeItem(item);
        m_items.erase(std::remove(m_items.begin(), m_items.end(), item), m_items.end());
        delete item;
    }
    if (project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr) {
        (void)proj->removeInstance(id);
    }
    updateWindowTitle();
    updateActionStates();
}

void MainWindow::moveInstanceTo(const core::InstanceId& id, core::PointMm positionMm)
{
    if (project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr) {
        if (project::ComponentInstance* inst = proj->findInstanceMutable(id)) {
            inst->position = positionMm;
        }
    }
    if (ComponentGraphicsItem* item = itemFor(id)) {
        item->setPositionMm(positionMm);
    }
}

void MainWindow::rotateInstanceTo(const core::InstanceId& id, double degrees)
{
    if (project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr) {
        if (project::ComponentInstance* inst = proj->findInstanceMutable(id)) {
            inst->rotationDegrees = degrees;
        }
    }
    if (ComponentGraphicsItem* item = itemFor(id)) {
        item->setRotationDegrees(degrees);
    }
}

void MainWindow::renameInstanceTo(const core::InstanceId& id, const QString& name)
{
    if (project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr) {
        if (project::ComponentInstance* inst = proj->findInstanceMutable(id)) {
            inst->displayName = name;
        }
    }
    if (ComponentGraphicsItem* item = itemFor(id)) {
        item->setToolTip(name);
        item->update();
    }
    statusBar()->showMessage(tr("Renamed %1 → %2").arg(id.value(), name), 3000);
}

void MainWindow::setInstanceValueTo(const core::InstanceId& id, const QString& value)
{
    if (project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr) {
        if (project::ComponentInstance* inst = proj->findInstanceMutable(id)) {
            inst->value = value;
            if (ComponentGraphicsItem* item = itemFor(id)) {
                item->setToolTip(inst->displayName + (value.isEmpty() ? QString() : QStringLiteral("\n") + value));
                item->update();
            }
        }
    }
}

void MainWindow::setInstanceReferenceTo(const core::InstanceId& from, const core::InstanceId& to)
{
    project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    if (proj == nullptr || proj->findInstance(from) == nullptr || proj->findInstance(to) != nullptr) {
        return;
    }
    project::ComponentInstance* inst = proj->findInstanceMutable(from);
    inst->instanceId = to;

    // The item caches the designator for its label, so it is re-created from
    // the updated instance (selection is restored).
    const bool wasSelected = itemFor(from) != nullptr && itemFor(from)->isSelected();
    if (ComponentGraphicsItem* item = itemFor(from)) {
        m_scene->removeItem(item);
        m_items.erase(std::remove(m_items.begin(), m_items.end(), item), m_items.end());
        delete item;
    }
    renderInstance(*proj->findInstance(to));
    if (ComponentGraphicsItem* item = itemFor(to); item != nullptr && wasSelected) {
        item->setSelected(true);
    }
    updateActionStates();
}

// ---------------------------------------------------------------------------
// Scene projection
// ---------------------------------------------------------------------------

ComponentGraphicsItem* MainWindow::itemFor(const core::InstanceId& id) const
{
    for (ComponentGraphicsItem* item : m_items) {
        if (item->instanceId() == id) {
            return item;
        }
    }
    return nullptr;
}

ComponentGraphicsItem* MainWindow::selectedComponentItem() const
{
    for (ComponentGraphicsItem* item : m_items) {
        if (item->isSelected()) {
            return item;
        }
    }
    return nullptr;
}

void MainWindow::renderInstance(const project::ComponentInstance& instance)
{
    if (instance.referenceState != project::ReferenceState::Resolved || !instance.catalogReference
        || m_deps.componentManager == nullptr) {
        return; // unresolved/legacy instances stay in the model but are not rendered
    }
    const project::CatalogReference& ref = *instance.catalogReference;
    const auto snapshot = m_deps.componentManager->load(ref.scope, ref.componentId, ref.versionId);
    if (!snapshot) {
        return;
    }
    auto* gfx = new ComponentGraphicsItem(snapshot.value(), instance.instanceId, m_scene->coordinateSystem());
    gfx->setPositionMm(instance.position);
    gfx->setRotationDegrees(instance.rotationDegrees);
    m_scene->addItem(gfx);
    m_items.push_back(gfx);
}

void MainWindow::clearSceneItems()
{
    for (ComponentGraphicsItem* item : m_items) {
        m_scene->removeItem(item);
        delete item;
    }
    m_items.clear();
}

void MainWindow::rebuildSceneFromProject()
{
    clearSceneItems();
    const project::Project* proj = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    if (proj == nullptr) {
        return;
    }
    if (const auto& c = proj->canvas(); c.gridMm > 0.0) {
        m_scene->setGridMm(c.gridMm, c.gridMm * 10.0);
    }
    for (const project::ComponentInstance& instance : proj->instances()) {
        renderInstance(instance);
    }
    m_view->fitSheet();
}

// ---------------------------------------------------------------------------
// Input: drag-to-move detection + cursor snap / anchor readout
// ---------------------------------------------------------------------------

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (m_view && watched == m_view->viewport()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && !m_view->spaceHeld()) {
                if (auto* item = dynamic_cast<ComponentGraphicsItem*>(m_view->itemAt(me->pos()))) {
                    m_dragActive = true;
                    m_dragId = item->instanceId();
                    m_dragStartMm = item->positionMm();
                }
            }
            break;
        }
        case QEvent::MouseMove:
            updateSnapReadout(static_cast<QMouseEvent*>(event)->pos());
            break;
        case QEvent::MouseButtonRelease: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (m_dragActive && me->button() == Qt::LeftButton) {
                m_dragActive = false;
                if (ComponentGraphicsItem* item = itemFor(m_dragId)) {
                    const core::PointMm dropped = canvas::CoordinateSystem::snapToGrid(item->positionMm(), gridMm());
                    if (dropped != m_dragStartMm) {
                        // The free drag already moved the item; the command records
                        // the snapped result and makes it a single undoable step.
                        m_undoStack->push(new MoveInstanceCommand(this, m_dragId, m_dragStartMm, dropped));
                    } else {
                        item->setPositionMm(m_dragStartMm); // re-snap in place
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

double MainWindow::gridMm() const
{
    const project::Project* p = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    if (p != nullptr && p->canvas().gridMm > 0.0) {
        return p->canvas().gridMm;
    }
    return 1.0;
}

void MainWindow::updateSnapReadout(const QPoint& viewportPos)
{
    if (m_view == nullptr) {
        return;
    }
    const core::PointMm mm = m_view->mmAt(viewportPos);
    const core::PointMm snapped = canvas::CoordinateSystem::snapToGrid(mm, gridMm());
    m_statusSnap->setText(tr("Snap: %1, %2 mm").arg(snapped.x, 0, 'f', 2).arg(snapped.y, 0, 'f', 2));

    // Nearest pin anchor within a screen-pixel tolerance. Uses scene→viewport
    // mapping (zoom+pan) and the item's own transform (rotation), so it stays
    // correct after zoom, pan, and rotation. Geometric assistance only — this
    // forms no electrical net.
    double bestPx = kAnchorTolerancePx;
    QString bestText;
    for (const ComponentGraphicsItem* item : m_items) {
        for (const components::Pin& pin : item->snapshot().pins()) {
            const QPointF scenePos = item->anchorScenePos(pin.anchor());
            const QPoint vp = m_view->mapFromScene(scenePos);
            const double d = std::hypot(static_cast<double>(vp.x() - viewportPos.x()),
                                        static_cast<double>(vp.y() - viewportPos.y()));
            if (d <= bestPx) {
                bestPx = d;
                const core::PointMm anchorMm = m_scene->coordinateSystem().toMm(scenePos);
                bestText = tr("Pin %1·%2 @ %3, %4 mm")
                               .arg(item->instanceId().value(), pin.pinNumber)
                               .arg(anchorMm.x, 0, 'f', 2)
                               .arg(anchorMm.y, 0, 'f', 2);
            }
        }
    }
    m_statusAnchor->setText(bestText.isEmpty() ? tr("Pin: —") : tr("Pin: %1").arg(bestText));
}

// ---------------------------------------------------------------------------
// Presentation
// ---------------------------------------------------------------------------

void MainWindow::onFitSheet()
{
    m_view->fitSheet();
}

void MainWindow::onToggleGrid(bool checked)
{
    m_scene->setGridVisible(checked);
}

void MainWindow::onCursorMoved(double xMm, double yMm)
{
    m_statusCursor->setText(tr("X: %1 mm  Y: %2 mm").arg(xMm, 0, 'f', 2).arg(yMm, 0, 'f', 2));
}

void MainWindow::onZoomChanged(double zoom)
{
    m_statusZoom->setText(tr("Zoom: %1%").arg(qRound(zoom * 100.0)));
}

void MainWindow::updateWindowTitle()
{
    const project::Project* p = m_deps.projectService ? m_deps.projectService->currentProject() : nullptr;
    QString title = QStringLiteral("ArduLab %1").arg(m_deps.applicationVersion);
    if (p != nullptr) {
        title = QStringLiteral("%1%2 — %3").arg(p->metadata().name, p->isDirty() ? QStringLiteral("*") : QString(), title);
    }
    setWindowTitle(title);
}

void MainWindow::updateActionStates()
{
    const bool hasProject = m_deps.projectService != nullptr && m_deps.projectService->hasOpenProject();
    m_actSave->setEnabled(hasProject);
    m_actSaveAs->setEnabled(hasProject);
    m_actClose->setEnabled(hasProject);
    m_actPlace->setEnabled(hasProject && m_catalogList->currentItem() != nullptr);
    const bool hasSelection = selectedComponentItem() != nullptr;
    m_actRotate->setEnabled(hasProject && hasSelection);
    m_actDelete->setEnabled(hasProject && hasSelection);
    m_actRename->setEnabled(hasProject && hasSelection);
    m_actValue->setEnabled(hasProject && hasSelection);
    m_actReference->setEnabled(hasProject && hasSelection);
}

void MainWindow::reportError(const core::Error& error)
{
    statusBar()->showMessage(error.toString(), 8000);
    QMessageBox::warning(this, tr("ArduLab"), error.toString());
}

} // namespace ardulab::ui
