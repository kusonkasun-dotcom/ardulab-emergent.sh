// Milestone 1 smoke test: verifies that the CMake/Ninja/Qt6 toolchain wiring
// produces a runnable Qt Test binary. Carries no domain assertions.

#include <QtTest/QtTest>
#include <QtGlobal>

class BuildSkeletonTests final : public QObject
{
    Q_OBJECT

private slots:
    void toolchainIsCpp17OrNewer()
    {
        QVERIFY2(__cplusplus >= 201703L, "ArduLab requires at least C++17");
    }

    void qtIsVersion6()
    {
        QCOMPARE(QT_VERSION_MAJOR, 6);
    }
};

QTEST_APPLESS_MAIN(BuildSkeletonTests)
#include "BuildSkeletonTests.moc"
