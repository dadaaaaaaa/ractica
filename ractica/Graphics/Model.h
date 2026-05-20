#pragma once
#include "../Core/Types.h"
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <functional>

class Model {
public:
    std::vector<Vertex> vertices;
    GLuint displayList;
    GLuint textureID;
    bool hasTexture;
    bool isCompiled;

    std::vector<float> heightMap;
    float minX, maxX, minZ, maxZ;
    float width, depth;

    // ✅ НОВЫЕ ПОЛЯ ДЛЯ BOUNDING SPHERE
    glm::vec3 boundingCenter;
    float boundingRadius;
    bool boundingSphereComputed;

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
    void computeNormals();

    // ✅ НОВЫЙ МЕТОД
    void computeBoundingSphere();
};