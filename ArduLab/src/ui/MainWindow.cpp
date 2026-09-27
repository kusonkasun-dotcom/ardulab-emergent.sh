#include "ui/MainWindow.h"

#include "components/ComponentManager.h"
#include "ui/ComponentGraphicsItem.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

namespace ardulab::ui {

namespace {
const QString kFalFilter = QStringLiteral("ArduLab Project (*.FAL *.fal);;All files (*)");
constexpr int kRoleComponentId = Qt::UserRole + 1;
constexpr int kRoleVersionId = Qt::UserRole + 2;
constexpr int kRoleScope = Qt::UserRole + 3;
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

    buildMenus();
    buildDocks();
    buildStatusBar();
    subscribeToEvents();

    connect(m_view, &canvas::A3CanvasView::cursorMovedMm, this, &MainWindow::onCursorMoved);
    connect(m_viewport, &canvas::ViewportController::zoomChanged, this, &MainWindow::onZoomChanged);

    onRefreshCatalog();
    updateWindowTitle();
    updateActionStates();
}

MainWindow::~MainWindow() = default;

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
    m_actPlace = catalogMenu->addAction(tr("&Place Selected Component"), QKeySequence(Qt::Key_Insert), this,
                                        &MainWindow::onPlaceSelectedComponent);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About ArduLab"), this, [this] {
        QMessageBox::about(this, tr("About ArduLab"),
                           tr("<b>ArduLab %1</b><br/>Offline-first Electronic Engineering Platform.<br/>"
                              "Clean foundation: Core · Project · A3 Canvas · Component Engine.")
                               .arg(m_deps.applicationVersion));
    });

    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName(QStringLiteral("MainToolBar"));
    toolbar->setMovable(false);
    toolbar->addAction(actNew);
    toolbar->addAction(actOpen);
    toolbar->addAction(m_actSave);
    toolbar->addSeparator();
    toolbar->addAction(m_actPlace);
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
    m_statusCursor->setMinimumWidth(180);
    m_statusZoom = new QLabel(tr("Zoom: 100%"), this);
    m_statusZoom->setMinimumWidth(100);
    m_statusCatalog = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusCatalog);
    statusBar()->addPermanentWidget(m_statusZoom);
    statusBar()->addPermanentWidget(m_statusCursor);
    statusBar()->showMessage(tr("Ready"), 3000);
}

void MainWindow::subscribeToEvents()
{
    if (!m_deps.eventBus) {
        return;
    }
    m_subscriptions.push_back(m_deps.eventBus->subscribe<project::ProjectOpenedEvent>([this](const project::ProjectOpenedEvent&) {
        rebuildSceneFromProject();
        updateWindowTitle();
        updateActionStates();
    }));
    m_subscriptions.push_back(m_deps.eventBus->subscribe<project::ProjectSavedEvent>([this](const project::ProjectSavedEvent& e) {
        statusBar()->showMessage(tr("Saved %1").arg(e.filePath), 4000);
        updateWindowTitle();
    }));
    m_subscriptions.push_back(m_deps.eventBus->subscribe<project::ProjectClosedEvent>([this](const project::ProjectClosedEvent&) {
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
    m_deps.projectService->close();
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

    project::ComponentInstance instance;
    const QString prefix = snapshot.value()->component().categoryId.value() == QLatin1String("PASSIVE")
        ? QStringLiteral("R") : QStringLiteral("U");
    instance.instanceId = core::InstanceId(prefix + QString::number(proj->instances().size() + 1));
    instance.libraryId = componentId.value();
    instance.displayName = snapshot.value()->component().name;
    // Default placement: sheet centre + offset per instance, snapped to 1 mm grid.
    const double offset = 15.0 * static_cast<double>(proj->instances().size());
    instance.position = canvas::CoordinateSystem::snapToGrid(core::PointMm(210.0 + offset, 148.5 + offset), 1.0);
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

    if (!proj->addInstance(instance)) {
        reportError(core::Error(core::ErrorCode::AlreadyExists, tr("instance id already exists"), instance.instanceId.value()));
        return;
    }

    auto* gfx = new ComponentGraphicsItem(snapshot.value(), instance.instanceId, m_scene->coordinateSystem());
    gfx->setPositionMm(instance.position);
    m_scene->addItem(gfx);
    m_items.push_back(gfx);
    updateWindowTitle();
    updateActionStates();
    statusBar()->showMessage(tr("Placed %1 (%2)").arg(instance.instanceId.value(), versionId.value()), 3000);
}

// ---------------------------------------------------------------------------
// Scene projection
// ---------------------------------------------------------------------------

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
        if (instance.referenceState != project::ReferenceState::Resolved || !instance.catalogReference
            || m_deps.componentManager == nullptr) {
            continue; // unresolved/legacy instances are preserved in the model but not rendered yet
        }
        const project::CatalogReference& ref = *instance.catalogReference;
        const auto snapshot = m_deps.componentManager->load(ref.scope, ref.componentId, ref.versionId);
        if (!snapshot) {
            continue;
        }
        auto* gfx = new ComponentGraphicsItem(snapshot.value(), instance.instanceId, m_scene->coordinateSystem());
        gfx->setPositionMm(instance.position);
        m_scene->addItem(gfx);
        m_items.push_back(gfx);
    }
    m_view->fitSheet();
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
}

void MainWindow::reportError(const core::Error& error)
{
    statusBar()->showMessage(error.toString(), 8000);
    QMessageBox::warning(this, tr("ArduLab"), error.toString());
}

} // namespace ardulab::ui
