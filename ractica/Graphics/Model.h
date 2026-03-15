#pragma once
#include "../Core/Types.h"
#include <vector>
#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <functional>

// Структура для 3D модели
class Model {
public:
    std::vector<Vertex> vertices;
    GLuint VAO, VBO;
    GLuint textureID;  // ID текстуры в OpenGL
    bool hasTexture;

    // Для пола - данные о высоте в каждой точке
    std::vector<float> heightMap;  // Карта высот
    float minX, maxX, minZ, maxZ;  // Границы модели
    float width, depth;             // Размеры модели

    Model();
    void setupBuffers();
    void draw() const;
    void cleanup();

    // Новый метод для получения высоты в точке
    float getHeightAt(float worldX, float worldZ) const;

    // Установка текстуры
    void setTexture(GLuint texID) {
        textureID = texID;
        hasTexture = (texID != 0);
    }
    float getMinY() const {
        if (vertices.empty()) return 0.0f;
        float minY = vertices[0].position.y;
        for (const auto& v : vertices) {
            minY = std::min(minY, v.position.y);
        }
        return minY;
    }
    // Вычисление границ модели
    void calculateBounds();
};