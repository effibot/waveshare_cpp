# CMakeLists.txt for Waveshare USB-CAN-A Tests
cmake_minimum_required(VERSION 3.14)

# Set the project name
project(waveshare_cpp VERSION 0.1 LANGUAGES CXX)

# Specify C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-unused-variable -O2)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/scripts)

# Add option to build tests
option(BUILD_TESTS "Build tests" ON)

# Add source files from the main project
add_subdirectory(src)

# Add scripts subdirectory
add_subdirectory(scripts)

# Build main executable
add_executable(main main.cpp)
#add_executable(wave_reader scripts/wave_reader.cpp)
target_link_libraries(main PRIVATE waveshare_cpp)
#target_link_libraries(wave_reader PRIVATE waveshare_cpp)

# Enable testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()



