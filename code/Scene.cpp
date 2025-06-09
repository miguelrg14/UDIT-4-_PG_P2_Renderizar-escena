
// Este código es de dominio público
// angel.rodriguez@udit.es

#pragma once

#include "Scene.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <glm.hpp>                          // vec3, vec4, ivec4, mat4
#include <gtc/matrix_transform.hpp>         // translate, rotate, scale, perspective
#include <gtc/type_ptr.hpp>                 // value_ptr

// Importar objetos (.obj) a escena
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Cargar texturas
#include <SOIL2.h>

using namespace std;
using namespace glm;

namespace udit
{
    const string Scene::vertex_shader_code =
        "#version 330\n"
        ""
        "struct Light\n"
        "{\n"
        "    vec4 position;\n"
        "    vec3 color;\n"
        "};\n"
        ""
        "uniform mat4 model_view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        ""
        "uniform Light light;\n"
        "uniform float ambient_intensity;\n"
        "uniform float diffuse_intensity;\n"
        ""
        "uniform vec3 material_color;\n"
        "uniform mat4 normal_matrix;\n"
        ""
        "layout (location = 0) in vec3 vertex_coordinates;\n"
        "layout (location = 1) in vec3 vertex_normal;\n"
        "layout (location = 2) in vec2 vertex_uv;\n"
        ""
        "out vec3 front_color;\n"
        "out vec2 texture_uv;\n"
        ""
        "void main()\n"
        "{\n"
        "    vec4 normal     = normal_matrix * vec4(vertex_normal, 0.0);\n"
        "    vec4 position   = model_view_matrix * vec4(vertex_coordinates, 1.0);\n"
        "    vec4 light_dir  = light.position - position;\n"
        "    float intensity = diffuse_intensity * max(dot(normalize(normal.xyz), normalize(light_dir.xyz)), 0.0);\n"
        "    front_color     = ambient_intensity * material_color + intensity * light.color * material_color;\n"
        "    texture_uv      = vertex_uv;\n"
        "    gl_Position     = projection_matrix * position;\n"
        "}";

    const string Scene::fragment_shader_code =
        "#version 330\n"
        ""
        "uniform sampler2D sampler;\n"
        ""
        "in  vec2 texture_uv;\n"
        "out vec4 fragment_color;\n"
        "in  vec3 front_color;\n"
        ""
        "void main()\n"
        "{\n"
        "    vec4 texture_color = texture(sampler, texture_uv);\n"
        ""
        "    fragment_color = vec4(front_color, 0.5) * texture_color;\n" // Resultado final visual
        "}";

    const string Scene::texture_path = "../assets/Stone_Base_Color.png";

    Scene::Scene(unsigned width, unsigned height)
        : 
        camera(glm::vec3(0, 0, 5)), 
        angle(0)
        //terrain(10.f, 10.f, 50, 50)
    {
        // Se compilan y se activan los shaders:
        GLuint program_id = compile_shaders ();

        glUseProgram (program_id);

        model_view_matrix_id = glGetUniformLocation (program_id, "model_view_matrix");
        projection_matrix_id = glGetUniformLocation (program_id, "projection_matrix");
            normal_matrix_id = glGetUniformLocation (program_id, "normal_matrix"    );

        //// Se carga la textura y se envía a la GPU:
        texture_id = create_texture_2d(texture_path);

        there_is_texture = texture_id > 0;

        // Se establece la altura máxima del height map en el vertex shader:
        glUniform1f(glGetUniformLocation(program_id, "max_height"), 5.f);
        
        configure_material (program_id);
        configure_light    (program_id);

        // Se establece la configuración básica:
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glClearColor(.1f, .1f, .1f, 1.f);

        resize (width, height);

        load_mesh("../assets/Terreno.obj");
    }
    Scene::~Scene()
    {
        glDeleteVertexArrays(1, &vao_id);
        glDeleteBuffers(VBO_COUNT, vbo_ids);
    }

    void Scene::update ()
    {
        angle += 0.01f; // Rotación de la escena en tiempo real
    }

    void Scene::render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Se selecciona la textura si está disponible:
        if (there_is_texture)
        {
            glBindTexture(GL_TEXTURE_2D, texture_id);
        }

        /// CÁMARA
        // MATRIZ DE VISTA (transformaciones de la cámara)
        glm::mat4 view = camera.get_view_matrix();

        /// PRIMERA ETAPA (RENDER DE LOS OBJETOS OPACOS):
        // MATRIZ DEL MODELO (transformaciones del cubo)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.f, -1.f, -3.f));  // Posición fija del cubo
        model = glm::rotate(model, angle, glm::vec3(1.f, 1.f, 0.f)); // Rotación sobre eje Y

        // COMBINACIÓN FINAL: Cámara + modelos
        glm::mat4 model_view_matrix = view * model;

        glUniformMatrix4fv(model_view_matrix_id, 1, GL_FALSE, glm::value_ptr(model_view_matrix));

        glm::mat4 normal_matrix = glm::transpose(glm::inverse(model_view_matrix));
        glUniformMatrix4fv(normal_matrix_id, 1, GL_FALSE, glm::value_ptr(normal_matrix));

        // Texturizado
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1i(glGetUniformLocation(program_id, "sampler"), 0);

        // Se dibuja la malla:
        glBindVertexArray(vao_id);
        glDrawElements(GL_TRIANGLES, number_of_indices, GL_UNSIGNED_SHORT, 0);

        //cube.render();
        // Se renderiza el terreno
        //terrain.render();

        /// SEGUNDA ETAPA (RENDER DE LOS OBJETOS TRANSPARENTES):
        // Se habilita la mezcla con el color de fondo usando el canal alpha y se deshabilita la escritura en el Z-Buffer:
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Se rota otro cubo y se empuja hacia el fondo:
        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(0.f, 0.f, -5.f));
        model = glm::rotate(model, angle, glm::vec3(0.f, 1.f, 0.f));
        model = glm::translate(model, glm::vec3(0.f, 0.f, +2.f));

        glUniformMatrix4fv(model_view_matrix_id, 1, GL_FALSE, glm::value_ptr(model_view_matrix));

        // Se renderiza un cubo:
        cube.render();

        // Se deshabilita la mezcla con el fondo y se restaura escritura en el Z-Buffer:
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
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
        GLint succeeded = GL_FALSE;

        // Se crean objetos para los shaders:
        GLuint   vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

        // Se carga el código de los shaders:
        const char*    vertex_shaders_code[] = {         vertex_shader_code.c_str() };
        const char*  fragment_shaders_code[] = {       fragment_shader_code.c_str() };
        const GLint    vertex_shaders_size[] = { (GLint)  vertex_shader_code.size() };
        const GLint  fragment_shaders_size[] = { (GLint)fragment_shader_code.size() };

        glShaderSource(  vertex_shader_id, 1,   vertex_shaders_code,   vertex_shaders_size);
        glShaderSource(fragment_shader_id, 1, fragment_shaders_code, fragment_shaders_size);

        // Se compilan los shaders:
        glCompileShader(  vertex_shader_id);
        glCompileShader(fragment_shader_id);

        // Se comprueba que si la compilación ha tenido éxito:
        glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &succeeded);
        if (!succeeded) show_compilation_error(vertex_shader_id);

        glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &succeeded);
        if (!succeeded) show_compilation_error(fragment_shader_id);

        // Se crea un objeto para un programa:
        GLuint program_id = glCreateProgram();

        // Se cargan los shaders compilados en el programa:
        glAttachShader(program_id, vertex_shader_id);
        glAttachShader(program_id, fragment_shader_id);

        // Se linkan los shaders:
        glLinkProgram(program_id);

        // Se comprueba si el linkage ha tenido éxito:
        glGetProgramiv(program_id, GL_LINK_STATUS, &succeeded);
        if (!succeeded) show_linkage_error(program_id);

        // Se liberan los shaders compilados una vez se han linkado:
        glDeleteShader(vertex_shader_id);
        glDeleteShader(fragment_shader_id);

        return (program_id);
    }


    /// <summary>
    ///     Importa un modelo 3D a la escena
    /// </summary>
    /// <param name="path"></param>
    void Scene::load_mesh(const std::string& mesh_file_path)
    {
        Assimp::Importer importer;

        auto scene = importer.ReadFile
        (
            mesh_file_path,
            aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
        );

        // Si scene es un puntero nulo significa que el archivo no se pudo cargar con éxito:
        if (scene && scene->mNumMeshes > 0)
        {
            // Para este ejemplo se coge la primera malla solamente:
            auto mesh = scene->mMeshes[0];
            size_t number_of_vertices = mesh->mNumVertices;

            // Se generan índices para los VBOs del cubo:
            glGenBuffers(VBO_COUNT, vbo_ids);
            glGenVertexArrays(1, &vao_id);
            // Se activa el VAO del cubo para configurarlo:
            glBindVertexArray(vao_id);

            // Coordenadas
            glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[COORDINATES_VBO]);
            glBufferData(GL_ARRAY_BUFFER, number_of_vertices * sizeof(aiVector3D), mesh->mVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


            // El archivo del modelo 3D de ejemplo no guarda un color por cada vértice, por lo que se va
            // a crear un array de colores aleatorios (tantos como vértices):
            // vector< vec3 > vertex_colors(number_of_vertices);
            // for (auto& color : vertex_colors)
            // {
            //     color = random_color();
            // }

            // Normales (para iluminación)
            if (mesh->HasNormals())
            {
                // Se suben a un VBO los datos de color y se vinculan al VAO:
                glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[COLORS_VBO]);
                glBufferData(GL_ARRAY_BUFFER, number_of_vertices * sizeof(aiVector3D), mesh->mNormals, GL_STATIC_DRAW);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
            }

            // Coordenadas de textura (UVs)
            if (mesh->HasTextureCoords(0))
            {
                vector<vec2> uvs(number_of_vertices);
                for (unsigned i = 0; i < number_of_vertices; ++i)
                {
                    uvs[i] = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                }
                glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[UVS_VBO]);
                glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), uvs.data(), GL_STATIC_DRAW);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
            }

            // Índices
            // Los índices en ASSIMP están repartidos en "faces", pero OpenGL necesita un array de enteros
            // por lo que vamos a mover los índices de las "faces" a un array de enteros:
            // Se asume que todas las "faces" son triángulos (revisar el flag aiProcess_Triangulate arriba).
            number_of_indices = mesh->mNumFaces * 3;
            vector<GLshort> indices(number_of_indices);
            auto vertex_index = indices.begin();
            for (unsigned i = 0; i < mesh->mNumFaces; ++i)
            {
                auto& face = mesh->mFaces[i];
                *vertex_index++ = face.mIndices[0];
                *vertex_index++ = face.mIndices[1];
                *vertex_index++ = face.mIndices[2];
            }

            // Se suben a un EBO los datos de índices:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_ids[INDICES_EBO]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLshort), indices.data(), GL_STATIC_DRAW);
        }
    }

    void Scene::configure_material(GLuint program_id)
    {
        GLint material_color = glGetUniformLocation(program_id, "material_color");

        glUniform3f(material_color, 1.f, 1.f, 1.f);
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

    GLuint Scene::create_texture_2d(const std::string& texture_path)
    {
        auto image = load_image(texture_path);

        if (texture_id == 0)
        {
            std::cerr << "Error cargando textura: " << SOIL_last_result() << std::endl;
        }
        if (image)
        {
            texture_id = SOIL_load_OGL_texture
            (
                texture_path.c_str(),            // Ruta de la textura
                SOIL_LOAD_AUTO,
                SOIL_CREATE_NEW_ID,
                SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
            );
            // Se habilitan las texturas, se genera un id para un búfer de textura,
            // se selecciona el búfer de textura creado y se configuran algunos de
            // sus parámetros:
            GLuint texture_id;

            glEnable(GL_TEXTURE_2D);
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D
            (
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                image->get_width(),
                image->get_height(),
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image->colors()
            );

            glGenerateMipmap(GL_TEXTURE_2D);

            return texture_id;
        }

        return -1;
    }

    unique_ptr< Scene::Color_Buffer > Scene::load_image(const string& image_path)
    {
        // Se carga la imagen del archivo usando SOIL2:
        int image_width = 0;
        int image_height = 0;
        int image_channels = 0;

        uint8_t* loaded_pixels = SOIL_load_image
        (
            image_path.c_str(),
            &image_width,
            &image_height,
            &image_channels,
            SOIL_LOAD_RGBA              // Indica que nos devuelva los pixels en formato RGB32
        );                              // al margen del formato usado en el archivo

        // Si loaded_pixels no es nullptr, la imagen se ha podido cargar correctamente:
        if (loaded_pixels)
        {
            auto image = make_unique< Color_Buffer >(image_width, image_height);

            // Se copian los bytes de un buffer a otro directamente:
            std::copy_n
            (
                loaded_pixels,
                size_t(image_width) * size_t(image_height) * sizeof(Color_Buffer::Color),
                reinterpret_cast<uint8_t*>(image->colors())
            );

            // Se libera la memoria que reservó SOIL2 para cargar la imagen:
            SOIL_free_image_data(loaded_pixels);

            return image;
        }

        return nullptr;
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

    glm::vec3 Scene::random_color()
    {
        return glm::vec3
        (
            float(rand()) / float(RAND_MAX),
            float(rand()) / float(RAND_MAX),
            float(rand()) / float(RAND_MAX)
        );
    }

}

