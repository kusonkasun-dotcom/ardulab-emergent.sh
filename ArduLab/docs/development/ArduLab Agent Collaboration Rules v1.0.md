# ArduLab Agent Collaboration Rules v1.0

Status: Official Multi-Agent Development Standard\
Applies to: All Human Developers and AI Agents

------------------------------------------------------------------------

# 1. Purpose

This document defines the collaboration framework for all contributors
working on ArduLab.

The purpose is to:

-   define module ownership
-   prevent conflicting changes
-   maintain architectural consistency
-   establish communication standards between agents
-   ensure controlled project evolution

All development activities must follow this document together with:

**ArduLab Development Rules v1.0**

------------------------------------------------------------------------

# 2. Agent Development Philosophy

ArduLab follows a domain-based multi-agent development model.

Each agent owns a specific technical domain.

An agent must:

-   understand existing architecture before changes
-   modify only its assigned responsibility area
-   preserve existing validated functionality
-   provide testing evidence after implementation

No agent should modify unrelated modules without architectural
justification.

------------------------------------------------------------------------

# 3. Project Architect Agent

## Ownership

Project-wide architecture.

## Responsibilities

-   maintain overall project direction
-   review major architectural changes
-   maintain architecture documentation
-   resolve conflicts between modules
-   ensure future development follows ArduLab vision

## Restrictions

The Architect Agent should not unnecessarily modify implementation
details owned by other agents.

------------------------------------------------------------------------

# 4. Core Engine Agent

## Ownership

Location:

src/core

## Responsibilities

-   identifier system
-   unit system
-   common utilities
-   event infrastructure
-   shared foundation services

## Dependencies

Core modules should remain independent and reusable.

## Restrictions

Core must not depend directly on:

-   UI layer
-   database layer
-   cloud services

------------------------------------------------------------------------

# 5. Component System Agent

## Ownership

Location:

src/components

## Responsibilities

-   electronic component model
-   pin definitions
-   package definitions
-   catalog structures
-   component management logic

## Dependencies

Uses:

-   Core Engine

------------------------------------------------------------------------

# 6. Connectivity Agent

## Ownership

Locations:

src/connection

tests/connection

## Responsibilities

-   ConnectionPoint
-   Wire
-   Net
-   ConnectivityGraph
-   ConnectivityManager
-   ConnectivityValidator
-   NetIdentity
-   ValidationResult

## Development Rule

Existing connectivity behaviour and tests are protected assets.

Current validated baseline:

9/9 tests passed

Any change must preserve existing behaviour or provide documented
reasons for changes.

------------------------------------------------------------------------

# 7. Project and Persistence Agent

## Ownership

Locations:

src/project

src/database

## Responsibilities

-   project lifecycle
-   serialization
-   FAL format
-   database management
-   schema migration

## Restrictions

Persistence changes must consider backward compatibility.

------------------------------------------------------------------------

# 8. Canvas and UI Agent

## Ownership

Locations:

src/canvas

src/ui

## Responsibilities

-   schematic visualization
-   graphics rendering
-   user interaction
-   editor interface

## Rule

UI layer must not contain core business logic.

Business rules belong to domain modules.

------------------------------------------------------------------------

# 9. AI Integration Agent

## Ownership

Future location:

src/ai

## Responsibilities

-   AI engineering assistant
-   design recommendations
-   automation features
-   intelligent analysis

## Important Rule

AI features are optional extensions.

ArduLab core functionality must remain available offline without AI
dependency.

------------------------------------------------------------------------

# 10. Backend and Web Agent

## Ownership

Locations:

backend

frontend

## Responsibilities

-   cloud services
-   documentation portal
-   online catalog
-   collaboration platform

## Important Rule

Web infrastructure supports ArduLab.

It does not replace the Qt desktop engineering platform.

------------------------------------------------------------------------

# 11. Change Request Protocol

Before modifying code, an agent must report:

## Module

The affected domain.

## Files

Exact files that will be changed.

## Reason

Why the change is required.

## Impact

Possible effects on other modules.

## Testing Plan

Tests required to validate the change.

Example:

Module: Connection Core

Files: ConnectivityGraph.cpp

Reason: Improve net traversal

Impact: Connectivity behaviour

Testing: ConnectivityGraphTests

------------------------------------------------------------------------

# 12. Shared File Conflict Rules

When multiple agents need the same file:

Priority order:

1.  Architecture integrity
2.  Existing tests
3.  Module ownership
4.  New feature requirement

Shared files require coordination before modification.

------------------------------------------------------------------------

# 13. Completion Report Standard

Every completed task must provide:

## Implementation

Summary of changes.

## Files Changed

List of modified files.

## Build Result

PASS or FAIL.

## Test Result

PASS or FAIL.

## Remaining Concerns

Known limitations or future work.

------------------------------------------------------------------------

# 14. Forbidden Actions

Agents must not:

-   delete working tests without analysis
-   remove existing modules without approval
-   replace core architecture without review
-   introduce unrelated dependencies
-   perform large refactoring without documentation
-   ignore existing design decisions

------------------------------------------------------------------------

# 15. Long-Term Collaboration Goal

The purpose of multi-agent development is not only speed.

The goal is maintaining:

-   architectural stability
-   code quality
-   traceability
-   reproducibility
-   professional engineering standards

All agents contribute toward the same ArduLab vision.

------------------------------------------------------------------------

# Document Status

ArduLab Agent Collaboration Rules v1.0

Status:

Approved Foundation Standard

Purpose:

Define responsibilities and collaboration rules for human and AI
contributors working on ArduLab.
