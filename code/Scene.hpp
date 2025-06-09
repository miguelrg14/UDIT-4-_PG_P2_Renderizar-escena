
// Este código es de dominio público
// angel.rodriguez@udit.es

#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>
#include <glm.hpp>
#include "Color.hpp"
#include "Color_Buffer.hpp"
#include "Camera.hpp"
#include "Cube.hpp"
//#include "Terrain.hpp"

namespace udit
{

    class Scene
    {
    private:

        typedef Color_Buffer< Rgba8888 > Color_Buffer;

        enum
        {
            COORDINATES_VBO,
            COLORS_VBO,
            INDICES_EBO,
            UVS_VBO,
            VBO_COUNT
        };

        static const std::string    vertex_shader_code;
        static const std::string    fragment_shader_code;
        static const std::string    texture_path;


        GLint   model_view_matrix_id;
        GLint   projection_matrix_id;
        GLint       normal_matrix_id;

        //Terrain terrain;

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
        bool   there_is_texture;

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

        GLuint create_texture_2d(const std::string& texture_path);
        std::unique_ptr< Color_Buffer > load_image(const std::string& image_path);
    };

}
