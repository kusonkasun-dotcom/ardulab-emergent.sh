# `connection/` — Connection Core boundary (contract-only)

**Status:** Declarations only. **No Connection Core behaviour is implemented in the
clean-foundation phase.** This directory exists to protect the future electrical
relationship layer (v0.1.4) as an independent module.

## What is here

| File | Purpose |
|---|---|
| `CatalogPinRef.h` | Immutable `scope + component_id + component_version_id + pin_number` reference (ADR §3.6). |
| `ConnectionPoint.h` | Declaration of a project-local attachment point. |
| `Wire.h` | Declaration of a project-local polyline. |
| `Net.h` | Declaration of a project-local electrical equivalence class. |

## Boundary rules

- Connection Core depends on **`core`** and on **stable component pin references**
  (`Pin`, `PinAnchor`, `ComponentSnapshot`, `CatalogPinRef`) **only**.
- It must never depend on SQL rows, the Component Renderer (`ui/`), or Schematic UI.
- `PinAnchor` is *supplied to* this module by Component Manager; this module never
  redefines or mutates it.
- `ConnectionPoint`, `Wire`, and `Net` are `.FAL` project data, not catalog rows.

## Explicitly absent in this phase

- Wire creation/editing, junctions, net labels.
- Net construction, connectivity graph, ERC.
- Any Schematic, Simulation, or PCB dependency.

Implementation of the above requires the **v0.1.4 Connection Core** architecture
discussion and approval (Development Rule 3).
