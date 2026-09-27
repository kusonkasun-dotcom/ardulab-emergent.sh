# ArduLab — Living Spec

## What this repository is

`/app/ArduLab/` is a **native Qt6/C++17 Windows desktop application** (CMake + Ninja),
NOT a web app. The `/app/frontend` and `/app/backend` folders are the unused platform
template and are irrelevant to the product. Do not write React/FastAPI code for ArduLab.

Authoritative documents (do not modify without approval):
- `/app/ArduLab Software Architecture Document v1.0.md`
- `/app/ArduLab v0.1.3 Catalog Architecture Decision Record (ADR) v1.0.md`
- `/app/ArduLab Clean Foundation Implementation Plan v1.0.md`

## Status

**Phase 1 (Clean Qt6/C++ foundation) — COMPLETE and building** (Linux/GCC verified in
container; Windows/MSVC preset authored, not yet run by the user).

Build/verify: `cd /app/ArduLab && cmake --preset linux-debug && cmake --build --preset linux-debug && ctest --preset linux-debug`
Toolchain in container: cmake, ninja-build, qt6-base-dev 6.4.2, g++ 12 (installed via apt).

## Module layout (static libs; dependency direction enforced by linker)

```
src/core        Result/Error, Identifiers (TypedId<Tag>), Units (mm), Event, EventBus
src/components  Component, ComponentVersion, Package, Pin, PinAnchor, ComponentSnapshot,
                ComponentSearchCriteria, IComponentCatalog, IComponentManager,
                ComponentManager, InMemoryComponentCatalog (test double + bootstrap adapter)
src/project     Project, ProjectMetadata, FalDocument, FalSerializer, ProjectService
src/canvas      CoordinateSystem, ViewportController, A3CanvasScene, A3CanvasView
src/connection  CatalogPinRef, ConnectionPoint, Wire, Net (DECLARATIONS ONLY) + README.md
src/ui          MainWindow (thin), ComponentGraphicsItem (renderer adapter)
src/app         ApplicationBootstrap (composition root); src/main.cpp
tests/          8 Qt Test suites; fixtures in tests/fixtures
resources/      ardulab.qrc, categories.v1.json, LegacyEmptyProject.FAL, Resistor.schema-1.0.json
```

## Key contracts

- IDs are text `TypedId<Tag>`: ComponentId, ComponentVersionId ("<id>@<semver>"), InstanceId, ...
- All geometry is millimeters (`PointMm`, `SizeMm`); origin top-left, +X right, +Y down.
- Scene resolution fixed at 10 scene units / mm; zoom/pan live only in ViewportController.
- `A3CanvasView` realises the controller pan via QGraphicsView scroll (`scroll = -zoom*pan`).
- Component lookups are exact `(scope, componentId, versionId)`; a missing version is
  `ComponentVersionNotFound` — never substituted. `loadActive()` is explicit opt-in.
- Versions are immutable (`AlreadyExists` on duplicate). `registerSnapshot` validates
  id/version consistency and unique pin numbers.
- `.FAL`: legacy `{project,components}` opens; unknown root keys and unknown instance fields
  round-trip; writer emits `fal_format_version: "1.0"` + `canvas` + `library` sections.
  Unresolved catalog references open read-only with a diagnostic.
- `EventBus::publishEvent<T>()` (not `emit` — Qt macro clash). Subscriptions are RAII tokens.
- Events: ProjectOpened/Saved/Closed, ComponentRegistered.

## Runtime (Phase 1)

Bootstrap seeds one in-memory component `R1-A-0805-10K@1.0.0` (USER/DRAFT) and opens
"Untitled Project". UI: File New/Open/Save/SaveAs/Close, View Zoom/Fit/Grid, Catalog
Refresh/Place (Insert key or double-click). Placing records the exact version + content
hash + fallback snapshot in the project instance.

## Not implemented (out of Phase 1 scope)

SQLite catalog & migrations, JSON importer/legacy mapper, Schematic, Simulation, PCB,
Manufacturing, Firmware, AI. `Qt6::Sql` is not linked anywhere.

## Next phase (needs user approval)

Phase 2 per Implementation Plan Steps 5–10: CatalogPaths, CatalogDatabase, CatalogTransaction,
SchemaMigration(+Registry/Migrator), Migration001InitialCatalog, SqliteComponentCatalog,
ComponentJsonSchema/LegacyComponentMapper/ComponentJsonImporter. Replace
InMemoryComponentCatalog in ApplicationBootstrap with SqliteComponentCatalog.
