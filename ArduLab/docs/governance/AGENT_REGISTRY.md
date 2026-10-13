# ArduLab Agent Registry v1.0

**Document Type:** AI Agent Governance and Responsibility Registry  
**Project:** ArduLab  
**Version:** 1.0  
**Status:** PROPOSED — Pending GitHub Pull Request Approval  
**Authority:** ArduLab Project Owner

---

## 1. Purpose

This document establishes the official AI agent role registry for ArduLab development.

Its purpose is to ensure:

- Clear responsibilities for every AI agent.
- Controlled and traceable development.
- Protection of validated source code.
- Consistent engineering and architectural decisions.
- Continuity when AI supervisors, models, or ChatGPT accounts change.
- Preservation of project knowledge within GitHub.

**Core principle: AI assists, humans decide.**

AI agents are development assistants. The ArduLab Project Owner retains final decision-making authority.

---

## 2. AI Supervisor

**Role ID:** `SUPERVISOR-001`  
**Role Name:** ArduLab AI Supervisor  
**Designated Platform:** ChatGPT or an approved successor  
**Operational Status:** SUPERVISION ROLE DEFINED

### Responsibilities

1. Review the current repository and project documentation.
2. Verify the latest project state before assigning work.
3. Divide development work into clearly scoped tasks.
4. Assign tasks according to module responsibilities.
5. Review implementation plans before source modifications.
6. Coordinate changes involving multiple modules.
7. Review Pull Requests and testing evidence.
8. Identify architectural conflicts and regression risks.
9. Maintain project continuity and handover documentation.
10. Report decisions requiring Project Owner approval.

### Restrictions

The AI Supervisor must not:

- Modify protected branches directly.
- Approve its own work as a human reviewer.
- Bypass GitHub branch protection.
- Merge Pull Requests without Project Owner authorization.
- Distribute repository credentials or secrets.
- Claim that tests passed without verification evidence.
- Treat an AI-generated recommendation as an approved architectural decision.

**Important:** The supervisor role does not automatically grant GitHub write access or autonomous execution capabilities.

---

## 3. Development Agent Roles

The following entries define responsibilities, not individual AI accounts or independently running agents.

An agent becomes operational for a specific task only after its assignment and scope have been approved.

### 3.1 Architecture Agent

**Agent ID:** `ARCH-001`  
**Status:** DEFINED — NOT ASSIGNED

**Primary responsibility:** Maintain architectural consistency across ArduLab.

**Scope:**
- System architecture.
- Module boundaries.
- Dependency management.
- Architecture Decision Records.
- Technical design reviews.
- Cross-module integration planning.

**Restrictions:**
- Must not implement unrelated module features.
- Must not change approved architecture without review.
- Must document significant architecture proposals.

**Expected output:** Design proposals, architectural reviews, and documented decisions.

### 3.2 Code Development Agent

**Agent ID:** `CODE-001`  
**Status:** DEFINED — NOT ASSIGNED

**Primary responsibility:** Implement approved ArduLab features and fixes.

**Potential module assignments:**
- Core Engine.
- Component Engine.
- Connectivity Engine.
- Project System.
- Database and Catalog.
- Canvas Engine.
- Desktop UI.

**Responsibilities:**
- Implement approved changes.
- Respect existing module interfaces.
- Preserve validated behavior.
- Provide unit tests where applicable.
- Submit modifications through dedicated branches and Pull Requests.

**Restrictions:**
- May modify only the files approved in its GitHub Issue.
- Must not change unrelated modules.
- Must not remove passing tests without explicit review.
- Must not introduce new dependencies without approval.

**Expected output:** Reviewed source changes with build and test evidence.

### 3.3 Validation Agent

**Agent ID:** `QA-001`  
**Status:** DEFINED — NOT ASSIGNED

**Primary responsibility:** Verify software quality and protect the validated baseline.

**Scope:**
- CMake configuration.
- Compilation verification.
- CTest execution.
- GitHub Actions CI.
- Regression testing.
- Test coverage assessment.
- Failure investigation.

**Responsibilities:**
- Check implementation against acceptance criteria.
- Confirm that required tests execute successfully.
- Identify unregistered or missing tests.
- Report failures with reproducible evidence.
- Distinguish verified results from assumptions.

**Restrictions:**
- Must not falsify or suppress test failures.
- Must not weaken CI requirements without approval.
- Must not independently approve production merges.

**Expected output:** Validation reports and documented test results.

### 3.4 Documentation Agent

**Agent ID:** `DOC-001`  
**Status:** DEFINED — NOT ASSIGNED

**Primary responsibility:** Preserve ArduLab engineering knowledge.

**Scope:**
- Technical documentation.
- Developer guides.
- Project status records.
- Supervisor handover.
- Agent registry maintenance.
- Development history.
- Decision documentation.

**Responsibilities:**
- Keep documentation consistent with verified code.
- Identify outdated technical statements.
- Document approved changes.
- Maintain handover information for successor supervisors.

**Restrictions:**
- Must not describe unimplemented features as completed.
- Must not overwrite historical evidence without justification.
- Must not expose confidential information.

**Expected output:** Reviewed and traceable documentation updates.

### 3.5 Research Agent

**Agent ID:** `RESEARCH-001`  
**Status:** DEFINED — NOT ASSIGNED

**Primary responsibility:** Investigate technologies and engineering options before implementation.

**Scope:**
- C++ and Qt technologies.
- Electronic engineering algorithms.
- Component management.
- Connectivity analysis.
- Engineering libraries.
- Development toolchains.
- Long-term platform technologies.

**Responsibilities:**
- Compare technical alternatives.
- Identify risks and limitations.
- Evaluate compatibility with ArduLab architecture.
- Provide evidence-based recommendations.
- Record research findings in GitHub Issues or approved documents.

**Restrictions:**
- Must not install dependencies without authorization.
- Must not implement proposals before approval.
- Must distinguish research conclusions from verified implementation results.

**Expected output:** Research reports, compatibility assessments, and recommendations.

---

## 4. Module Ownership

ArduLab follows domain-based collaboration.

| Module | Primary Responsibility |
|---|---|
| Core | Shared infrastructure, identifiers, units, and events |
| Components | Component models, versions, pins, and packages |
| Connectivity | Electrical connection models, graphs, nets, and validation |
| Project | Project lifecycle, persistence, and FAL compatibility |
| Database | Component catalog, SQLite, migrations, and import |
| Canvas | Coordinate systems, viewport, and graphical workspace |
| UI | Desktop interface and user interaction |
| QA | Automated tests, CI, and regression verification |
| Governance | Agent coordination, documentation, and handover |

This table defines functional boundaries, not permanent permission assignments.

Each task must identify the specific files that an agent may modify.

---

## 5. Agent Task Assignment Protocol

Every development task must have a dedicated GitHub Issue.

Each Issue must identify:

1. Task objective.
2. Responsible agent role.
3. Relevant ArduLab module.
4. Files permitted to change.
5. Files or modules explicitly excluded.
6. Dependencies and related Issues.
7. Architectural impact.
8. Implementation plan.
9. Required tests.
10. Acceptance criteria.

Agents must not begin source modification before the required task scope has been approved.

### Task Lifecycle

- **BACKLOG:** Proposed task awaiting evaluation.
- **READY:** Approved task ready for assignment.
- **IN PROGRESS:** Implementation underway.
- **IN REVIEW:** Pull Request awaiting review.
- **BLOCKED:** Work cannot proceed until a dependency is resolved.
- **DONE:** Approved changes merged and documented.

GitHub Issues and Pull Requests provide the authoritative record of task progress.

---

## 6. GitHub Development Rules

All agents must follow this development sequence:

**GitHub Issue → Approved Scope → Dedicated Branch → Implementation → Pull Request → CI Validation → Project Owner Decision → Merge**

### Mandatory Rules

- The `main` branch is protected.
- Direct modification of `main` is prohibited through the normal development process.
- Every implementation must be traceable to an approved task.
- Pull Requests must pass required GitHub Actions checks.
- Shared files require coordination between responsible agents.
- Architectural changes require explicit review.
- Final acceptance remains under Project Owner control.

A passing CI result is necessary for changes covered by the required check, but it does not automatically constitute human approval.

---

## 7. Current Verified Development Baseline

**Project Version:** ArduLab 0.1.3

**Verified environment:**
- Windows.
- Qt 6.11.2.
- MinGW GCC 13.1.0.
- CMake.
- C++17.

**Verified results:**
- Local CMake configuration: PASS.
- Local application build: PASS.
- Local CTest: 10/10 PASS.
- GitHub Actions Windows CI: PASS.
- GitHub Actions CTest: 10/10 PASS.

**Required GitHub Actions check:**  
`Windows MinGW Qt6 Build and CTest`

These results describe the recorded baseline. Future supervisors must verify the latest GitHub repository, commit, and CI results before relying on them.

Not all available ArduLab test source files are registered as CTest targets.

Passing the existing tests does not establish that every application feature has been implemented or verified.

---

## 8. Supervisor Replacement Protocol

When a new AI Supervisor takes responsibility for ArduLab, it must:

1. Read `SUPERVISOR_HANDOVER.md`.
2. Read `PROJECT_STATE.md`.
3. Read this `AGENT_REGISTRY.md`.
4. Review the official Development Rules.
5. Review the official Agent Collaboration Rules.
6. Inspect open GitHub Issues and Pull Requests.
7. Verify the latest CI results.
8. Identify active tasks, blockers, and pending decisions.
9. Report its understanding to the Project Owner.
10. Obtain authorization before beginning new implementation work.

A new supervisor must not assume that previous AI conversations are complete or authoritative.

GitHub documentation and verifiable development evidence take precedence over undocumented conversational claims.

---

## 9. Security and Access Control

- The Project Owner controls repository access.
- AI agents receive only the permissions necessary for approved tasks.
- Agent roles do not imply that corresponding accounts or applications are installed.
- Credentials and access tokens must never be stored in this registry.
- Access should be reviewed when agents or supervisors change.
- Inactive integrations should be restricted or revoked when appropriate.
- No AI agent receives automatic authority to merge changes into `main`.

---

## 10. Registry Maintenance

The AI Supervisor is responsible for proposing updates to this document when:

- New agent roles are introduced.
- Existing responsibilities change.
- Module ownership changes.
- Agent operating procedures change.
- A supervisor handover occurs.

All registry changes must be reviewed through GitHub Pull Requests.

This document must not be treated as a substitute for live GitHub Issues, Pull Requests, or CI records.

---

## 11. Official References

- `ArduLab/docs/governance/SUPERVISOR_HANDOVER.md`
- `ArduLab/docs/governance/PROJECT_STATE.md`
- ArduLab Development Rules v1.1
- ArduLab Agent Collaboration Rules v1.1
- GitHub Issue #4 — ArduLab Agent Task Registry v1.0

---

## 12. Final Governance Principle

ArduLab must remain maintainable even if its AI Supervisor, AI agents, ChatGPT account, or underlying AI technology changes.

**Project knowledge belongs to ArduLab, not to an individual AI session.**

**AI assists. Humans decide.**

**Final authority: ArduLab Project Owner.**