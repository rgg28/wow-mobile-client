cmake_minimum_required(VERSION 3.22.1)

project("wowmobile" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# 1. DIRECTORIOS DE CABECERAS
# ============================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/extern/vk-bootstrap/src
)

# ============================================================
# 2. GLM
# ============================================================

if(GLM_INCLUDE_DIR)
    message(STATUS "GLM include: ${GLM_INCLUDE_DIR}")
    include_directories(${GLM_INCLUDE_DIR})
endif()

# ============================================================
# 3. OPENSSL
# ============================================================

if(NOT OPENSSL_INCLUDE_DIR)
    message(FATAL_ERROR
        "OPENSSL_INCLUDE_DIR no esta definido."
    )
endif()

if(NOT EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/bn.h")
    message(FATAL_ERROR
        "No se encuentra openssl/bn.h en: ${OPENSSL_INCLUDE_DIR}"
    )
endif()

if(NOT OPENSSL_CRYPTO_LIBRARY)
    message(FATAL_ERROR
        "OPENSSL_CRYPTO_LIBRARY no esta definido."
    )
endif()

if(NOT OPENSSL_SSL_LIBRARY)
    message(FATAL_ERROR
        "OPENSSL_SSL_LIBRARY no esta definido."
    )
endif()

message(STATUS "==============================================")
message(STATUS "WoW Mobile OpenSSL")
message(STATUS "==============================================")
message(STATUS "Include : ${OPENSSL_INCLUDE_DIR}")
message(STATUS "Crypto  : ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "SSL     : ${OPENSSL_SSL_LIBRARY}")
message(STATUS "==============================================")

include_directories(
    ${OPENSSL_INCLUDE_DIR}
)

# ============================================================
# 4. SDL3
# ============================================================

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../sdl3"
    sdl3_build
)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../sdl3/include"
)

# ============================================================
# 5. LIBRERÍA PRINCIPAL
# ============================================================

add_library(wowmobile SHARED

    # --------------------------------------------------------
    # Android / integración
    # --------------------------------------------------------

    wow_engine.cpp
    wow_integration.cpp

    # --------------------------------------------------------
    # WoWee - AUTH
    # --------------------------------------------------------

    ../../../wowee/src/auth/auth_handler.cpp
    ../../../wowee/src/auth/auth_opcodes.cpp
    ../../../wowee/src/auth/auth_packets.cpp
    ../../../wowee/src/auth/big_num.cpp
    ../../../wowee/src/auth/crypto.cpp
    ../../../wowee/src/auth/srp.cpp
    ../../../wowee/src/auth/vanilla_crypt.cpp

    # --------------------------------------------------------
    # WoWee - NETWORK
    # --------------------------------------------------------

    ../../../wowee/src/network/packet.cpp
    ../../../wowee/src/network/socket.cpp
    ../../../wowee/src/network/tcp_socket.cpp
    ../../../wowee/src/network/world_socket.cpp

    # --------------------------------------------------------
    # WoWee - GAME
    # --------------------------------------------------------

    ../../../wowee/src/game/opcodes.cpp
    ../../../wowee/src/game/opcode_table.cpp
    ../../../wowee/src/game/game_handler_packets.cpp

    # --------------------------------------------------------
    # WoWee - CORE
    # --------------------------------------------------------

    ../../../wowee/src/core/window.cpp
    ../../../wowee/src/core/application.cpp
    ../../../wowee/src/core/app_clock.cpp
    ../../../wowee/src/core/input.cpp
    ../../../wowee/src/core/logger.cpp

    # --------------------------------------------------------
    # WoWee - RENDERING
    # --------------------------------------------------------

    ../../../wowee/src/rendering/renderer.cpp
    ../../../wowee/src/rendering/render_graph.cpp
    ../../../wowee/src/rendering/vk_context.cpp
    ../../../wowee/src/rendering/vk_pipeline.cpp
    ../../../wowee/src/rendering/vk_buffer.cpp
    ../../../wowee/src/rendering/vk_shader.cpp
    ../../../wowee/src/rendering/vk_texture.cpp
    ../../../wowee/src/rendering/vk_utils.cpp

    # --------------------------------------------------------
    # ImGui
    # --------------------------------------------------------

    ../../../wowee/extern/imgui/imgui.cpp
    ../../../wowee/extern/imgui/imgui_draw.cpp
    ../../../wowee/extern/imgui/imgui_tables.cpp
    ../../../wowee/extern/imgui/imgui_widgets.cpp

    ../../../wowee/extern/imgui/backends/imgui_impl_sdl3.cpp
    ../../../wowee/extern/imgui/backends/imgui_impl_vulkan.cpp

    # --------------------------------------------------------
    # vk-bootstrap
    # --------------------------------------------------------

    ../../../wowee/extern/vk-bootstrap/src/VkBootstrap.cpp
)

# ============================================================
# 6. INCLUDE DIRECTORIES DEL TARGET
# ============================================================

target_include_directories(wowmobile PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}

    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/include

    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/extern/imgui

    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/extern/vk-bootstrap/src

    ${CMAKE_CURRENT_SOURCE_DIR}/../../../sdl3/include

    ${OPENSSL_INCLUDE_DIR}
)

if(GLM_INCLUDE_DIR)
    target_include_directories(
        wowmobile PRIVATE
        ${GLM_INCLUDE_DIR}
    )
endif()

# ============================================================
# 7. LIBRERÍAS ANDROID
# ============================================================

find_library(
    log-lib
    log
)

find_library(
    vulkan-lib
    vulkan
)

find_library(
    android-lib
    android
)

if(NOT log-lib)
    message(FATAL_ERROR "No se encontro liblog.")
endif()

if(NOT vulkan-lib)
    message(FATAL_ERROR "No se encontro Vulkan.")
endif()

if(NOT android-lib)
    message(FATAL_ERROR "No se encontro libandroid.")
endif()

# ============================================================
# 8. ENLACE FINAL
# ============================================================

target_link_libraries(
    wowmobile PRIVATE

    SDL3-shared

    ${vulkan-lib}

    ${log-lib}

    ${android-lib}

    ${OPENSSL_CRYPTO_LIBRARY}

    ${OPENSSL_SSL_LIBRARY}
)

# ============================================================
# 9. DEFINICIONES
# ============================================================

target_compile_definitions(
    wowmobile PRIVATE

    VK_USE_PLATFORM_ANDROID_KHR

    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
)

# ============================================================
# 10. INFORMACIÓN DE CONFIGURACIÓN
# ============================================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "WOW MOBILE NATIVE ENGINE")
message(STATUS "==============================================")
message(STATUS "C++ Standard : C++20")
message(STATUS "ABI          : ${ANDROID_ABI}")
message(STATUS "Platform     : ${ANDROID_PLATFORM}")
message(STATUS "OpenSSL      : ${OPENSSL_INCLUDE_DIR}")
message(STATUS "SDL3         : enabled")
message(STATUS "Vulkan       : enabled")
message(STATUS "WoWee        : enabled")
message(STATUS "==============================================")
