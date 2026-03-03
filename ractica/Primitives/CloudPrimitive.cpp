#include "../pch.h"
#include "CloudPrimitive.h"
#include "PrimitiveBase.h"

void CloudPrimitive::create(Model& model) {
    model.vertices.clear();

    // Облако из нескольких сфер
    std::vector<Vertex> cloudVertices;

    // Центральная часть
    PrimitiveBase::createSphere(model, 8, 4);

    // Сохраняем центральную часть
    cloudVertices = model.vertices;
    model.vertices.clear();

    // Добавляем дополнительные части
    PrimitiveBase::createSphere(model, 6, 3);

    // Смещаем и добавляем к облаку
    for (auto& v : model.vertices) {
        v.position.x += 0.3f;
        v.position.y += 0.1f;
        cloudVertices.push_back(v);
    }

    model.vertices.clear();
    PrimitiveBase::createSphere(model, 6, 3);
    for (auto& v : model.vertices) {
        v.position.x -= 0.3f;
        v.position.y += 0.1f;
        cloudVertices.push_back(v);
    }

    model.vertices.clear();
    PrimitiveBase::createSphere(model, 5, 3);
    for (auto& v : model.vertices) {
        v.position.y += 0.3f;
        cloudVertices.push_back(v);
    }

    model.vertices = cloudVertices;
    model.hasTexture = false;
    model.setupBuffers();
}