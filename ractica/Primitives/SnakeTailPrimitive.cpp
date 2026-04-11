#include "../pch.h"
#include "SnakeTailPrimitive.h"
#include "PrimitiveBase.h"

void SnakeTailPrimitive::create(Model& model) {
    model.vertices.clear();

    // Используем готовый цилиндр из PrimitiveBase и сужаем его
    PrimitiveBase::createCylinder(model, 0.45f, 0.7f, 24);

    // Сужаем цилиндр к концу, чтобы получился конус
    for (auto& vertex : model.vertices) {
        // Определяем фактор сужения в зависимости от Z
        float t = (vertex.position.z + 0.35f) / 0.7f;  // от 0 до 1 от начала до конца
        float scaleFactor = 1.0f - t * 0.9f;  // Сужаем к концу

        vertex.position.x *= scaleFactor;
        vertex.position.y *= scaleFactor;

        // Пересчитываем нормали
        vertex.normal = glm::normalize(vertex.position);
    }

    model.hasTexture = false;
    model.setupBuffers();
}