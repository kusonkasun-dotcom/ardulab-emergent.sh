# ArduLab test target definitions.


set(ARDULAB_TEST_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")


function(ardulab_add_test name source)


    add_executable(${name} ${source})


    target_link_libraries(${name}
        PRIVATE
        ${ARGN}
        Qt6::Test
    )


    target_compile_definitions(${name} PRIVATE
        ARDULAB_TEST_FIXTURES_DIR="${ARDULAB_TEST_FIXTURES_DIR}"
    )


    target_include_directories(${name}
        PRIVATE
        ${ARDULAB_SRC}
    )


    ardulab_apply_warnings(${name})


    add_test(
        NAME ${name}
        COMMAND ${name}
    )


    set_tests_properties(
        ${name}
        PROPERTIES
        ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen"
    )


endfunction()



# -------------------------------------------------------
# Core Tests
# -------------------------------------------------------

ardulab_add_test(
    BuildSkeletonTests
    core/BuildSkeletonTests.cpp
    ardulab_core
)

ardulab_add_test(
    EventBusTests
    core/EventBusTests.cpp
    ardulab_core
)



# -------------------------------------------------------
# Connection Core Tests
# -------------------------------------------------------

ardulab_add_test(
    ConnectivityGraphTests
    connection/ConnectivityGraphTests.cpp
    ardulab_connection
)


ardulab_add_test(
    ConnectivityGraphBehaviourTests
    connection/ConnectivityGraphBehaviourTests.cpp
    ardulab_connection
)


ardulab_add_test(
    ConnectivityGraphNetBuildTests
    connection/ConnectivityGraphNetBuildTests.cpp
    ardulab_connection
)


ardulab_add_test(
    ConnectivityGraphNetStabilityTests
    connection/ConnectivityGraphNetStabilityTests.cpp
    ardulab_connection
)


ardulab_add_test(
    ConnectivityManagerTests
    connection/ConnectivityManagerTests.cpp
    ardulab_connection
)

ardulab_add_test(
    ConnectivityValidatorTests
    connection/ConnectivityValidatorTests.cpp
    ardulab_connection
)

# -------------------------------------------------------
# Validation Tests
# -------------------------------------------------------

ardulab_add_test(
    ValidationResultTests
    connection/ValidationResultTests.cpp
    ardulab_connection
)



# -------------------------------------------------------
# Net Identity Tests
# -------------------------------------------------------

ardulab_add_test(
    NetIdentityTests
    connection/NetIdentityTests.cpp
    ardulab_connection
)

# -------------------------------------------------------
# Components Tests
# -------------------------------------------------------

ardulab_add_test(
    PinAnchorTests
    components/PinAnchorTests.cpp
    ardulab_components
)