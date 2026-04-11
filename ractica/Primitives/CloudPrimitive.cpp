#include "../pch.h"
#include "CloudPrimitive.h"
#include "PrimitiveBase.h"

void CloudPrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(2000);

    // Центральная часть (самая большая)
    PrimitiveBase::createSphere(model, 14, 10);

    // Сохраняем центральную часть
    std::vector<Vertex> cloudVertices = model.vertices;
    model.vertices.clear();

    // Левая часть
    PrimitiveBase::createSphere(model, 12, 8);
    for (auto& v : model.vertices) {
        v.position.x -= 0.5f;
        v.position.y -= 0.05f;
        v.position.z += 0.1f;
        cloudVertices.push_back(v);
    }

    // Правая часть
    PrimitiveBase::createSphere(model, 12, 8);
    for (auto& v : model.vertices) {
        v.position.x += 0.5f;
        v.position.y -= 0.05f;
        v.position.z -= 0.05f;
        cloudVertices.push_back(v);
    }

    // Верхняя часть
    PrimitiveBase::createSphere(model, 12, 8);
    for (auto& v : model.vertices) {
        v.position.y += 0.35f;
        v.position.z -= 0.05f;
        cloudVertices.push_back(v);
    }

    // Нижняя левая
    PrimitiveBase::createSphere(model, 10, 8);
    for (auto& v : model.vertices) {
        v.position.x -= 0.28f;
        v.position.y -= 0.25f;
        v.position.z -= 0.1f;
        v.position.x *= 0.9f;
        v.position.z *= 0.9f;
        cloudVertices.push_back(v);
    }

    // Нижняя правая
    PrimitiveBase::createSphere(model, 10, 8);
    for (auto& v : model.vertices) {
        v.position.x += 0.32f;
        v.position.y -= 0.23f;
        v.position.z += 0.08f;
        v.position.x *= 0.9f;
        v.position.z *= 0.9f;
        cloudVertices.push_back(v);
    }

    // Задняя часть
    PrimitiveBase::createSphere(model, 10, 8);
    for (auto& v : model.vertices) {
        v.position.z -= 0.48f;
        v.position.y += 0.02f;
        v.position.x *= 0.85f;
        cloudVertices.push_back(v);
    }

    // Передняя часть
    PrimitiveBase::createSphere(model, 10, 8);
    for (auto& v : model.vertices) {
        v.position.z += 0.52f;
        v.position.y += 0.05f;
        v.position.x *= 0.85f;
        cloudVertices.push_back(v);
    }

    // Маленькая верхушка
    PrimitiveBase::createSphere(model, 10, 8);
    for (auto& v : model.vertices) {
        v.position.x += 0.18f;
        v.position.y += 0.55f;
        v.position.z -= 0.08f;
        v.position.x *= 0.8f;
        v.position.y *= 0.9f;
        v.position.z *= 0.8f;
        cloudVertices.push_back(v);
    }

    model.vertices = cloudVertices;
    model.hasTexture = false;
    model.setupBuffers();
}