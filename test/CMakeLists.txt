include(CTest)

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

# Register the tests for Catch2
include(Catch)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain waveshare_cpp)
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    catch_discover_tests(${test_name})
endforeach()
