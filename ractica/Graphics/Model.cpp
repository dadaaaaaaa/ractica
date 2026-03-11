#include "../pch.h"
#include "Model.h"
#include <algorithm>
#include <iostream>

Model::Model() : VAO(0), VBO(0), textureID(0), hasTexture(false),
minX(0), maxX(0), minZ(0), maxZ(0), width(0), depth(0) {
}

void Model::setupBuffers() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Вычисляем границы после установки вершин
    calculateBounds();
}

void Model::cleanup() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (textureID) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    vertices.clear();
    heightMap.clear();
    hasTexture = false;
}

void Model::draw() const {
    if (hasTexture && textureID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    if (hasTexture && textureID != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
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

    // Создаем карту высот для быстрого поиска
    heightMap.clear();
    heightMap.resize(vertices.size() / 3);

    for (size_t i = 0; i < vertices.size() / 3; i++) {
        // Берем среднюю высоту треугольника
        float y1 = vertices[i * 3].position.y;
        float y2 = vertices[i * 3 + 1].position.y;
        float y3 = vertices[i * 3 + 2].position.y;
        heightMap[i] = (y1 + y2 + y3) / 3.0f;
    }
}

float Model::getHeightAt(float worldX, float worldZ) const {
    if (vertices.empty() || heightMap.empty()) return 0.0f;

    // Упрощенный поиск - используем сетку
    static std::map<std::pair<int, int>, float> heightCache;

    // Округляем координаты до ближайшей клетки
    int gridX = static_cast<int>(worldX * 10.0f);
    int gridZ = static_cast<int>(worldZ * 10.0f);

    auto key = std::make_pair(gridX, gridZ);
    auto it = heightCache.find(key);
    if (it != heightCache.end()) {
        return it->second; // Возвращаем кэшированное значение
    }

    // Если нет в кэше, ищем ближайший треугольник
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

    // Кэшируем результат
    heightCache[key] = bestY;

    return bestY;
}