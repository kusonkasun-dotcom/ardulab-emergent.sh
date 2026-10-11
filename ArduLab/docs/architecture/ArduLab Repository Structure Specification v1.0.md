# ArduLab Repository Structure Specification v1.0

Status: Official Repository Architecture Document

## 1. Repository Philosophy

The ArduLab repository is not only a place to store source code. It
represents active development, technical knowledge, project history,
engineering decisions, and future evolution.

Principle:

> Preserve history, not confusion.

History is protected while active development remains clear and
maintainable.

------------------------------------------------------------------------

## 2. Human-Centered Repository Principle

ArduLab uses human-friendly terminology to organize technical
structures.

The structure should be understandable by:

-   engineers
-   learners
-   contributors
-   future generations

A good structure should be understandable by both machines and humans.

------------------------------------------------------------------------

## 3. Guided Evolution Migration Strategy

ArduLab follows a Guided Evolution Migration approach.

Previous work is not discarded. Validated work, knowledge, and
environment information are preserved as historical assets.

Progress means transforming experience into a stronger foundation.

------------------------------------------------------------------------

## 4. Target Repository Structure

``` text
ArduLab/

├── applications/
│   └── home/
│       └── ArduLab/
│           ├── src/
│           ├── include/
│           ├── resources/
│           ├── tests/
│           ├── cmake/
│           └── CMakeLists.txt
│
├── intelligence/
│   └── assistant/
│
├── knowledge/
│   ├── library/
│   ├── history/
│   └── examples/
│
├── community/
├── services/
├── web/
├── docs/
├── tools/
├── archive/
├── README.md
└── LICENSE
```

------------------------------------------------------------------------

## 5. Folder Responsibilities

### applications/home

The primary ArduLab environment where users create projects, learn
electronics, experiment, and improve designs.

### intelligence/assistant

Contains ArduLab AI Assistant development.

AI supports:

-   workspace analysis
-   recommendations
-   reports

Principle:

> AI assists, humans decide.

### knowledge

Contains reusable and educational knowledge.

library: - components - symbols - footprints - templates

history: - project evolution - design changes - experiments -
improvements

examples: - educational projects - reference designs

### community

Future collaboration and knowledge sharing area.

### services

Supporting systems and optional integrations.

ArduLab core should remain functional without external services.

### web

Web-based extensions and future collaboration interfaces.

### docs

Official documentation:

``` text
docs/
├── vision/
├── architecture/
├── development/
├── ADR/
└── features/
```

### tools

Development utilities.

### archive

Historical materials that preserve previous development stages.

Archive is not active development.

------------------------------------------------------------------------

## 6. Active Development and Historical Separation

ArduLab separates:

Active development: - applications - intelligence - knowledge

Historical archive: - archive

This prevents confusion while preserving knowledge.

------------------------------------------------------------------------

## 7. Environment Preservation Policy

Preserve:

-   compiler version
-   Qt version
-   dependencies
-   build instructions
-   testing procedures

Do not store:

-   build folders
-   temporary files
-   generated binaries
-   local cache

------------------------------------------------------------------------

## 8. Module Ownership Principle

Each area has clear responsibility:

-   applications: core application
-   intelligence: AI Assistant
-   knowledge: reusable resources
-   docs: architecture and rules
-   services: supporting systems

------------------------------------------------------------------------

## 9. Future Expansion Principle

The repository must support future growth:

-   simulation engine
-   PCB development
-   advanced AI systems
-   educational platform
-   collaboration ecosystem

Future expansion should extend the structure without destroying existing
foundations.

------------------------------------------------------------------------

## 10. ArduLab Evolution Principle

Every stage of ArduLab development is part of its identity.

A stronger future is created by understanding the past.

> Every improvement is built upon previous experience.

------------------------------------------------------------------------

# Document Status

ArduLab Repository Structure Specification v1.0

Status:

Approved Foundation Architecture
