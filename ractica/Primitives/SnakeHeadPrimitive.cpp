#include "../pch.h"
#include "SnakeHeadPrimitive.h"
#include "PrimitiveBase.h"

void SnakeHeadPrimitive::create(Model& model) {
    model.vertices.clear();

    // Используем готовый куб из PrimitiveBase
    PrimitiveBase::createCube(model);

    // Масштабируем вершины для формы головы
    for (auto& vertex : model.vertices) {
        vertex.position.x *= 1.1f;   // Шире
        vertex.position.y *= 1.05f;  // Выше
        vertex.position.z *= 0.9f;   // Короче
        vertex.normal = glm::normalize(vertex.position);
    }

    // Создаём отдельную модель для глаз
    Model eyeModel;
    PrimitiveBase::createSphere(eyeModel, 8, 6);

    // Левый глаз
    for (auto& vertex : eyeModel.vertices) {
        vertex.position.x -= 0.35f;
        vertex.position.y += 0.35f;
        vertex.position.z += 0.52f;
        vertex.position *= 0.12f;  // Размер глаза
        model.vertices.push_back(vertex);
    }

    // Правый глаз
    for (auto& vertex : eyeModel.vertices) {
        vertex.position.x += 0.70f;  // 0.35 + 0.35 = 0.70
        vertex.position.y += 0.35f;
        vertex.position.z += 0.52f;
        vertex.position *= 0.12f;
        model.vertices.push_back(vertex);
    }

    // Зрачки (маленькие сферы)
    Model pupilModel;
    PrimitiveBase::createSphere(pupilModel, 6, 4);

    // Левый зрачок
    for (auto& vertex : pupilModel.vertices) {
        vertex.position.x -= 0.35f;
        vertex.position.y += 0.32f;
        vertex.position.z += 0.58f;
        vertex.position *= 0.06f;
        model.vertices.push_back(vertex);
    }

    // Правый зрачок
    for (auto& vertex : pupilModel.vertices) {
        vertex.position.x += 0.70f;
        vertex.position.y += 0.32f;
        vertex.position.z += 0.58f;
        vertex.position *= 0.06f;
        model.vertices.push_back(vertex);
    }

    model.hasTexture = false;
    model.setupBuffers();
}