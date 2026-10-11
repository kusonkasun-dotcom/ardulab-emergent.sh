# ArduLab Development Rules.md v1.0

Status: Official Development Standard\
Applies to: All Human Developers and AI Agents

------------------------------------------------------------------------

# 1. Project Identity

## Project Name

ArduLab

## Project Vision

ArduLab is an offline-first Electronic Engineering Platform designed to
provide an integrated environment for:

-   schematic design
-   electronic component management
-   circuit connectivity analysis
-   simulation preparation
-   engineering assistance
-   future AI-assisted electronics workflows

ArduLab aims to evolve into a complete engineering platform while
maintaining reliability, transparency, and professional software
engineering practices.

------------------------------------------------------------------------

# 2. Origin and Development Direction

## Historical Foundation

ArduLab originated from the Emergent.sh generated project foundation.

Emergent.sh provided:

-   initial repository structure
-   development templates
-   documentation foundation
-   frontend/backend foundation

Emergent.sh is considered the historical foundation, not the final
architectural authority.

## Current Product Direction

The primary product is:

ArduLab Desktop Application

Technology:

-   C++17
-   Qt6
-   CMake
-   SQLite

The desktop engineering platform remains the core direction.

Future systems such as cloud services, web portals, AI services, and
collaboration features must support the ArduLab core vision and must not
replace the desktop engineering foundation.

------------------------------------------------------------------------

# 3. Core Architecture Principle

ArduLab follows a modular architecture.

Each module must have:

-   Clear Responsibility
-   Independent Interface
-   Implementation
-   Automated Test

No module should become a dependency container for unrelated
functionality.

------------------------------------------------------------------------

# 4. Current Core Modules

## Core Module

Location:

src/core

Responsibility:

-   identifiers
-   units
-   event system
-   common infrastructure

## Component Module

Location:

src/components

Responsibility:

-   electronic components
-   pins
-   packages
-   component catalog model

## Connection Module

Location:

src/connection

Responsibility:

-   connection points
-   wires
-   nets
-   connectivity graph
-   validation foundation

Current implemented foundation:

-   ConnectionPoint
-   Wire
-   Net
-   ConnectivityGraph
-   ConnectivityManager
-   ConnectivityValidator
-   NetIdentity
-   ValidationResult

## Project Module

Location:

src/project

Responsibility:

-   project management
-   serialization
-   persistence workflow

## Database Module

Location:

src/database

Responsibility:

-   SQLite storage
-   schema migration
-   catalog database

## Canvas/UI Module

Location:

src/canvas\
src/ui

Responsibility:

-   schematic visualization
-   user interaction
-   rendering layer

------------------------------------------------------------------------

# 5. Development Rules for AI Agents

Every AI agent working on ArduLab MUST follow these rules.

## Rule 001 --- Understand Before Editing

Before modifying any file, the agent must identify:

1.  Current architecture role
2.  Dependencies
3.  Existing tests
4.  Possible impact

No blind modification is allowed.

## Rule 002 --- Full Replacement Preferred

For important source files:

Analyze file → Design replacement → Review replacement → Replace
complete file → Build → Run tests

Avoid uncontrolled partial edits.

## Rule 003 --- Preserve Existing Working Features

Existing passing tests are protected assets.

Current baseline:

9/9 tests passed

A new implementation must not break existing validated behavior.

## Rule 004 --- Test Required

Every new feature requires:

Implementation + Unit Test

No feature is considered complete without validation.

------------------------------------------------------------------------

# 6. Testing Standard

Testing layers:

## Unit Test

Technology:

-   Qt Test
-   CTest

Location:

ArduLab/tests

Every module should eventually have:

ModuleName

-   Source
-   Header
-   Tests

------------------------------------------------------------------------

# 7. File Ownership Rules

## Source Code

Owned by:

src/

Only production code belongs here.

## Tests

Owned by:

tests/

Tests must not contain production logic.

## Documentation

Owned by:

docs/

Architecture decisions must be documented.

------------------------------------------------------------------------

# 8. Emergent.sh Integration Rules

Emergent.sh resources may be used for:

-   web services
-   documentation portal
-   cloud infrastructure
-   future collaboration system

The following are prohibited:

-   replacing Qt desktop architecture with web architecture
-   removing validated C++ modules
-   ignoring existing design decisions

------------------------------------------------------------------------

# 9. Git Repository Rules

Repository must not include:

-   build/
-   \*.obj
-   \*.exe
-   CMakeCache.txt
-   .qtcreator/

Only source, documentation, and required resources belong in Git.

------------------------------------------------------------------------

# 10. Agent Collaboration Protocol

Before making changes, every agent must report:

1.  Files affected
2.  Purpose of change
3.  Architectural impact
4.  Testing plan

After completion:

1.  Build result
2.  Test result
3.  Remaining concerns

------------------------------------------------------------------------

# 11. Long-Term Vision

Future ArduLab development may include:

ArduLab Desktop

-   Component Intelligence
-   Simulation Engine
-   AI Engineering Assistant
-   Cloud Collaboration Platform

All future development must maintain:

-   engineering accuracy
-   modular architecture
-   reproducibility
-   testability

------------------------------------------------------------------------

# Document Status

ArduLab Development Rules v1.0

Status:

Approved Foundation Standard

Purpose:

Prevent architectural drift and maintain project vision across human and
AI contributors.
