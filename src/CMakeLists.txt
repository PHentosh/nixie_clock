cmake_minimum_required(VERSION 3.28)

add_library(platform INTERFACE)



add_subdirectory(mcp23017)
add_subdirectory(core)
add_subdirectory(bal)
add_subdirectory(wifi)
add_subdirectory(main)

target_link_libraries(platform INTERFACE
        mcp23017
        _core
        bal
        wifi
)

#add_subdirectory(main)

#list(APPEND _COMPONENTS main)
#
#get_property(_EXT_COMPONENTS TARGET platform PROPERTY IDF_EXTERNAL_COMPONENTS)
#list(APPEND _EXT_COMPONENTS ${CMAKE_CURRENT_LIST_DIR}/main)
#foreach(i ${_EXT_COMPONENTS})
#    idf_build_component(${i})
#endforeach()
#
#idf_build_process("esp32"
#        SDKCONFIG_DEFAULTS ${SDK_CONFIG_PATHS}
#        BUILD_DIR ${CMAKE_BINARY_DIR}
#        EXTRA_COMPONENT_DIRS ${_COMPONENTS}
#)
#
#idf_build_executable(${elf_file})
#set_property(DIRECTORY APPEND PROPERTY TARGET_SRCS ${CMAKE_CURRENT_LIST_DIR}/main/main_stub.c)
