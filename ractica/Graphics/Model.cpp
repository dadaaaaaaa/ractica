#include "../pch.h"
#include "Model.h"
#include <iostream>

// Конструктор модели - инициализирует все ресурсы нулевыми значениями
Model::Model() : VAO(0), VBO(0), textureID(0), m_hasTexture(false) { // Исправлено имя
}

// Деструктор модели - автоматически очищает ресурсы
Model::~Model() {
    cleanup();
}

// Настраивает Vertex Array Object и Vertex Buffer Object для рендеринга
void Model::setupBuffers() {
    // Проверяем, что есть вершины для рендеринга
    if (vertices.empty()) {
        std::cout << "⚠️ Warning: Attempting to setup buffers for empty model" << std::endl;
        return;
    }

    // Генерируем VAO и VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Привязываем VAO для настройки атрибутов
    glBindVertexArray(VAO);

    // Привязываем VBO и загружаем данные вершин
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
        vertices.data(), GL_STATIC_DRAW);

    // ============================================================================
    // НАСТРОЙКА АТРИБУТОВ ВЕРШИН
    // ============================================================================

    // Атрибут 0: Позиция вершины (3 компонента float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Атрибут 1: Нормаль вершины (3 компонента float)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // Атрибут 2: Текстурные координаты (2 компонента float)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);

    // Отвязываем буферы для безопасности
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "✅ Model buffers setup complete: " << vertices.size()
        << " vertices, VAO: " << VAO << ", VBO: " << VBO << std::endl;
}

// Очищает все ресурсы модели (VRAM и RAM)
void Model::cleanup() {
    // Удаляем Vertex Array Object
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
        std::cout << "🗑️  Deleted VAO" << std::endl;
    }

    // Удаляем Vertex Buffer Object
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
        std::cout << "🗑️  Deleted VBO" << std::endl;
    }

    // Удаляем текстуру если она была загружена
    if (textureID) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
        std::cout << "🗑️  Deleted texture" << std::endl;
    }

    // Очищаем вектор вершин
    vertices.clear();
    m_hasTexture = false; // Исправлено имя

    std::cout << "✅ Model cleanup complete" << std::endl;
}

// Рендерит модель с использованием текущих шейдеров
void Model::draw() const {
    // Проверяем, что модель готова к рендерингу
    if (VAO == 0 || vertices.empty()) {
        std::cout << "❌ Cannot draw model: not properly initialized" << std::endl;
        return;
    }

    // Привязываем текстуру если она есть
    if (m_hasTexture && textureID != 0) { // Исправлено имя
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    // Привязываем VAO и рисуем треугольники
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    // Отвязываем текстуру после рендеринга
    if (m_hasTexture && textureID != 0) { // Исправлено имя
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Рендерит модель с указанным примитивом (точки, линии, треугольники)
void Model::draw(GLenum primitive) const {
    if (VAO == 0 || vertices.empty()) {
        std::cout << "❌ Cannot draw model: not properly initialized" << std::endl;
        return;
    }

    if (m_hasTexture && textureID != 0) { // Исправлено имя
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    glBindVertexArray(VAO);
    glDrawArrays(primitive, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    if (m_hasTexture && textureID != 0) { // Исправлено имя
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Добавляет вершину в модель
void Model::addVertex(const Vertex& vertex) {
    vertices.push_back(vertex);
}

// Добавляет несколько вершин в модель
void Model::addVertices(const std::vector<Vertex>& newVertices) {
    vertices.insert(vertices.end(), newVertices.begin(), newVertices.end());
}

// Устанавливает текстуру для модели
void Model::setTexture(GLuint newTextureID) {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
    textureID = newTextureID;
    m_hasTexture = (textureID != 0); // Исправлено имя
}

// Удаляет текстуру модели
void Model::removeTexture() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    m_hasTexture = false; // Исправлено имя
}

// Создает bounding box для модели (для оптимизаций)
void Model::calculateBoundingBox() { // Убрали возвращаемый тип и const
    if (vertices.empty()) {
        boundingBox = BoundingBox();
        return;
    }

    // Инициализируем минимумы и максимумы первой вершиной
    boundingBox.min = vertices[0].position;
    boundingBox.max = vertices[0].position;

    // Находим реальные границы модели
    for (const auto& vertex : vertices) {
        // Поэлементное сравнение для min
        boundingBox.min.x = min(boundingBox.min.x, vertex.position.x);
        boundingBox.min.y = min(boundingBox.min.y, vertex.position.y);
        boundingBox.min.z = min(boundingBox.min.z, vertex.position.z);

        // Поэлементное сравнение для max
        boundingBox.max.x = max(boundingBox.max.x, vertex.position.x);
        boundingBox.max.y = max(boundingBox.max.y, vertex.position.y);
        boundingBox.max.z = max(boundingBox.max.z, vertex.position.z);
    }
}

// Выводит информацию о модели для отладки
void Model::printInfo() const {
    std::cout << "=== MODEL INFO ===" << std::endl;
    std::cout << "Vertices: " << vertices.size() << std::endl;
    std::cout << "Triangles: " << getTriangleCount() << std::endl;
    std::cout << "VAO: " << VAO << std::endl;
    std::cout << "VBO: " << VBO << std::endl;
    std::cout << "Has texture: " << (m_hasTexture ? "Yes" : "No") << std::endl; // Исправлено имя
    std::cout << "Texture ID: " << textureID << std::endl;

    if (!vertices.empty()) {
        // Создаем временный объект для вычисления bounding box
        Model* nonConstThis = const_cast<Model*>(this);
        nonConstThis->calculateBoundingBox();

        std::cout << "Bounding Box: " << std::endl;
        std::cout << "  Min: (" << boundingBox.min.x << ", " << boundingBox.min.y << ", " << boundingBox.min.z << ")" << std::endl;
        std::cout << "  Max: (" << boundingBox.max.x << ", " << boundingBox.max.y << ", " << boundingBox.max.z << ")" << std::endl;
    }
    std::cout << "==================" << std::endl;
}

// Проверяет, находится ли точка внутри ограничивающего объема
bool Model::isPointInBoundingBox(const glm::vec3& point) const {
    return point.x >= boundingBox.min.x && point.x <= boundingBox.max.x &&
        point.y >= boundingBox.min.y && point.y <= boundingBox.max.y &&
        point.z >= boundingBox.min.z && point.z <= boundingBox.max.z;
}

// Проверяет корректность данных модели перед настройкой буферов
bool Model::validateModelData() const {
    if (vertices.empty()) {
        std::cout << "❌ Model validation failed: no vertices" << std::endl;
        return false;
    }

    if (vertices.size() % 3 != 0) {
        std::cout << "⚠️ Model validation warning: vertex count not divisible by 3" << std::endl;
    }

    return true;
}

// Обновляет ограничивающий объем после изменения вершин
void Model::updateBoundingBox() {
    calculateBoundingBox();
}