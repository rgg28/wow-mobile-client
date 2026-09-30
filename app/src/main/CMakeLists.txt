add_library(wowmobile SHARED
    # TUS DOS ARCHIVOS DE FUENTE LOCALES EN MAIN
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
