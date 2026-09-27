#include "app/ApplicationBootstrap.h"

#include "components/ComponentManager.h"
#include "core/EventBus.h"
#include "database/CatalogDatabase.h"
#include "database/CatalogPaths.h"
#include "database/SchemaMigrationRegistry.h"
#include "database/SchemaMigrator.h"
#include "database/SqliteComponentCatalog.h"
#include "database/import/ComponentJsonImporter.h"
#include "project/ProjectService.h"
#include "ui/MainWindow.h"

#include <QCoreApplication>
#include <QMessageBox>

namespace ardulab::app {

namespace {

// Bundled canonical example imported on first run so the catalog is usable
// immediately; re-importing is idempotent (skipped once persisted).
const QString kSeedResource = QStringLiteral(":/ardulab/examples/Resistor.schema-1.0.json");

} // namespace

struct ApplicationBootstrap::Impl
{
    // Construction order == dependency order; destruction is the reverse, so
    // the SQLite connection closes only after every consumer is gone.
    std::shared_ptr<core::EventBus> eventBus;
    std::unique_ptr<database::CatalogDatabase> catalogDb;
    std::unique_ptr<database::SqliteComponentCatalog> catalog;
    std::unique_ptr<components::ComponentManager> componentManager;
    std::unique_ptr<database::ComponentJsonImporter> importer;
    std::unique_ptr<project::ProjectService> projectService;
    std::unique_ptr<ui::MainWindow> window;
};

ApplicationBootstrap::ApplicationBootstrap()
    : m_impl(std::make_unique<Impl>())
{
}

ApplicationBootstrap::~ApplicationBootstrap() = default;

int ApplicationBootstrap::start()
{
    Impl& impl = *m_impl;

    // 1. Core
    impl.eventBus = core::EventBus::create();

    // 2. Per-user SQLite catalog: resolve path, open, migrate.
    const auto dbFile = database::CatalogPaths::defaultDatabaseFile();
    if (!dbFile) {
        QMessageBox::critical(nullptr, QStringLiteral("ArduLab startup"),
                              QStringLiteral("Catalog location error: %1").arg(dbFile.error().toString()));
        return 2;
    }
    impl.catalogDb = std::make_unique<database::CatalogDatabase>(dbFile.value());
    if (const auto opened = impl.catalogDb->open(); !opened) {
        QMessageBox::critical(nullptr, QStringLiteral("ArduLab startup"),
                              QStringLiteral("Could not open catalog: %1").arg(opened.error().toString()));
        return 2;
    }

    const database::SchemaMigrationRegistry registry = database::SchemaMigrationRegistry::foundation();
    database::SchemaMigrator migrator(*impl.catalogDb, registry, QCoreApplication::applicationVersion());
    if (const auto migrated = migrator.migrate(); !migrated) {
        QMessageBox::critical(nullptr, QStringLiteral("ArduLab startup"),
                              QStringLiteral("Catalog migration failed: %1").arg(migrated.error().toString()));
        return 2;
    }

    // 3. Catalog boundary + Component Manager + JSON importer.
    impl.catalog = std::make_unique<database::SqliteComponentCatalog>(*impl.catalogDb);
    impl.componentManager = std::make_unique<components::ComponentManager>(*impl.catalog, impl.eventBus);
    impl.importer = std::make_unique<database::ComponentJsonImporter>(*impl.componentManager);

    // Seed the bundled canonical example through the real import pipeline
    // (idempotent: skipped once it is already persisted).
    if (const auto seeded = impl.importer->importFromFile(kSeedResource, components::CatalogScope::User); !seeded) {
        // A seed failure is non-fatal; the app still runs with an empty catalog.
        qWarning("ArduLab: could not seed example component: %s", qUtf8Printable(seeded.error().toString()));
    }

    // 4. Project Service
    impl.projectService = std::make_unique<project::ProjectService>(impl.componentManager.get(), impl.eventBus);

    // 5. UI shell
    ui::MainWindowDependencies deps;
    deps.eventBus = impl.eventBus;
    deps.projectService = impl.projectService.get();
    deps.componentManager = impl.componentManager.get();
    deps.componentImporter = impl.importer.get();
    deps.applicationVersion = QCoreApplication::applicationVersion();
    impl.window = std::make_unique<ui::MainWindow>(std::move(deps));
    impl.window->show();

    // 6. Default project so the A3 sheet is immediately usable.
    if (const auto created = impl.projectService->createNew(QStringLiteral("Untitled Project")); !created) {
        impl.window->reportError(created.error());
    }
    return 0;
}

} // namespace ardulab::app
