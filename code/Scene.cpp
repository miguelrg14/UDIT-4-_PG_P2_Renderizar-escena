
// Este código es de dominio público
// angel.rodriguez@udit.es

#pragma once

#include "Scene.hpp"

#include <iostream>
#include <cassert>

#include <glm.hpp>                          // vec3, vec4, ivec4, mat4
#include <gtc/matrix_transform.hpp>         // translate, rotate, scale, perspective
#include <gtc/type_ptr.hpp>                 // value_ptr

// Importar objetos (.obj) a escena
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Cargar texturas
#include <SOIL2.h>

namespace udit
{
    
    using namespace std;

    const string Scene::vertex_shader_code =

        "#version 330\n"
        ""
        "struct Light"
        "{"
        "    vec4 position;"
        "    vec3 color;"
        "};"
        ""
        "uniform Light light;"
        "uniform float ambient_intensity;"
        "uniform float diffuse_intensity;"
        ""
        "uniform vec3 material_color;"
        ""
        "uniform mat4 model_view_matrix;"
        "uniform mat4 projection_matrix;"
        "uniform mat4     normal_matrix;"
        ""
        "layout (location = 0) in vec3 vertex_coordinates;"
        "layout (location = 1) in vec3 vertex_normal;"
        ""
        "out vec3 front_color;"
        ""
        "void main()"
        "{"
        "    vec4  normal   = normal_matrix * vec4(vertex_normal, 0.0);"
        "    vec4  position = model_view_matrix * vec4(vertex_coordinates, 1.0);"
        ""
        "    vec4  light_direction = light.position - position;"
        "    float light_intensity = diffuse_intensity * max (dot (normalize (normal.xyz), normalize (light_direction.xyz)), 0.0);"
        ""
        "    front_color = ambient_intensity * material_color + diffuse_intensity * light_intensity * light.color * material_color;"
        "    gl_Position = projection_matrix * position;"
        "}";

    //const string Scene::fragment_shader_code =
    //    "#version 330\n"
    //    "in  vec3 front_color;"
    //    "in  vec2 uv;"
    //    "out vec4 fragment_color;"
    //    "uniform sampler2D texture_sampler;"
    //    "uniform bool use_vertex_color;"
    //    // Gestiona si se tiñe o no la textura según la información que le de al implementarla en "glUniform1i(use_vertex_color_id, GL_FALSE);" en render.
    //    "void main()"
    //    "{"
    //    "    vec4 tex_color = texture(texture_sampler, uv);"
    //    "    if (use_vertex_color)"
    //    "        fragment_color = tex_color * vec4(front_color, 1.0);"
    //    "    else"
    //    "        fragment_color = tex_color;"
    //    "}";

    //const string Scene::fragment_shader_code =

    //    "#version 330\n"
    //    ""
    //    "in  vec3    front_color;"
    //    "out vec4 fragment_color;"
    //    ""
    //    "void main()"
    //    "{"
    //    "    fragment_color = vec4(front_color, 1.0);"
    //    "}";

    const string Scene::fragment_shader_code =

        "#version 330\n"
        ""
        "in  vec3    front_color;"
        "out vec4 fragment_color;"
        ""
        "void main()"
        "{"
        "    fragment_color = vec4(front_color, 1.0);"
        "}";

    Scene::Scene(unsigned width, unsigned height) 
        : 
        camera(glm::vec3(0, 0, 5)), 
        angle(0)
    {
        // Se establece la configuración básica:
        glEnable     (GL_CULL_FACE );
        glEnable     (GL_DEPTH_TEST);
        glClearColor (.1f, .1f, .1f, 1.f);

        // Se compilan y se activan los shaders:
        GLuint program_id = compile_shaders ();

        glUseProgram (program_id);

        model_view_matrix_id = glGetUniformLocation (program_id, "model_view_matrix");
        projection_matrix_id = glGetUniformLocation (program_id, "projection_matrix");
            normal_matrix_id = glGetUniformLocation (program_id, "normal_matrix"    );
        
        configure_material (program_id);
        configure_light    (program_id);


        //// Almacenar la ID
        //program_id = compile_shaders();
        //glUseProgram(program_id);

        //// Importar modelos 3D a la escena
        //load_model("../binaries/Terreno.obj");
        ////load_model("../binaries/Estadio.obj");

        //// Texturizado
        //use_vertex_color_id = glGetUniformLocation(program_id, "use_vertex_color");

        //texture_id = SOIL_load_OGL_texture("../binaries/Textures/Stone_Base_Color.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);

        //if (texture_id == 0)
        //    std::cerr << "Error al cargar la textura: " << SOIL_last_result() << std::endl;

        //glBindTexture(GL_TEXTURE_2D, texture_id);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ////

        resize (width, height);
    }

    void Scene::update ()
    {
        angle += 0.01f; // Rotación de la escena en tiempo real
    }

    void Scene::render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // MATRIZ DEL MODELO (transformaciones del cubo)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.f, -1.f, -3.f));  // Posición fija del cubo
        model = glm::rotate(model, angle, glm::vec3(1.f, 1.f, 0.f)); // Rotación sobre eje Y

        // MATRIZ DE VISTA (transformaciones de la cámara)
        glm::mat4 view = camera.get_view_matrix();

        // COMBINACIÓN FINAL
        glm::mat4 model_view_matrix = view * model;

        glUniformMatrix4fv(model_view_matrix_id, 1, GL_FALSE, glm::value_ptr(model_view_matrix));

        glm::mat4 normal_matrix = glm::transpose(glm::inverse(model_view_matrix));
        glUniformMatrix4fv(normal_matrix_id, 1, GL_FALSE, glm::value_ptr(normal_matrix));

        cube.render();
    }

    /// <summary>
    ///  OpenGL adapta el campo visual horizontal/vertical según la nueva forma de la ventana si se cambia su tamaño
    /// </summary>
    /// <param name="width"></param>
    /// <param name="height"></param>
    void Scene::resize (unsigned width, unsigned height)
    {
        glm::mat4 projection_matrix = glm::perspective (20.f, GLfloat(width) / height, 1.f, 5000.f);

        glUniformMatrix4fv (projection_matrix_id, 1, GL_FALSE, glm::value_ptr(projection_matrix));

        glViewport (0, 0, width, height);
    }

    GLuint Scene::compile_shaders ()
    {
        /// Compilar los sombreadores individuales
        GLint succeeded = GL_FALSE;

        // Se crean objetos para los sombreadores:
        GLuint   vertex_shader_id = glCreateShader (GL_VERTEX_SHADER  );
        GLuint fragment_shader_id = glCreateShader (GL_FRAGMENT_SHADER);

        // Se carga el código de los sombreadores:
        const char *   vertex_shaders_code[] = {          vertex_shader_code.c_str () };
        const char * fragment_shaders_code[] = {        fragment_shader_code.c_str () };
        const GLint    vertex_shaders_size[] = { (GLint)  vertex_shader_code.size  () };
        const GLint  fragment_shaders_size[] = { (GLint)fragment_shader_code.size  () };

        glShaderSource  (  vertex_shader_id, 1,   vertex_shaders_code,   vertex_shaders_size);
        glShaderSource  (fragment_shader_id, 1, fragment_shaders_code, fragment_shaders_size);

        // Se compilan los sombreadores:
        glCompileShader (  vertex_shader_id);
        glCompileShader (fragment_shader_id);

        /// ---------------------------------------------------
        // Se comprueba si la compilación ha tenido éxito:
        glGetShaderiv   (  vertex_shader_id, GL_COMPILE_STATUS, &succeeded);
        if (!succeeded) show_compilation_error (  vertex_shader_id);

        glGetShaderiv   (fragment_shader_id, GL_COMPILE_STATUS, &succeeded);
        if (!succeeded) show_compilation_error (fragment_shader_id);
        /// ---------------------------------------------------

        /// Combinar los sombreadores compilados anteriormente en un programa para usarlos en la GPU
        // Se crea un objeto para un programa:
        GLuint program_id = glCreateProgram ();

        // Se cargan los sombreadores compilados en el programa:
        glAttachShader  (program_id,   vertex_shader_id);
        glAttachShader  (program_id, fragment_shader_id);

        // Se enlazan los sombreadores:
        glLinkProgram   (program_id);

        // Se comprueba si el enlace ha tenido éxito:
        glGetProgramiv  (program_id, GL_LINK_STATUS, &succeeded);
        if (!succeeded) show_linkage_error (program_id);

        // Se liberan los sombreadores compilados una vez se han enlazado:
        glDeleteShader (  vertex_shader_id);
        glDeleteShader (fragment_shader_id);

        return (program_id);
    }


    /// <summary>
    ///     Importa un modelo 3D a la escena
    /// </summary>
    /// <param name="path"></param>
    void Scene::load_model(const std::string& path)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        aiMesh* mesh = scene->mMeshes[0]; // Usamos solo la primera malla

        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            // Posición
            vertices.push_back(mesh->mVertices[i].x);
            vertices.push_back(mesh->mVertices[i].y);
            vertices.push_back(mesh->mVertices[i].z);

            // Color (fijo)
            vertices.push_back(1.0f);
            vertices.push_back(0.5f);
            vertices.push_back(0.2f);

            // UVs (si existen)
            if (mesh->mTextureCoords[0]) 
            {
                vertices.push_back(mesh->mTextureCoords[0][i].x);
                vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
            else 
            {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        index_count = indices.size();

        glGenVertexArrays(1, &vao_model);
        glGenBuffers(1, &vbo_model);
        glGenBuffers(1, &ebo_model);

        glBindVertexArray(vao_model);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_model);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Posición
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // UV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    void Scene::configure_material(GLuint program_id)
    {
        GLint material_color = glGetUniformLocation(program_id, "material_color");

        glUniform3f(material_color, 0.f, 1.f, 0.f);
    }

    void Scene::configure_light(GLuint program_id)
    {
        GLint light_position    = glGetUniformLocation(program_id, "light.position"   );
        GLint light_color       = glGetUniformLocation(program_id, "light.color"      );
        GLint ambient_intensity = glGetUniformLocation(program_id, "ambient_intensity");
        GLint diffuse_intensity = glGetUniformLocation(program_id, "diffuse_intensity");

        glUniform4f(light_position   , 10.0f, 10.f, 10.f, 1.f);
        glUniform3f(light_color      , 1.0f, 1.f, 1.f        );
        glUniform1f(ambient_intensity, 0.2f                  );
        glUniform1f(diffuse_intensity, 0.8f                  );
    }

    /// ------------------ Error utilities ---------------------------------

    void Scene::show_compilation_error (GLuint shader_id)
    {
        string info_log;
        GLint  info_log_length;

        glGetShaderiv (shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

        info_log.resize (info_log_length);

        glGetShaderInfoLog (shader_id, info_log_length, NULL, &info_log.front ());

        cerr << info_log.c_str () << endl;

        #ifdef _MSC_VER
            //OutputDebugStringA (info_log.c_str ());
        #endif

        assert(false);
    }

    void Scene::show_linkage_error (GLuint program_id)
    {
        string info_log;
        GLint  info_log_length;

        glGetProgramiv (program_id, GL_INFO_LOG_LENGTH, &info_log_length);

        info_log.resize (info_log_length);

        glGetProgramInfoLog (program_id, info_log_length, NULL, &info_log.front ());

        cerr << info_log.c_str () << endl;

        #ifdef _MSC_VER
            //OutputDebugStringA (info_log.c_str ());
        #endif

        assert(false);
    }

    /// ---------------------------------------------------
}

