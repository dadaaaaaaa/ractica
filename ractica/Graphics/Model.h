#pragma once
#include "../Core/Types.h"
#include <vector>
#include <GLEW/glew.h>

// Структура для 3D модели
struct Model {
    std::vector<Vertex> vertices;
    GLuint VAO, VBO;
    bool hasTexture;

    Model();
    void setupBuffers();
    void draw() const;
};