cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(experiments)

set(elf_file ${CMAKE_PROJECT_NAME}.elf CACHE INTERNAL "")

add_executable(${elf_file}
        main/main.cpp
)

#include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_subdirectory(src)
#add_subdirectory(main)


target_link_libraries(${elf_file} PUBLIC platform)
#
#add_subdirectory(main)

#list(APPEND _COMPONENTS main)

#get_property(_EXT_COMPONENTS TARGET platform PROPERTY IDF_EXTERNAL_COMPONENTS)
##list(APPEND _EXT_COMPONENTS ${CMAKE_CURRENT_LIST_DIR}/main)
#foreach(i ${_EXT_COMPONENTS})
#    idf_build_component(${i})
#endforeach()
#
#idf_build_process("esp32"
#        SDKCONFIG_DEFAULTS ${SDK_CONFIG_PATHS}
#        BUILD_DIR ${CMAKE_BINARY_DIR}
#        EXTRA_COMPONENT_DIRS ${_COMPONENTS}
#)
#idf_build_executable(${elf_file})

#target_link_libraries(${elf_file} PUBLIC platform)
