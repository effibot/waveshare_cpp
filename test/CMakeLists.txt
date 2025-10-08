# CMakeLists.txt for Waveshare USB-CAN-A Tests
cmake_minimum_required(VERSION 3.14)

# Set the project name
project(waveshare_tests CXX)

# Specify C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
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

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Add source files from the main project
set(WAVESHARE_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/config_frame.cpp
    ${CMAKE_SOURCE_DIR}/../src/fixed_frame.cpp
    ${CMAKE_SOURCE_DIR}/../src/variable_frame.cpp
)

# Create test executables
add_executable(test_config_frame
    test_config_frame.cpp
    ${WAVESHARE_SOURCES}
)

add_executable(test_fixed_frame
    test_fixed_frame.cpp
    ${WAVESHARE_SOURCES}
)

add_executable(test_variable_frame
    test_variable_frame.cpp
    ${WAVESHARE_SOURCES}
)

# Link against Catch2
target_link_libraries(test_config_frame PRIVATE Catch2::Catch2WithMain)
target_link_libraries(test_fixed_frame PRIVATE Catch2::Catch2WithMain)
target_link_libraries(test_variable_frame PRIVATE Catch2::Catch2WithMain)

# Enable testing
enable_testing()

# Register the tests with CTest
add_test(NAME ConfigFrameTests COMMAND test_config_frame)
add_test(NAME FixedFrameTests COMMAND test_fixed_frame)
add_test(NAME VariableFrameTests COMMAND test_variable_frame)

# Optional: Add Catch2 test discovery for individual test case reporting
# This is optional and only works if Catch2 provides the Catch.cmake module
if(Catch2_FOUND)
    include(Catch)
    catch_discover_tests(test_config_frame)
    catch_discover_tests(test_fixed_frame)
    catch_discover_tests(test_variable_frame)
endif()
