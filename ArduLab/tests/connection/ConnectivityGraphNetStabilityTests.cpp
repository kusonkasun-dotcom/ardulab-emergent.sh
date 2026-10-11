#include <QtTest>

#include "connection/ConnectivityGraph.h"


namespace ardulab::connection {


class ConnectivityGraphNetStabilityTests final : public QObject
{
    Q_OBJECT


private slots:

    void differentInsertionOrderProducesSameNetId();

};



void ConnectivityGraphNetStabilityTests::
    differentInsertionOrderProducesSameNetId()
{
    /*
     * ============================
     * Graph A
     * ============================
     */

    ConnectivityGraph graphA;


    ConnectionPoint u1;
    u1.pointId =
        core::ConnectionPointId("CP-U001-1");
    u1.instanceId =
        core::InstanceId("U001");
    u1.pin.pinNumber =
        "1";


    ConnectionPoint r1;
    r1.pointId =
        core::ConnectionPointId("CP-R001-1");
    r1.instanceId =
        core::InstanceId("R001");
    r1.pin.pinNumber =
        "1";


    ConnectionPoint c1;
    c1.pointId =
        core::ConnectionPointId("CP-C001-1");
    c1.instanceId =
        core::InstanceId("C001");
    c1.pin.pinNumber =
        "1";



    QVERIFY(graphA.addConnectionPoint(u1));
    QVERIFY(graphA.addConnectionPoint(r1));
    QVERIFY(graphA.addConnectionPoint(c1));



    Wire w1;

    w1.wireId =
        WireId("W001");

    w1.startPointId =
        u1.pointId;

    w1.endPointId =
        r1.pointId;



    Wire w2;

    w2.wireId =
        WireId("W002");

    w2.startPointId =
        r1.pointId;

    w2.endPointId =
        c1.pointId;



    QVERIFY(graphA.addWire(w1));
    QVERIFY(graphA.addWire(w2));



    graphA.rebuildNets();



    QCOMPARE(
        graphA.nets().size(),
        static_cast<size_t>(1)
        );



    const auto netA =
        graphA.nets().front().netId;



    /*
     * ============================
     * Graph B
     * Different insertion order
     * ============================
     */

    ConnectivityGraph graphB;



    QVERIFY(graphB.addConnectionPoint(c1));
    QVERIFY(graphB.addConnectionPoint(u1));
    QVERIFY(graphB.addConnectionPoint(r1));



    QVERIFY(graphB.addWire(w2));
    QVERIFY(graphB.addWire(w1));



    graphB.rebuildNets();



    QCOMPARE(
        graphB.nets().size(),
        static_cast<size_t>(1)
        );



    const auto netB =
        graphB.nets().front().netId;



    /*
     * Core requirement:
     *
     * Same electrical topology
     * must produce same identity.
     */
    QCOMPARE(
        netA.value(),
        netB.value()
        );
}


} // namespace ardulab::connection


QTEST_MAIN(ardulab::connection::ConnectivityGraphNetStabilityTests)

#include "ConnectivityGraphNetStabilityTests.moc"