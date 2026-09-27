# ArduLab — Clean Foundation (v0.1.3, Phase 1)

Offline-first Electronic Engineering Platform. Native Qt6/C++17 desktop application.

This tree is the **clean-room foundation** produced under the approved
*ArduLab Clean Foundation Implementation Plan v1.0*. It implements Phase 1 only.

## Build (Windows, primary target)

Prerequisites: Visual Studio 2022 (MSVC v143), CMake ≥ 3.21, Ninja, Qt 6.x
(`msvc2019_64` or `msvc2022_64` kit with Core/Gui/Widgets/Test).

```powershell
# From an "x64 Native Tools Command Prompt for VS 2022"
set CMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
build\windows-debug\ArduLab.exe
```

Deploy Qt runtime next to the executable with `windeployqt build\windows-debug\ArduLab.exe`.

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
| `ArduLab` (exe) | `src/main.cpp`, `src/app` | all of the above |

Forbidden edges (Plan §7) hold: components ✗ Widgets/Sql · canvas ✗ project ·
ui ✗ SQL/JSON parsing · connection has no `.cpp`. `Qt6::Sql` is not linked anywhere
yet — the SQLite catalog is the next phase.

## Phase 1 scope delivered

- **Core Engine** — `Result<T>`/`Error`, typed IDs, millimeter units, `EventBus`.
- **Project System** — `Project`, `ProjectService` (new/open/save/saveAs/close, atomic write),
  `.FAL` serializer with legacy-shape compatibility and unknown-field preservation.
- **Canvas Engine** — `CoordinateSystem` (mm ↔ scene), `ViewportController` (bounded zoom/pan),
  `A3CanvasScene` (420 × 297 mm, grid, frame), `A3CanvasView` (input adapter).
- **Component Engine foundation** — `Component`, `ComponentVersion`, `Package`, `Pin`,
  `PinAnchor`, `ComponentSnapshot`, `IComponentCatalog`, `IComponentManager`,
  `ComponentManager`, in-memory catalog adapter.
- **UI** — thin `MainWindow` (menus, catalog dock, status bar), `ComponentGraphicsItem` renderer adapter.
- **Connection boundary** — `CatalogPinRef`, declaration-only `ConnectionPoint`/`Wire`/`Net`.

Not implemented (by design): SQLite, JSON import, Schematic, Simulation, PCB,
Manufacturing, Firmware, AI.

## Tests

`ctest` runs 8 suites / 60+ cases: build skeleton, core, component manager, PinAnchor,
`.FAL` compatibility, coordinate system + viewport, renderer isolation + connection
contracts, and an end-to-end foundation smoke that writes
`build/<preset>/smoke/ardulab-foundation-smoke.png`.
