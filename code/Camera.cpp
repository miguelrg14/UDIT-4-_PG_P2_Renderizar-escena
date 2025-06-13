
#include "Camera.hpp"
#include <SDL.h>

namespace udit 
{

    void Camera::process_keyboard(const Uint8* keys, float deltaTime)
    {
        // Direcci�n �forward�
        glm::vec3 forward = glm::normalize(glm::vec3(target - location));
        // Eje �right� a partir de �forward� y up global (0,1,0)
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
        float velocity = movementSpeed * deltaTime;

        if (keys[SDL_SCANCODE_W]) move(forward * velocity);
        if (keys[SDL_SCANCODE_S]) move(-forward * velocity);
        if (keys[SDL_SCANCODE_A]) move(-right * velocity);
        if (keys[SDL_SCANCODE_D]) move(right * velocity);
    }

    void Camera::process_mouse(int xrel, int yrel)
    {
        // Aplica sensibilidad
        float xoffset = xrel * mouseSensitivity;
        float yoffset = yrel * mouseSensitivity;

        // 1) Ajusta yaw y pitch
        yaw += xoffset;   // mover rat�n a la derecha aumenta yaw
        pitch -= yoffset;   // mover rat�n hacia arriba aumenta pitch

        // 2) Clamp al pitch para evitar 'voltear' la c�mara
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        // 3) Reconstruye el target a partir de esos �ngulos
        updateCameraVectors();
    }

}