# ArduLab Supervisor Continuity & Handover — v1.0

**Status:** PROPOSED — effective only after the project owner's approval and merge through a Pull Request.  
**Last drafted:** 2026-10-12  
**Scope:** Human project owner, AI supervisor(s), AI agents, and human developers.  
**Companion record:** [PROJECT_STATE.md](PROJECT_STATE.md)  
**Repository:** https://github.com/kusonkasun-dotcom/ardulab-emergent.sh

## 1. Purpose

Make ArduLab independently maintainable when a ChatGPT account, model, tool provider, supervisor, or developer changes. GitHub files, Issues, Pull Requests, test results, and approved decisions are the durable project record. Chat memory and chat transcripts are **not** the authoritative project record.

**Operating principle:** AI assists; humans decide. A new supervisor must be able to resume safely without access to previous conversations.

## 2. Authority and source-of-truth order

1. **Project owner:** final authority for roadmap, permissions, architectural exceptions, releases, and merging substantial changes.
2. **Approved project standards:** [Development Rules v1.1](../development/ArduLab%20Development%20Rules%20v1.1.md), [Agent Collaboration Rules v1.1](../development/ArduLab%20Agent%20Collaboration%20Rules%20v1.1.md), and approved architecture specifications and ADRs.
3. **Current Git state:** `main`, active Pull Requests, Issues, code, test registrations, workflow definitions, and CI results.
4. **[PROJECT_STATE.md](PROJECT_STATE.md):** timestamped snapshot and outstanding work; **verify against current GitHub state** before relying on it.
5. **AI recommendations:** proposals only, never automatic overrides of the above.

If standards, implementation, test results, and the state record disagree, **stop**, identify the discrepancy, report it, and ask the owner to approve resolution. Do not silently rewrite history to make reports consistent.

## 3. Roles and approval boundaries

| Role | Responsibilities | Restrictions |
|---|---|---|
| Project Owner | Scope, priority, permission grants, acceptance, final merge decisions | Should not need to manually reassemble source files |
| AI Supervisor | Task allocation, dependency coordination, architecture review, risk management, handover, evidence review | No self-approval of high-impact changes; no implied permission to merge |
| Engineering Agents | Research, tests, implementation on isolated branches, draft PRs | No unrelated file edits; no independent changes to shared files without coordination |
| QA / Integration Agent | Reproducible build/test verification, change review, CI diagnostics | A PASS is not proof of correctness outside tested coverage |
| GitHub Actions | Automated builds, tests, required status checks | CI passing does not substitute for human product/architecture approval |

An AI account or integration has only the repository permissions actually granted to it. Do not assume GitHub write access from the ability to read a public repository. Never share, record, or commit passwords, OAuth tokens, API keys, personal access tokens, or private credentials.

**Important limitation at this snapshot:** GitHub's `main` ruleset requires a PR and CI, but zero reviewer approvals; the project owner's final approval is currently a **process rule**, not a fully enforced GitHub review gate. Re-evaluate access policy before assigning unattended write/merge capabilities to AI agents.

## 4. First-session onboarding for a replacement supervisor

Complete these checks **before making or requesting code changes**:

1. Read this document, [PROJECT_STATE.md](PROJECT_STATE.md), and current `ArduLab/HANDOFF.md` and `ArduLab/README.md`. Note that historical documents may be stale.
2. Read the approved development, collaboration, and architecture rules. The target architecture in the repository structure specification is an **evolution goal**, not proof that the current repository has already been restructured.
3. Verify the repository identity, visibility, default branch, and newest commits. Verify that `main` has no unreviewed local changes.
4. Inspect **open** GitHub Issues and Pull Requests, including check runs, comments, and pending reviews.
5. Confirm the `ArduLab - Protect Main Branch` ruleset and the required `Windows MinGW Qt6 Build and CTest` check are still active.
6. Compare the current GitHub Actions workflow and registered tests with the baseline recorded in `PROJECT_STATE.md`; do not trust historic suite counts in isolation.
7. If a working environment is available, configure, build, and run CTest using a compatible Qt/MinGW setup. Record exact command, environment, SHA, result, and timestamp.
8. Report discrepancies, blockers, and the next safe task. Request the owner's approval before implementation or permission changes.

**Handover acceptance gate:** the new supervisor produces a brief verification report with repo + branch/SHA, active PRs, baseline CI outcome, known risks, and a proposed next task. The owner explicitly accepts the handover.

## 5. Controlled work cycle for all AI agents

**Before implementation:** create or reference an Issue/task contract recording: objective, module owner, file scope, dependencies, architecture impact, expected behavior, negative cases, test plan, rollback plan, and human approval status.

**During work:**

- Use one focused branch per change (for example `test/pin-anchor`, `fix/fal-roundtrip`, `docs/supervisor-handover`).
- Never make independent concurrent edits to the same file from separate agents; coordinate shared files such as `ArduLab/CMakeLists.txt` and `ArduLab/tests/CMakeLists.txt`.
- Preserve original module boundaries, validated behavior, fixtures, and backward compatibility unless an explicit change is approved.
- Keep generated build trees, runtime databases, Qt Creator local state, binaries, and secrets out of commits.
- Do not merge directly to `main`; submit a PR and wait for required checks and owner review.

**On completion:** report implementation, affected files, build result, test result, CI URL, risks, and any changes to `PROJECT_STATE.md`. A claimed PASS requires actual output/evidence. If the result fails or is unverified, label it clearly.

## 6. CI and environment policy

- Repository application sources: `ArduLab/src/`; tests: `ArduLab/tests/`; CMake source directory: `ArduLab/`.
- Current tracked workflow: [`.github/workflows/ardulab-windows-ci.yml`](../../../.github/workflows/ardulab-windows-ci.yml), Windows runner with MSYS2 MINGW64, CMake + Ninja, Qt6, `ctest --output-on-failure`.
- A separate previously verified local setup used Windows Qt 6.11.2 with MinGW GCC 13.1.0; **do not assume the CI runner uses those exact versions**.
- The workflow's required job name must remain stable while it is referenced by branch rules. If renamed, coordinate changes to the Ruleset and validate that merges are not blocked.
- As of this draft, there were deprecation warnings for MSYS2 MINGW64 and Node.js 20-based `actions/checkout@v4`; migrate in a separate tested PR, not by silent toolchain replacement.
- Test suite coverage is incomplete: CTest counts are executable targets, not a count of all individual Qt test methods or all `tests/*.cpp` files.

## 7. Continuity record lifecycle

Update [PROJECT_STATE.md](PROJECT_STATE.md) whenever one of the following occurs:

- Baseline build/CTest result, CI job name, or toolchain changes;
- A major feature or architectural contract is accepted;
- A new risk, regression, or blocking Issue appears;
- An agent or supervisor is reassigned;
- A permission, visibility, or branch protection policy changes;
- A handover takes place.

Each entry should contain a date/time, commit SHA, evidence link, what was verified versus inferred, responsible reviewer, and remaining limitations. Prefer short, auditable updates over vague claims.

## 8. Normal supervisor-to-supervisor handover

1. Finish or clearly suspend open agent tasks; prevent uncoordinated writes.
2. Update `PROJECT_STATE.md` with newest SHA, checks, Issues, PRs, known risks, and next recommended task.
3. Confirm that all approved work is committed to a branch or PR and that uncommitted work is not silently lost.
4. Transfer **references to** GitHub resources, not private credentials.
5. Replacement supervisor performs onboarding (Section 4) independently.
6. Owner acknowledges the report and authorizes the next task.

## 9. Emergency takeover (previous supervisor unavailable)

1. Suspend new merges and write-heavy agent operations until repository state is known.
2. Check repository Settings: collaborators, installed GitHub Apps, Actions, branch rules, secrets exposure and access changes. Do not change permissions without owner approval unless immediate account-security action is necessary and authorized.
3. Inspect `main`, pending PRs, Issues, and last green CI run; distinguish tested commit SHA from an untested current head.
4. Re-run or review baseline verification; open an Issue for any inconsistency.
5. Draft a recovery plan and seek project-owner approval. Never overwrite `main` from an old ZIP or working copy just to recover earlier test results.

## 10. Task-completion and handover template

```text
TASK / HANDOVER ID:
Date / timezone:
Repository / base branch / current SHA:
Supervisor / agent role:
Approved objective:
Files changed / open PRs / Issues:
Architecture impact / dependencies:
Environment and exact build command:
Build result (PASS / FAIL / NOT RUN) + evidence:
Test result (PASS / FAIL / NOT RUN) + count + evidence:
GitHub Actions status + run link:
Known risks, blockers, unresolved discrepancies:
Owner review / acceptance status:
Next safe action:
```

## 11. Non-goals and boundaries

This policy does **not** enable autonomous/background execution, grant GitHub write permissions, require a particular AI vendor, or replace GitHub access controls. It does **not** establish that ArduLab is feature-complete or production-safe simply because CI passes. Work remains owner-directed.

---

**Adoption:** This document is a proposed v1.0. Upon owner approval, commit it in a documentation-only branch, pass CI, and merge by PR. Record the resulting commit in `PROJECT_STATE.md`.
