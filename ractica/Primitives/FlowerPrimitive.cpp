#include "../pch.h"
#include "FlowerPrimitive.h"
#include "PrimitiveBase.h"

void FlowerPrimitive::create(Model& model) {
    model.vertices.clear();

    std::vector<Vertex> flowerVertices;

    // Центр цветка
    PrimitiveBase::createCircle(flowerVertices, 0.0f, 0.0f, 0.1f, 8);

    // Лепестки
    for (int i = 0; i < 6; i++) {
        float angle = i * 3.14159f / 3.0f;
        float x = cos(angle) * 0.25f;
        float y = sin(angle) * 0.25f;
        PrimitiveBase::createCircle(flowerVertices, x, y, 0.08f, 6);
    }

    // Стебель
    Vertex stem[] = {
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    model.vertices = flowerVertices;
    model.vertices.insert(model.vertices.end(), std::begin(stem), std::end(stem));

    model.hasTexture = false;
    model.setupBuffers();
}