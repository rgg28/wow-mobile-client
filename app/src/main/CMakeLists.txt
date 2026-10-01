cmake_minimum_required(VERSION 3.22.1)

project("wowmobile" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# RUTAS BASE
# ============================================================

set(WOWEE_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee"
)

set(SDL3_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../sdl3"
)

# ============================================================
# OPENSSL
# ============================================================

if(NOT DEFINED OPENSSL_INCLUDE_DIR OR
   "${OPENSSL_INCLUDE_DIR}" STREQUAL "")
    message(FATAL_ERROR
        "OPENSSL_INCLUDE_DIR no esta definido."
    )
endif()

if(NOT EXISTS
   "${OPENSSL_INCLUDE_DIR}/openssl/bn.h")
    message(FATAL_ERROR
        "No se encuentra openssl/bn.h en: "
        "${OPENSSL_INCLUDE_DIR}"
    )
endif()

if(NOT DEFINED OPENSSL_CRYPTO_LIBRARY OR
   "${OPENSSL_CRYPTO_LIBRARY}" STREQUAL "")
    message(FATAL_ERROR
        "OPENSSL_CRYPTO_LIBRARY no esta definido."
    )
endif()

if(NOT EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
    message(FATAL_ERROR
        "No existe libcrypto: "
        "${OPENSSL_CRYPTO_LIBRARY}"
    )
endif()

if(NOT DEFINED OPENSSL_SSL_LIBRARY OR
   "${OPENSSL_SSL_LIBRARY}" STREQUAL "")
    message(FATAL_ERROR
        "OPENSSL_SSL_LIBRARY no esta definido."
    )
endif()

if(NOT EXISTS "${OPENSSL_SSL_LIBRARY}")
    message(FATAL_ERROR
        "No existe libssl: "
        "${OPENSSL_SSL_LIBRARY}"
    )
endif()

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "WOW MOBILE - OPENSSL")
message(STATUS "==============================================")
message(STATUS "OPENSSL_INCLUDE_DIR     = ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_CRYPTO_LIBRARY  = ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OPENSSL_SSL_LIBRARY     = ${OPENSSL_SSL_LIBRARY}")
message(STATUS "==============================================")
message(STATUS "")

# ============================================================
# INCLUDE PATHS
# ============================================================

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${WOWEE_ROOT}/include"
    "${WOWEE_ROOT}/extern/imgui"
    "${WOWEE_ROOT}/extern/vk-bootstrap/src"
    "${SDL3_ROOT}/include"
    "${OPENSSL_INCLUDE_DIR}"
)

if(DEFINED GLM_INCLUDE_DIR AND
   NOT "${GLM_INCLUDE_DIR}" STREQUAL "")

    message(STATUS
        "GLM_INCLUDE_DIR = ${GLM_INCLUDE_DIR}"
    )

    include_directories(
        "${GLM_INCLUDE_DIR}"
    )

endif()

# ============================================================
# SDL3
# ============================================================

add_subdirectory(
    "${SDL3_ROOT}"
    sdl3_build
)

# ============================================================
# WOW MOBILE
# ============================================================

add_library(
    wowmobile
    SHARED

    wow_engine.cpp
    wow_integration.cpp

    # --------------------------------------------------------
    # AUTH
    # --------------------------------------------------------

    "${WOWEE_ROOT}/src/auth/auth_handler.cpp"
    "${WOWEE_ROOT}/src/auth/auth_opcodes.cpp"
    "${WOWEE_ROOT}/src/auth/auth_packets.cpp"
    "${WOWEE_ROOT}/src/auth/big_num.cpp"
    "${WOWEE_ROOT}/src/auth/crypto.cpp"
    "${WOWEE_ROOT}/src/auth/srp.cpp"
    "${WOWEE_ROOT}/src/auth/vanilla_crypt.cpp"

    # --------------------------------------------------------
    # NETWORK
    # --------------------------------------------------------

    "${WOWEE_ROOT}/src/network/packet.cpp"
    "${WOWEE_ROOT}/src/network/socket.cpp"
    "${WOWEE_ROOT}/src/network/tcp_socket.cpp"
    "${WOWEE_ROOT}/src/network/world_socket.cpp"

    # --------------------------------------------------------
    # GAME
    # --------------------------------------------------------

    "${WOWEE_ROOT}/src/game/opcodes.cpp"
    "${WOWEE_ROOT}/src/game/opcode_table.cpp"
    "${WOWEE_ROOT}/src/game/game_handler_packets.cpp"

    # --------------------------------------------------------
    # CORE
    # --------------------------------------------------------

    "${WOWEE_ROOT}/src/core/window.cpp"
    "${WOWEE_ROOT}/src/core/application.cpp"
    "${WOWEE_ROOT}/src/core/app_clock.cpp"
    "${WOWEE_ROOT}/src/core/input.cpp"
    "${WOWEE_ROOT}/src/core/logger.cpp"

    # --------------------------------------------------------
    # RENDERING
    # --------------------------------------------------------

    "${WOWEE_ROOT}/src/rendering/renderer.cpp"
    "${WOWEE_ROOT}/src/rendering/render_graph.cpp"
    "${WOWEE_ROOT}/src/rendering/vk_context.cpp"
    "${WOWEE_ROOT}/src/rendering/vk_pipeline.cpp"
    "${WOWEE_ROOT}/src/rendering/vk_buffer.cpp"
    "${WOWEE_ROOT}/src/rendering/vk_shader.cpp"
    "${WOWEE_ROOT}/src/rendering/vk_texture.cpp"
    "${WOWEE_ROOT}/src/rendering/vk_utils.cpp"

    # --------------------------------------------------------
    # IMGUI
    # --------------------------------------------------------

    "${WOWEE_ROOT}/extern/imgui/imgui.cpp"
    "${WOWEE_ROOT}/extern/imgui/imgui_draw.cpp"
    "${WOWEE_ROOT}/extern/imgui/imgui_tables.cpp"
    "${WOWEE_ROOT}/extern/imgui/imgui_widgets.cpp"

    "${WOWEE_ROOT}/extern/imgui/backends/imgui_impl_sdl3.cpp"
    "${WOWEE_ROOT}/extern/imgui/backends/imgui_impl_vulkan.cpp"

    # --------------------------------------------------------
    # VK-BOOTSTRAP
    # --------------------------------------------------------

    "${WOWEE_ROOT}/extern/vk-bootstrap/src/VkBootstrap.cpp"
)

# ============================================================
# TARGET INCLUDES
# ============================================================

target_include_directories(
    wowmobile
    PRIVATE

    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${WOWEE_ROOT}/include"
    "${WOWEE_ROOT}/extern/imgui"
    "${WOWEE_ROOT}/extern/vk-bootstrap/src"
    "${SDL3_ROOT}/include"
    "${OPENSSL_INCLUDE_DIR}"
)

if(DEFINED GLM_INCLUDE_DIR AND
   NOT "${GLM_INCLUDE_DIR}" STREQUAL "")

    target_include_directories(
        wowmobile
        PRIVATE
        "${GLM_INCLUDE_DIR}"
    )

endif()

# ============================================================
# ANDROID LIBRARIES
# ============================================================

find_library(
    LOG_LIB
    log
)

find_library(
    VULKAN_LIB
    vulkan
)

find_library(
    ANDROID_LIB
    android
)

if(NOT LOG_LIB)
    message(FATAL_ERROR
        "No se encontro liblog."
    )
endif()

if(NOT VULKAN_LIB)
    message(FATAL_ERROR
        "No se encontro Vulkan."
    )
endif()

if(NOT ANDROID_LIB)
    message(FATAL_ERROR
        "No se encontro libandroid."
    )
endif()

# ============================================================
# LINK
# ============================================================

target_link_libraries(
    wowmobile

    PRIVATE

    SDL3-shared

    "${VULKAN_LIB}"
    "${LOG_LIB}"
    "${ANDROID_LIB}"

    "${OPENSSL_CRYPTO_LIBRARY}"
    "${OPENSSL_SSL_LIBRARY}"
)

# ============================================================
# DEFINICIONES
# ============================================================

target_compile_definitions(
    wowmobile
    PRIVATE

    VK_USE_PLATFORM_ANDROID_KHR
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
)

# ============================================================
# INFORMACION DE BUILD
# ============================================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "WOW MOBILE NATIVE ENGINE")
message(STATUS "==============================================")
message(STATUS "C++ Standard : C++20")
message(STATUS "Android ABI  : ${ANDROID_ABI}")
message(STATUS "Android API  : ${ANDROID_PLATFORM}")
message(STATUS "OpenSSL      : STATIC .a")
message(STATUS "SDL3         : ENABLED")
message(STATUS "Vulkan       : ENABLED")
message(STATUS "WoWee        : ENABLED")
message(STATUS "==============================================")
message(STATUS "")
