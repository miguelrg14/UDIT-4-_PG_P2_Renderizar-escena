
// Este código es de dominio público
// angel.rodriguez@udit.es

#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <string>
#include "Camera.hpp"
#include "Cube.hpp"

namespace udit
{

    class Scene
    {
    private:

        enum
        {
            COORDINATES_VBO,
            COLORS_VBO,
            INDICES_EBO,
            VBO_COUNT
        };

        static const std::string    vertex_shader_code;
        static const std::string    fragment_shader_code;

        GLint   model_view_matrix_id;
        GLint   projection_matrix_id;
        GLint       normal_matrix_id;

        Cube    cube;
        float   angle;

        // Cargar modelos 3D
        GLuint  vao_model, vbo_model, ebo_model;
        GLsizei index_count = 0;

        // Color aleatorio
        GLuint  vbo_ids[VBO_COUNT];
        GLuint  vao_id;
        GLsizei number_of_indices;

        // Cargar texturas
        GLuint program_id;
        GLuint texture_id = 0;
        GLuint use_vertex_color_id;

    public:

        // Cámara
        Camera camera;

        Scene(unsigned width, unsigned height);
       ~Scene();

        void   update       ();
        void   render       ();
        void   resize       (unsigned width, unsigned height);
        //void   load_model   (const std::string& path);

    private:

        GLuint compile_shaders        ();
        void   show_compilation_error (GLuint  shader_id);
        void   show_linkage_error     (GLuint program_id);
        void   load_mesh(const std::string& mesh_file_path);
        glm::vec3   random_color();

        void   configure_material(GLuint program_id);
        void   configure_light(GLuint program_id);
    };

}
