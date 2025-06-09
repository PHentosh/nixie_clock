cmake_minimum_required(VERSION 3.28)

add_library(wifi STATIC)
target_sources(wifi PRIVATE
        RTC_time.cpp
)
target_include_directories(wifi PUBLIC include)
target_link_libraries(wifi PRIVATE _core idf::esp_wifi idf::nvs_flash)
