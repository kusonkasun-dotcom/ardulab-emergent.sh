# ArduLab — Handoff (MVP closing checkpoint)

Date of this checkpoint: see the Git commit that carries this file.
Product version string: `0.1.3`.

## 1. What is done

| Area | State | Verified how |
|---|---|---|
| Phase 1 — core, project/`.FAL`, canvas, component engine, thin UI | COMPLETE | `ctest` (Linux/GCC, offscreen) |
| Phase 2 — SQLite catalog, migration framework, canonical JSON import | COMPLETE | `ctest` (Linux/GCC, offscreen) |
| MVP polish — cursor snap readout | COMPLETE | `CursorSnapReadoutTests` (zoom, pan, rotation, pixel tolerance) |
| MVP polish — undo/redo command stack | COMPLETE | `EditUndoRedoTests` (place, drag, rotate, delete, value, reference, dirty flag) |
| MVP polish — `.FAL` roundtrip integrity | COMPLETE | `ProjectRoundtripTests` (identity, snapshot, changed/absent catalog, corrupt file) |
| Windows / MSVC build | **NOT VERIFIED** | presets + instructions authored only |
| Interactive (on-screen) GUI use | **NOT VERIFIED** | all runs are headless `offscreen` QPA |

## 2. Actual test result (do not quote older numbers)

```
cd /app/ArduLab
cmake --build build/linux-debug
ctest --test-dir build/linux-debug --output-on-failure
```

→ **14/14 suites passed · 108 test functions · 0 failed · 0 skipped** (Qt 6.4.2, GCC 12, Debug).

Per suite (test functions incl. init/cleanup): BuildSkeleton 4 · EventBus 14 ·
ComponentManager 12 · PinAnchor 7 · FalCompatibility 9 · ProjectRoundtrip 8 ·
CoordinateSystem 13 · RendererIsolation 5 · FoundationSmoke 3 · CatalogDatabase 9 ·
SchemaMigrator 6 · ComponentJsonImporter 7 · EditUndoRedo 5 · CursorSnapReadout 6.

## 3. MVP behaviour contracts

**Cursor snap readout** (`MainWindow::updateSnapReadout`)
- `Snap: x, y mm` = pointer position converted to millimeters and quantised to
  `Project::canvas().gridMm` (default 1.0 mm). Coordinates are **always millimeters**.
- `Pin <instance>·<pin> @ x, y mm` = nearest pin anchor within **12 screen pixels**
  (`kAnchorTolerancePx`), measured in viewport pixels so the tolerance stays constant at any
  zoom. The anchor is taken through the item's own transform, so rotation is honoured.
- Hovering/snapping is geometric assistance only: **no net, wire or connection is created.**

**Undo/redo** (single `QUndoStack` owned by `MainWindow`)
- `PlaceInstanceCommand`, `DeleteInstanceCommand` (keeps a full instance copy),
  `MoveInstanceCommand`, `RotateInstanceCommand`, `RenameInstanceCommand` (display name),
  `ChangeValueCommand` (engineering value), `ChangeReferenceCommand` (reference designator).
- Commands only call `MainWindow` edit primitives, which update the project model **and** the
  scene item together, so undo/redo can never desynchronise the two.
- One drag = one step: press records the start position, release pushes a single
  `MoveInstanceCommand` with the grid-snapped drop position.
- Dirty indicator: `QUndoStack::cleanChanged` drives `Project::markClean/markDirty`; Save calls
  `setClean()`. Save → no `*`; Undo below the clean index → `*`; Redo back to it → no `*`.
- `ProjectOpened`/`ProjectClosed` clear the stack (no cross-document undo).

**`.FAL` roundtrip**
- Instance identity (`instance_id`), library id, `value`, `name`, `position`, `rotation_deg`,
  `catalog_reference` (scope, component id, exact version id, content hash) and the
  `fallback_snapshot` all survive save → close → reopen.
- Unknown root sections and unknown instance fields are preserved verbatim.
- Catalog changed (version bumped) or unavailable → instance opens as `Unresolved` with a
  warning; the recorded version is **never** substituted with another one.
- `ProjectService::open` is transactional: parse + resolve happen first, so a corrupt or missing
  file returns `ParseFailure` / `IoFailure` and the currently open document stays intact.

## 4. Known limitations / deliberate deviations

1. Windows/MSVC is unverified; the `windows-msvc-*` presets and README steps are authored blind.
2. No interactive GUI verification — only headless offscreen runs.
3. `value` is free text; there is no unit parsing or per-category parameter validation.
4. Reference designators are validated for non-emptiness and uniqueness only (no prefix rules).
5. Legacy component-JSON mapper deferred (no real legacy example on disk).
6. `MainWindow.cpp` still hosts the `QUndoCommand` subclasses; extracting them to
   `src/ui/commands/` is a deliberate post-MVP task.
7. Schematic/connections, simulation, PCB, manufacturing, firmware and AI are **not started**.

## 5. Where to resume

1. Run the Windows/MSVC build and the interactive GUI pass; mark them verified here.
2. Then, with approval: schematic/connection core, or the downstream catalog tables
   (datasheets/footprints/symbols/simulation models) + migrations.
