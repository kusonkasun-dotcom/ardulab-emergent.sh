# ArduLab Development Rules v1.1

Status: Official Development Standard\
Applies to: Human Developers and AI Agents

------------------------------------------------------------------------

# 1. Project Identity

ArduLab is an offline-first electronic technology development and
learning platform.

ArduLab provides an environment for:

-   electronic design
-   component management
-   connectivity analysis
-   engineering experimentation
-   project learning
-   AI-assisted recommendations

ArduLab is designed not only as software, but as a platform where humans
understand, create, improve, and share technology.

------------------------------------------------------------------------

# 2. Development Philosophy

All development decisions must support the long-term ArduLab vision.

Core principles:

-   Human-centered engineering
-   Modular architecture
-   Knowledge preservation
-   Continuous improvement
-   Reproducible development

Technology decisions must consider both technical quality and human
usability.

------------------------------------------------------------------------

# 3. Historical Foundation and Evolution

ArduLab originated from the Emergent.sh foundation project.

The foundation phase remains part of ArduLab history.

Existing validated work must be preserved when possible.

Principle:

> Preserve history, not confusion.

Previous implementations may become references, but future development
follows the approved ArduLab architecture.

------------------------------------------------------------------------

# 4. Architecture Principles

ArduLab follows modular architecture.

Each module must have:

-   Clear responsibility
-   Independent interface
-   Controlled dependencies
-   Implementation
-   Automated tests

No module should become a container for unrelated functionality.

------------------------------------------------------------------------

# 5. Home Application Principle

The primary ArduLab environment is the Home Application.

The Home Application is the place where users:

-   create electronic projects
-   learn concepts
-   experiment
-   improve designs

Internal modules may include:

-   core
-   components
-   connection
-   project
-   database
-   canvas
-   UI

Repository naming and application structure should follow the ArduLab
Repository Structure Specification.

------------------------------------------------------------------------

# 6. AI Assistant Development Rule

AI is an assistant, not a controller.

AI responsibilities:

-   analyze workspace history
-   generate reports
-   provide recommendations
-   identify possible improvements

Human users remain responsible for:

-   choosing direction
-   accepting changes
-   making final decisions

Principle:

> AI assists, humans decide.

------------------------------------------------------------------------

# 7. Development Workflow

For important changes:

1.  Understand current architecture
2.  Identify affected modules
3.  Review dependencies
4.  Design solution
5.  Replace or modify carefully
6.  Build
7.  Run tests
8.  Report results

Blind modification is prohibited.

------------------------------------------------------------------------

# 8. Full Replacement Preference

For important source files:

Analyze file → Design replacement → Review replacement → Replace
complete file → Build → Test

Controlled replacement is preferred over uncontrolled partial editing.

------------------------------------------------------------------------

# 9. Testing Standard

Existing passing tests are protected assets.

Every new feature requires:

Implementation + Validation

Testing layers:

-   Unit Test
-   Integration Test
-   Future System Test

No feature is considered complete without verification.

------------------------------------------------------------------------

# 10. Documentation and Knowledge Preservation

Important decisions must be documented.

Documentation should preserve:

-   architecture decisions
-   development reasoning
-   migration history
-   future considerations

Development knowledge is part of ArduLab's long-term value.

------------------------------------------------------------------------

# 11. Git and Repository Rules

Git should contain:

-   source code
-   documentation
-   required resources
-   tests

Do not include:

-   build folders
-   temporary files
-   generated binaries
-   local environment cache

------------------------------------------------------------------------

# 12. Completion Report

Every completed task should report:

## Implementation

Summary of changes.

## Files Changed

Affected files.

## Build Result

PASS or FAIL.

## Test Result

PASS or FAIL.

## Remaining Concerns

Known limitations or future work.

------------------------------------------------------------------------

# Document Status

ArduLab Development Rules v1.1

Status:

Approved Evolution Standard

Purpose:

Maintain ArduLab development quality while preserving its history,
vision, and future growth.
