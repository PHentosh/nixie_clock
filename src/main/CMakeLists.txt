set(CMAKE_CXX_EXTENSIONS ON)
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
# Add external components to build process
get_property(_EXT_COMPONENTS TARGET platform PROPERTY IDF_EXTERNAL_COMPONENTS)
list(APPEND _EXT_COMPONENTS ${CMAKE_CURRENT_LIST_DIR}/main)
foreach(i ${_EXT_COMPONENTS})
    idf_build_component(${i})
endforeach()

# IDF build process
get_property(_IDF_COMPONENTS TARGET platform PROPERTY IDF_COMPONENTS)
idf_build_process("esp32"
        SDKCONFIG_DEFAULTS ${SDK_CONFIG_PATHS}
        BUILD_DIR ${CMAKE_BINARY_DIR}
        COMPONENTS main esp_wifi ${_IDF_COMPONENTS}
)
idf_build_executable(${elf_file})
# Entrance point
target_sources(${elf_file} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/main/main_stub.c)
#set_property(DIRECTORY APPEND PROPERTY TARGET_SRCS ${CMAKE_CURRENT_LIST_DIR}/main/main_stub.c)


set(mapfile "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
target_link_options(${elf_file} PRIVATE "-Wl,--cref")
target_link_options(${elf_file} PRIVATE "-Wl,--Map=${mapfile}")
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${mapfile}")

# Write project description JSON file
set(PROJECT_EXECUTABLE ${elf_file})
idf_build_get_property(common_component_reqs __COMPONENT_REQUIRES_COMMON)
make_json_list("${common_component_reqs}" common_component_reqs_json)
make_json_list("${build_component_info};${test_component_info}" build_component_info_json)
make_json_list("${all_component_info};${test_component_info}" all_component_info_json)
make_json_list("${build_components};${test_components}" build_components_json)
make_json_list("${build_component_paths};${test_component_paths}" build_component_paths_json)
configure_file("${idf_path}/tools/cmake/project_description.json.in" "${CMAKE_BINARY_DIR}/project_description.json")
