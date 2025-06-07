#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <SDL.h>

class Camera {
public:
    Camera(glm::vec3 position, float yaw = -90.0f, float pitch = 0.0f);

    void process_keyboard (const Uint8* keystate, float delta_time);
    void process_mouse    (int dx, int dy);

    glm::mat4 get_view_matrix () const;
    glm::vec3 get_position    () const;

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    void update_vectors();
};