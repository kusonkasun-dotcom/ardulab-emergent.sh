#pragma once

#include "connection/ConnectivityGraph.h"

namespace ardulab::connection {


/**
 * ConnectivityManager
 *
 * Application-facing gateway for electrical connectivity operations.
 *
 * Responsibilities:
 * - coordinate ConnectivityGraph operations
 * - manage rebuild policy
 * - support interactive and batch workflows
 *
 * Modes:
 *
 * Interactive:
 *   modification -> mark dirty -> rebuild
 *
 * Batch:
 *   beginBatch()
 *        |
 *        multiple modifications
 *        |
 *   endBatch()
 *        |
 *        single rebuild
 *
 * Independent from:
 * - UI
 * - Canvas
 * - Database
 */
class ConnectivityManager final
{
public:

    explicit ConnectivityManager(
        ConnectivityGraph& graph
        );


    /**
     * Register electrical connection point.
     */
    bool addConnectionPoint(
        const ConnectionPoint& point
        );


    /**
     * Add electrical wire.
     */
    bool connectWire(
        const Wire& wire
        );


    /**
     * Remove electrical wire.
     */
    bool disconnectWire(
        const WireId& wireId
        );


    /**
     * Force rebuild of electrical nets.
     */
    void rebuild();



    /**
     * Begin batch operation.
     *
     * Rebuild will be delayed until endBatch().
     */
    void beginBatch();



    /**
     * Finish batch operation.
     *
     * If connectivity changed,
     * rebuild will happen once.
     */
    void endBatch();



    /**
     * Indicates whether connectivity
     * requires rebuild.
     */
    [[nodiscard]]
    bool isDirty() const noexcept;



    /**
     * Current generated nets.
     */
    [[nodiscard]]
    const std::vector<Net>& nets() const noexcept;



private:

    /**
     * Mark graph state changed.
     */
    void markDirty();



    /**
     * Rebuild only when allowed.
     */
    void rebuildIfRequired();



private:

    ConnectivityGraph& m_graph;


    /**
     * Connectivity changed after last rebuild.
     */
    bool m_dirty = false;


    /**
     * Batch operation state.
     *
     * When true:
     * rebuild is postponed.
     */
    bool m_batchMode = false;

};


} // namespace ardulab::connection