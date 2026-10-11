#include "connection/ConnectivityValidator.h"


namespace ardulab::connection {


ValidationResult ConnectivityValidator::validate(
    const ConnectivityGraph& graph
    ) const
{
    ValidationResult result;



    /*
     * Validate wire endpoint references.
     *
     * A wire must reference existing
     * connection points.
     */
    for(const auto& wirePair : graph.wires())
    {
        const Wire& wire =
            wirePair;



        const bool startExists =
            graph.points().contains(
                wire.startPointId
                );


        const bool endExists =
            graph.points().contains(
                wire.endPointId
                );



        if(!startExists || !endExists)
        {
            ValidationIssue issue;


            issue.severity =
                ValidationSeverity::Error;


            issue.code =
                "WIRE-INVALID-POINT";


            issue.message =
                "Wire references missing connection point";


            result.addIssue(issue);
        }
    }





    /*
     * Validate dangling connection points.
     *
     * A connection point without any
     * wire attachment is considered
     * incomplete.
     */
    for(auto iterator = graph.points().cbegin();
         iterator != graph.points().cend();
         ++iterator)
    {
        const auto pointId =
            iterator.key();



        bool connected =
            false;



        for(const auto& wirePair : graph.wires())
        {
            const Wire& wire =
                wirePair;



            if(wire.startPointId == pointId ||
                wire.endPointId == pointId)
            {
                connected = true;
                break;
            }
        }



        if(!connected)
        {
            ValidationIssue issue;


            issue.severity =
                ValidationSeverity::Warning;


            issue.code =
                "POINT-DANGLING";


            issue.message =
                "Connection point has no wire connection";


            result.addIssue(issue);
        }
    }





    /*
     * Validate generated nets.
     */
    for(const Net& net : graph.nets())
    {

        if(net.members.empty())
        {
            ValidationIssue issue;


            issue.severity =
                ValidationSeverity::Error;


            issue.code =
                "NET-EMPTY";


            issue.message =
                "Net has no members";


            result.addIssue(issue);
        }



        if(net.members.size() == 1)
        {
            ValidationIssue issue;


            issue.severity =
                ValidationSeverity::Warning;


            issue.code =
                "NET-SINGLE";


            issue.message =
                "Net contains only one member";


            result.addIssue(issue);
        }



        if(net.wires.empty())
        {
            ValidationIssue issue;


            issue.severity =
                ValidationSeverity::Warning;


            issue.code =
                "NET-NOWIRE";


            issue.message =
                "Net has no wire connection";


            result.addIssue(issue);
        }

    }



    return result;
}


}