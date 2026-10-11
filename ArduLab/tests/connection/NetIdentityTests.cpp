#include <QtTest>

#include "connection/NetIdentity.h"


namespace ardulab::connection {


class NetIdentityTests final : public QObject
{
    Q_OBJECT


private slots:

    void sameConnectionsDifferentOrderProduceSameNetId();

    void differentConnectionsProduceDifferentNetId();

};


void NetIdentityTests::sameConnectionsDifferentOrderProduceSameNetId()
{
    NetIdentityInput first;

    first.connectionKeys =
        {
            "U001.VDD",
            "C101.1",
            "C102.1"
        };


    NetIdentityInput second;

    second.connectionKeys =
        {
            "C102.1",
            "U001.VDD",
            "C101.1"
        };


    const core::NetId firstId =
        NetIdentity::generate(first);


    const core::NetId secondId =
        NetIdentity::generate(second);


    QVERIFY(firstId.isValid());

    QCOMPARE(
        firstId.value(),
        secondId.value()
        );
}


void NetIdentityTests::differentConnectionsProduceDifferentNetId()
{
    NetIdentityInput first;

    first.connectionKeys =
        {
            "U001.VDD",
            "C101.1"
        };


    NetIdentityInput second;

    second.connectionKeys =
        {
            "U001.VDD",
            "C102.1"
        };


    const core::NetId firstId =
        NetIdentity::generate(first);


    const core::NetId secondId =
        NetIdentity::generate(second);


    QVERIFY(firstId.isValid());

    QVERIFY(secondId.isValid());


    QVERIFY(
        firstId != secondId
        );
}


} // namespace ardulab::connection


QTEST_MAIN(ardulab::connection::NetIdentityTests)

#include "NetIdentityTests.moc"