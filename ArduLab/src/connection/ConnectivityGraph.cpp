#include "connection/ConnectivityGraph.h"
#include "connection/NetIdentity.h"


namespace ardulab::connection {


bool ConnectivityGraph::addConnectionPoint(
    const ConnectionPoint& point
    )
{
    const auto id = point.pointId;


    if (m_points.contains(id)) {
        return false;
    }


    m_points.insert(id, point);


    return true;
}



bool ConnectivityGraph::addWire(
    const Wire& wire
    )
{
    const auto id = wire.wireId;


    if (m_wires.contains(id)) {
        return false;
    }


    m_wires.insert(id, wire);


    return true;
}



bool ConnectivityGraph::removeWire(
    const WireId& wireId
    )
{
    return m_wires.remove(wireId) > 0;
}



void ConnectivityGraph::rebuildNets()
{
    clearNets();


    rebuildGraphIndex();


    QSet<core::ConnectionPointId> visited;



    for (auto iterator = m_points.cbegin();
         iterator != m_points.cend();
         ++iterator)
    {
        const auto pointId = iterator.key();


        if (visited.contains(pointId)) {
            continue;
        }


        Net net;


        traverseNet(
            pointId,
            visited,
            net
            );


        if (!net.members.empty())
        {
            NetIdentityInput input;


            for (const auto& member : net.members)
            {
                const QString key =
                    member.instanceId.value()
                    + QStringLiteral("|")
                    + member.pin.componentId.value()
                    + QStringLiteral("|")
                    + member.pin.versionId.value()
                    + QStringLiteral("|")
                    + member.pin.pinNumber;


                input.connectionKeys.push_back(key);
            }


            net.netId =
                NetIdentity::generate(input);


            m_nets.push_back(net);
        }
    }
}



const std::vector<Net>& ConnectivityGraph::nets() const noexcept
{
    return m_nets;
}



const QHash<
    core::ConnectionPointId,
    ConnectionPoint
    >& ConnectivityGraph::points() const noexcept
{
    return m_points;
}



const QHash<
    WireId,
    Wire
    >& ConnectivityGraph::wires() const noexcept
{
    return m_wires;
}



void ConnectivityGraph::clearNets()
{
    m_nets.clear();
}



void ConnectivityGraph::rebuildGraphIndex()
{
    m_pointAdjacency.clear();
    m_wireAdjacency.clear();



    for (const auto& wire : m_wires)
    {
        m_pointAdjacency[wire.startPointId]
            .push_back(
                wire.endPointId
                );


        m_pointAdjacency[wire.endPointId]
            .push_back(
                wire.startPointId
                );



        m_wireAdjacency[wire.startPointId]
            .push_back(
                wire.wireId
                );


        m_wireAdjacency[wire.endPointId]
            .push_back(
                wire.wireId
                );
    }
}



void ConnectivityGraph::traverseNet(
    const core::ConnectionPointId& startPoint,
    QSet<core::ConnectionPointId>& visited,
    Net& net
    )
{
    if (visited.contains(startPoint)) {
        return;
    }


    visited.insert(startPoint);



    auto pointIterator =
        m_points.find(startPoint);



    if (pointIterator != m_points.end())
    {
        NetMember member;


        member.instanceId =
            pointIterator->instanceId;


        member.pin =
            pointIterator->pin;


        net.members.push_back(member);
    }



    const auto wires =
        m_wireAdjacency.value(startPoint);



    for (const auto& wireId : wires)
    {
        bool exists = false;


        for (const auto& existing : net.wires)
        {
            if (existing == wireId)
            {
                exists = true;
                break;
            }
        }


        if (!exists)
        {
            net.wires.push_back(wireId);
        }
    }



    const auto neighbours =
        m_pointAdjacency.value(startPoint);



    for (const auto& next : neighbours)
    {
        traverseNet(
            next,
            visited,
            net
            );
    }
}


} // namespace ardulab::connection