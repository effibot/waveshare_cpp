file(GLOB WAVESHARE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(waveshare_cpp ${WAVESHARE_SOURCES})
target_include_directories(waveshare_cpp PUBLIC ${PROJECT_SOURCE_DIR}/include)