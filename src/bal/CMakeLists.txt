cmake_minimum_required(VERSION 3.28)

add_library(bal STATIC)
target_sources(bal PRIVATE
        board.cpp
        buttons.cpp
        buzzer.cpp
        dial.cpp
)
target_include_directories(bal PUBLIC include)
target_link_libraries(bal PUBLIC idf::esp_wifi idf::nvs_flash mcp23017 _core)
