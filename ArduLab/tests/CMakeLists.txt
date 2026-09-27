# ArduLab test target definitions.
# Each test binary links ONLY the public module target it verifies, so that a
# test cannot accidentally reach across a protected module boundary.

set(ARDULAB_TEST_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

function(ardulab_add_test name source)
    # Remaining arguments are the module libraries the test is allowed to use.
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${ARGN} Qt6::Test)
    target_compile_definitions(${name} PRIVATE
        ARDULAB_TEST_FIXTURES_DIR="${ARDULAB_TEST_FIXTURES_DIR}"
    )
    ardulab_apply_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
    # Qt Test binaries must not require a display server on CI machines.
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

# Milestone 1: build-system smoke test (proves Qt6::Test + toolchain wiring).
ardulab_add_test(BuildSkeletonTests   core/BuildSkeletonTests.cpp   Qt6::Core)

# Later milestones append their tests below this line.

# Milestone 2: Core Engine
ardulab_add_test(EventBusTests        core/EventBusTests.cpp        ardulab_core)

# Milestone 3: Component Engine foundation (no SQL, no Widgets)
ardulab_add_test(ComponentManagerTests components/ComponentManagerTests.cpp ardulab_components)
ardulab_add_test(PinAnchorTests        components/PinAnchorTests.cpp        ardulab_components)

# Milestone 4: Project System / .FAL compatibility
ardulab_add_test(FalCompatibilityTests project/FalCompatibilityTests.cpp    ardulab_project)

# Milestone 5: Canvas Engine (offscreen QPA, no project/components linkage)
ardulab_add_test(CoordinateSystemTests canvas/CoordinateSystemTests.cpp     ardulab_canvas)

# Milestone 6: Renderer isolation + Connection boundary contracts
ardulab_add_test(RendererIsolationTests ui/RendererIsolationTests.cpp      ardulab_ui ardulab_connection)

# Milestone 6: end-to-end foundation smoke (Plan Step 15) with screenshot output
ardulab_add_test(FoundationSmokeTests   app/FoundationSmokeTests.cpp       ardulab_ui)
target_compile_definitions(FoundationSmokeTests PRIVATE
    ARDULAB_SMOKE_OUTPUT_DIR="${CMAKE_BINARY_DIR}/smoke"
)

# Milestone 7: Phase 2 SQLite catalog + JSON import (temporary in-memory DBs)
ardulab_add_test(CatalogDatabaseTests      database/CatalogDatabaseTests.cpp      ardulab_database)
ardulab_add_test(SchemaMigratorTests       database/SchemaMigratorTests.cpp       ardulab_database)
ardulab_add_test(ComponentJsonImporterTests database/ComponentJsonImporterTests.cpp ardulab_database)
