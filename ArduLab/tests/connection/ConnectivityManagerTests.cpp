#include <QtTest>

#include "connection/ConnectivityManager.h"


namespace ardulab::connection {


class ConnectivityManagerTests final : public QObject
{
    Q_OBJECT


private slots:


    void managerCanAddAndRemoveWire()
    {
        ConnectivityGraph graph;

        ConnectivityManager manager(graph);


        Wire wire;

        wire.wireId =
            WireId("W001");


        QVERIFY(
            manager.connectWire(wire)
            );


        QVERIFY(
            manager.disconnectWire(
                WireId("W001")
                )
            );
    }



    void managerCanTriggerRebuild()
    {
        ConnectivityGraph graph;

        ConnectivityManager manager(graph);


        manager.rebuild();


        QCOMPARE(
            manager.nets().size(),
            static_cast<size_t>(0)
            );


        QVERIFY(
            !manager.isDirty()
            );
    }



    void managerBuildsNetFromConnections()
    {
        ConnectivityGraph graph;

        ConnectivityManager manager(graph);



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



        QVERIFY(
            manager.addConnectionPoint(u1)
            );


        QVERIFY(
            manager.addConnectionPoint(r1)
            );



        Wire wire;

        wire.wireId =
            WireId("W001");

        wire.startPointId =
            u1.pointId;

        wire.endPointId =
            r1.pointId;



        QVERIFY(
            manager.connectWire(wire)
            );



        manager.rebuild();



        QCOMPARE(
            manager.nets().size(),
            static_cast<size_t>(1)
            );



        const Net& net =
            manager.nets().front();



        QCOMPARE(
            net.members.size(),
            static_cast<size_t>(2)
            );


        QVERIFY(
            !manager.isDirty()
            );
    }



    void managerSynchronizesAfterModification()
    {
        ConnectivityGraph graph;

        ConnectivityManager manager(graph);



        QVERIFY(
            !manager.isDirty()
            );



        Wire wire;

        wire.wireId =
            WireId("W-NORMAL-001");



        QVERIFY(
            manager.connectWire(wire)
            );



        /*
         * Normal mode:
         * modification triggers automatic rebuild.
         */
        QVERIFY(
            !manager.isDirty()
            );



        manager.beginBatch();



        Wire batchWire;

        batchWire.wireId =
            WireId("W-BATCH-001");



        QVERIFY(
            manager.connectWire(batchWire)
            );



        /*
         * Batch mode:
         * modification waits for rebuild.
         */
        QVERIFY(
            manager.isDirty()
            );



        manager.endBatch();



        QVERIFY(
            !manager.isDirty()
            );
    }



    void batchModeDelaysRebuild()
    {
        ConnectivityGraph graph;

        ConnectivityManager manager(graph);



        manager.beginBatch();



        Wire wire;

        wire.wireId =
            WireId("W-BATCH-002");



        QVERIFY(
            manager.connectWire(wire)
            );



        QVERIFY(
            manager.isDirty()
            );



        QCOMPARE(
            manager.nets().size(),
            static_cast<size_t>(0)
            );



        manager.endBatch();



        QVERIFY(
            !manager.isDirty()
            );


        QCOMPARE(
            manager.nets().size(),
            static_cast<size_t>(0)
            );
    }


};


} // namespace ardulab::connection


QTEST_MAIN(ardulab::connection::ConnectivityManagerTests)

#include "ConnectivityManagerTests.moc"