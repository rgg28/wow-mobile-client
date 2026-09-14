#include <SDL.h>
#include <stdbool.h>

// Configuración de la interfaz táctil
#define MAX_BUTTONS 8

typedef struct {
    SDL_Rect rect;
    SDL_Color color;
    bool pressed;
} TouchButton;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) return -1;

    SDL_Window* window = SDL_CreateWindow("WoW Mobile Client C", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Inicializar los 8 botones de macros/habilidades (en la derecha)
    TouchButton buttons[MAX_BUTTONS];
    for(int i = 0; i < MAX_BUTTONS; i++) {
        buttons[i].rect.x = 900 + (i % 2) * 160;
        buttons[i].rect.y = 200 + (i / 2) * 100;
        buttons[i].rect.w = 140;
        buttons[i].rect.h = 80;
        buttons[i].color = (SDL_Color){100, 100, 250, 255}; // Azul básico
        buttons[i].pressed = false;
    }

    // Configuración del Joystick táctil (en la izquierda)
    int joyX = 200, joyY = 500, joyRadius = 80;
    int knobX = 200, knobY = 500;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            // Procesar los toques de los dedos en la pantalla táctil del móvil
            if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION) {
                int touchX = (int)(event.tfinger.x * 1280);
                int touchY = (int)(event.tfinger.y * 720);

                // Verificar colisión con el Joystick
                int dx = touchX - joyX;
                int dy = touchY - joyY;
                if ((dx*dx + dy*dy) < (joyRadius*joyRadius)) {
                    knobX = touchX;
                    knobY = touchY;
                }

                // Verificar colisión con los 8 botones
                for(int i=0; i<MAX_BUTTONS; i++) {
                    if(touchX >= buttons[i].rect.x && touchX <= (buttons[i].rect.x + buttons[i].rect.w) &&
                       touchY >= buttons[i].rect.y && touchY <= (buttons[i].rect.y + buttons[i].rect.h)) {
                        buttons[i].pressed = true;
                        buttons[i].color = (SDL_Color){250, 100, 100, 255}; // Cambia a rojo al tocar
                    }
                }
            }
            if (event.type == SDL_FINGERUP) {
                knobX = joyX;
                knobY = joyY;
                for(int i=0; i<MAX_BUTTONS; i++) {
                    buttons[i].pressed = false;
                    buttons[i].color = (SDL_Color){100, 100, 250, 255};
                }
            }
        }

        // --- RENDERIZADO GRÁFICO ---
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255); // Fondo oscuro
        SDL_RenderClear(renderer);

        // Dibujar los 8 botones de macros/habilidades
        for(int i=0; i<MAX_BUTTONS; i++) {
            SDL_SetRenderDrawColor(renderer, buttons[i].color.r, buttons[i].color.g, buttons[i].color.b, buttons[i].color.a);
            SDL_RenderFillRect(renderer, &buttons[i].rect);
        }

        // Dibujar el Joystick táctil básico
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_Rect baseRect = {joyX - joyRadius, joyY - joyRadius, joyRadius*2, joyRadius*2};
        SDL_RenderDrawRect(renderer, &baseRect); // Base del stick

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect knobRect = {knobX - 25, knobY - 25, 50, 50};
        SDL_RenderFillRect(renderer, &knobRect); // Perilla central móvil

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // Forzar 60 FPS estables
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
