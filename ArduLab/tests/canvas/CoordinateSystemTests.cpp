// Canvas Engine tests (Plan §10.1, §10.4): mm ↔ scene conversion, A3
// dimensions, and zoom/pan independence from engineering coordinates.

#include "canvas/A3CanvasScene.h"
#include "canvas/A3CanvasView.h"
#include "canvas/CoordinateSystem.h"
#include "canvas/ViewportController.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

#include <cmath>

using namespace ardulab::core;
using namespace ardulab::canvas;

class CoordinateSystemTests final : public QObject
{
    Q_OBJECT

private slots:
    void a3SheetIsExactly420by297Millimeters()
    {
        const CoordinateSystem cs;
        QCOMPARE(cs.sheetSizeMm().width, 420.0);
        QCOMPARE(cs.sheetSizeMm().height, 297.0);
        QCOMPARE(cs.sheetSceneRect(), QRectF(0.0, 0.0, 4200.0, 2970.0));
    }

    void millimetersToSceneAndBackAreLossless()
    {
        const CoordinateSystem cs(10.0);
        const PointMm p(123.456, 78.9);
        const QPointF s = cs.toScene(p);
        QCOMPARE(s, QPointF(1234.56, 789.0));
        QVERIFY(cs.toMm(s) == p);

        QCOMPARE(cs.toScene(Millimeters(2.54)), 25.4);
        QVERIFY(cs.toMm(25.4) == Millimeters(2.54));

        const SizeMm size(2.0, 1.25);
        QCOMPARE(cs.toScene(size), QSizeF(20.0, 12.5));
        QVERIFY(cs.toMm(QSizeF(20.0, 12.5)) == size);
    }

    void originIsTopLeftAndAxesAreNotFlipped()
    {
        const CoordinateSystem cs;
        QCOMPARE(cs.toScene(PointMm(0.0, 0.0)), QPointF(0.0, 0.0));
        const QPointF down = cs.toScene(PointMm(0.0, 10.0));
        QVERIFY(down.y() > 0.0); // +Y mm → +Y scene (down), ADR §3.6
        const QPointF right = cs.toScene(PointMm(10.0, 0.0));
        QVERIFY(right.x() > 0.0);
    }

    void invalidResolutionFallsBackToDefault()
    {
        const CoordinateSystem cs(0.0, SizeMm(0.0, -1.0));
        QCOMPARE(cs.sceneUnitsPerMm(), CoordinateSystem::kDefaultSceneUnitsPerMm);
        QCOMPARE(cs.sheetSizeMm().width, 420.0);
    }

    void gridSnapping()
    {
        QVERIFY(CoordinateSystem::snapToGrid(PointMm(10.4, 20.6), 1.0) == PointMm(10.0, 21.0));
        QVERIFY(CoordinateSystem::snapToGrid(PointMm(3.9, 3.9), 2.54) == PointMm(5.08, 5.08));
        QVERIFY(CoordinateSystem::snapToGrid(PointMm(1.234, 5.678), 0.0) == PointMm(1.234, 5.678)); // no grid
    }

    void viewportZoomIsBoundedAndSignalled()
    {
        ViewportController vp;
        QSignalSpy zoomSpy(&vp, &ViewportController::zoomChanged);
        QCOMPARE(vp.zoom(), 1.0);
        QCOMPARE(vp.setZoom(1000.0), ViewportController::kMaxZoom);
        QCOMPARE(vp.setZoom(0.0), ViewportController::kMinZoom);
        QCOMPARE(vp.setZoom(std::nan("")), ViewportController::kMinZoom); // ignored
        QCOMPARE(zoomSpy.count(), 2);
        vp.reset();
        QCOMPARE(vp.zoom(), 1.0);
        QCOMPARE(vp.zoomBy(1), ViewportController::kDefaultZoomStep);
    }

    void zoomAroundAnchorKeepsAnchorFixed()
    {
        ViewportController vp;
        const QPointF anchorScene(1000.0, 500.0);
        const QPointF anchorViewport(300.0, 200.0);
        // Position anchor under viewport point at zoom 1.
        vp.setPanOffset(anchorViewport - anchorScene);
        QCOMPARE(vp.transform().map(anchorScene), anchorViewport);

        vp.zoomAround(2.5, anchorScene, anchorViewport);
        QCOMPARE(vp.zoom(), 2.5);
        const QPointF mapped = vp.transform().map(anchorScene);
        QVERIFY(qFuzzyCompare(mapped.x(), anchorViewport.x()));
        QVERIFY(qFuzzyCompare(mapped.y(), anchorViewport.y()));
    }

    void zoomAndPanNeverAlterEngineeringCoordinates()
    {
        const CoordinateSystem cs;
        ViewportController vp;
        const PointMm physical(100.0, 50.0);
        const QPointF sceneBefore = cs.toScene(physical);

        vp.setZoom(7.0);
        vp.panBy(QPointF(-321.0, 123.0));

        // Scene coordinates of a physical point are independent of the viewport.
        QCOMPARE(cs.toScene(physical), sceneBefore);
        QVERIFY(cs.toMm(sceneBefore) == physical);

        // The view transform only changes where it is *drawn*.
        const QPointF drawn = vp.transform().map(sceneBefore);
        QVERIFY(drawn != sceneBefore);
        // And inverting the transform recovers the same scene point → same mm.
        const QPointF recovered = vp.transform().inverted().map(drawn);
        QVERIFY(cs.toMm(recovered) == physical);
    }

    void fitZoomComputesLimitingAxis()
    {
        // 4200x2970 sheet into 1000x1000 viewport with 4% margin → limited by width.
        const double z = ViewportController::fitZoom(QSizeF(4200.0, 2970.0), QSizeF(1000.0, 1000.0), 0.04);
        QVERIFY(qFuzzyCompare(z, 960.0 / 4200.0));
        QCOMPARE(ViewportController::fitZoom(QSizeF(0.0, 0.0), QSizeF(10.0, 10.0)), 1.0);
    }

    void viewRealisesControllerPanThroughRealMapping()
    {
        // Regression: QGraphicsView's own scroll/alignment offset must not add
        // a second pan authority. fitSheet() must centre the sheet, and the
        // controller contract viewport = zoom*(scene+pan) must hold through
        // the *actual* view mapping.
        A3CanvasScene scene;
        ViewportController vp;
        A3CanvasView view(&scene, &vp);
        view.resize(1000, 700);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        view.fitSheet();
        QCoreApplication::processEvents();
        const QSize vpSize = view.viewport()->size();
        const QPointF sheetCentreInView = view.mapFromScene(scene.sheetRect().center());
        QVERIFY2(std::abs(sheetCentreInView.x() - vpSize.width() / 2.0) <= 1.5,
                 qPrintable(QStringLiteral("sheet centre x=%1, viewport centre x=%2").arg(sheetCentreInView.x()).arg(vpSize.width() / 2.0)));
        QVERIFY2(std::abs(sheetCentreInView.y() - vpSize.height() / 2.0) <= 1.5,
                 qPrintable(QStringLiteral("sheet centre y=%1, viewport centre y=%2").arg(sheetCentreInView.y()).arg(vpSize.height() / 2.0)));

        // Whole sheet visible after fit.
        const QRectF sheetInView = view.mapFromScene(scene.sheetRect()).boundingRect();
        QVERIFY(QRectF(QPointF(0, 0), QSizeF(vpSize)).contains(sheetInView));

        // Arbitrary pan/zoom: the controller contract must match the view mapping (±1 px rounding).
        vp.setZoom(0.5);
        vp.setPanOffset(QPointF(-1234.0, 321.0));
        QCoreApplication::processEvents();
        const QPointF scenePt(2000.0, 1000.0);
        const QPointF expected = vp.transform().map(scenePt);
        const QPointF actual = view.mapFromScene(scenePt);
        QVERIFY(std::abs(expected.x() - actual.x()) <= 1.0);
        QVERIFY(std::abs(expected.y() - actual.y()) <= 1.0);
    }

    void sceneExposesSheetAndGridWithoutAWindow()
    {
        A3CanvasScene scene;
        QCOMPARE(scene.sheetRect(), QRectF(0.0, 0.0, 4200.0, 2970.0));
        QVERIFY(scene.sceneRect().contains(scene.sheetRect()));
        QCOMPARE(scene.minorGridMm(), 1.0);
        QCOMPARE(scene.majorGridMm(), 10.0);
        scene.setGridMm(2.54, 25.4);
        QCOMPARE(scene.minorGridMm(), 2.54);
        scene.setGridMm(-1.0, 0.0); // ignored
        QCOMPARE(scene.minorGridMm(), 2.54);
        QCOMPARE(scene.majorGridMm(), 25.4);
        QVERIFY(scene.isGridVisible());
        scene.setGridVisible(false);
        QVERIFY(!scene.isGridVisible());
    }
};

QTEST_MAIN(CoordinateSystemTests)
#include "CoordinateSystemTests.moc"
