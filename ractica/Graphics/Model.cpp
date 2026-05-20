#include "../pch.h"
#include "Model.h"
#include <algorithm>
#include <iostream>

Model::Model()
    : displayList(0)
    , textureID(0)
    , hasTexture(false)
    , isCompiled(false)
    , minX(0), maxX(0), minZ(0), maxZ(0)
    , width(0), depth(0)
    , boundingCenter(0.0f)
    , boundingRadius(0.0f)
    , boundingSphereComputed(false) {
}

void Model::setupBuffers() {
    if (vertices.empty()) {
        std::cout << "Warning: No vertices to setup buffers!" << std::endl;
        return;
    }

    if (displayList != 0) {
        glDeleteLists(displayList, 1);
        displayList = 0;
    }

    displayList = glGenLists(1);
    if (displayList == 0) {
        std::cout << "Error: Failed to generate display list!" << std::endl;
        return;
    }

    glNewList(displayList, GL_COMPILE);

    glBegin(GL_TRIANGLES);
    for (const auto& vertex : vertices) {
        glNormal3f(vertex.normal.x, vertex.normal.y, vertex.normal.z);

        if (hasTexture && textureID != 0) {
            glTexCoord2f(vertex.texCoords.x, vertex.texCoords.y);
        }

        glVertex3f(vertex.position.x, vertex.position.y, vertex.position.z);
    }
    glEnd();

    glEndList();

    isCompiled = true;
    calculateBounds();

    // ✅ ВЫЗЫВАЕМ ВЫЧИСЛЕНИЕ BOUNDING SPHERE
    computeBoundingSphere();
}

void Model::cleanup() {
    if (displayList != 0) {
        glDeleteLists(displayList, 1);
        displayList = 0;
    }

    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    vertices.clear();
    heightMap.clear();
    hasTexture = false;
    isCompiled = false;
    boundingSphereComputed = false;
}

void Model::draw() const {
    if (!isCompiled || displayList == 0) {
        if (!vertices.empty()) {
            glBegin(GL_TRIANGLES);
            for (const auto& vertex : vertices) {
                glNormal3f(vertex.normal.x, vertex.normal.y, vertex.normal.z);
                if (hasTexture && textureID != 0) {
                    glTexCoord2f(vertex.texCoords.x, vertex.texCoords.y);
                }
                glVertex3f(vertex.position.x, vertex.position.y, vertex.position.z);
            }
            glEnd();
        }
        return;
    }

    if (hasTexture && textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    glCallList(displayList);

    if (hasTexture && textureID != 0) {
        glDisable(GL_TEXTURE_2D);
    }
}

void Model::calculateBounds() {
    if (vertices.empty()) return;

    minX = maxX = vertices[0].position.x;
    minZ = maxZ = vertices[0].position.z;

    for (const auto& v : vertices) {
        minX = std::min(minX, v.position.x);
        maxX = std::max(maxX, v.position.x);
        minZ = std::min(minZ, v.position.z);
        maxZ = std::max(maxZ, v.position.z);
    }

    width = maxX - minX;
    depth = maxZ - minZ;

    heightMap.clear();
    heightMap.resize(vertices.size() / 3);

    for (size_t i = 0; i < vertices.size() / 3; i++) {
        float y1 = vertices[i * 3].position.y;
        float y2 = vertices[i * 3 + 1].position.y;
        float y3 = vertices[i * 3 + 2].position.y;
        heightMap[i] = (y1 + y2 + y3) / 3.0f;
    }
}

// ✅ НОВАЯ ФУНКЦИЯ - ВЫЧИСЛЕНИЕ BOUNDING SPHERE
void Model::computeBoundingSphere() {
    if (vertices.empty()) {
        boundingSphereComputed = false;
        return;
    }

    float minX = vertices[0].position.x;
    float maxX = minX, minY = minX, maxY = minX, minZ = minX, maxZ = minX;

    for (const auto& vert : vertices) {
        minX = std::min(minX, vert.position.x);
        maxX = std::max(maxX, vert.position.x);
        minY = std::min(minY, vert.position.y);
        maxY = std::max(maxY, vert.position.y);
        minZ = std::min(minZ, vert.position.z);
        maxZ = std::max(maxZ, vert.position.z);
    }

    boundingCenter = glm::vec3(
        (minX + maxX) * 0.5f,
        (minY + maxY) * 0.5f,
        (minZ + maxZ) * 0.5f
    );

    float dx = maxX - minX;
    float dy = maxY - minY;
    float dz = maxZ - minZ;
    boundingRadius = std::max({ dx, dy, dz }) * 0.5f;

    boundingSphereComputed = true;
}

float Model::getHeightAt(float worldX, float worldZ) const {
    if (vertices.empty() || heightMap.empty()) return 0.0f;

    float bestDist = 1e9;
    float bestY = 0.0f;

    for (size_t i = 0; i < vertices.size() / 3; i++) {
        float cx = (vertices[i * 3].position.x + vertices[i * 3 + 1].position.x + vertices[i * 3 + 2].position.x) / 3.0f;
        float cz = (vertices[i * 3].position.z + vertices[i * 3 + 1].position.z + vertices[i * 3 + 2].position.z) / 3.0f;

        float dx = cx - worldX;
        float dz = cz - worldZ;
        float dist = dx * dx + dz * dz;

        if (dist < bestDist) {
            bestDist = dist;
            bestY = heightMap[i];
        }
    }

    return bestY;
}

void Model::computeNormals() {
    if (vertices.empty()) return;

    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    for (size_t i = 0; i < vertices.size(); i += 3) {
        glm::vec3& v0 = vertices[i].position;
        glm::vec3& v1 = vertices[i + 1].position;
        glm::vec3& v2 = vertices[i + 2].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 faceNormal = glm::cross(edge1, edge2);
        float length = glm::length(faceNormal);
        if (length > 0.0001f) {
            faceNormal = faceNormal / length;
        }
        else {
            faceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        vertices[i].normal += faceNormal;
        vertices[i + 1].normal += faceNormal;
        vertices[i + 2].normal += faceNormal;
    }

    for (auto& vertex : vertices) {
        float length = glm::length(vertex.normal);
        if (length > 0.0001f) {
            vertex.normal = vertex.normal / length;
        }
        else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}