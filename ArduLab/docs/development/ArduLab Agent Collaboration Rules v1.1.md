# ArduLab Agent Collaboration Rules v1.1

Status: Official Multi-Agent Development Standard\
Applies to: Human Developers and AI Agents

------------------------------------------------------------------------

# 1. Purpose

This document defines collaboration rules for contributors working on
ArduLab.

Goals:

-   maintain architectural consistency
-   prevent conflicting changes
-   define responsibilities
-   support controlled evolution
-   maintain project history

------------------------------------------------------------------------

# 2. Multi-Agent Philosophy

ArduLab follows domain-based collaboration.

Each agent has a responsibility area.

Every agent must:

-   understand architecture before changes
-   respect module ownership
-   preserve validated functionality
-   provide testing evidence

No agent should modify unrelated areas without justification.

------------------------------------------------------------------------

# 3. Project Architect Agent

Responsibility:

-   maintain overall direction
-   review major architectural changes
-   maintain architecture documents
-   resolve cross-module conflicts
-   protect ArduLab vision

The Architect Agent guides decisions but does not unnecessarily modify
owned implementation areas.

------------------------------------------------------------------------

# 4. Home Application Agent

Ownership:

applications/home/ArduLab

Responsibilities:

-   desktop engineering environment
-   workspace integration
-   user project environment
-   application-level coordination

------------------------------------------------------------------------

# 5. Core Engineering Agent

Responsibilities:

-   identifiers
-   units
-   common infrastructure
-   shared foundations

Rules:

Core must remain independent from:

-   UI
-   database
-   cloud services

------------------------------------------------------------------------

# 6. Component Knowledge Agent

Responsibilities:

-   component models
-   pins
-   packages
-   library structures
-   component resources

Focus:

Building reusable electronic knowledge resources.

------------------------------------------------------------------------

# 7. Connectivity Agent

Responsibilities:

-   ConnectionPoint
-   Wire
-   Net
-   ConnectivityGraph
-   ConnectivityManager
-   ConnectivityValidator
-   NetIdentity
-   ValidationResult

Existing connectivity behavior and tests are protected assets.

------------------------------------------------------------------------

# 8. Knowledge Agent

Responsibilities:

-   examples
-   learning resources
-   reusable engineering knowledge
-   project knowledge organization

------------------------------------------------------------------------

# 9. AI Assistant Agent

Ownership:

intelligence/assistant

Responsibilities:

-   workspace analysis
-   recommendations
-   reports
-   intelligent assistance

Rules:

AI remains optional.

ArduLab core must remain functional offline without AI dependency.

------------------------------------------------------------------------

# 10. Community Agent

Future responsibility:

-   shared knowledge
-   collaboration features
-   community resources

Community features must support ArduLab learning goals.

------------------------------------------------------------------------

# 11. Change Request Protocol

Before modifying code, report:

1.  Module
2.  Files affected
3.  Purpose
4.  Architectural impact
5.  Testing plan

After completion:

1.  Implementation summary
2.  Files changed
3.  Build result
4.  Test result
5.  Remaining concerns

------------------------------------------------------------------------

# 12. Shared File Conflict Rules

Priority order:

1.  Architecture integrity
2.  Existing tests
3.  Module ownership
4.  New feature requirements

Shared files require coordination.

------------------------------------------------------------------------

# 13. Forbidden Actions

Agents must not:

-   delete working tests without analysis
-   remove validated modules without approval
-   replace architecture without review
-   introduce unrelated dependencies
-   ignore documented decisions

------------------------------------------------------------------------

# 14. Long-Term Collaboration Goal

Multi-agent development is not only about speed.

The goal is:

-   architectural stability
-   code quality
-   traceability
-   reproducibility
-   knowledge preservation

All contributors work toward the same ArduLab vision.

------------------------------------------------------------------------

# Document Status

ArduLab Agent Collaboration Rules v1.1

Status:

Approved Evolution Standard
