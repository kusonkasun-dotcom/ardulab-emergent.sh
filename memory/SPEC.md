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

**Phase 2 (SQLite catalog + migrations + canonical JSON import) — COMPLETE and building**
(Linux/GCC verified in container). Windows/MSVC **NOT VERIFIED**.

**MVP polish (cursor snap readout · undo/redo · `.FAL` roundtrip) — COMPLETE.**
Actual verification (Linux/GCC, offscreen QPA): `ctest` **14/14 suites pass, 108 test
functions, 0 failed, 0 skipped**. Windows/MSVC and interactive (on-screen) GUI use remain
**NOT VERIFIED** — only headless Linux runs have been executed.

MVP polish detail:
- **Cursor snap readout** — status bar shows `Snap: x, y mm` (grid-quantised millimeters) plus
  `Pin <instance>·<pin> @ x, y mm` for the nearest pin anchor within a **12 screen-pixel**
  tolerance. Uses scene→viewport mapping and the item transform, so it stays correct after
  zoom, pan and rotation. Geometric assistance only — it forms **no electrical connection**.
- **Undo/redo** — one `QUndoStack` in `MainWindow` with `PlaceInstanceCommand`,
  `DeleteInstanceCommand`, `MoveInstanceCommand`, `RotateInstanceCommand`,
  `RenameInstanceCommand` (display name), `ChangeValueCommand` (engineering value) and
  `ChangeReferenceCommand` (reference designator = instance id). One drag = exactly one step
  (press records the start, release pushes the grid-snapped result). The unsaved-changes
  indicator follows the stack's clean index: Save → clean, Undo below it → dirty,
  Redo back to it → clean.
- **`.FAL` roundtrip** — save → close → reopen preserves instance id, library id, value,
  display name, position, rotation and the embedded fallback snapshot. A **changed or
  unavailable catalog** opens the stored snapshot as `Unresolved` (never substituted).
  `ProjectService::open` is now **transactional**: a corrupt/unreadable file returns an error
  and leaves the document currently open untouched (previously the UI closed first and lost it).

New/changed for MVP polish:
- `ComponentInstance::value` (`"value"` key in `.FAL`, interpreted + round-tripped).
- `ProjectService::open` transactional (no `ProjectAlreadyOpen` guard; publishes
  `ProjectClosedEvent` then `ProjectOpenedEvent` when it replaces a document).
- `MainWindow::~MainWindow` tears down connections/event filter before children are destroyed
  (fixed a real shutdown crash: child signals reaching a half-destroyed window).
- Tests: `tests/ui/EditUndoRedoTests.cpp`, `tests/ui/CursorSnapReadoutTests.cpp`,
  shared `tests/ui/EditingFixture.h`, extended `tests/project/ProjectRoundtripTests.cpp`.

Build/verify: `cd /app/ArduLab && cmake --preset linux-debug && cmake --build --preset linux-debug && ctest --preset linux-debug`
Toolchain in container: cmake, ninja-build, qt6-base-dev 6.4.2 (incl. Qt6::Sql + libqsqlite),
g++ 12 (installed via apt). Configure needs `-DCMAKE_PREFIX_PATH=/usr/lib/aarch64-linux-gnu/cmake`
if a bare `cmake --preset` reconfigure cannot find Qt6.

## Phase 2 catalog (database module)

- Per-user SQLite catalog at `QStandardPaths::AppLocalDataLocation/catalog/ardulab_catalog.sqlite3`
  (`%LOCALAPPDATA%\ArduLab\catalog\...` on Windows). `CatalogPaths` creates the dir; tests use
  `":memory:"`, never the user path.
- `CatalogDatabase` owns one uniquely-named Qt SQL connection, enables `foreign_keys`,
  `busy_timeout`, WAL. `CatalogTransaction` is an RAII commit/rollback guard.
- Migration framework: `SchemaMigration` (id/description/checksum/apply), `SchemaMigrationRegistry`
  (ordered, rejects duplicate ids), `SchemaMigrator` (reads current version, refuses a catalog
  newer than supported, applies pending in a transaction, records id/checksum/appversion/timestamp).
- `Migration001InitialCatalog` (id=1) creates ALL foundation tables in one step —
  `schema_migrations, catalog_metadata, categories, manufacturers, packages, components,
  component_versions, pins, component_parameters` + indexes + FKs — and seeds the 9 system
  categories. **Deviation:** the Plan Step 7 consolidates the ADR's design-level
  Migration_001–006 into this single Migration 001; documented intentional consolidation.
- `SqliteComponentCatalog` implements `IComponentCatalog` with prepared queries + a transactional
  `store()`; exact `(scope, component, version)` resolution, metadata `search()` with filters,
  `listVersions()` (newest first), `activeVersionOf()`. Row↔snapshot mapping preserves mm anchors.
- JSON import: `ComponentJsonSchema` detects `schema_version`/`component_schema_version` (canonical
  major 1 supported; unsupported major → VAL016). `ComponentJsonImporter` (implements
  `IComponentImporter`) parses the flat canonical v1.0 shape (matches
  `resources/examples/Resistor.schema-1.0.json`), runs structural validation (VAL003/005/006/007/
  008/009/013/014/016), computes a deterministic SHA-256 content hash, imports as **DRAFT**, and
  **skips + reports** an already-present `(scope, component, version)` (never overwrites). Stores
  only through `ComponentManager.registerSnapshot`.
- **Deviation (per user instruction "do not invent a legacy JSON format without a real example"):**
  `LegacyComponentMapper` / legacy-schema import is DEFERRED — only the canonical v1.0 shape (the
  one real example on disk) is imported. Re-introduce when a real legacy fixture is supplied.

## Bootstrap / UI integration

`ApplicationBootstrap` now: resolve catalog path → open `CatalogDatabase` → run `SchemaMigrator` →
build `SqliteComponentCatalog` + `ComponentManager` + `ComponentJsonImporter` → seed the bundled
`:/ardulab/examples/Resistor.schema-1.0.json` through the real import pipeline (idempotent skip on
later runs) → build `MainWindow`. `MainWindow` gained a **Catalog ▸ Import Component…** action that
forwards a file path to `IComponentImporter` (no JSON/SQL logic in the UI) and shows the report.
The catalog browser + placement workflow are unchanged (they already go through `IComponentManager`).


## Module layout (static libs; dependency direction enforced by linker)

```
src/core        Result/Error, Identifiers (TypedId<Tag>), Units (mm), Event, EventBus
src/components  Component, ComponentVersion, Package, Pin, PinAnchor, ComponentSnapshot,
                ComponentSearchCriteria, IComponentCatalog, IComponentManager,
                ComponentManager, InMemoryComponentCatalog (test double + bootstrap adapter)
src/project     Project, ProjectMetadata, FalDocument, FalSerializer, ProjectService
src/canvas      CoordinateSystem, ViewportController, A3CanvasScene, A3CanvasView
src/connection  CatalogPinRef, ConnectionPoint, Wire, Net (DECLARATIONS ONLY) + README.md
src/database    CatalogPaths, CatalogDatabase, CatalogTransaction, SchemaMigration,
                SchemaMigrationRegistry, SchemaMigrator, migrations/Migration001InitialCatalog,
                SqliteComponentCatalog (implements IComponentCatalog),
                import/{ComponentJsonDocument, ComponentJsonSchema, ComponentJsonImporter}
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

## Runtime

Bootstrap opens the per-user SQLite catalog, runs Migration 001, and seeds `R1-A-0805-10K@1.0.0`
(USER/DRAFT) via the JSON import pipeline (idempotent), then opens "Untitled Project". UI:
File New/Open/Save/SaveAs/Close, View Zoom/Fit/Grid, Catalog Refresh/Import/Place (Insert key or
double-click). Placing records the exact version + content hash + fallback snapshot in the project
instance. `InMemoryComponentCatalog` remains as a test double only (no longer the production catalog).

## Not implemented (out of current scope)

Legacy-JSON mapper (deferred — no real legacy example on disk), datasheets/footprints/footprint_pads/
simulation_models/symbols tables, lifecycle activation beyond DRAFT, JSON export, backup/restore,
Schematic, Simulation, PCB, Manufacturing, Firmware, AI.

## Possible next steps (need approval)

- Windows/MSVC build + interactive GUI verification (presets authored; **NOT VERIFIED**).
- Downstream catalog tables (datasheets/footprints/symbols/simulation models) + their migrations.
- Legacy JSON import once a real legacy fixture is supplied.
- Splitting the `QUndoCommand` classes out of `MainWindow.cpp` into `src/ui/commands/`
  (functional today, deferred on purpose during MVP closing).

