#include <QtTest>

#include "connection/ConnectivityValidator.h"


namespace ardulab::connection {


class ConnectivityValidatorTests final : public QObject
{
    Q_OBJECT


private:


    ConnectionPoint createTestPoint(
        const QString& id,
        const QString& instance,
        const QString& pinNumber
        )
    {
        ConnectionPoint point;


        point.pointId =
            core::ConnectionPointId(id);


        point.instanceId =
            core::InstanceId(instance);


        point.pin.componentId =
            core::ComponentId(
                "TEST-COMPONENT"
                );


        point.pin.versionId =
            core::ComponentVersionId(
                "TEST-COMPONENT@1.0"
                );


        point.pin.pinNumber =
            pinNumber;


        return point;
    }



    bool containsIssueCode(
        const ValidationResult& result,
        const QString& code
        )
    {
        for(const auto& issue : result.issues())
        {
            if(issue.code == code)
            {
                return true;
            }
        }


        return false;
    }



private slots:


    void emptyGraphIsValid()
    {
        ConnectivityGraph graph;


        ConnectivityValidator validator;


        ValidationResult result =
            validator.validate(graph);



        QVERIFY(result.isValid());

        QVERIFY(!result.hasErrors());


        QCOMPARE(
            result.issues().size(),
            static_cast<size_t>(0)
            );
    }



    void singleConnectionPointProducesWarnings()
    {
        ConnectivityGraph graph;



        graph.addConnectionPoint(
            createTestPoint(
                "CP-U1-1",
                "U1",
                "1"
                )
            );



        graph.rebuildNets();



        ConnectivityValidator validator;


        ValidationResult result =
            validator.validate(graph);



        QVERIFY(result.isValid());

        QVERIFY(!result.hasErrors());



        QCOMPARE(
            result.issues().size(),
            static_cast<size_t>(3)
            );



        QVERIFY(
            containsIssueCode(
                result,
                "NET-SINGLE"
                )
            );


        QVERIFY(
            containsIssueCode(
                result,
                "NET-NOWIRE"
                )
            );


        QVERIFY(
            containsIssueCode(
                result,
                "POINT-DANGLING"
                )
            );
    }



    void normalConnectedNetProducesNoIssues()
    {
        ConnectivityGraph graph;



        ConnectionPoint pointA =
            createTestPoint(
                "CP-U1-1",
                "U1",
                "1"
                );


        ConnectionPoint pointB =
            createTestPoint(
                "CP-U2-1",
                "U2",
                "1"
                );



        graph.addConnectionPoint(pointA);

        graph.addConnectionPoint(pointB);



        Wire wire;


        wire.wireId =
            WireId("W001");


        wire.startPointId =
            pointA.pointId;


        wire.endPointId =
            pointB.pointId;



        graph.addWire(wire);



        graph.rebuildNets();



        ConnectivityValidator validator;


        ValidationResult result =
            validator.validate(graph);



        QVERIFY(result.isValid());

        QVERIFY(!result.hasErrors());


        QCOMPARE(
            result.issues().size(),
            static_cast<size_t>(0)
            );
    }



    void invalidWireReferenceProducesError()
    {
        ConnectivityGraph graph;



        ConnectionPoint point =
            createTestPoint(
                "CP-U1-1",
                "U1",
                "1"
                );



        graph.addConnectionPoint(point);



        Wire wire;


        wire.wireId =
            WireId("W-BROKEN");


        wire.startPointId =
            point.pointId;


        wire.endPointId =
            core::ConnectionPointId(
                "CP-NOT-EXISTS"
                );



        graph.addWire(wire);



        graph.rebuildNets();



        ConnectivityValidator validator;


        ValidationResult result =
            validator.validate(graph);



        QVERIFY(result.hasErrors());

        QVERIFY(!result.isValid());



        QVERIFY(
            containsIssueCode(
                result,
                "WIRE-INVALID-POINT"
                )
            );
    }



    void danglingConnectionPointProducesWarning()
    {
        ConnectivityGraph graph;



        graph.addConnectionPoint(
            createTestPoint(
                "CP-DANGLING",
                "U3",
                "5"
                )
            );



        graph.rebuildNets();



        ConnectivityValidator validator;


        ValidationResult result =
            validator.validate(graph);



        QVERIFY(
            containsIssueCode(
                result,
                "POINT-DANGLING"
                )
            );
    }



};


}


QTEST_MAIN(
    ardulab::connection::ConnectivityValidatorTests
    )


#include "ConnectivityValidatorTests.moc"