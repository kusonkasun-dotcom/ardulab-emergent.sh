#include "connection/ConnectivityManager.h"


namespace ardulab::connection {


ConnectivityManager::ConnectivityManager(
    ConnectivityGraph& graph
    )
    : m_graph(graph)
{
}



bool ConnectivityManager::addConnectionPoint(
    const ConnectionPoint& point
    )
{
    const bool result =
        m_graph.addConnectionPoint(point);


    if (result) {
        markDirty();
    }


    return result;
}



bool ConnectivityManager::connectWire(
    const Wire& wire
    )
{
    const bool result =
        m_graph.addWire(wire);


    if (result) {
        markDirty();
    }


    return result;
}



bool ConnectivityManager::disconnectWire(
    const WireId& wireId
    )
{
    const bool result =
        m_graph.removeWire(wireId);


    if (result) {
        markDirty();
    }


    return result;
}



void ConnectivityManager::rebuild()
{
    m_graph.rebuildNets();

    m_dirty = false;
}



void ConnectivityManager::beginBatch()
{
    m_batchMode = true;
}



void ConnectivityManager::endBatch()
{
    m_batchMode = false;

    rebuildIfRequired();
}



bool ConnectivityManager::isDirty() const noexcept
{
    return m_dirty;
}



const std::vector<Net>& ConnectivityManager::nets() const noexcept
{
    return m_graph.nets();
}



void ConnectivityManager::markDirty()
{
    m_dirty = true;

    rebuildIfRequired();
}



void ConnectivityManager::rebuildIfRequired()
{
    if (!m_dirty) {
        return;
    }


    if (m_batchMode) {
        return;
    }


    rebuild();
}


} // namespace ardulab::connection