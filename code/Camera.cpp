
#include "Camera.hpp"
#include <SDL.h>

namespace udit {

    void Camera::process_keyboard(const Uint8* keys, float deltaTime)
    {
        // Dirección «forward»
        glm::vec3 forward = glm::normalize(glm::vec3(target - location));
        // Eje «right» a partir de «forward» y up global (0,1,0)
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
        float velocity = movementSpeed * deltaTime;

        if (keys[SDL_SCANCODE_W]) move(forward * velocity);
        if (keys[SDL_SCANCODE_S]) move(-forward * velocity);
        if (keys[SDL_SCANCODE_A]) move(-right * velocity);
        if (keys[SDL_SCANCODE_D]) move(right * velocity);
    }

    void Camera::process_mouse(int xrel, int yrel)
    {
        float xoffset = xrel * mouseSensitivity;
        float yoffset = yrel * mouseSensitivity;

        // Primero rotamos en yaw (alrededor de Y global)
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(-xoffset), glm::vec3(0.f, 1.f, 0.f));

        // Luego pitch (alrededor del eje «right» de la cámara)
        glm::vec3 forward = glm::normalize(glm::vec3(target - location));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
        R = glm::rotate(R, glm::radians(-yoffset), right);

        rotate(R);
    }

}