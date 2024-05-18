cmake_minimum_required(VERSION 3.28)

add_library(mcp23017 STATIC)
target_sources(mcp23017 PRIVATE
        mcp23017.cpp
)
target_include_directories(mcp23017 PUBLIC include)
target_link_libraries(mcp23017 PUBLIC idf::driver)
