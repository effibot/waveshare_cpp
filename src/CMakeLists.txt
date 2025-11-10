# ==============================================================================
# Waveshare Core Library
# ==============================================================================
file(GLOB WAVESHARE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(waveshare_cpp ${WAVESHARE_SOURCES})
target_include_directories(waveshare_cpp PUBLIC ${PROJECT_SOURCE_DIR}/include)

# ==============================================================================
# CANopen Library  
# ==============================================================================
file(GLOB CANOPEN_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/canopen/*.cpp")
add_library(waveshare_canopen ${CANOPEN_SOURCES})
target_include_directories(waveshare_canopen PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(waveshare_canopen PUBLIC waveshare_cpp)

# Find and link nlohmann_json for ObjectDictionary
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(waveshare_canopen PUBLIC nlohmann_json::nlohmann_json)
else()
    message(WARNING "nlohmann_json not found via CMake, assuming header-only in system paths")
endif()