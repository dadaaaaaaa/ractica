#include "../pch.h"
#include "TreePrimitive.h"
#include "PrimitiveBase.h"

void TreePrimitive::create(Model& model) {
    model.vertices.clear();

    // Ствол
    std::vector<Vertex> trunkVertices;
    float trunkRadius = 0.15f;
    float trunkHeight = 0.8f;

    for (int i = 0; i < 8; i++) {
        float angle1 = 2.0f * 3.14159f * i / 8;
        float angle2 = 2.0f * 3.14159f * (i + 1) / 8;

        glm::vec3 p1(cos(angle1) * trunkRadius, 0.0f, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, 0.0f, sin(angle2) * trunkRadius);
        glm::vec3 p3(cos(angle1) * trunkRadius, trunkHeight, sin(angle1) * trunkRadius);
        glm::vec3 p4(cos(angle2) * trunkRadius, trunkHeight, sin(angle2) * trunkRadius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        trunkVertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        trunkVertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        trunkVertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        trunkVertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        trunkVertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        trunkVertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    model.vertices = trunkVertices;

    // Крона - несколько сфер
    std::vector<Vertex> crownVertices;
    PrimitiveBase::createSphere(model, 10, 6);

    // Смещаем и масштабируем вершины для кроны
    int startIdx = trunkVertices.size();
    for (int i = startIdx; i < model.vertices.size(); i++) {
        model.vertices[i].position.y += trunkHeight + 0.2f;
        model.vertices[i].position.x *= 1.5f;
        model.vertices[i].position.z *= 1.5f;
    }

    model.hasTexture = false;
    model.setupBuffers();
}