#include <QtTest>

#include "connection/ValidationResult.h"


namespace ardulab::connection {


class ValidationResultTests final : public QObject
{
    Q_OBJECT


private slots:


    void emptyResultIsValid()
    {
        ValidationResult result;


        QVERIFY(
            result.isValid()
            );


        QVERIFY(
            !result.hasErrors()
            );


        QCOMPARE(
            result.issues().size(),
            static_cast<size_t>(0)
            );
    }



    void errorIssueInvalidatesResult()
    {
        ValidationResult result;


        ValidationIssue issue;

        issue.severity =
            ValidationSeverity::Error;

        issue.code =
            "NET-001";

        issue.message =
            "Short circuit detected";



        result.addIssue(issue);



        QVERIFY(
            result.hasErrors()
            );


        QVERIFY(
            !result.isValid()
            );


        QCOMPARE(
            result.issues().size(),
            static_cast<size_t>(1)
            );
    }


};


}


QTEST_MAIN(
    ardulab::connection::ValidationResultTests
    )

#include "ValidationResultTests.moc"