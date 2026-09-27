// Cursor snap readout (MVP completion). Proves the status-bar readout stays
// correct in millimeters after zoom, pan and rotation, that nearest-pin
// detection uses a screen-pixel tolerance, and that hovering a pin forms no
// electrical connection (geometric assistance only).

#include "canvas/CoordinateSystem.h"
#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "core/EventBus.h"
#include "project/ProjectService.h"
#include "ui/ComponentGraphicsItem.h"
#include "ui/EditingFixture.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QMetaObject>
#include <QMouseEvent>
#include <QtTest/QtTest>

#include <memory>

using namespace ardulab;
using ardulab::testing::firstItem;
using ardulab::testing::seedResistor;

class CursorSnapReadoutTests final : public QObject
{
    Q_OBJECT

private:
    std::shared_ptr<core::EventBus> m_bus;
    std::unique_ptr<components::InMemoryComponentCatalog> m_catalog;
    std::unique_ptr<components::ComponentManager> m_manager;
    std::unique_ptr<project::ProjectService> m_service;
    std::unique_ptr<ui::MainWindow> m_window;
    QLabel* m_snap = nullptr;
    QLabel* m_anchor = nullptr;

    void hover(QPoint viewportPos)
    {
        QWidget* vp = m_window->canvasView()->viewport();
        QMouseEvent move(QEvent::MouseMove, QPointF(viewportPos), vp->mapToGlobal(QPointF(viewportPos)),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(vp, &move);
        QCoreApplication::processEvents();
    }

    /// Viewport pixel of the given pin's anchor, through the live view transform.
    [[nodiscard]] QPoint anchorPixel(const QString& pinNumber) const
    {
        const ui::ComponentGraphicsItem* item = firstItem(*m_window);
        for (const components::Pin& pin : item->snapshot().pins()) {
            if (pin.pinNumber == pinNumber) {
                return m_window->canvasView()->mapFromScene(item->anchorScenePos(pin.anchor()));
            }
        }
        return QPoint(-1000, -1000);
    }

    /// Bring the placed item to the middle of the viewport at the current zoom
    /// (viewport = zoom * (scene + pan)), so anchor pixels are always visible.
    void centerOnItem()
    {
        const ui::ComponentGraphicsItem* item = firstItem(*m_window);
        const double zoom = m_window->canvasView()->viewportController()->zoom();
        const QRect rect = m_window->canvasView()->viewport()->rect();
        const QPointF scenePos = item->scenePos();
        m_window->canvasView()->viewportController()->setPanOffset(
            QPointF(rect.center().x() / zoom - scenePos.x(), rect.center().y() / zoom - scenePos.y()));
        QCoreApplication::processEvents();
    }

    [[nodiscard]] QString expectedSnapText(QPoint viewportPos) const    {
        const core::PointMm mm = m_window->canvasView()->mmAt(viewportPos);
        const double grid = m_service->currentProject()->canvas().gridMm;
        const core::PointMm snapped = canvas::CoordinateSystem::snapToGrid(mm, grid);
        return QStringLiteral("Snap: %1, %2 mm").arg(snapped.x, 0, 'f', 2).arg(snapped.y, 0, 'f', 2);
    }

private slots:
    void initTestCase()
    {
        m_bus = core::EventBus::create();
        m_catalog = std::make_unique<components::InMemoryComponentCatalog>();
        m_manager = std::make_unique<components::ComponentManager>(*m_catalog, m_bus);
        QVERIFY(m_manager->registerSnapshot(seedResistor()).isSuccess());
        m_service = std::make_unique<project::ProjectService>(m_manager.get(), m_bus);

        ui::MainWindowDependencies deps;
        deps.eventBus = m_bus;
        deps.projectService = m_service.get();
        deps.componentManager = m_manager.get();
        deps.applicationVersion = QStringLiteral("0.1.3-snap");
        m_window = std::make_unique<ui::MainWindow>(std::move(deps));
        m_window->resize(1200, 800);
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));

        QVERIFY(m_service->createNew(QStringLiteral("SNAP")).isSuccess());
        auto* list = m_window->findChild<QListWidget*>(QStringLiteral("CatalogList"));
        QVERIFY(list && list->count() == 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(m_window.get(), "onPlaceSelectedComponent", Qt::DirectConnection));
        m_window->canvasView()->fitSheet();
        QCoreApplication::processEvents();

        m_snap = m_window->findChild<QLabel*>(QStringLiteral("StatusSnap"));
        m_anchor = m_window->findChild<QLabel*>(QStringLiteral("StatusAnchor"));
        QVERIFY(m_snap && m_anchor);
        QVERIFY(firstItem(*m_window) != nullptr);
    }

    void gridSnapReadoutIsMillimetersAtEveryZoomLevel()
    {
        const QList<double> zooms{0.3, 1.0, 4.0, 12.0};
        for (const double zoom : zooms) {
            m_window->canvasView()->viewportController()->setZoom(zoom);
            QCoreApplication::processEvents();
            for (const QPoint probe : {QPoint(120, 90), QPoint(400, 300), QPoint(733, 517)}) {
                hover(probe);
                QCOMPARE(m_snap->text(), expectedSnapText(probe));
                // The readout is quantised to the millimeter grid.
                const QStringList parts = m_snap->text().mid(6).split(QStringLiteral(", "));
                QCOMPARE(parts.size(), 2);
                const double x = parts.at(0).toDouble();
                const double grid = m_service->currentProject()->canvas().gridMm;
                QVERIFY(std::abs(x / grid - std::round(x / grid)) < 1e-6);
            }
        }
    }

    void snapReadoutSurvivesPan()
    {
        m_window->canvasView()->viewportController()->setZoom(2.0);
        m_window->canvasView()->viewportController()->setPanOffset(QPointF(-150.0, -90.0));
        QCoreApplication::processEvents();
        const QPoint probe(500, 400);
        hover(probe);
        QCOMPARE(m_snap->text(), expectedSnapText(probe));

        m_window->canvasView()->viewportController()->panBy(QPointF(60.0, 40.0));
        QCoreApplication::processEvents();
        hover(probe);
        // Same pixel, different sheet coordinate after the pan.
        QCOMPARE(m_snap->text(), expectedSnapText(probe));
    }

    void nearestPinAnchorIsReportedWithinScreenTolerance()
    {
        m_window->canvasView()->viewportController()->setZoom(6.0);
        centerOnItem();

        const QPoint pin1 = anchorPixel(QStringLiteral("1"));
        QVERIFY2(m_window->canvasView()->viewport()->rect().contains(pin1),
                 "pin 1 must be inside the centred viewport");
        hover(pin1);
        QVERIFY2(m_anchor->text().contains(QStringLiteral("Pin ")), qPrintable(m_anchor->text()));
        QVERIFY(m_anchor->text().contains(QStringLiteral("·1")));
        QVERIFY(m_anchor->text().contains(QStringLiteral("mm")));

        // Far outside the pixel tolerance → no anchor reported, and hovering
        // never created a connection: the project holds instances only.
        hover(pin1 + QPoint(80, 80));
        QCOMPARE(m_anchor->text(), QStringLiteral("Pin: —"));
        QCOMPARE(m_service->currentProject()->instances().size(), std::size_t{1});
        QVERIFY(m_service->currentProject()->preservedSections().isEmpty());
    }

    void anchorReadoutFollowsRotationAndZoom()
    {
        m_window->canvasView()->viewportController()->setZoom(8.0);
        centerOnItem();

        ui::ComponentGraphicsItem* item = firstItem(*m_window);
        item->setSelected(true);
        const QPoint beforeRotation = anchorPixel(QStringLiteral("2"));
        QVERIFY(QMetaObject::invokeMethod(m_window.get(), "onRotateSelected", Qt::DirectConnection));
        QCoreApplication::processEvents();
        QCOMPARE(firstItem(*m_window)->rotationDegrees(), 90.0);

        // After rotation the anchor moved; the readout must follow the item's
        // own transform, not the unrotated pin coordinates.
        const QPoint rotatedPin2 = anchorPixel(QStringLiteral("2"));
        QVERIFY(rotatedPin2 != beforeRotation);
        hover(beforeRotation);
        QCOMPARE(m_anchor->text(), QStringLiteral("Pin: —")); // stale position no longer matches
        QVERIFY(m_window->canvasView()->viewport()->rect().contains(rotatedPin2));
        hover(rotatedPin2);
        QVERIFY2(m_anchor->text().contains(QStringLiteral("·2")), qPrintable(m_anchor->text()));

        // Zooming out keeps the anchor detectable at its new pixel position.
        m_window->canvasView()->viewportController()->setZoom(2.0);
        centerOnItem();
        const QPoint zoomedOut = anchorPixel(QStringLiteral("2"));
        QVERIFY(m_window->canvasView()->viewport()->rect().contains(zoomedOut));
        hover(zoomedOut);
        QVERIFY2(m_anchor->text().contains(QStringLiteral("·2")), qPrintable(m_anchor->text()));

        // Restore for independence of test order.
        QVERIFY(QMetaObject::invokeMethod(m_window.get(), "onRotateSelected", Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(m_window.get(), "onRotateSelected", Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(m_window.get(), "onRotateSelected", Qt::DirectConnection));
        QCoreApplication::processEvents();
        QCOMPARE(firstItem(*m_window)->rotationDegrees(), 0.0);
    }

    void cleanupTestCase()
    {
        m_window.reset();
        m_service.reset();
        m_manager.reset();
        m_catalog.reset();
    }
};

QTEST_MAIN(CursorSnapReadoutTests)
#include "CursorSnapReadoutTests.moc"
