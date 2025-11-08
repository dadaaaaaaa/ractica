#pragma once
#include "../Core/Types.h"
#include <vector>
#include <GL/glew.h>
#include "../pch.h"

// Структура для ограничивающего объема (bounding box)
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox() {
        min = glm::vec3(0.0f);
        max = glm::vec3(0.0f);
    }
    BoundingBox(const glm::vec3& minPoint, const glm::vec3& maxPoint) {
        min = minPoint;
        max = maxPoint;
    }
};

// Класс для работы с 3D моделями в OpenGL
class Model {
private:
    // ============================================================================
    // ДАННЫЕ МОДЕЛИ
    // ============================================================================

    std::vector<Vertex> vertices;    // Массив вершин модели
    GLuint VAO;                      // Vertex Array Object
    GLuint VBO;                      // Vertex Buffer Object  
    GLuint textureID;                // ID текстуры в OpenGL
    bool m_hasTexture;               // Переименовано для избежания конфликта
    BoundingBox boundingBox;         // Ограничивающий объем для оптимизации

public:
    // ============================================================================
    // КОНСТРУКТОР И ДЕСТРУКТОР
    // ============================================================================

    Model();
    ~Model();

    // ============================================================================
    // ОСНОВНЫЕ МЕТОДЫ РАБОТЫ С МОДЕЛЬЮ
    // ============================================================================

    void setupBuffers();
    void draw() const;
    void draw(GLenum primitive) const;
    void cleanup();

    // ============================================================================
    // МЕТОДЫ ДОБАВЛЕНИЯ И УПРАВЛЕНИЯ ВЕРШИНАМИ
    // ============================================================================

    void addVertex(const Vertex& vertex);
    void addVertices(const std::vector<Vertex>& newVertices);
    void clearVertices() { vertices.clear(); }

    // ============================================================================
    // МЕТОДЫ РАБОТЫ С ТЕКСТУРАМИ
    // ============================================================================

    void setTexture(GLuint newTextureID);
    void removeTexture();

    // ============================================================================
    // ИНФОРМАЦИОННЫЕ МЕТОДЫ
    // ============================================================================

    size_t getVertexCount() const { return vertices.size(); }
    size_t getTriangleCount() const { return vertices.size() / 3; }
    bool isReady() const { return VAO != 0 && !vertices.empty(); }
    bool hasTexture() const { return m_hasTexture && textureID != 0; } // Исправлено имя
    const BoundingBox& getBoundingBox() const { return boundingBox; }

    // ============================================================================
    // ГЕТТЕРЫ ДЛЯ ДОСТУПА К ДАННЫМ
    // ============================================================================

    const std::vector<Vertex>& getVertices() const { return vertices; }
    GLuint getTextureID() const { return textureID; }
    GLuint getVAO() const { return VAO; }
    GLuint getVBO() const { return VBO; }

    // ============================================================================
    // СЛУЖЕБНЫЕ МЕТОДЫ
    // ============================================================================

    void calculateBoundingBox(); // Убрали const!
    void printInfo() const;
    bool isPointInBoundingBox(const glm::vec3& point) const;

private:
    // ============================================================================
    // ПРИВАТНЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    // ============================================================================

    bool validateModelData() const;
    void updateBoundingBox();
};