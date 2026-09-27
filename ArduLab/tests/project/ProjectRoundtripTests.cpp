// Project .FAL roundtrip regression (Plan Step 11, ADR §11.5 / §8.3).
//
// Proves that save → read preserves instance id, reference, value (libraryId),
// position, rotation, and the embedded component-definition fallback snapshot;
// and that when the catalog is unavailable the project opens against the
// stored snapshot as Unresolved, without silently substituting a definition.

#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "core/EventBus.h"
#include "project/FalSerializer.h"
#include "project/Project.h"
#include "project/ProjectService.h"

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using namespace ardulab::core;
using namespace ardulab::project;
using ardulab::components::CatalogScope;

namespace {

Project makeProjectWithInstance()
{
    ProjectMetadata meta;
    meta.projectId = ProjectId(QStringLiteral("proj-roundtrip-1"));
    meta.name = QStringLiteral("Roundtrip Project");
    meta.version = QStringLiteral("0.1");
    meta.falFormatVersion = kCurrentFalFormatVersion;
    Project project(std::move(meta));

    ComponentInstance inst;
    inst.instanceId = InstanceId(QStringLiteral("R7"));
    inst.libraryId = QStringLiteral("R1-A-0805-10K");  // "value"/library reference
    inst.displayName = QStringLiteral("Resistor 10k 0805");
    inst.position = PointMm(123.5, 67.25);
    inst.rotationDegrees = 90.0;

    CatalogReference ref;
    ref.scope = CatalogScope::User;
    ref.componentId = ComponentId(QStringLiteral("R1-A-0805-10K"));
    ref.versionId = ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0"));
    ref.contentHash = QStringLiteral("sha256:deadbeef");
    CatalogReference::FallbackSnapshot fb;
    fb.name = QStringLiteral("Resistor 10k 0805");
    fb.referencePrefix = QStringLiteral("R");
    fb.packageId = PackageId(QStringLiteral("0805"));
    fb.pinNumbers << QStringLiteral("1") << QStringLiteral("2");
    ref.fallback = std::move(fb);
    inst.catalogReference = std::move(ref);
    inst.referenceState = ReferenceState::Resolved;

    (void)project.addInstance(inst);
    return project;
}

/// Same component id as the stored reference, but a different version — the
/// "catalog changed under the project" case.
ardulab::components::ComponentSnapshot snapshotForVersion(const QString& version)
{
    using namespace ardulab::components;
    Component c;
    c.componentId = ComponentId(QStringLiteral("R1-A-0805-10K"));
    c.name = QStringLiteral("Resistor 10k 0805");
    c.categoryId = CategoryId(QStringLiteral("PASSIVE"));
    c.version = version;
    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = version;
    v.versionId = ardulab::core::makeComponentVersionId(c.componentId, version);
    v.contentHash = QStringLiteral("sha256:other");
    Package p;
    p.packageId = PackageId(QStringLiteral("0805"));
    p.bodySize = SizeMm(2.0, 1.25);
    p.pinCount = 2;
    Pin p1;
    p1.pinNumber = QStringLiteral("1");
    p1.position = PointMm(-1.0, 0.0);
    p1.anchorPosition = p1.position;
    Pin p2 = p1;
    p2.pinNumber = QStringLiteral("2");
    p2.position = PointMm(1.0, 0.0);
    p2.anchorPosition = p2.position;
    return ComponentSnapshot(c, v, p, {p1, p2}, {});
}

} // namespace

class ProjectRoundtripTests final : public QObject
{
    Q_OBJECT

private slots:
    void writeReadPreservesInstanceFields()
    {
        const Project original = makeProjectWithInstance();
        const QByteArray bytes = FalSerializer::write(original);

        const auto read = FalSerializer::read(bytes);
        QVERIFY2(read.isSuccess(), qPrintable(read.error().toString()));
        const Project& reopened = read.value().project;

        QCOMPARE(reopened.instances().size(), std::size_t{1});
        const ComponentInstance& inst = reopened.instances().front();

        QCOMPARE(inst.instanceId.value(), QStringLiteral("R7"));
        QCOMPARE(inst.libraryId, QStringLiteral("R1-A-0805-10K"));
        QCOMPARE(inst.displayName, QStringLiteral("Resistor 10k 0805"));
        QCOMPARE(inst.position.x, 123.5);
        QCOMPARE(inst.position.y, 67.25);
        QCOMPARE(inst.rotationDegrees, 90.0);

        QVERIFY(inst.catalogReference.has_value());
        const CatalogReference& ref = *inst.catalogReference;
        QCOMPARE(ref.scope, CatalogScope::User);
        QCOMPARE(ref.componentId.value(), QStringLiteral("R1-A-0805-10K"));
        QCOMPARE(ref.versionId.value(), QStringLiteral("R1-A-0805-10K@1.0.0"));
        QCOMPARE(ref.contentHash, QStringLiteral("sha256:deadbeef"));

        QVERIFY(ref.fallback.has_value());
        QCOMPARE(ref.fallback->name, QStringLiteral("Resistor 10k 0805"));
        QCOMPARE(ref.fallback->referencePrefix, QStringLiteral("R"));
        QCOMPARE(ref.fallback->packageId.value(), QStringLiteral("0805"));
        QCOMPARE(ref.fallback->pinNumbers, QStringList({QStringLiteral("1"), QStringLiteral("2")}));
    }

    void projectIdAndMetadataSurviveRoundtrip()
    {
        const Project original = makeProjectWithInstance();
        const auto read = FalSerializer::read(FalSerializer::write(original));
        QVERIFY(read.isSuccess());
        QCOMPARE(read.value().project.metadata().projectId.value(), QStringLiteral("proj-roundtrip-1"));
        QCOMPARE(read.value().project.metadata().name, QStringLiteral("Roundtrip Project"));
    }

    void unavailableCatalogOpensStoredSnapshotWithoutSubstitution()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = QDir(dir.path()).filePath(QStringLiteral("roundtrip.FAL"));
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(FalSerializer::write(makeProjectWithInstance()));
        }

        // No component manager ⇒ catalog is unavailable for resolution.
        ProjectService service(nullptr, nullptr);
        const auto opened = service.open(path);
        QVERIFY2(opened.isSuccess(), qPrintable(opened.error().toString()));
        QCOMPARE(opened.value().unresolvedReferences, std::size_t{1});

        const Project* project = service.currentProject();
        QVERIFY(project != nullptr);
        const ComponentInstance& inst = project->instances().front();
        // Opened read-only against the stored reference; nothing substituted.
        QCOMPARE(inst.referenceState, ReferenceState::Unresolved);
        QVERIFY(inst.catalogReference.has_value());
        QCOMPARE(inst.catalogReference->versionId.value(), QStringLiteral("R1-A-0805-10K@1.0.0"));
        QVERIFY(inst.catalogReference->fallback.has_value());
        QCOMPARE(inst.catalogReference->fallback->name, QStringLiteral("Resistor 10k 0805"));
        // Position/rotation still intact after the service open path.
        QCOMPARE(inst.position.x, 123.5);
        QCOMPARE(inst.rotationDegrees, 90.0);
    }
    void valueAndReferenceDesignatorSurviveRoundtrip()
    {
        Project original = makeProjectWithInstance();
        original.instances().front().value = QStringLiteral("10k 1%");

        const auto read = FalSerializer::read(FalSerializer::write(original));
        QVERIFY2(read.isSuccess(), qPrintable(read.error().toString()));
        const ComponentInstance& inst = read.value().project.instances().front();
        QCOMPARE(inst.instanceId.value(), QStringLiteral("R7")); // reference designator
        QCOMPARE(inst.value, QStringLiteral("10k 1%"));          // engineering value
    }

    void changedCatalogVersionOpensSnapshotWithoutSubstitution()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = QDir(dir.path()).filePath(QStringLiteral("changed-catalog.FAL"));
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(FalSerializer::write(makeProjectWithInstance()));
        }

        // The catalog now carries a *newer* version of the same component; the
        // exact version recorded in the file (1.0.0) is gone.
        auto bus = ardulab::core::EventBus::create();
        ardulab::components::InMemoryComponentCatalog catalog;
        ardulab::components::ComponentManager manager(catalog, bus);
        QVERIFY(manager.registerSnapshot(snapshotForVersion(QStringLiteral("2.0.0"))).isSuccess());

        ProjectService service(&manager, nullptr);
        const auto opened = service.open(path);
        QVERIFY2(opened.isSuccess(), qPrintable(opened.error().toString()));
        QCOMPARE(opened.value().unresolvedReferences, std::size_t{1});
        QCOMPARE(opened.value().resolvedReferences, std::size_t{0});

        const ComponentInstance& inst = service.currentProject()->instances().front();
        QCOMPARE(inst.referenceState, ReferenceState::Unresolved);
        // Still the version the project recorded — never swapped for 2.0.0.
        QCOMPARE(inst.catalogReference->versionId.value(), QStringLiteral("R1-A-0805-10K@1.0.0"));
        QVERIFY(inst.catalogReference->fallback.has_value());
        QCOMPARE(inst.catalogReference->fallback->pinNumbers.size(), 2);
        QVERIFY(!opened.value().report.warnings.isEmpty());
    }

    void corruptFileDoesNotReplaceTheActiveDocument()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString good = QDir(dir.path()).filePath(QStringLiteral("good.FAL"));
        const QString broken = QDir(dir.path()).filePath(QStringLiteral("broken.FAL"));
        {
            QFile file(good);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(FalSerializer::write(makeProjectWithInstance()));
        }
        {
            QFile file(broken);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(QByteArrayLiteral("{ \"project\": { \"name\": \"oops\" "));
        }

        ProjectService service(nullptr, nullptr);
        QVERIFY(service.open(good).isSuccess());
        const QString openPath = service.currentFilePath();

        const auto failed = service.open(broken);
        QVERIFY(!failed.isSuccess());
        QCOMPARE(failed.error().code(), ErrorCode::ParseFailure);

        // The previously opened document is still the active one.
        QVERIFY(service.hasOpenProject());
        QCOMPARE(service.currentFilePath(), openPath);
        QCOMPARE(service.currentProject()->metadata().name, QStringLiteral("Roundtrip Project"));
        QCOMPARE(service.currentProject()->instances().size(), std::size_t{1});
        QCOMPARE(service.currentProject()->instances().front().instanceId.value(), QStringLiteral("R7"));

        // A missing file is equally harmless to the open document.
        QCOMPARE(service.open(QDir(dir.path()).filePath(QStringLiteral("nope.FAL"))).error().code(),
                 ErrorCode::IoFailure);
        QVERIFY(service.hasOpenProject());
        QCOMPARE(service.currentProject()->instances().size(), std::size_t{1});
    }
};

QTEST_GUILESS_MAIN(ProjectRoundtripTests)
#include "ProjectRoundtripTests.moc"
