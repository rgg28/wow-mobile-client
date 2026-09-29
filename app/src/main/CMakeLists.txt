cmake_minimum_required(VERSION 3.22.1)

project("wowmobile")

add_library(
        wowmobile
        SHARED
        wow_engine.cpp
)

find_library(
        log-lib
        log
)

find_library(
        android-lib
        android
)

target_link_libraries(
        wowmobile
        ${log-lib}
        ${android-lib}
)
