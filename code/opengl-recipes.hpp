
// Este código es de dominio público
// angel.rodriguez@udit.es

#pragma once

#include "Color.hpp"
#include "Color_Buffer.hpp"
#include <glad/glad.h>
#include <memory>
#include <SOIL2.h>
#include <string>

namespace udit
{

    GLuint compile_shaders        (const std::string & vertex_shader_code, const std::string & fragment_shader_code);
    void   show_compilation_error (GLuint  shader_id);
    void   show_linkage_error     (GLuint program_id);

    // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //

    template< typename COLOR_FORMAT >
    std::unique_ptr< Color_Buffer< COLOR_FORMAT > > load_image (const std::string & image_path)
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
            auto image = std::make_unique< Color_Buffer<COLOR_FORMAT> >(image_width, image_height);

            // Se copian los bytes de un buffer a otro directamente:
            std::copy_n
            (
                loaded_pixels,
                size_t(image_width) * size_t(image_height) * sizeof(typename Color_Buffer<COLOR_FORMAT>::Color),
                reinterpret_cast<uint8_t*>(image->colors())
            );

            // Se libera la memoria que reservó SOIL2 para cargar la imagen:
            SOIL_free_image_data(loaded_pixels);

            return image;
        }

        return nullptr;
    }

    // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //

    template< typename COLOR_FORMAT >
    GLuint create_texture_2d (const std::string & texture_path)
    {
        auto image = load_image<COLOR_FORMAT>(texture_path);

        if (image)
        {
            // Se habilitan las texturas, se genera un id para un búfer de textura,
            // se selecciona el búfer de textura creado y se configuran algunos de
            // sus parámetros:
            GLuint texture_id;

            texture_id = SOIL_load_OGL_texture
            (
                texture_path.c_str(),   // Ruta de la textura
                SOIL_LOAD_AUTO,
                SOIL_CREATE_NEW_ID,
                SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
            );

            glEnable(GL_TEXTURE_2D);
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
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
}
