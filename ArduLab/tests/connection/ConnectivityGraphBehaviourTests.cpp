#include <QtTest>

#include "connection/ConnectivityGraph.h"


namespace ardulab::connection {


class ConnectivityGraphBehaviourTests final : public QObject
{
    Q_OBJECT

private slots:

    void emptyGraphStartsWithoutNets()
    {
        ConnectivityGraph graph;

        QCOMPARE(
            graph.nets().size(),
            static_cast<size_t>(0)
            );
    }


    void rebuildEmptyGraphKeepsStateValid()
    {
        ConnectivityGraph graph;

        graph.rebuildNets();

        QCOMPARE(
            graph.nets().size(),
            static_cast<size_t>(0)
            );
    }


    void wireLifecycle()
    {
        ConnectivityGraph graph;

        Wire wire;

        wire.wireId = WireId("W001");


        QVERIFY(
            graph.addWire(wire)
            );


        QVERIFY(
            graph.removeWire(
                WireId("W001")
                )
            );
    }

};


} // namespace ardulab::connection


QTEST_MAIN(ardulab::connection::ConnectivityGraphBehaviourTests)

#include "ConnectivityGraphBehaviourTests.moc"