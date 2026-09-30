cmake_minimum_required(VERSION 3.22.1)
project("wowmobile" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# 1. Inclusión de Directorios de Cabeceras
# ============================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}                                 # Tu carpeta main
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/include          # WoWee includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/extern/imgui     # ImGui de WoWee
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../wowee/extern/vk-bootstrap/src
)

# Inyección forzada de la ruta absoluta de GLM enviada por Actions
if(GLM_INCLUDE_DIR)
    include_directories(${GLM_INCLUDE_DIR})
else()
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-src)
endif()

if(OPENSSL_INCLUDE_DIR)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# ============================================================
# 2. Compilar SDL3 Oficial
# ============================================================
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../sdl3" sdl3_build)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../sdl3/include")

# ============================================================
# 3. Definición del Target (Tus Fuentes + Módulos de WoWee)
# ============================================================
add_library(wowmobile SHARED
    wow_engine.cpp
    wow_integration.cpp

    # --- NÚCLEO DE RED Y AUTENTICACIÓN DE WOWEE ---
    ../../../wowee/src/auth/auth_handler.cpp
    ../../../wowee/src/auth/auth_opcodes.cpp
    ../../../wowee/src/auth/auth_packets.cpp
    ../../../wowee/src/auth/big_num.cpp
    ../../../wowee/src/auth/crypto.cpp
    ../../../wowee/src/auth/srp.cpp
    ../../../wowee/src/auth/vanilla_crypt.cpp
    ../../../wowee/src/network/packet.cpp
    ../../../wowee/src/network/socket.cpp
    ../../../wowee/src/network/tcp_socket.cpp
    ../../../wowee/src/network/world_socket.cpp
    ../../../wowee/src/game/opcodes.cpp
    ../../../wowee/src/game/opcode_table.cpp
    ../../../wowee/src/game/game_handler_packets.cpp

    # --- NÚCLEO DE RENDER DE WOWEE ---
    ../../../wowee/src/core/window.cpp
    ../../../wowee/src/core/application.cpp
    ../../../wowee/src/core/app_clock.cpp
    ../../../wowee/src/core/input.cpp
    ../../../wowee/src/core/logger.cpp
    ../../../wowee/src/rendering/renderer.cpp
    ../../../wowee/src/rendering/render_graph.cpp
    ../../../wowee/src/rendering/vk_context.cpp
    ../../../wowee/src/rendering/vk_pipeline.cpp
    ../../../wowee/src/rendering/vk_buffer.cpp
    ../../../wowee/src/rendering/vk_shader.cpp
    ../../../wowee/src/rendering/vk_texture.cpp
    ../../../wowee/src/rendering/vk_utils.cpp

    # --- DEPENDENCIAS INTERNAS ---
    ../../../wowee/extern/imgui/imgui.cpp
    ../../../wowee/extern/imgui/imgui_draw.cpp
    ../../../wowee/extern/imgui/imgui_tables.cpp
    ../../../wowee/extern/imgui/imgui_widgets.cpp
    ../../../wowee/extern/imgui/backends/imgui_impl_sdl3.cpp
    ../../../wowee/extern/imgui/backends/imgui_impl_vulkan.cpp
    ../../../wowee/extern/vk-bootstrap/src/VkBootstrap.cpp
)

# ============================================================
# 4. Enlace de Librerías del Sistema
# ============================================================
find_library(log-lib log)
find_library(vulkan-lib vulkan)
find_library(android-lib android)

target_link_libraries(wowmobile PRIVATE
    SDL3-shared
    ${vulkan-lib}
    ${log-lib}
    ${android-lib}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
)
