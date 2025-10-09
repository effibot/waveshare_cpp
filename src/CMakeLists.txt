# Header-only library (all frame implementations are in headers)
add_library(waveshare_cpp INTERFACE)
target_include_directories(waveshare_cpp INTERFACE ${PROJECT_SOURCE_DIR}/include)