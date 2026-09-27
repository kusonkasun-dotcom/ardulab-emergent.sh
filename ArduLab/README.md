# ArduLab — Clean Foundation (v0.1.3, Phase 1 + Phase 2 + MVP polish)

Offline-first Electronic Engineering Platform. Native Qt6/C++17 desktop application.

This tree is the **clean-room foundation** produced under the approved
*ArduLab Clean Foundation Implementation Plan v1.0*: Phase 1 (core/project/canvas/
components/UI), Phase 2 (SQLite catalog + migrations + canonical JSON import) and the
MVP polish set (cursor snap readout, undo/redo, `.FAL` roundtrip hardening).

## Build (Windows, primary target) — NOT YET VERIFIED

Prerequisites:

| Dependency | Version / notes |
|---|---|
| Visual Studio 2022 | MSVC v143 toolset, "Desktop development with C++" |
| CMake | ≥ 3.21 (the VS-bundled one works) |
| Ninja | ≥ 1.10 (bundled with VS, or `winget install Ninja-build.Ninja`) |
| Qt | 6.5 – 6.8, kit `msvc2019_64` / `msvc2022_64` |
| Qt modules | **Core, Gui, Widgets, Sql, Test** (Sql brings the bundled `qsqlite` driver) |

```powershell
# From an "x64 Native Tools Command Prompt for VS 2022"
set CMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
build\windows-debug\ArduLab.exe
```

Deploy the Qt runtime next to the executable with
`windeployqt build\windows-debug\ArduLab.exe` (this also copies
`sqldrivers\qsqlite.dll`, which the catalog needs).

The per-user catalog is created at
`%LOCALAPPDATA%\ArduLab\catalog\ardulab_catalog.sqlite3`.

## Build (Linux, CI / verification)

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug          # runs headless (QT_QPA_PLATFORM=offscreen)
QT_QPA_PLATFORM=offscreen ./build/linux-debug/ArduLab
```

Options: `-DARDULAB_BUILD_TESTS=OFF`, `-DARDULAB_WARNINGS_AS_ERRORS=ON`.

## Module map (each is a static library; dependencies are enforced by the linker)

| Target | Directory | May depend on |
|---|---|---|
| `ardulab_core` | `src/core` | Qt6::Core |
| `ardulab_components` | `src/components` | core |
| `ardulab_project` | `src/project` | core, components |
| `ardulab_canvas` | `src/canvas` | core, Qt6::Gui, Qt6::Widgets |
| `ardulab_connection` (INTERFACE) | `src/connection` | core, components — **contract-only** |
| `ardulab_ui` | `src/ui` | canvas, components, project |
| `ardulab_database` | `src/database` | core, components, Qt6::Sql |
| `ArduLab` (exe) | `src/main.cpp`, `src/app` | all of the above |

Forbidden edges (Plan §7) hold: components ✗ Widgets/Sql · canvas ✗ project ·
ui ✗ SQL/JSON parsing · connection has no `.cpp`. `Qt6::Sql` is linked **only** by
`ardulab_database`.

## Scope delivered

- **Core Engine** — `Result<T>`/`Error`, typed IDs, millimeter units, `EventBus`.
- **Project System** — `Project`, `ProjectService` (new/open/save/saveAs/close, atomic write,
  **transactional open**: a corrupt file never replaces the open document),
  `.FAL` serializer with legacy-shape compatibility and unknown-field preservation.
- **Canvas Engine** — `CoordinateSystem` (mm ↔ scene), `ViewportController` (bounded zoom/pan),
  `A3CanvasScene` (420 × 297 mm, grid, frame), `A3CanvasView` (input adapter).
- **Component Engine** — `Component`, `ComponentVersion`, `Package`, `Pin`, `PinAnchor`,
  `ComponentSnapshot`, `IComponentCatalog`, `IComponentManager`, `ComponentManager`.
- **Catalog (Phase 2)** — per-user SQLite catalog, migration framework + Migration 001,
  `SqliteComponentCatalog`, canonical component JSON importer (imports as DRAFT, skips
  duplicates, never overwrites).
- **UI** — thin `MainWindow` (menus, catalog dock, status bar), `ComponentGraphicsItem`.
- **MVP polish** —
  *Cursor snap readout*: grid-snapped mm plus nearest pin anchor within a 12 px screen
  tolerance (correct after zoom/pan/rotation; forms no electrical net).
  *Undo/redo*: place, move (one drag = one step), rotate, delete, and property changes
  (display name, value, reference designator); the title-bar `*` follows the stack's clean index.
  *Roundtrip*: save → close → reopen keeps identity and the embedded snapshot; a changed or
  missing catalog version opens read-only as `Unresolved`, never substituted.
- **Connection boundary** — `CatalogPinRef`, declaration-only `ConnectionPoint`/`Wire`/`Net`.

Not implemented (by design): Schematic/connections, Simulation, PCB, Manufacturing,
Firmware, AI, legacy-JSON mapper (no real fixture supplied).

## Tests

`ctest` runs **14 suites / 108 test functions** headlessly (`QT_QPA_PLATFORM=offscreen`):
build skeleton, core event bus, component manager, PinAnchor, `.FAL` compatibility,
`.FAL` roundtrip (incl. changed/unavailable catalog and corrupt-file safety), coordinate
system + viewport, renderer isolation + connection contracts, foundation smoke (writes
`build/<preset>/smoke/ardulab-foundation-smoke.png`), catalog database, schema migrator,
JSON importer, undo/redo editing, and the cursor snap readout.

Sample data used by the app and the tests:
`resources/examples/Resistor.schema-1.0.json` (canonical component JSON),
`resources/LegacyEmptyProject.FAL` and `tests/fixtures/*.FAL` / `*.json`.
