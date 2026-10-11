#pragma once

#include "connection/ConnectionPoint.h"
#include "connection/Net.h"
#include "connection/Wire.h"

#include <QHash>
#include <QSet>

#include <vector>


namespace ardulab::connection {


/**
 * ConnectivityGraph
 *
 * Foundation of the electrical connectivity engine.
 *
 * Responsible for:
 * - storing connection points
 * - storing wires
 * - maintaining graph indexes
 * - rebuilding electrical nets
 *
 * Independent from:
 * - UI
 * - Canvas
 * - Renderer
 * - Database
 */
class ConnectivityGraph final
{
public:

    ConnectivityGraph() = default;



    /**
     * Register a connection point.
     */
    bool addConnectionPoint(
        const ConnectionPoint& point
        );



    /**
     * Register a wire.
     */
    bool addWire(
        const Wire& wire
        );



    /**
     * Remove a wire from graph.
     */
    bool removeWire(
        const WireId& wireId
        );



    /**
     * Recalculate electrical nets.
     */
    void rebuildNets();



    /**
     * Current generated electrical networks.
     */
    [[nodiscard]]
    const std::vector<Net>& nets() const noexcept;



    /**
     * Read-only access to registered connection points.
     *
     * Used by:
     * - ConnectivityValidator
     * - future ERC analysis
     *
     * Graph remains the owner of data.
     */
    [[nodiscard]]
    const QHash<
        core::ConnectionPointId,
        ConnectionPoint
        >& points() const noexcept;



    /**
     * Read-only access to registered wires.
     *
     * Used by:
     * - ConnectivityValidator
     * - future ERC analysis
     *
     * Graph remains the owner of data.
     */
    [[nodiscard]]
    const QHash<
        WireId,
        Wire
        >& wires() const noexcept;



private:


    /**
     * Build internal graph indexes.
     *
     * Creates:
     *
     * Point -> Point
     *
     * and
     *
     * Point -> Wire
     */
    void rebuildGraphIndex();



    /**
     * Traverse connected points to construct a net.
     */
    void traverseNet(
        const core::ConnectionPointId& startPoint,
        QSet<core::ConnectionPointId>& visited,
        Net& net
        );



    /**
     * Remove generated nets.
     */
    void clearNets();



private:


    /**
     * Registered electrical attachment points.
     */
    QHash<
        core::ConnectionPointId,
        ConnectionPoint
        > m_points;



    /**
     * Registered wires.
     */
    QHash<
        WireId,
        Wire
        > m_wires;



    /**
     * Point-to-point electrical relationship.
     *
     * Example:
     *
     * CP1 -> CP2
     * CP2 -> CP3
     */
    QHash<
        core::ConnectionPointId,
        std::vector<core::ConnectionPointId>
        > m_pointAdjacency;



    /**
     * Point-to-wire relationship.
     *
     * Example:
     *
     * CP1 -> W001
     * CP2 -> W001
     * CP2 -> W002
     */
    QHash<
        core::ConnectionPointId,
        std::vector<WireId>
        > m_wireAdjacency;



    /**
     * Generated electrical networks.
     */
    std::vector<Net> m_nets;

};


} // namespace ardulab::connection