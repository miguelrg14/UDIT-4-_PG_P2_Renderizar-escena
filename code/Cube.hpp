
// Este código es de dominio público
// angel.rodriguez@udit.es

#ifndef CUBE_HEADER
#define CUBE_HEADER

    #include <glad/glad.h>

    namespace udit
    {

        class Cube
        {
        private:

            // Índices para indexar el array vbo_ids:

            enum
            {
                COORDINATES_VBO,
                NORMALS_VBO,
                INDICES_IBO,
                VBO_COUNT
            };

            // Arrays de datos del cubo base:

            static const GLfloat coordinates[];
            static const GLfloat normals    [];
            static const GLubyte indices    [];

        private:

            GLuint vbo_ids[VBO_COUNT];      // Ids de los VBOs que se usan
            GLuint vao_id;                  // Id del VAO del cubo

        public:

            Cube();
           ~Cube();

            void render ();

        };
        //struct IMesh { virtual void draw(const glm::mat4&, GLuint) = 0; };
        //class Cube : public IMesh 
        //{
        //        void draw(const glm::mat4& modelMatrix, GLuint program) override {
        //        GLint loc = glGetUniformLocation(program, "model_view_matrix");
        //        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        //        glBindVertexArray(vao_id);
        //        glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_BYTE, 0);
        //    }
        //};


    }

#endif
