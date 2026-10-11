#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "include/blp_loader.h"

const char* BASE_DATA_PATH = "./data/";

void build_wow_path(char* output_buffer, size_t buffer_size, const char* relative_path) {
    snprintf(output_buffer, buffer_size, "%s%s", BASE_DATA_PATH, relative_path);
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error al inicializar SDL3: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    // Ventana simulando pantalla móvil (800x480)
    if (!SDL_CreateWindowAndRenderer("WoW MoP Engine - Decodificador BLP Nativo", 800, 480, 0, &window, &renderer)) {
        SDL_Log("Error al crear ventana/renderizador: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    char full_path[512];
    build_wow_path(full_path, sizeof(full_path), "Interface/Glues/LOADINGSCREENS/LoadScreenPvpBattleground.blp");
    
    SDL_Log("Buscando archivo en: %s", full_path);

    SDL_Texture* gpu_texture = NULL;

    FILE* file = fopen(full_path, "rb");
    if (!file) {
        SDL_Log("Error critico: No se pudo encontrar el archivo en el banco de datos.");
    } else {
        BLPHeader header;
        fread(&header, sizeof(BLPHeader), 1, file);

        if (memcmp(header.magic, "BLP2", 4) == 0) {
            SDL_Log("=========================================");
            SDL_Log("¡Cabecera BLP2 encontrada!");
            SDL_Log("Dimensiones: %d x %d pixeles", header.width, header.height);
            SDL_Log("Codificacion: %d (2 = Directo/Paleta)", header.colorEncoding);
            SDL_Log("Compresion: %d (0 = Uncompressed Paletted)", header.preferredFormat);
            SDL_Log("=========================================");

            if (header.colorEncoding == 2 && header.preferredFormat == 0) {
                // 1. Leer la paleta de colores (256 colores de 4 bytes cada uno = 1024 bytes)
                uint8_t palette[256][4];
                fseek(file, header.offsets[0], SEEK_SET); // Ir al inicio de los datos del Mipmap 0
                
                // En el formato Paletizado, la paleta de 1024 bytes va justo ANTES de los indices de pixeles
                // o al inicio de la seccion de datos segun la estructura de Blizzard.
                // Ajustamos el puntero para leer la paleta de 256 colores RGBA:
                fseek(file, sizeof(BLPHeader), SEEK_SET); 
                fread(palette, 1024, 1, file);

                // 2. Asignar memoria para los indices del archivo (1 byte por pixel)
                uint8_t* pixel_indices = (uint8_t*)malloc(header.width * header.height);
                fseek(file, header.offsets[0], SEEK_SET);
                fread(pixel_indices, header.width * header.height, 1, file);

                // 3. Asignar memoria para la imagen final descomprimida en RGBA (4 bytes por pixel)
                uint32_t* rgba_pixels = (uint32_t*)malloc(header.width * header.height * sizeof(uint32_t));

                // 4. Mapear cada indice a su color real RGBA
                for (uint32_t i = 0; i < header.width * header.height; i++) {
                    uint8_t idx = pixel_indices[i];
                    uint8_t r = palette[idx][0];
                    uint8_t g = palette[idx][1];
                    uint8_t b = palette[idx][2];
                    uint8_t a = palette[idx][3];

                    // Empaquetamos los componentes en un entero de 32 bits en formato RGBA
                    rgba_pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
                }

                // 5. Crear la textura nativa en la GPU con SDL3
                gpu_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, header.width, header.height);
                if (gpu_texture) {
                    SDL_UpdateTexture(gpu_texture, NULL, rgba_pixels, header.width * sizeof(uint32_t));
                    SDL_Log("¡Textura cargada y procesada en VRAM correctamente!");
                }

                // Liberar memoria RAM local que ya no necesitamos
                free(pixel_indices);
                free(rgba_pixels);
            } else {
                SDL_Log("Alerta: Este archivo usa compresion DXT. Se requiere un decodificador avanzado.");
            }
        } else {
            SDL_Log("Error: Archivo de textura no valido.");
        }
        fclose(file);
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Limpiar la pantalla
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Si la textura se proceso correctamente, la dibujamos en toda la ventana
        if (gpu_texture) {
            SDL_RenderTexture(renderer, gpu_texture, NULL, NULL);
        }

        SDL_RenderPresent(renderer);
    }

    if (gpu_texture) SDL_DestroyTexture(gpu_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
