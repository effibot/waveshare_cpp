# Waveshare C++ Library - Test Suite (using Catch2)
# Tests are run using Catch2's native test discovery, not CTest

# Set CMake policy for linker search path behavior
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Copy bridge_config.json from config/ to build root directory for tests
if(EXISTS "${PROJECT_SOURCE_DIR}/config/bridge_config.json")
    configure_file(
        "${PROJECT_SOURCE_DIR}/config/bridge_config.json"
        "${PROJECT_BINARY_DIR}/bridge_config.json"
        COPYONLY
    )
    message(STATUS "Copied config/bridge_config.json to build directory for tests")
endif()

# Find or fetch Catch2
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0  # Use latest stable version
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Find source files for tests
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB CANOPEN_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/canopen/*.cpp")

# Build tests (Catch2 provides its own test runner)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain waveshare_cpp)
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
endforeach()

# CANopen tests
foreach(test_src ${CANOPEN_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain waveshare_canopen waveshare_cpp)
    target_include_directories(${test_name} PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/canopen  # Make test_utils_canopen.hpp available
    )
endforeach()
