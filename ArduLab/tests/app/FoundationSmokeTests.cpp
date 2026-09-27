// End-to-end foundation smoke (Plan Step 15, adapted to Phase 1 scope):
// bootstrap composition → seeded catalog → default project → place a
// component through the UI slot path → exact-version reference recorded →
// save .FAL → reopen → resolved reference → renderer shows the item.
//
// Runs headless under the offscreen QPA and writes a PNG of the main window
// for visual verification.

#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "core/EventBus.h"
#include "project/ProjectService.h"
#include "ui/ComponentGraphicsItem.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QGraphicsItem>
#include <QListWidget>
#include <QMetaObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace ardulab;

namespace {

components::ComponentSnapshot seedResistor()
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
    v.contentHash = QStringLiteral("sha256:e2e");
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

} // namespace

class FoundationSmokeTests final : public QObject
{
    Q_OBJECT

private slots:
    void endToEndFoundationFlow()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        auto bus = core::EventBus::create();
        components::InMemoryComponentCatalog catalog;
        components::ComponentManager manager(catalog, bus);
        QVERIFY(manager.registerSnapshot(seedResistor()).isSuccess());
        project::ProjectService projectService(&manager, bus);

        ui::MainWindowDependencies deps;
        deps.eventBus = bus;
        deps.projectService = &projectService;
        deps.componentManager = &manager;
        deps.applicationVersion = QStringLiteral("0.1.3-e2e");
        ui::MainWindow window(std::move(deps));
        window.resize(1400, 900);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        // Catalog dock populated from IComponentManager::search
        auto* list = window.findChild<QListWidget*>(QStringLiteral("CatalogList"));
        QVERIFY(list != nullptr);
        QCOMPARE(list->count(), 1);
        QVERIFY(list->item(0)->text().contains(QStringLiteral("R1-A-0805-10K")));

        // Default project (as bootstrap does)
        QVERIFY(projectService.createNew(QStringLiteral("E2E")).isSuccess());
        QCoreApplication::processEvents();
        QVERIFY(window.windowTitle().startsWith(QStringLiteral("E2E")));

        // Place the selected catalog entry through the UI slot path
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(&window, "onPlaceSelectedComponent", Qt::DirectConnection));
        QCoreApplication::processEvents();

        const project::Project* proj = projectService.currentProject();
        QVERIFY(proj != nullptr);
        QCOMPARE(proj->instances().size(), std::size_t{1});
        const project::ComponentInstance& inst = proj->instances().front();
        QCOMPARE(inst.instanceId.value(), QStringLiteral("R1"));
        QVERIFY(inst.catalogReference.has_value());
        QCOMPARE(inst.catalogReference->versionId.value(), QStringLiteral("R1-A-0805-10K@1.0.0"));
        QCOMPARE(inst.catalogReference->contentHash, QStringLiteral("sha256:e2e"));
        QCOMPARE(inst.referenceState, project::ReferenceState::Resolved);
        QVERIFY(proj->isDirty());
        QVERIFY(window.windowTitle().startsWith(QStringLiteral("E2E*")));

        // Renderer shows exactly one ComponentGraphicsItem at the placement
        int rendered = 0;
        for (QGraphicsItem* gi : window.canvasView()->canvasScene()->items()) {
            if (auto* item = qgraphicsitem_cast<ui::ComponentGraphicsItem*>(gi)) {
                ++rendered;
                QVERIFY(item->positionMm() == inst.position);
                QCOMPARE(item->snapshot().pins().size(), std::size_t{2});
            }
        }
        QCOMPARE(rendered, 1);

        // Zoom/pan the view; model coordinates must not move
        window.canvasView()->viewportController()->setZoom(3.0);
        window.canvasView()->viewportController()->panBy(QPointF(-500.0, 200.0));
        QCoreApplication::processEvents();
        QVERIFY(projectService.currentProject()->instances().front().position == inst.position);

        // Save .FAL through the service (dialog-free path), then reopen
        const QString path = dir.filePath(QStringLiteral("E2E.FAL"));
        QVERIFY(projectService.saveAs(path).isSuccess());
        QVERIFY(!projectService.currentProject()->isDirty());
        QCoreApplication::processEvents();
        QVERIFY(!window.windowTitle().contains(QLatin1Char('*')));

        projectService.close();
        QCoreApplication::processEvents();
        rendered = 0;
        for (QGraphicsItem* gi : window.canvasView()->canvasScene()->items()) {
            if (qgraphicsitem_cast<ui::ComponentGraphicsItem*>(gi)) ++rendered;
        }
        QCOMPARE(rendered, 0); // scene cleared on ProjectClosed

        const auto reopened = projectService.open(path);
        QVERIFY2(reopened.isSuccess(), qPrintable(reopened.error().toString()));
        QCOMPARE(reopened.value().resolvedReferences, std::size_t{1});
        QCoreApplication::processEvents();
        rendered = 0;
        for (QGraphicsItem* gi : window.canvasView()->canvasScene()->items()) {
            if (qgraphicsitem_cast<ui::ComponentGraphicsItem*>(gi)) ++rendered;
        }
        QCOMPARE(rendered, 1); // rebuilt from the reopened project

        // Visual artefact for manual review
        const QString shot = QDir(QStringLiteral(ARDULAB_SMOKE_OUTPUT_DIR)).filePath(QStringLiteral("ardulab-foundation-smoke.png"));
        QDir().mkpath(QStringLiteral(ARDULAB_SMOKE_OUTPUT_DIR));
        window.canvasView()->fitSheet();
        QCoreApplication::processEvents();
        QVERIFY(window.grab().save(shot));
        qInfo() << "screenshot:" << shot;
    }
};

QTEST_MAIN(FoundationSmokeTests)
#include "FoundationSmokeTests.moc"
