
#include "Camera.hpp"

Camera::Camera(glm::vec3 pos, float yaw, float pitch)
    : 
    position    (pos), 
    yaw         (yaw), 
    pitch       (pitch), 
    speed       (5.0f), 
    sensitivity (0.1f),
    world_up    (0.0f, 1.0f, 0.0f)
{
    update_vectors();
}

void Camera::process_keyboard(const Uint8* keystate, float delta_time)
{
    float velocity = speed * delta_time;
    if (keystate[SDL_SCANCODE_W]) position += front * velocity;
    if (keystate[SDL_SCANCODE_S]) position -= front * velocity;
    if (keystate[SDL_SCANCODE_A]) position -= right * velocity;
    if (keystate[SDL_SCANCODE_D]) position += right * velocity;
}

void Camera::process_mouse(int dx, int dy)
{
    yaw     += dx * sensitivity;
    pitch   -= dy * sensitivity;

    if (pitch >  89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    update_vectors();
}

void Camera::update_vectors()
{
    glm::vec3 fwd;
    fwd.x   = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    fwd.y   = sin(glm::radians(pitch));
    fwd.z   = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front   = glm::normalize(fwd);
    right   = glm::normalize(glm::cross(front, world_up));
    up      = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::get_view_matrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::vec3 Camera::get_position() const
{
    return position;
}