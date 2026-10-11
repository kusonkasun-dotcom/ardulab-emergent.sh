#include <QtTest>

#include "connection/ConnectivityGraph.h"


namespace ardulab::connection {


class ConnectivityGraphNetBuildTests final : public QObject
{
    Q_OBJECT


private slots:

    void connectedPointsBecomeSingleNet();

};



void ConnectivityGraphNetBuildTests::connectedPointsBecomeSingleNet()
{
    ConnectivityGraph graph;


    ConnectionPoint pointA;

    pointA.pointId =
        core::ConnectionPointId("CP-U1-1");

    pointA.instanceId =
        core::InstanceId("U1");

    pointA.pin.pinNumber =
        "1";



    ConnectionPoint pointB;

    pointB.pointId =
        core::ConnectionPointId("CP-R1-1");

    pointB.instanceId =
        core::InstanceId("R1");

    pointB.pin.pinNumber =
        "1";



    ConnectionPoint pointC;

    pointC.pointId =
        core::ConnectionPointId("CP-C1-1");

    pointC.instanceId =
        core::InstanceId("C1");

    pointC.pin.pinNumber =
        "1";



    QVERIFY(
        graph.addConnectionPoint(pointA)
        );

    QVERIFY(
        graph.addConnectionPoint(pointB)
        );

    QVERIFY(
        graph.addConnectionPoint(pointC)
        );



    Wire wire1;

    wire1.wireId =
        WireId("W001");

    wire1.startPointId =
        pointA.pointId;

    wire1.endPointId =
        pointB.pointId;



    Wire wire2;

    wire2.wireId =
        WireId("W002");

    wire2.startPointId =
        pointB.pointId;

    wire2.endPointId =
        pointC.pointId;



    QVERIFY(
        graph.addWire(wire1)
        );

    QVERIFY(
        graph.addWire(wire2)
        );


    graph.rebuildNets();



    QCOMPARE(
        graph.nets().size(),
        static_cast<size_t>(1)
        );


    const Net& net =
        graph.nets().front();



    QCOMPARE(
        net.members.size(),
        static_cast<size_t>(3)
        );


    QCOMPARE(
        net.wires.size(),
        static_cast<size_t>(2)
        );
}


} // namespace ardulab::connection


QTEST_MAIN(ardulab::connection::ConnectivityGraphNetBuildTests)

#include "ConnectivityGraphNetBuildTests.moc"