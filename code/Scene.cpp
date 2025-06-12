
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
        /// Definición del struct que describe una luz puntual
        "struct Light\n"
        "{\n"
        "    vec4 position;\n"  // Posición de la luz en espacio ojo (eye‐space)
        "    vec3 color;\n"     // Color/intensidad de la luz (RGB)
        "};\n"
        ""
        /// Matrices uniformes enviadas desde la CPU/C++
        "uniform mat4 model_view_matrix;\n" // Combina modelo y vista: lleva coordenadas de modelo a eye‐space
        "uniform mat4 projection_matrix;\n" // Proyección de cámara (perspectiva u ortográfica)
        "uniform mat4 normal_matrix;\n"     // Matriz para transformar normales correctamente
        ""
        /// Parámetros de iluminación especular
        "uniform float specular_intensity;" // Intensidad global del componente especular
        "uniform float shininess;"          // Exponente de “dureza” del brillo (mayor = más punto pequeño y concentrado)
        "uniform vec3  specular_color;"     // Color del brillo especular
        ""
        /// Parámetros de la luz y los componentes
        "uniform Light light;\n"                // Datos de la luz (posición + color)
        "uniform float ambient_intensity;\n"    // Intensidad de luz ambiental 
        "uniform float diffuse_intensity;\n"    // Intensidad de luz difusa
        ""
        /// Propiedades del material
        "uniform vec3 material_color;\n"        // Color base del material (difuso)
        ""
        /// Atributos de vértice (entradas del VAO)
        "layout (location = 0) in vec3 vertex_coordinates;\n"   // Coordenadas XYZ del vértice
        "layout (location = 1) in vec3 vertex_normal;\n"        // Normal del vértice (para iluminación)
        "layout (location = 2) in vec2 vertex_uv;\n"            // Coordenadas UV para texturizado
        ""
        /// Salidas (para pasar al fragment shader)
        "out vec3 front_color;\n"   // Color resultante tras mezcla de ambient, diffuse y specular
        "out vec2 texture_uv;\n"    // Coordenadas UV para muestrear la textura en el fragment
        ""
        /// Función principal del shader de vértices
        "void main()\n"
        "{\n"
        // 1) Transformar posición del vértice a espacio ojo (eye‐space)
        "vec4 pos_view = model_view_matrix * vec4(vertex_coordinates, 1.0);"
        // 2) Transformar y normalizar la normal normal_matrix corrige escalados/no‐uniformes de model_view
        "vec3 N = normalize((normal_matrix * vec4(vertex_normal, 0.0)).xyz);"
        // 3) Vector desde vértice hacia la luz
        "vec3 L = normalize((light.position - pos_view).xyz);"
        // 4) Vector desde vértice hacia la cámara (en eye‐space la cámara está en el origen)
        "vec3 V = normalize(-pos_view.xyz);"     // cámara en origen de eye‐space

        /// Cálculo del término difuso (Lambert)
        // dot(N, L) = cos(θ) entre normal y luz
        // max(..., 0) para no iluminar caras opuestas
        "float diff = diffuse_intensity * max(dot(N, L), 0.0);" // Componente difusa de iluminación

        /// Cálculo del término especular (Blinn-Phong)
        // vector half‐way H entre luz y vista
        // dot(N, H) = cos(φ) entre normal y half‐way
        // pow(..., shininess) ajusta la concentración del brillo
        "vec3 H = normalize(L + V);"
        "float spec = specular_intensity * pow(max(dot(N, H), 0.0), shininess);"
        // Alternativa (Phong)
        //"vec3 R = reflect(-L, N);"
        //"float spec = specular_intensity * pow(max(dot(R, V), 0.0), shininess);"

        /// Mezcla final de los tres componentes: (Ambient + Diffuse + Specular)
        "front_color =" 
        "ambient_intensity * material_color"        // luz ambiental
        "+ diff * light.color * material_color "    // componente difuso coloreado
        "+ spec * specular_color;"                  // componente especular

        // 5) Pasamos la UV al fragment shader
        "texture_uv = vertex_uv;"
        // 6) Calculamos la posición final en screen‐space
        "gl_Position = projection_matrix * pos_view;"
        "}";

    const string Scene::fragment_shader_code =
        "#version 330\n"
        ""
        /// Uniform que representa la textura activa (unit 0) a muestrear
        "uniform sampler2D sampler;\n"
        ""
        /// Entradas desde el vertex shader
        "in  vec2 texture_uv;\n"        // Coordenadas UV interpoladas para texturizado
        "in  vec3 front_color;\n"       // Color calculado (ambient + difuso + especular)
        ""
        /// Salida del fragment shader
        "out vec4 fragment_color;\n"    // Color final del píxel
        ""
        "void main()\n"
        "{\n"
        // 1) Muestreamos la textura en las coordenadas UV proporcionadas
        "    vec4 texture_color = texture(sampler, texture_uv);\n"
        ""
        // 2) Mezclamos el color de iluminación con el color de la textura.
        //    Se usa un alpha de 0.5 para semi-transparencia (puedes ajustarlo).
        //    front_color contiene el RGB; le damos alpha 0.5 y luego multiplicamos
        //    por el RGBA de la textura para obtener el color final.
        "    fragment_color = vec4(front_color, 0.5) * texture_color;\n" // Resultado final visual
        // Ahora fragment_color es lo que se escribirá en el frame-buffer.
        "}";

    /// Vertex Shader para renderizar el quad de post-procesado
    const string Scene::effect_vertex_shader_code =

        "#version 330\n"
        ""
        /// Atributos de entrada (VAO):
        "layout (location = 0) in vec3 vertex_coordinates;" // Posición del vértice en clip-space (-1 a +1)
        "layout (location = 1) in vec2 vertex_texture_uv;"  // Coordenadas UV para muestrear la textura
        ""
        /// Salida al fragment shader:
        "out vec2 texture_uv;"  // Se pasa la UV para usar en el muestreo
        ""
        "void main()"
        "{"
        // 1) Asigna la posición directamente (ya está en clip-space)
        "   gl_Position = vec4(vertex_coordinates, 1.0);"
        // 2) Propaga el UV al fragment shader
        "   texture_uv  = vertex_texture_uv;"
        "}";

    /// Fragment Shader de ejemplo para un efecto simple
    const string Scene::effect_fragment_shader_code =
        "#version 330\n"
        ""
        /// Uniform para la textura renderizada en el framebuffer
        "uniform sampler2D sampler2d;"
        ""
        /// Entrada desde el vertex shader:
        "in  vec2 texture_uv;"      // Coordenadas UV interpoladas

        /// Salida del fragment shader:
        "out vec4 fragment_color;"  // Color final del fragmento
        ""
        "void main()"
        "{"
        /// Ejemplo de efecto: tono sepia amortiguado
        //// 1) Muestreamos el color original de la textura
        //"   vec3 color = texture (sampler2d, texture_uv.st).rgb;"
        //// 2) Convertimos a intensidad luminosa promedio
        //"   float i = (color.r + color.g + color.b) * 0.3333333333;"
        //// 3) Aplicamos un tinte amarronado (sepia suave)
        //"   vec3 sepia = vec3(1.0, 0.75, 0.5);"
        //"   fragment_color = vec4(vec3(i, i, i) * sepia, 1.0);"
        
        /// Alternativa: (Aplicar textura original sin modificaciones)
        "   fragment_color = texture(sampler2d, texture_uv);"
        "}";

    const string Scene::texture_path = "../assets/Stone_Base_Color.png";

    Scene::Scene(unsigned width, unsigned height)
        : 
        camera(glm::vec3(0, 0, 5)), 
        angle(0)
        //terrain(10.f, 10.f, 50, 50)
    {
        /// Postprocesado
        // Se crea la textura y se dibuja algo en ella:
        build_framebuffer();

        // Se compilan y se activan los shaders:
        program_id = compile_shaders(vertex_shader_code, fragment_shader_code);
        effect_program_id = compile_shaders(effect_vertex_shader_code, effect_fragment_shader_code);

        glUseProgram(program_id);

        model_view_matrix_id = glGetUniformLocation(program_id, "model_view_matrix");
        projection_matrix_id = glGetUniformLocation(program_id, "projection_matrix");
            normal_matrix_id = glGetUniformLocation(program_id,     "normal_matrix");

        // Se carga la textura y se envía a la GPU:
              texture_id = create_texture_2d(texture_path);
        there_is_texture = texture_id > 0;

        // Se establece la altura máxima del height map en el vertex shader:
        //glUniform1f(glGetUniformLocation(program_id, "max_height"), 5.f);

        configure_material(program_id);
        configure_light(program_id);

        // Se establece la configuración básica:
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);     // PANTALLAZO NEGRO CON ESTO ACTIVADO!!!
        glClearColor(0.f, 0.f, 0.f, 1.f);

        resize(width, height);

        load_mesh("../assets/Terreno.obj");
    }

    Scene::~Scene()
    {
        glDeleteVertexArrays(1, &vao_id);
        glDeleteBuffers(VBO_COUNT, vbo_ids);

        glDeleteVertexArrays(1, &framebuffer_quad_vao);
        glDeleteBuffers(2, framebuffer_quad_vbos);

        glDeleteProgram(program_id);

        if (there_is_texture)
        {
            glDeleteTextures(1, &texture_id);
        }
    }

    void Scene::update ()
    {
        angle += 0.01f; // Rotación de la escena en tiempo real
    }

    void Scene::render()
    {
        glViewport(0, 0, framebuffer_width, framebuffer_height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);         // Se activa el framebuffer de la textura
        glUseProgram(program_id);

        glClearColor(.8f, .8f, .8f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Se selecciona la textura si está disponible:
        if (there_is_texture)
        {
            glBindTexture(GL_TEXTURE_2D, texture_id);
        }

        /// CÁMARA
        // MATRIZ DE VISTA (transformaciones de la cámara)
        glm::mat4 view = camera.get_view_matrix();

        /// MATRIZ DEL MODELO (transformaciones del cubo)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.f, -1.f, -3.f));  // Posición fija del cubo
        model = glm::rotate(model, angle, glm::vec3(1.f, 1.f, 0.f)); // Rotación sobre eje Y

        // COMBINACIÓN FINAL: Cámara + modelos
        glm::mat4 model_view_matrix = view * model;

        /// PRIMERA ETAPA (RENDER DE LOS OBJETOS OPACOS):
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

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

        /// SEGUNDA ETAPA (RENDER DE LOS OBJETOS TRANSPARENTES):
        // Se habilita la mezcla con el color de fondo usando el canal alpha y se deshabilita la escritura en el Z-Buffer:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        // Se rota otro cubo y se empuja hacia el fondo:
        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(0.f, 0.f, -5.f));
        model = glm::rotate(model, angle, glm::vec3(0.f, 1.f, 0.f));
        model = glm::translate(model, glm::vec3(0.f, 0.f, +2.f));

        glUniformMatrix4fv(model_view_matrix_id, 1, GL_FALSE, glm::value_ptr(model_view_matrix));

        // Se renderiza el cubo en el framebuffer:
        cube.render();

        // Se deshabilita la mezcla con el fondo y se restaura escritura en el Z-Buffer:
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // Se desactiva la prueba de profundidad antes de renderizar el framebuffer
        glDisable(GL_DEPTH_TEST);
        render_framebuffer();   // Dibuja el framebuffer en pantalla
    }


    /// <summary>
    ///  OpenGL adapta el campo visual horizontal/vertical según la nueva forma de la ventana si se cambia su tamaño
    /// </summary>
    /// <param name="width"></param>
    /// <param name="height"></param>
    void Scene::resize (unsigned width, unsigned height)
    {
        window_width  = width;
        window_height = height;

        glm::mat4 projection_matrix = glm::perspective (20.f, GLfloat(width) / height, 1.f, 5000.f);

        glUniformMatrix4fv (projection_matrix_id, 1, GL_FALSE, glm::value_ptr(projection_matrix));

        glViewport (0, 0, width, height);
    }

    /// ------------------ POSTPROCESADO ------------------

    void Scene::build_framebuffer()
    {
        // Se crea un framebuffer en el que poder renderizar:
        {
            glGenFramebuffers(1, &framebuffer_id);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
        }

        // Se crea una textura que será el búffer de color vinculado al framebuffer:
        {
            glGenTextures(1, &out_texture_id);
            glBindTexture(GL_TEXTURE_2D, out_texture_id);

            // El búfer de color tendrá formato RGB:

            glTexImage2D
            (
                GL_TEXTURE_2D,
                0,
                GL_RGB,
                framebuffer_width,
                framebuffer_height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                0
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

        // Se crea un Z-Buffer para usarlo en combinación con el framebuffer:
        {
            glGenRenderbuffers(1, &depthbuffer_id);
            glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_id);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebuffer_width, framebuffer_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer_id);
        }

        // Se configura el framebuffer:
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, out_texture_id, 0);

            const GLenum draw_buffer = GL_COLOR_ATTACHMENT0;

            glDrawBuffers(1, &draw_buffer);
        }

        // Se comprueba que el framebuffer está listo:

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // Se desvincula el framebuffer:

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Se crea la malla del quad necesario para hacer el render del framebuffer:

        static const GLfloat quad_positions[] =
        {
            +1.0f, -1.0f, 0.0f,
            +1.0f, +1.0f, 0.0f,
            -1.0f, +1.0f, 0.0f,
            -1.0f, +1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            +1.0f, -1.0f, 0.0f,
        };

        static const GLfloat quad_texture_uvs[] =
        {
            +1.0f,  0.0f,
            +1.0f, +1.0f,
             0.0f, +1.0f,
             0.0f, +1.0f,
             0.0f,  0.0f,
             1.0f,  0.0f,
        };

        glGenVertexArrays(1, &framebuffer_quad_vao);
        glGenBuffers(2, framebuffer_quad_vbos);

        glBindVertexArray(framebuffer_quad_vao);

        glBindBuffer(GL_ARRAY_BUFFER, framebuffer_quad_vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_positions), quad_positions, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, framebuffer_quad_vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_texture_uvs), quad_texture_uvs, GL_STATIC_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    void Scene::render_framebuffer()
    {
        glViewport(0, 0, window_width, window_height);

        // Se activa el framebuffer de la ventana:

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(effect_program_id);

        // Se activa la textura del framebuffer y se renderiza en la ventana:

        glBindTexture(GL_TEXTURE_2D, out_texture_id);

        glBindVertexArray(framebuffer_quad_vao);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    ///----------------------------------------------------

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

    GLuint Scene::compile_shaders(const std::string& vertex_shader_code, const std::string& fragment_shader_code)
    {
        GLint succeeded = GL_FALSE;

        // Se crean objetos para los shaders:

        GLuint   vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

        // Se carga el código de los shaders:

        const char* vertex_shaders_code[] = { vertex_shader_code.c_str() };
        const char* fragment_shaders_code[] = { fragment_shader_code.c_str() };
        const GLint    vertex_shaders_size[] = { GLint(vertex_shader_code.size()) };
        const GLint  fragment_shaders_size[] = { GLint(fragment_shader_code.size()) };

        glShaderSource(vertex_shader_id, 1, vertex_shaders_code, vertex_shaders_size);
        glShaderSource(fragment_shader_id, 1, fragment_shaders_code, fragment_shaders_size);

        // Se compilan los shaders:

        glCompileShader(vertex_shader_id);
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
        GLint    light_position = glGetUniformLocation(program_id, "light.position"   );
        GLint       light_color = glGetUniformLocation(program_id, "light.color"      );
        GLint ambient_intensity = glGetUniformLocation(program_id, "ambient_intensity");
        GLint diffuse_intensity = glGetUniformLocation(program_id, "diffuse_intensity");
        GLint      spec_int_loc = glGetUniformLocation(program_id, "specular_intensity");
        GLint     shininess_loc = glGetUniformLocation(program_id, "shininess");
        GLint    spec_color_loc = glGetUniformLocation(program_id, "specular_color");

        glUniform4f(light_position   , 10.0f, 10.f, 10.f, 1.f);
        glUniform3f(light_color      , 1.0f, 1.f, 1.f        );
        glUniform1f(ambient_intensity, 0.2f                  );
        glUniform1f(diffuse_intensity, 0.8f                  );
        glUniform1f(spec_int_loc, 1.0f);   // fuerza del brillo
        glUniform1f(shininess_loc, 32.0f);   // “dureza” del material
        glUniform3f(spec_color_loc, 1.0f, 1.0f, 1.0f); // color del reflejo (blanco)
    }

    /// ------------------ TEXTURIZADO ------------------ 

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
            //GLuint texture_id;

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

    /// -------------------------------------------------
    
    /// ------------------ ERRORES (Utilidades) -----------------

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

    /// ---------------------------------------------------------

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

