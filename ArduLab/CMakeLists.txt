cmake_minimum_required(VERSION 3.21)

project(ArduLab
    VERSION 0.1.3
    DESCRIPTION "ArduLab - offline-first Electronic Engineering Platform"
    LANGUAGES CXX
)

# --------------------------------------------------------------------------
# Language baseline (Plan §2.1: C++17; C++20 requires an explicit decision)
# --------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(ARDULAB_BUILD_TESTS "Build ArduLab unit/regression tests" ON)
option(ARDULAB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ArduLabWarnings)

# --------------------------------------------------------------------------
# Qt6 (Plan §11: Core, Gui, Widgets; Sql is deferred to the database phase)
# --------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)
if(ARDULAB_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

set(ARDULAB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# --------------------------------------------------------------------------
# Module targets. Each module is a separate static library so that the
# allowed-dependency table (Plan §3.1) is enforced by the linker:
#
#   core        -> Qt6::Core
#   components  -> core                    (no Sql, no Widgets)
#   project     -> core, components        (no Widgets)
#   canvas      -> core, Qt6::Widgets      (no project, no components)
#   ui          -> canvas, components, project, Qt6::Widgets
#   app (exe)   -> everything above
#
# Module blocks are appended milestone by milestone below.
# --------------------------------------------------------------------------

# ==== MODULE TARGETS BEGIN ====

# ---- core (Milestone 2) --------------------------------------------------
add_library(ardulab_core STATIC
    ${ARDULAB_SRC}/core/Result.h
    ${ARDULAB_SRC}/core/Error.h
    ${ARDULAB_SRC}/core/Identifiers.h
    ${ARDULAB_SRC}/core/Units.h
    ${ARDULAB_SRC}/core/Event.h
    ${ARDULAB_SRC}/core/EventBus.h
    ${ARDULAB_SRC}/core/EventBus.cpp
)
target_include_directories(ardulab_core PUBLIC ${ARDULAB_SRC})
target_link_libraries(ardulab_core PUBLIC Qt6::Core)
ardulab_apply_warnings(ardulab_core)

# ---- components (Milestone 3) --------------------------------------------
# Component Engine: depends on core only. Must never link Qt6::Sql/Widgets.
add_library(ardulab_components STATIC
    ${ARDULAB_SRC}/components/Component.h
    ${ARDULAB_SRC}/components/ComponentVersion.h
    ${ARDULAB_SRC}/components/ComponentParameter.h
    ${ARDULAB_SRC}/components/Package.h
    ${ARDULAB_SRC}/components/Pin.h
    ${ARDULAB_SRC}/components/PinAnchor.h
    ${ARDULAB_SRC}/components/ComponentSnapshot.h
    ${ARDULAB_SRC}/components/ComponentSearchCriteria.h
    ${ARDULAB_SRC}/components/IComponentCatalog.h
    ${ARDULAB_SRC}/components/IComponentManager.h
    ${ARDULAB_SRC}/components/IComponentImporter.h
    ${ARDULAB_SRC}/components/ImportReport.h
    ${ARDULAB_SRC}/components/ComponentManager.h
    ${ARDULAB_SRC}/components/ComponentManager.cpp
    ${ARDULAB_SRC}/components/InMemoryComponentCatalog.h
    ${ARDULAB_SRC}/components/InMemoryComponentCatalog.cpp
)
target_link_libraries(ardulab_components PUBLIC ardulab_core)
ardulab_apply_warnings(ardulab_components)

# ---- project (Milestone 4) -----------------------------------------------
# Project domain + .FAL adapter: depends on core and component *value* types.
add_library(ardulab_project STATIC
    ${ARDULAB_SRC}/project/Project.h
    ${ARDULAB_SRC}/project/Project.cpp
    ${ARDULAB_SRC}/project/ProjectMetadata.h
    ${ARDULAB_SRC}/project/ProjectService.h
    ${ARDULAB_SRC}/project/ProjectService.cpp
    ${ARDULAB_SRC}/project/FalDocument.h
    ${ARDULAB_SRC}/project/FalSerializer.h
    ${ARDULAB_SRC}/project/FalSerializer.cpp
)
target_link_libraries(ardulab_project PUBLIC ardulab_core ardulab_components)
ardulab_apply_warnings(ardulab_project)

# ---- canvas (Milestone 5) ------------------------------------------------
# Engineering workspace: depends on core + Qt Gui/Widgets. No project, no
# components, no persistence.
add_library(ardulab_canvas STATIC
    ${ARDULAB_SRC}/canvas/CoordinateSystem.h
    ${ARDULAB_SRC}/canvas/CoordinateSystem.cpp
    ${ARDULAB_SRC}/canvas/ViewportController.h
    ${ARDULAB_SRC}/canvas/ViewportController.cpp
    ${ARDULAB_SRC}/canvas/A3CanvasScene.h
    ${ARDULAB_SRC}/canvas/A3CanvasScene.cpp
    ${ARDULAB_SRC}/canvas/A3CanvasView.h
    ${ARDULAB_SRC}/canvas/A3CanvasView.cpp
)
target_link_libraries(ardulab_canvas PUBLIC ardulab_core Qt6::Gui Qt6::Widgets)
ardulab_apply_warnings(ardulab_canvas)

# ---- connection (Milestone 6, contract-only) -----------------------------
# Header-only value contracts protecting the future Connection Core boundary.
add_library(ardulab_connection INTERFACE)
target_sources(ardulab_connection INTERFACE
    ${ARDULAB_SRC}/connection/CatalogPinRef.h
    ${ARDULAB_SRC}/connection/ConnectionPoint.h
    ${ARDULAB_SRC}/connection/Wire.h
    ${ARDULAB_SRC}/connection/Net.h
)
target_link_libraries(ardulab_connection INTERFACE ardulab_core ardulab_components)

# ---- ui (Milestone 6) ----------------------------------------------------
# Thin presentation: MainWindow + Renderer adapter. No Sql, no JSON parsing.
add_library(ardulab_ui STATIC
    ${ARDULAB_SRC}/ui/MainWindow.h
    ${ARDULAB_SRC}/ui/MainWindow.cpp
    ${ARDULAB_SRC}/ui/ComponentGraphicsItem.h
    ${ARDULAB_SRC}/ui/ComponentGraphicsItem.cpp
)
target_link_libraries(ardulab_ui PUBLIC ardulab_canvas ardulab_components ardulab_project Qt6::Widgets)
ardulab_apply_warnings(ardulab_ui)

# ---- database (Milestone 7: Phase 2 SQLite catalog + JSON import) --------
# SQLite catalog adapter, migration framework, and JSON importer. Depends on
# core + components (domain) + Qt Sql. Implements IComponentCatalog and
# IComponentImporter; the UI never links this module (it uses the interfaces).
add_library(ardulab_database STATIC
    ${ARDULAB_SRC}/database/CatalogPaths.h
    ${ARDULAB_SRC}/database/CatalogPaths.cpp
    ${ARDULAB_SRC}/database/CatalogDatabase.h
    ${ARDULAB_SRC}/database/CatalogDatabase.cpp
    ${ARDULAB_SRC}/database/CatalogTransaction.h
    ${ARDULAB_SRC}/database/SchemaMigration.h
    ${ARDULAB_SRC}/database/SchemaMigrationRegistry.h
    ${ARDULAB_SRC}/database/SchemaMigrationRegistry.cpp
    ${ARDULAB_SRC}/database/SchemaMigrator.h
    ${ARDULAB_SRC}/database/SchemaMigrator.cpp
    ${ARDULAB_SRC}/database/SqliteComponentCatalog.h
    ${ARDULAB_SRC}/database/SqliteComponentCatalog.cpp
    ${ARDULAB_SRC}/database/migrations/Migration001InitialCatalog.h
    ${ARDULAB_SRC}/database/migrations/Migration001InitialCatalog.cpp
    ${ARDULAB_SRC}/database/import/ComponentJsonDocument.h
    ${ARDULAB_SRC}/database/import/ComponentJsonSchema.h
    ${ARDULAB_SRC}/database/import/ComponentJsonSchema.cpp
    ${ARDULAB_SRC}/database/import/ComponentJsonImporter.h
    ${ARDULAB_SRC}/database/import/ComponentJsonImporter.cpp
)
target_link_libraries(ardulab_database PUBLIC ardulab_core ardulab_components Qt6::Core Qt6::Sql)
ardulab_apply_warnings(ardulab_database)

# ==== MODULE TARGETS END ====

# --------------------------------------------------------------------------
# Application executable (composition root)
# --------------------------------------------------------------------------
qt_add_executable(ArduLab
    ${ARDULAB_SRC}/main.cpp
    ${ARDULAB_SRC}/app/ApplicationBootstrap.h
    ${ARDULAB_SRC}/app/ApplicationBootstrap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/ardulab.qrc
)
target_include_directories(ArduLab PRIVATE ${ARDULAB_SRC})
target_compile_definitions(ArduLab PRIVATE ARDULAB_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(ArduLab PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
set_target_properties(ArduLab PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)
ardulab_apply_warnings(ArduLab)

# ==== APP MODULE LINKS BEGIN ====
target_link_libraries(ArduLab PRIVATE ardulab_core ardulab_components ardulab_project ardulab_canvas ardulab_ui ardulab_database)
# ==== APP MODULE LINKS END ====

# --------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------
if(ARDULAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
