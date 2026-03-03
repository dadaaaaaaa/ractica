#include "../pch.h"
#include "BirdPrimitive.h"
#include "PrimitiveBase.h"

void BirdPrimitive::create(Model& model) {
    model.vertices.clear();

    // Тело птицы (сфера)
    PrimitiveBase::createSphere(model, 8, 4);

    // Голова (маленькая сфера)
    std::vector<Vertex> headVertices;
    PrimitiveBase::createSphere(model, 6, 3);

    // Смещаем голову
    for (auto& v : model.vertices) {
        v.position.x += 0.4f;
        v.position.y += 0.1f;
        headVertices.push_back(v);
    }

    // Клюв (конус)
    Vertex beak[] = {
        {{0.55f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.7f, 0.1f, -0.05f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.7f, 0.1f, 0.05f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.55f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.7f, 0.15f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        {{0.7f, 0.1f, -0.05f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    // Хвост
    Vertex tail[] = {
        {{-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.6f, 0.0f, -0.15f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.6f, 0.0f, 0.15f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.6f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        {{-0.6f, 0.0f, -0.15f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    // Крылья (два треугольника)
    Vertex wing1[] = {
        {{0.2f, 0.0f, 0.2f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}},
        {{0.5f, 0.2f, 0.3f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.1f, 0.3f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}
    };

    Vertex wing2[] = {
        {{0.2f, 0.0f, -0.2f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}},
        {{0.5f, 0.2f, -0.3f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.1f, -0.3f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}
    };

    model.vertices.insert(model.vertices.end(), headVertices.begin(), headVertices.end());
    model.vertices.insert(model.vertices.end(), std::begin(beak), std::end(beak));
    model.vertices.insert(model.vertices.end(), std::begin(tail), std::end(tail));
    model.vertices.insert(model.vertices.end(), std::begin(wing1), std::end(wing1));
    model.vertices.insert(model.vertices.end(), std::begin(wing2), std::end(wing2));

    model.hasTexture = false;
    model.setupBuffers();
}