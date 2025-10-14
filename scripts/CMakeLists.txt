file(GLOB WAVESHARE_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# add scripts as executables
foreach(script_src ${WAVESHARE_SCRIPTS})
    get_filename_component(script_name ${script_src} NAME_WE)
    add_executable(${script_name} ${script_src})
    target_link_libraries(${script_name} PRIVATE waveshare_cpp)
    target_include_directories(${script_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
endforeach()