#pragma once
#include "../Core/Types.h"
#include <vector>
#include <GLEW/glew.h>

// Структура для 3D модели
// В Model.h
class Model {
public:
    std::vector<Vertex> vertices;
    GLuint VAO, VBO;
    GLuint textureID;  // ID текстуры в OpenGL
    bool hasTexture;

    Model();
    void setupBuffers();
    void draw() const;
    void cleanup();
};