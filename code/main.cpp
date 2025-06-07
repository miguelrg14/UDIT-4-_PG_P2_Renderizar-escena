// Este código es de dominio público
// angel.rodriguez@udit.es

#include "Scene.hpp"
#include "Window.hpp"

using udit::Scene;
using udit::Window;

int main(int, char* [])
{
    constexpr unsigned viewport_width = 1024;
    constexpr unsigned viewport_height = 576;

    Window window
    (
        "OpenGL example",
        Window::Position::CENTERED,
        Window::Position::CENTERED,
        viewport_width,
        viewport_height,
        { 3, 3 }
    );

    // Activar modo relativo del ratón para captura continua y ocultar cursor
    SDL_SetRelativeMouseMode(SDL_TRUE);

    Scene scene(viewport_width, viewport_height);

    bool exit = false;
    bool camera_active = true;  // Modo FPS activado al inicio
    SDL_SetRelativeMouseMode(SDL_TRUE); // Oculta cursor y captura ratón

    do
    {
        SDL_Event event;
        int mouse_dx = 0, mouse_dy = 0;

        while (SDL_PollEvent(&event) > 0)
        {
            if (event.type == SDL_QUIT)
            {
                exit = true;
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                if (camera_active) {
                    mouse_dx = event.motion.xrel;
                    mouse_dy = event.motion.yrel;
                }
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                camera_active = !camera_active; // Alternar modo
                SDL_SetRelativeMouseMode(camera_active ? SDL_TRUE : SDL_FALSE);
            }
        }

        // Leer el estado actual del teclado
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        // Tiempo fijo entre frames (ajusta según tu temporizador real si tienes)
        float delta_time = 1.0f / 60.0f;

        if (camera_active)
        {
            // Procesar input para mover la cámara
            scene.camera.process_keyboard(keystate, delta_time);
            scene.camera.process_mouse(mouse_dx, mouse_dy);
        }

        // Actualizar la escena (puede contener otras animaciones)
        scene.update();
        // Renderizar la escena (ya debe usar camera.get_view_matrix() internamente)
        scene.render();
        // Actualizar la ventana
        window.swap_buffers();
    } while (!exit);

    SDL_Quit();

    return 0;
}