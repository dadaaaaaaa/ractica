#include "../pch.h"
#include "SnakeBodyPrimitive.h"
#include "PrimitiveBase.h"

void SnakeBodyPrimitive::create(Model& model) {
    model.vertices.clear();

    // Используем готовый куб из PrimitiveBase
    PrimitiveBase::createCube(model);

    // Масштабируем вершины для формы тела
    for (auto& vertex : model.vertices) {
        vertex.position.z *= 1.15f;  // Вытягиваем вдоль движения
        vertex.position.y *= 0.95f;  // Чуть приплюснутое
        vertex.normal = glm::normalize(vertex.position);
    }

    model.hasTexture = false;
    model.setupBuffers();
}