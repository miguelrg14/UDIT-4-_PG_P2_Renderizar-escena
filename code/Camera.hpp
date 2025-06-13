
// Este c�digo es de dominio p�blico
// angel.rodriguez@udit.es

#ifndef CAMERA_HEADER
#define CAMERA_HEADER

    #include <glm.hpp>                          // vec3, vec4, ivec4, mat4
    #include <gtc/matrix_transform.hpp>         // translate, rotate, scale, perspective
    #include <gtc/type_ptr.hpp>                 // value_ptr
    #include <SDL_stdinc.h>

    namespace udit
    {

        class Camera
        {
            using Point    = glm::vec4;
            using Vector   = glm::vec4;
            using Matrix44 = glm::mat4;

        private:

            float    fov;
            float    near_z;
            float    far_z;
            float    ratio;

            Point    location;
            Point    target;

            Matrix44 projection_matrix;

            float movementSpeed = 5.f;   // unidades/segundo
            float mouseSensitivity = 0.1f;  // grados/pixel

            float yaw;    // �ngulo horizontal, grados
            float pitch;  // �ngulo vertical, grados

        public:

            Camera(float ratio = 1.f)
            {
                reset (60.f, 0.1f, 1000.f, ratio);
                updateCameraVectors();
            }

            Camera(float near_z, float far_z, float ratio = 1.f)
            {
                reset (60.f, near_z, far_z, ratio);
                updateCameraVectors();
            }

            Camera(float fov_degrees, float near_z, float far_z, float ratio)
            {
                reset (fov_degrees, near_z, far_z, ratio);
                updateCameraVectors();
            }

        public:

            float         get_fov      () const { return fov;    }
            float         get_near_z   () const { return near_z; }
            float         get_far_z    () const { return far_z;  }
            float         get_ratio    () const { return ratio;  }

            const Point & get_location () const { return location; }
            const Point & get_target   () const { return target;   }

        public:

            void set_fov      (float new_fov   ) { fov    = new_fov;    calculate_projection_matrix (); }
            void set_near_z   (float new_near_z) { near_z = new_near_z; calculate_projection_matrix (); }
            void set_far_z    (float new_far_z ) { far_z  = new_far_z;  calculate_projection_matrix (); }
            void set_ratio    (float new_ratio ) { ratio  = new_ratio;  calculate_projection_matrix (); }

            void set_location (float x, float y, float z) { location[0] = x; location[1] = y; location[2] = z; updateCameraVectors(); }
            void set_target   (float x, float y, float z) { target  [0] = x; target  [1] = y; target  [2] = z; }

            void reset (float new_fov, float new_near_z, float new_far_z, float new_ratio)
            {
                // 1) Orientaci�n por defecto
                yaw = -90.0f;
                pitch = 0.0f;
                // 2) Proyecci�n
                set_fov(new_fov);
                set_near_z(new_near_z);
                set_far_z(new_far_z);
                set_ratio(new_ratio);
                // 3) Posici�n y reconstrucci�n del target
                set_location(0.f, 0.f, 0.f);
                updateCameraVectors();       // target = location + direcci�n calculada
                // 4) Matriz de proyecci�n
                calculate_projection_matrix();
            }

        public:

            void move (const glm::vec3 & translation)
            {
                location += glm::vec4 (translation, 1.f);
                target   += glm::vec4 (translation, 1.f);
            }

            void rotate (const glm::mat4 & rotation)
            {
                target = location + rotation * (target - location);
            }

        public:

            const glm::mat4 & get_projection_matrix () const
            {
                return projection_matrix;
            }

            glm::mat4 get_transform_matrix_inverse () const
            {
                return glm::lookAt
                (
                    glm::vec3(location[0], location[1], location[2]),
                    glm::vec3(target  [0], target  [1], target  [2]),
                    glm::vec3(       0.0f,        1.0f,        0.0f)
                );
            }

            void process_keyboard(const Uint8* keys, float deltaTime);
            void process_mouse(int xrel, int yrel);

        private:

            // Reconstruye 'target' desde yaw/pitch
            void updateCameraVectors() 
            {
                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(pitch));
                front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                front = glm::normalize(front);
                target = location + glm::vec4(front, 0.0f);
            }

            void calculate_projection_matrix ()
            {
                projection_matrix = glm::perspective (glm::radians (fov), ratio, near_z, far_z);
            }

        };

    }

#endif
