# ArduLab Project State — v1.0

**Status:** INITIAL SNAPSHOT / PROPOSED — verify on every handover; not a substitute for live GitHub state.  
**Snapshot date:** 2026-10-12 (user local date, UTC+07).  
**Repository:** https://github.com/kusonkasun-dotcom/ardulab-emergent.sh  
**Default branch:** `main`  
**Companion procedure:** [SUPERVISOR_HANDOVER.md](SUPERVISOR_HANDOVER.md)

## 1. What has been verified

| Area | Evidence / result | Limits |
|---|---|---|
| Project source | `ArduLab/` in the GitHub repository; Qt6 / C++17 / CMake; project version `0.1.3` | Repository also contains legacy/other top-level assets; do not delete them as part of housekeeping |
| Local Windows baseline | CMake configure PASS, full build PASS, CTest **10/10 PASS**, Windows Qt **6.11.2**, MinGW GCC **13.1.0** | Verified in a local checkout; output was inspected in the project conversation |
| GitHub Actions CI | Windows MSYS2 MINGW64 + Qt6 + CMake/Ninja: configure PASS, build PASS, CTest **10/10 PASS** | CI uses its own toolchain and does not verify interactive Windows GUI behavior |
| CI evidence | [ArduLab Windows CI run #1](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/actions/runs/38172941037), job `Windows MinGW Qt6 Build and CTest`, 10/10 in logs | Evidence is for the tested workflow/run; verify the newest `main` commit separately |
| Core/Connectivity baseline import | [Pull Request #1](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/pull/1) merged | Did not register all tests present in source tree |
| Windows CI workflow | [Pull Request #2](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/pull/2) merged; merge commit `96ea44611810018e4876bbd7553fe09b2bb55aef` | CI workflow dependencies can evolve independently |
| Branch governance | Ruleset [ArduLab - Protect Main Branch](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/settings/rules/24892381) created and shown Active, targeting `main` | Ruleset was configured through GitHub UI; verify it has not since changed |
| Required check | `Windows MinGW Qt6 Build and CTest` selected as required under status checks | Required reviewer approvals remained at **0** at snapshot time |
| GitHub visibility | **Public** | Never commit private data or credentials |

**Terminology:** `10/10 PASS` is ten registered CTest executable targets passed, **not** ten individual Qt test functions and **not** complete coverage of all implementation modules.

## 2. Registered CTest targets at the snapshot

These are listed in [`ArduLab/tests/CMakeLists.txt`](../../tests/CMakeLists.txt):

1. `BuildSkeletonTests`
2. `EventBusTests`
3. `ConnectivityGraphTests`
4. `ConnectivityGraphBehaviourTests`
5. `ConnectivityGraphNetBuildTests`
6. `ConnectivityGraphNetStabilityTests`
7. `ConnectivityManagerTests`
8. `ConnectivityValidatorTests`
9. `ValidationResultTests`
10. `NetIdentityTests`

## 3. Test sources not yet registered in CTest

A prior source inventory identified **22** test `.cpp` files, of which 10 were registered and **12 remained unregistered**. Recount after every test-registration change:

- `tests/app/FoundationSmokeTests.cpp`
- `tests/canvas/CoordinateSystemTests.cpp`
- `tests/components/ComponentManagerTests.cpp`
- `tests/components/PinAnchorTests.cpp`
- `tests/database/CatalogDatabaseTests.cpp`
- `tests/database/ComponentJsonImporterTests.cpp`
- `tests/database/SchemaMigratorTests.cpp`
- `tests/project/FalCompatibilityTests.cpp`
- `tests/project/ProjectRoundtripTests.cpp`
- `tests/ui/CursorSnapReadoutTests.cpp`
- `tests/ui/EditUndoRedoTests.cpp`
- `tests/ui/RendererIsolationTests.cpp`

Test registration is not the same as test success. Add target registrations incrementally, build, run targeted CTest, then run the complete suite. Do not assume previously unregistered tests will pass without investigation.

## 4. Confirmed areas requiring follow-up review

The following are observations from earlier read-only architecture/code audits, **not newly fixed issues**:

- **Project `.FAL` preservation:** unknown root-level data is preserved in some paths, while nested unknown-field round-trip behavior requires focused regression tests.
- **Unsaved work / dirty tracking:** UI Exit and some direct editing paths need review for consistent confirmation and dirty-state handling.
- **Connectivity-to-editor integration:** Connectivity Graph and tests exist, but integration with actual Canvas/Project/UI editor workflows is incomplete.
- **Net identity stability:** shortened hash and identity-construction choices require future collision/ambiguity threat analysis.
- **Legacy documentation drift:** `ArduLab/HANDOFF.md` cites a historical `14/14` Linux test run; `ArduLab/README.md` contains older Windows/MSVC notes. Do not treat these as the currently verified 10-target Windows baseline.
- **CI maintenance:** MINGW64 and `actions/checkout@v4` emitted deprecation warnings in the first GitHub run; address in separate, verified PRs.
- **GitHub ownership and permissions:** collaborator count, GitHub App installations and ChatGPT connector access are independent; the linked ChatGPT GitHub account was observed with read access but not push permissions for ArduLab. No specific user credentials should be written into this document.

These concerns should become scoped GitHub Issues before implementation. Severity, impact, and reproduction must be assessed for each issue; do not present them as already resolved.

## 5. Immediate recommended backlog (proposals, not approved tasks)

| Priority | Candidate task | Acceptance evidence |
|---|---|---|
| P0 | Adopt supervisor continuity documents in a docs-only PR | Review by owner, CI PASS, merge SHA recorded |
| P0 | Verify GitHub branch review/merge permissions for AI integrations | Least-privilege review, no unattended merge rights |
| P1 | Register `PinAnchorTests` as Phase 1.2 | New executable passes; old ten remain green; expected 11/11 if all pass |
| P1 | Activate other existing tests incrementally | Each target reports independent test result and full suite regression |
| P1 | Add `.FAL` nested-field regression coverage | Reproducible loss/no-loss expectations and approved fix plan |
| P1 | Verify exit/unsaved state behavior | Dedicated repeatable manual and/or automated test |
| P2 | Update GitHub Actions away from deprecated MINGW64 and Node20-dependent action | Separate toolchain PR; clean or documented warnings; all CI checks pass |
| P2 | Reconcile stale README/HANDOFF claims with measured state | Documentation PR, evidence and historical context retained |

## 6. Change ledger (append after accepted changes)

| Date | Baseline change | Commit / PR / CI evidence | Verified by |
|---|---|---|---|
| 2026-10-12 | Initial supervisor-continuity snapshot proposed | [PR #1](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/pull/1), [PR #2](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/pull/2), [CI #1](https://github.com/kusonkasun-dotcom/ardulab-emergent.sh/actions/runs/38172941037) | Conversation-based verification + GitHub source review; owner review pending |

## 7. Maintenance instructions

Before reporting any new baseline, record:

```text
Snapshot timestamp/timezone:
Repository + branch + current commit SHA:
CI workflow + job + run URL + outcome:
CMake configuration/build outcome:
CTest registered target count and pass/fail result:
Test source inventory changes:
Open issues and PRs affecting this state:
New risks / resolved risks + evidence:
Ruleset/access changes (exclude secrets and personal details):
Approval (owner / reviewer):
Next safe action:
```

If no evidence is available, write **NOT VERIFIED** rather than guessing. Update this record via a documentation PR, not by overwriting information on `main` outside review.

---

**Adoption:** Initial snapshot drafted for owner review. Once these files are merged, the reviewing supervisor must add the merged commit SHA and update any differences from live GitHub state.
