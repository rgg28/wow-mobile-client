cmake_minimum_required(VERSION 3.22.1)

project("wowmobile")

# ============================================================
# WoW Mobile + WoWee Android
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Ubicación de WoWee
#
# GitHub Actions debe dejar WoWee en:
#
#   ${CMAKE_SOURCE_DIR}/../../../../wowee
#
# Desde:
#   app/src/main/
#
# Si el workflow utiliza otra ubicación puede sobrescribirse:
#
#   -DWOWEE_SOURCE_DIR=/ruta/a/WoWee
# ------------------------------------------------------------

set(WOWEE_SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/../../../wowee"
        CACHE PATH "WoWee source directory"
)

if(NOT EXISTS "${WOWEE_SOURCE_DIR}/src")
    message(FATAL_ERROR
            "WoWee no encontrado en: ${WOWEE_SOURCE_DIR}\n"
            "GitHub Actions debe descargar Kelsidavis/WoWee."
    )
endif()

message(STATUS "==============================================")
message(STATUS "WoW Mobile native build")
message(STATUS "WoWee source: ${WOWEE_SOURCE_DIR}")
message(STATUS "ABI: ${ANDROID_ABI}")
message(STATUS "==============================================")


# ============================================================
# Android libraries
# ============================================================

find_library(LOG_LIB log)
find_library(ANDROID_LIB android)
find_library(VULKAN_LIB vulkan)
find_library(DL_LIB dl)

if(NOT LOG_LIB)
    message(FATAL_ERROR "Android log library not found")
endif()

if(NOT ANDROID_LIB)
    message(FATAL_ERROR "Android native library not found")
endif()

if(NOT VULKAN_LIB)
    message(FATAL_ERROR
            "Android Vulkan loader not found. "
            "WoWee requires Vulkan on Android."
    )
endif()


# ============================================================
# SDL2
#
# WoWee's Android build currently fetches SDL2 itself.
# We do the same here instead of requiring SDL2 on the runner.
# ============================================================

include(FetchContent)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.10
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL2)

if(NOT TARGET SDL2::SDL2)
    message(FATAL_ERROR "SDL2::SDL2 target was not created")
endif()


# ============================================================
# GLM
# ============================================================

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glm)

if(TARGET glm::glm)
    set(WOWMOBILE_GLM_TARGET glm::glm)
else()
    set(WOWMOBILE_GLM_TARGET "")
endif()


# ============================================================
# Vulkan
#
# Android NDK supplies the Vulkan headers and loader.
# ============================================================

find_path(
        VULKAN_INCLUDE_DIR
        vulkan/vulkan.h
        PATHS
        "${ANDROID_NDK}/toolchains/llvm/prebuilt/${CMAKE_HOST_SYSTEM_NAME}/sysroot/usr/include"
        "${ANDROID_NDK}/sources/third_party/vulkan/src/include"
)

if(NOT VULKAN_INCLUDE_DIR)

    # CMake/NDK normally already expose the Vulkan headers.
    if(EXISTS
       "${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/vulkan/vulkan.h")

        set(
                VULKAN_INCLUDE_DIR
                "${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include"
        )

    endif()

endif()

if(NOT VULKAN_INCLUDE_DIR)

    message(WARNING
            "Could not explicitly locate Vulkan headers. "
            "Trying compiler include paths."
    )

endif()


# ============================================================
# OpenSSL
#
# GitHub Actions should build/provide OpenSSL and pass:
#
# -DOPENSSL_ROOT_DIR=...
#
# WoWee uses OpenSSL for SRP6/authentication/crypto.
# ============================================================

find_package(OpenSSL REQUIRED)

if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL is required by WoWee")
endif()


# ============================================================
# Threads
# ============================================================

find_package(Threads REQUIRED)


# ============================================================
# ZLIB
# ============================================================

find_package(ZLIB REQUIRED)


# ============================================================
# Compatibility definitions
# ============================================================

add_compile_definitions(
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_DEPTH_ZERO_TO_ONE

        # Android logging
        __ANDROID__

        # We don't build optional desktop FFmpeg support here.
        WOWEE_HAS_AMD_FSR2=0
)

# Disable optional features that are irrelevant to our Android
# integration.

add_compile_definitions(
        WOWEE_MOBILE_ANDROID=1
        WOWEE_ANDROID_CLIENT=1
)


# ============================================================
# WoWee include directories
# ============================================================

target_include_directories(wowmobile
        PRIVATE

        "${WOWEE_SOURCE_DIR}/include"
        "${WOWEE_SOURCE_DIR}/src"

        "${WOWEE_SOURCE_DIR}/extern"

        "${WOWEE_SOURCE_DIR}/extern/imgui"
        "${WOWEE_SOURCE_DIR}/extern/imgui/backends"

        "${WOWEE_SOURCE_DIR}/extern/vk-bootstrap/src"

        "${WOWEE_SOURCE_DIR}/third_party"

        ${VULKAN_INCLUDE_DIR}
)


# ============================================================
# WoWee source collection
#
# We intentionally collect the actual engine source tree instead
# of copying WoWee's giant multiplatform CMakeLists.txt.
# ============================================================

file(GLOB_RECURSE WOWEE_CORE_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/core/*.cpp"
)

file(GLOB_RECURSE WOWEE_MATH_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/math/*.cpp"
)

file(GLOB_RECURSE WOWEE_NETWORK_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/network/*.cpp"
)

file(GLOB_RECURSE WOWEE_AUTH_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/auth/*.cpp"
)

file(GLOB_RECURSE WOWEE_GAME_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/game/*.cpp"
)

file(GLOB_RECURSE WOWEE_AUDIO_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/audio/*.cpp"
)

file(GLOB_RECURSE WOWEE_PIPELINE_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/pipeline/*.cpp"
)

file(GLOB_RECURSE WOWEE_RENDERING_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/rendering/*.cpp"
)

file(GLOB_RECURSE WOWEE_UI_SOURCES
        CONFIGURE_DEPENDS
        "${WOWEE_SOURCE_DIR}/src/ui/*.cpp"
)


# ============================================================
# Remove desktop/application entry points
#
# We are NOT using WoWee's own desktop/SDL application entry
# point. MainActivity owns the Android application lifecycle.
# ============================================================

set(WOWEE_ALL_SOURCES
        ${WOWEE_CORE_SOURCES}
        ${WOWEE_MATH_SOURCES}
        ${WOWEE_NETWORK_SOURCES}
        ${WOWEE_AUTH_SOURCES}
        ${WOWEE_GAME_SOURCES}
        ${WOWEE_AUDIO_SOURCES}
        ${WOWEE_PIPELINE_SOURCES}
        ${WOWEE_RENDERING_SOURCES}
        ${WOWEE_UI_SOURCES}
)

list(FILTER WOWEE_ALL_SOURCES
        EXCLUDE REGEX ".*/main\\.cpp$"
)

list(FILTER WOWEE_ALL_SOURCES
        EXCLUDE REGEX ".*/tests?/.*\\.cpp$"
)

list(FILTER WOWEE_ALL_SOURCES
        EXCLUDE REGEX ".*/test_.*\\.cpp$"
)

list(FILTER WOWEE_ALL_SOURCES
        EXCLUDE REGEX ".*/tools/.*\\.cpp$"
)


# ============================================================
# ImGui
# ============================================================

set(IMGUI_DIR
        "${WOWEE_SOURCE_DIR}/extern/imgui"
)

set(IMGUI_SOURCES)

if(EXISTS "${IMGUI_DIR}/imgui.cpp")

    list(APPEND IMGUI_SOURCES
            "${IMGUI_DIR}/imgui.cpp"
            "${IMGUI_DIR}/imgui_draw.cpp"
            "${IMGUI_DIR}/imgui_tables.cpp"
            "${IMGUI_DIR}/imgui_widgets.cpp"
            "${IMGUI_DIR}/imgui_demo.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    )

else()

    message(FATAL_ERROR
            "WoWee ImGui submodule was not found at ${IMGUI_DIR}"
    )

endif()


add_library(
        wowmobile_imgui
        STATIC
        ${IMGUI_SOURCES}
)

target_include_directories(
        wowmobile_imgui
        SYSTEM
        PUBLIC
        "${IMGUI_DIR}"
        "${IMGUI_DIR}/backends"
)

target_link_libraries(
        wowmobile_imgui
        PUBLIC
        SDL2::SDL2
        ${VULKAN_LIB}
)


# ============================================================
# vk-bootstrap
# ============================================================

set(VK_BOOTSTRAP_DIR
        "${WOWEE_SOURCE_DIR}/extern/vk-bootstrap"
)

if(NOT EXISTS "${VK_BOOTSTRAP_DIR}/src/VkBootstrap.cpp")

    message(FATAL_ERROR
            "WoWee vk-bootstrap submodule missing:\n"
            "${VK_BOOTSTRAP_DIR}"
    )

endif()


add_library(
        wowmobile_vkbootstrap
        STATIC

        "${VK_BOOTSTRAP_DIR}/src/VkBootstrap.cpp"
)

target_include_directories(
        wowmobile_vkbootstrap
        SYSTEM
        PUBLIC

        "${VK_BOOTSTRAP_DIR}/src"
)

target_link_libraries(
        wowmobile_vkbootstrap
        PUBLIC

        ${VULKAN_LIB}
)


# ============================================================
# Our JNI/native bridge
# ============================================================

set(WOWMOBILE_BRIDGE_SOURCES

        "${CMAKE_CURRENT_SOURCE_DIR}/wow_engine.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/wow_renderer.cpp"
)


# ============================================================
# Main shared library
# ============================================================

add_library(
        wowmobile
        SHARED

        ${WOWMOBILE_BRIDGE_SOURCES}
        ${WOWEE_ALL_SOURCES}
)


# ============================================================
# Include paths for the actual engine target
# ============================================================

target_include_directories(
        wowmobile
        PRIVATE

        "${CMAKE_CURRENT_SOURCE_DIR}"

        "${WOWEE_SOURCE_DIR}/include"
        "${WOWEE_SOURCE_DIR}/src"

        "${WOWEE_SOURCE_DIR}/extern"
        "${WOWEE_SOURCE_DIR}/extern/imgui"
        "${WOWEE_SOURCE_DIR}/extern/imgui/backends"
        "${WOWEE_SOURCE_DIR}/extern/vk-bootstrap/src"

        "${WOWEE_SOURCE_DIR}/third_party"

        ${VULKAN_INCLUDE_DIR}
)


# ============================================================
# Android compiler options
# ============================================================

target_compile_options(
        wowmobile
        PRIVATE

        -fexceptions
        -frtti

        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-sign-compare
        -Wno-deprecated-declarations
)


# ============================================================
# WoWee compile definitions
# ============================================================

target_compile_definitions(
        wowmobile
        PRIVATE

        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_DEPTH_ZERO_TO_ONE

        WOWEE_MOBILE_ANDROID=1
        WOWEE_ANDROID_CLIENT=1

        WOWEE_HAS_AMD_FSR2=0
)


# ============================================================
# Link everything
# ============================================================

target_link_libraries(
        wowmobile

        PRIVATE

        wowmobile_imgui
        wowmobile_vkbootstrap

        SDL2::SDL2

        OpenSSL::SSL
        OpenSSL::Crypto

        ZLIB::ZLIB

        Threads::Threads

        ${VULKAN_LIB}
        ${ANDROID_LIB}
        ${LOG_LIB}
        ${DL_LIB}
)


# ============================================================
# Android native libraries required by SDL / Vulkan
# ============================================================

if(ANDROID)

    target_link_libraries(
            wowmobile
            PRIVATE

            android
            log
            vulkan
            dl
            GLESv3
            EGL
    )

endif()


# ============================================================
# ABI
# ============================================================

if(ANDROID)

    if(NOT ANDROID_ABI STREQUAL "arm64-v8a")

        message(FATAL_ERROR
                "This WoW Mobile build supports arm64-v8a only."
        )

    endif()

endif()


# ============================================================
# Diagnostics
# ============================================================

message(STATUS "")
message(STATUS "========== WoW Mobile native configuration ==========")
message(STATUS "WoWee source       : ${WOWEE_SOURCE_DIR}")
message(STATUS "Android ABI        : ${ANDROID_ABI}")
message(STATUS "C++ standard       : C++20")
message(STATUS "Vulkan loader      : ${VULKAN_LIB}")
message(STATUS "Vulkan headers     : ${VULKAN_INCLUDE_DIR}")
message(STATUS "OpenSSL            : ${OPENSSL_VERSION}")
message(STATUS "SDL2               : enabled")
message(STATUS "GLM                : enabled")
message(STATUS "WoWee sources      : ${WOWEE_ALL_SOURCES}")
message(STATUS "====================================================")
message(STATUS "")
