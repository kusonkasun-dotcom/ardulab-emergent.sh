// .FAL backward-compatibility tests (Plan §8.3, §10.3; ADR §11.5).

#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "project/FalSerializer.h"
#include "project/ProjectService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace ardulab::core;
using namespace ardulab::components;
using namespace ardulab::project;

namespace {

QByteArray readFixture(const char* name)
{
    QFile f(QStringLiteral(ARDULAB_TEST_FIXTURES_DIR) + QLatin1Char('/') + QLatin1String(name));
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    return f.readAll();
}

ComponentSnapshot makeEsp32(CatalogScope scope)
{
    Component c;
    c.componentId = ComponentId(QStringLiteral("MMCU1-A-ESP32-WROOM32"));
    c.name = QStringLiteral("ESP32-WROOM-32");
    c.categoryId = CategoryId(QStringLiteral("MCU_MODULE"));
    c.scope = scope;
    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = QStringLiteral("1.0.0");
    v.versionId = makeComponentVersionId(c.componentId, v.version);
    Pin p;
    p.pinNumber = QStringLiteral("1");
    return ComponentSnapshot(c, v, Package{}, {p}, {});
}

} // namespace

class FalCompatibilityTests final : public QObject
{
    Q_OBJECT

private slots:
    void legacyMinimalShapeOpens()
    {
        const auto r = FalSerializer::read(readFixture("legacy-empty-project.FAL"));
        QVERIFY2(r.isSuccess(), qPrintable(r.error().toString()));
        const Project& p = r.value().project;
        QCOMPARE(p.metadata().name, QStringLiteral("Example Project"));
        QCOMPARE(p.metadata().version, QStringLiteral("0.1"));
        QVERIFY(p.metadata().projectId.isValid()); // generated
        QVERIFY(p.instances().empty());
        QVERIFY(r.value().report.isLegacyShape);
        QVERIFY(r.value().report.detectedFormatVersion.isEmpty());
    }

    void missingSectionsDefaultSafely()
    {
        const auto r = FalSerializer::read(readFixture("legacy-empty-project.FAL"));
        QVERIFY(r.isSuccess());
        const Project& p = r.value().project;
        QCOMPARE(p.canvas().widthMm, 420.0);
        QCOMPARE(p.canvas().heightMm, 297.0);
        QCOMPARE(p.canvas().gridMm, 1.0);
        QVERIFY(p.catalogVersion().isEmpty());
        QVERIFY(p.preservedSections().isEmpty());
    }

    void unknownFieldsAndInstancesArePreservedThroughRoundTrip()
    {
        const auto r = FalSerializer::read(readFixture("legacy-with-instances.FAL"));
        QVERIFY2(r.isSuccess(), qPrintable(r.error().toString()));
        const Project& p = r.value().project;
        QCOMPARE(p.instances().size(), std::size_t{2});
        QCOMPARE(r.value().report.preservedUnknownKeys, QStringList{QStringLiteral("future_section")});

        const ComponentInstance* u1 = p.findInstance(InstanceId(QStringLiteral("U1")));
        QVERIFY(u1 != nullptr);
        QCOMPARE(u1->position, PointMm(100.5, 80.25));
        QVERIFY(u1->extra.contains(QStringLiteral("legacy_visual")));
        QVERIFY(u1->catalogReference.has_value());
        QCOMPARE(u1->catalogReference->scope, CatalogScope::Official);
        QCOMPARE(u1->catalogReference->versionId.value(), QStringLiteral("MMCU1-A-ESP32-WROOM32@1.0.0"));
        QVERIFY(u1->catalogReference->fallback.has_value());
        QCOMPARE(u1->catalogReference->fallback->pinNumbers.size(), 2);

        const QByteArray written = FalSerializer::write(p);
        const QJsonObject root = QJsonDocument::fromJson(written).object();
        QCOMPARE(root.value(QStringLiteral("fal_format_version")).toString(), kCurrentFalFormatVersion);
        QCOMPARE(root.value(QStringLiteral("future_section")).toObject().value(QStringLiteral("keep")).toBool(), true);
        QCOMPARE(root.value(QStringLiteral("library")).toObject().value(QStringLiteral("component_references")).toArray().size(), 1);

        // Re-read the written document: semantic equality.
        const auto again = FalSerializer::read(written);
        QVERIFY(again.isSuccess());
        const Project& p2 = again.value().project;
        QVERIFY(!again.value().report.isLegacyShape);
        QCOMPARE(p2.metadata().projectId, p.metadata().projectId);
        QCOMPARE(p2.metadata().name, p.metadata().name);
        QCOMPARE(p2.instances().size(), std::size_t{2});
        const ComponentInstance* u1b = p2.findInstance(InstanceId(QStringLiteral("U1")));
        QVERIFY(u1b != nullptr);
        QCOMPARE(u1b->position, u1->position);
        QCOMPARE(u1b->libraryId, QStringLiteral("MMCU1-A-ESP32-WROOM32"));
        QCOMPARE(u1b->extra.value(QStringLiteral("legacy_visual")).toObject().value(QStringLiteral("scale")).toInt(), 10);
        QCOMPARE(u1b->catalogReference->contentHash, QStringLiteral("sha256:abc"));
        const ComponentInstance* r1 = p2.findInstance(InstanceId(QStringLiteral("R1")));
        QVERIFY(r1 != nullptr);
        QVERIFY(!r1->catalogReference.has_value());
        QCOMPARE(r1->referenceState, ReferenceState::Unreferenced);
    }

    void unsupportedMajorFormatVersionIsRejected()
    {
        const QByteArray doc = R"({"fal_format_version":"2.0","project":{"name":"x","version":"1"},"components":[]})";
        const auto r = FalSerializer::read(doc);
        QVERIFY(r.isFailure());
        QCOMPARE(r.error().code(), ErrorCode::UnsupportedFormatVersion);
        QVERIFY(FalSerializer::canRead(QStringLiteral("1.7")));
        QVERIFY(!FalSerializer::canRead(QStringLiteral("2.0")));
    }

    void malformedJsonIsAParseFailureNotACrash()
    {
        const auto r = FalSerializer::read(QByteArrayLiteral("{ not json"));
        QVERIFY(r.isFailure());
        QCOMPARE(r.error().code(), ErrorCode::ParseFailure);
        const auto arr = FalSerializer::read(QByteArrayLiteral("[]"));
        QCOMPARE(arr.error().code(), ErrorCode::ParseFailure);
    }

    void serviceOpenResolvesExactVersionOnlyAndNeverSubstitutes()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("legacy.FAL"));
        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(readFixture("legacy-with-instances.FAL"));
        }

        // Catalog has the component only in USER scope at version 2.0.0 — neither matches.
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        {
            auto snap = makeEsp32(CatalogScope::User);
            QVERIFY(manager.registerSnapshot(snap).isSuccess());
        }
        ProjectService service(&manager, nullptr);
        const auto opened = service.open(path);
        QVERIFY2(opened.isSuccess(), qPrintable(opened.error().toString()));
        QCOMPARE(opened.value().unresolvedReferences, std::size_t{1});
        QCOMPARE(opened.value().resolvedReferences, std::size_t{0});
        QCOMPARE(service.currentProject()->unresolvedReferenceCount(), std::size_t{1});
        QVERIFY(std::any_of(opened.value().report.warnings.begin(), opened.value().report.warnings.end(),
                            [](const QString& w) { return w.contains(QStringLiteral("no substitution")); }));
        service.close();

        // Now register the exact OFFICIAL 1.0.0 version -> resolves.
        QVERIFY(manager.registerSnapshot(makeEsp32(CatalogScope::Official)).isSuccess());
        const auto opened2 = service.open(path);
        QVERIFY(opened2.isSuccess());
        QCOMPARE(opened2.value().resolvedReferences, std::size_t{1});
        QCOMPARE(opened2.value().unresolvedReferences, std::size_t{0});
    }

    void serviceLifecycleAndAtomicSave()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        auto bus = EventBus::create();
        QStringList events;
        Subscription s1 = bus->subscribe<ProjectOpenedEvent>([&](const ProjectOpenedEvent& e) { events << QStringLiteral("opened:") + e.projectName; });
        Subscription s2 = bus->subscribe<ProjectSavedEvent>([&](const ProjectSavedEvent&) { events << QStringLiteral("saved"); });
        Subscription s3 = bus->subscribe<ProjectClosedEvent>([&](const ProjectClosedEvent&) { events << QStringLiteral("closed"); });

        ProjectService service(nullptr, bus);
        QVERIFY(!service.hasOpenProject());
        QCOMPARE(service.save().error().code(), ErrorCode::ProjectNotOpen);

        QVERIFY(service.createNew(QStringLiteral("Bench")).isSuccess());
        QCOMPARE(service.createNew(QStringLiteral("Again")).error().code(), ErrorCode::ProjectAlreadyOpen);
        QCOMPARE(service.save().error().code(), ErrorCode::InvalidState); // no path yet

        ComponentInstance inst;
        inst.instanceId = InstanceId(QStringLiteral("R1"));
        inst.position = PointMm(10.0, 20.0);
        QVERIFY(service.currentProject()->addInstance(inst));
        QVERIFY(service.currentProject()->isDirty());

        const QString path = dir.filePath(QStringLiteral("sub/Bench.FAL"));
        QVERIFY2(service.saveAs(path).isSuccess(), "saveAs must create directories and write atomically");
        QVERIFY(QFile::exists(path));
        QVERIFY(!service.currentProject()->isDirty());
        QVERIFY(service.save().isSuccess());
        service.close();
        QVERIFY(!service.hasOpenProject());

        const auto reopened = service.open(path);
        QVERIFY(reopened.isSuccess());
        QCOMPARE(service.currentProject()->metadata().name, QStringLiteral("Bench"));
        QCOMPARE(service.currentProject()->instances().size(), std::size_t{1});
        QCOMPARE(service.currentProject()->instances().front().position, PointMm(10.0, 20.0));

        QCOMPARE(events, (QStringList{QStringLiteral("opened:Bench"), QStringLiteral("saved"), QStringLiteral("saved"),
                                      QStringLiteral("closed"), QStringLiteral("opened:Bench")}));

        QCOMPARE(service.open(QStringLiteral("/nonexistent/x.FAL")).error().code(), ErrorCode::ProjectAlreadyOpen);
        service.close();
        QCOMPARE(service.open(dir.filePath(QStringLiteral("missing.FAL"))).error().code(), ErrorCode::IoFailure);
    }
};

QTEST_GUILESS_MAIN(FalCompatibilityTests)
#include "FalCompatibilityTests.moc"
