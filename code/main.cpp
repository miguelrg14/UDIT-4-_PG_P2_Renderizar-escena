
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

    Scene scene(viewport_width, viewport_height);

    bool exit = false;
    int  mouse_x = 0;
    int  mouse_y = 0;
    bool button_down = false;

    bool camera_active = true;  // Modo FPS activado al inicio
    SDL_SetRelativeMouseMode(SDL_TRUE);

    do
    {
        // Se procesan los eventos acumulados:

        SDL_Event event;

        while (SDL_PollEvent(&event) > 0)
        {
            switch (event.type)
            {
            case SDL_MOUSEMOTION:
            {
                //SDL_GetMouseState(&mouse_x, &mouse_y);

                if (camera_active) 
                {
                    mouse_x = event.motion.xrel;
                    mouse_y = event.motion.yrel;
                    scene.camera.process_mouse(mouse_x, mouse_y);
                }

                break;
            }

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) 
                {
                case SDLK_ESCAPE:
                    camera_active = !camera_active; // Alternar modo
                    SDL_SetRelativeMouseMode(camera_active ? SDL_TRUE : SDL_FALSE);
                    break;

                //case SDLK_w || SDLK_a || SDLK_s || SDLK_d:
                //    scene.camera.process_keyboard(keystate, delta_time);
                //    // puedes añadir más cases para otras teclas
                default:
                    break;
                }
                break;

            case SDL_QUIT:
            {
                exit = true;
            }
            }
        }

        // Leer el estado actual del teclado
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        // Tiempo fijo entre frames (ajusta según tu temporizador real si tienes)
        float delta_time = 1.0f / 60.0f;

        scene.camera.process_keyboard(keystate, delta_time);

        // Se actualiza la escena:
        scene.update();

        // Se redibuja la escena:
        scene.render();

        // Se actualiza el contenido de la ventana:
        window.swap_buffers();
    } while (not exit);

    SDL_Quit();

    return 0;
}
