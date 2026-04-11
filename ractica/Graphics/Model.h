#pragma once
#include "../Core/Types.h"
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <functional>

// Структура для 3D модели - Fixed Pipeline версия
class Model {
public:
    std::vector<Vertex> vertices;
    GLuint displayList;     // Используем Display List вместо VBO для Fixed Pipeline
    GLuint textureID;
    bool hasTexture;
    bool isCompiled;        // Флаг, скомпилирована ли display list

    // Для пола - данные о высоте в каждой точке
    std::vector<float> heightMap;
    float minX, maxX, minZ, maxZ;
    float width, depth;

    Model();
    void setupBuffers();
    void draw() const;
    void cleanup();

    float getHeightAt(float worldX, float worldZ) const;
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

    void calculateBounds();
};