#include "../pch.h"
#include "ApplePrimitive.h"
#include "PrimitiveBase.h"

void ApplePrimitive::create(Model& model) {
    model.vertices.clear();

    // Яблоко - сфера
    PrimitiveBase::createSphere(model, 12, 8);

    // Добавляем черенок
    std::vector<Vertex> stemVertices;
    float stemHeight = 0.3f;
    float stemRadius = 0.05f;

    for (int i = 0; i < 8; i++) {
        float angle1 = 2.0f * 3.14159f * i / 8;
        float angle2 = 2.0f * 3.14159f * (i + 1) / 8;

        glm::vec3 p1(0.0f + cos(angle1) * stemRadius, 0.5f, 0.0f + sin(angle1) * stemRadius);
        glm::vec3 p2(0.0f + cos(angle2) * stemRadius, 0.5f, 0.0f + sin(angle2) * stemRadius);
        glm::vec3 p3(0.0f + cos(angle1) * stemRadius, 0.5f + stemHeight, 0.0f + sin(angle1) * stemRadius);
        glm::vec3 p4(0.0f + cos(angle2) * stemRadius, 0.5f + stemHeight, 0.0f + sin(angle2) * stemRadius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        stemVertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        stemVertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        stemVertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        stemVertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        stemVertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        stemVertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    // Добавляем черенок к яблоку
    model.vertices.insert(model.vertices.end(), stemVertices.begin(), stemVertices.end());
    model.hasTexture = false;
    model.setupBuffers();
}