cmake_minimum_required(VERSION 3.22.1)

project("wowmobile")

add_library(
        wowmobile
        SHARED

        wow_engine.cpp
        wow_renderer.cpp
)

find_library(
        log-lib
        log
)

find_library(
        android-lib
        android
)

find_library(
        gles-lib
        GLESv3
)

find_library(
        egl-lib
        EGL
)

target_link_libraries(
        wowmobile

        ${log-lib}
        ${android-lib}
        ${gles-lib}
        ${egl-lib}
)
