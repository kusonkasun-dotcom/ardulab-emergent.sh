#include "app/ApplicationBootstrap.h"

#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "core/EventBus.h"
#include "project/ProjectService.h"
#include "ui/MainWindow.h"

#include <QCoreApplication>
#include <QMessageBox>

namespace ardulab::app {

namespace {

// Foundation seed: the canonical R1-A-0805-10K example (mirrors
// resources/examples/Resistor.schema-1.0.json). Constructed in memory so the
// UI has a real snapshot to render before the JSON importer exists. The JSON
// import path replaces this seed in the import phase.
components::ComponentSnapshot canonicalResistorSeed()
{
    using namespace components;
    Component c;
    c.componentId = core::ComponentId(QStringLiteral("R1-A-0805-10K"));
    c.name = QStringLiteral("Resistor 10k 0805");
    c.categoryId = core::CategoryId(QStringLiteral("PASSIVE"));
    c.description = QStringLiteral("Generic 10 kOhm thick-film chip resistor, 0805, 1%");
    c.status = LifecycleStatus::Draft;
    c.scope = CatalogScope::User;
    c.version = QStringLiteral("1.0.0");
    c.componentSchemaVersion = kCurrentComponentSchemaVersion;

    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = c.version;
    v.versionId = core::makeComponentVersionId(c.componentId, c.version);
    v.componentSchemaVersion = kCurrentComponentSchemaVersion;
    v.contentHash = QStringLiteral("sha256:seed-r1-a-0805-10k-1.0.0");
    v.sourceType = SourceType::Manual;
    v.sourceReference = QStringLiteral(":/ardulab/examples/Resistor.schema-1.0.json");
    v.createdBy = QStringLiteral("bootstrap-seed");

    Package p;
    p.packageId = core::PackageId(QStringLiteral("0805"));
    p.packageType = QStringLiteral("0805");
    p.bodySize = core::SizeMm(2.0, 1.25);
    p.pinCount = 2;

    Pin p1;
    p1.pinNumber = QStringLiteral("1");
    p1.pinName = QStringLiteral("P1");
    p1.type = PinType::Passive;
    p1.direction = PinDirection::Passive;
    p1.side = PinSide::Left;
    p1.position = core::PointMm(-1.0, 0.0);
    p1.anchorPosition = core::PointMm(-1.0, 0.0);

    Pin p2 = p1;
    p2.pinNumber = QStringLiteral("2");
    p2.pinName = QStringLiteral("P2");
    p2.side = PinSide::Right;
    p2.position = core::PointMm(1.0, 0.0);
    p2.anchorPosition = core::PointMm(1.0, 0.0);

    std::vector<ComponentParameter> params{
        {QStringLiteral("resistance"), QStringLiteral("10000"), QStringLiteral("Ohm"), ParameterType::Real},
        {QStringLiteral("tolerance"), QStringLiteral("1"), QStringLiteral("%"), ParameterType::Real},
        {QStringLiteral("power"), QStringLiteral("0.125"), QStringLiteral("W"), ParameterType::Real},
    };
    return ComponentSnapshot(c, v, p, {p1, p2}, params);
}

} // namespace

struct ApplicationBootstrap::Impl
{
    // Construction order == dependency order; destruction is the reverse.
    std::shared_ptr<core::EventBus> eventBus;
    std::unique_ptr<components::InMemoryComponentCatalog> catalog;
    std::unique_ptr<components::ComponentManager> componentManager;
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

    // 2. Catalog boundary + Component Manager
    impl.catalog = std::make_unique<components::InMemoryComponentCatalog>();
    impl.componentManager = std::make_unique<components::ComponentManager>(*impl.catalog, impl.eventBus);
    if (const auto seeded = impl.componentManager->registerSnapshot(canonicalResistorSeed()); !seeded) {
        QMessageBox::critical(nullptr, QStringLiteral("ArduLab startup"),
                              QStringLiteral("Catalog seed failed: %1").arg(seeded.error().toString()));
        return 2;
    }

    // 3. Project Service
    impl.projectService = std::make_unique<project::ProjectService>(impl.componentManager.get(), impl.eventBus);

    // 4. UI shell
    ui::MainWindowDependencies deps;
    deps.eventBus = impl.eventBus;
    deps.projectService = impl.projectService.get();
    deps.componentManager = impl.componentManager.get();
    deps.applicationVersion = QCoreApplication::applicationVersion();
    impl.window = std::make_unique<ui::MainWindow>(std::move(deps));
    impl.window->show();

    // 5. Default project so the A3 sheet is immediately usable.
    if (const auto created = impl.projectService->createNew(QStringLiteral("Untitled Project")); !created) {
        impl.window->reportError(created.error());
    }
    return 0;
}

} // namespace ardulab::app
