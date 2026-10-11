#include <QtTest>

#include "connection/ConnectivityGraph.h"


namespace ardulab::connection {


class ConnectivityGraphTests final : public QObject
{
    Q_OBJECT

private slots:

    void canAddConnectionPoint()
    {
        ConnectivityGraph graph;

        ConnectionPoint point;

        point.instanceId = core::InstanceId("U1");
        point.pin.pinNumber = "1";

        QVERIFY(
            graph.addConnectionPoint(point)
            );
    }


    void canAddWire()
    {
        ConnectivityGraph graph;

        Wire wire;

        wire.wireId = WireId("W001");

        QVERIFY(
            graph.addWire(wire)
            );
    }


    void canRemoveWire()
    {
        ConnectivityGraph graph;

        Wire wire;

        wire.wireId = WireId("W001");

        graph.addWire(wire);

        QVERIFY(
            graph.removeWire(
                WireId("W001")
                )
            );
    }


    void emptyGraphCanRebuild()
    {
        ConnectivityGraph graph;

        graph.rebuildNets();

        QCOMPARE(
            graph.nets().size(),
            static_cast<size_t>(0)
            );
    }

};


}


QTEST_MAIN(ardulab::connection::ConnectivityGraphTests)

#include "ConnectivityGraphTests.moc"