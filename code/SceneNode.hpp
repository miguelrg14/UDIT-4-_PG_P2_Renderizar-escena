//
//#pragma once
//#include <vector>
//#include <memory>
//#include <glm.hpp>
//#include <glad/glad.h>
//
//namespace udit 
//{
//    class Mesh; // O Cube, o terrain, etc.
//
//    class SceneNode 
//    {
//    public:
//        glm::mat4 localTransform = glm::mat4(1.0f);
//        std::shared_ptr<Mesh> mesh;           // malla: Cube, AssimpMesh, etc.
//        std::vector<std::unique_ptr<SceneNode>> children;
//
//        void addChild(std::unique_ptr<SceneNode> child)
//        {
//            children.push_back(std::move(child));
//        }
//
//        // Recorrido y dibujo recursivo:
//        void draw(const glm::mat4& parentTransform, GLuint shaderProgram) 
//        {
//            glm::mat4 globalTransform = parentTransform * localTransform;
//            if (mesh) 
//            {
//                mesh->draw(globalTransform, shaderProgram);
//            }
//            for (auto& child : children)
//                child->draw(globalTransform, shaderProgram);
//        }
//    };
//}