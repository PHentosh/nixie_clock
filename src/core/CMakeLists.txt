cmake_minimum_required(VERSION 3.28)

add_library(_core STATIC)
target_sources(_core PRIVATE
        osal.cpp
)
target_include_directories(_core PUBLIC include)
target_link_libraries(_core PUBLIC idf::freertos)
