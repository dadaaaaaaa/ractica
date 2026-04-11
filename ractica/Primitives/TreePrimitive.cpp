#include "../pch.h"
#include "TreePrimitive.h"
#include "PrimitiveBase.h"

void TreePrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(2000);  // Резервируем память

    // Ствол - цилиндр
    const int segments = 12;
    float trunkRadius = 0.15f;
    float trunkHeight = 0.6f;

    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, 0.0f, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, 0.0f, sin(angle2) * trunkRadius);
        glm::vec3 p3(cos(angle1) * trunkRadius, trunkHeight, sin(angle1) * trunkRadius);
        glm::vec3 p4(cos(angle2) * trunkRadius, trunkHeight, sin(angle2) * trunkRadius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        // Треугольник 1
        model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        // Треугольник 2
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    // Нижняя крышка ствола
    glm::vec3 centerBottom(0.0f, 0.0f, 0.0f);
    glm::vec3 normalBottom(0.0f, -1.0f, 0.0f);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, 0.0f, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, 0.0f, sin(angle2) * trunkRadius);

        model.vertices.push_back({ centerBottom, normalBottom, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p1, normalBottom, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p2, normalBottom, glm::vec2(1.0f, 0.0f) });
    }

    // Верхняя крышка ствола
    glm::vec3 centerTop(0.0f, trunkHeight, 0.0f);
    glm::vec3 normalTop(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, trunkHeight, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, trunkHeight, sin(angle2) * trunkRadius);

        model.vertices.push_back({ centerTop, normalTop, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p2, normalTop, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p1, normalTop, glm::vec2(0.0f, 0.0f) });
    }

    // Крона - сфера (нижний ярус)
    const int crownSegments = 16;
    const int crownRings = 12;
    float crownRadius = 0.45f;
    glm::vec3 crownCenter(0.0f, trunkHeight + 0.1f, 0.0f);

    for (int i = 0; i < crownRings; ++i) {
        for (int j = 0; j < crownSegments; ++j) {
            float theta1 = (float)i / crownRings * 3.14159f;
            float theta2 = (float)(i + 1) / crownRings * 3.14159f;
            float phi1 = (float)j / crownSegments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / crownSegments * 2.0f * 3.14159f;

            glm::vec3 v1 = crownCenter + glm::vec3(
                crownRadius * sin(theta1) * cos(phi1),
                crownRadius * cos(theta1),
                crownRadius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = crownCenter + glm::vec3(
                crownRadius * sin(theta1) * cos(phi2),
                crownRadius * cos(theta1),
                crownRadius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = crownCenter + glm::vec3(
                crownRadius * sin(theta2) * cos(phi2),
                crownRadius * cos(theta2),
                crownRadius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = crownCenter + glm::vec3(
                crownRadius * sin(theta2) * cos(phi1),
                crownRadius * cos(theta2),
                crownRadius * sin(theta2) * sin(phi1)
            );

            glm::vec3 n1 = glm::normalize(v1 - crownCenter);
            glm::vec3 n2 = glm::normalize(v2 - crownCenter);
            glm::vec3 n3 = glm::normalize(v3 - crownCenter);
            glm::vec3 n4 = glm::normalize(v4 - crownCenter);

            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ v2, n2, glm::vec2(1.0f, 0.0f) });
            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });

            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });
            model.vertices.push_back({ v4, n4, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
        }
    }

    // Верхушка кроны (маленькая сфера)
    float topRadius = 0.3f;
    glm::vec3 topCenter(0.0f, trunkHeight + 0.55f, 0.0f);
    int topRings = crownRings / 2;

    for (int i = 0; i < topRings; ++i) {
        for (int j = 0; j < crownSegments; ++j) {
            float theta1 = (float)i / topRings * 3.14159f;
            float theta2 = (float)(i + 1) / topRings * 3.14159f;
            float phi1 = (float)j / crownSegments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / crownSegments * 2.0f * 3.14159f;

            glm::vec3 v1 = topCenter + glm::vec3(
                topRadius * sin(theta1) * cos(phi1),
                topRadius * cos(theta1),
                topRadius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = topCenter + glm::vec3(
                topRadius * sin(theta1) * cos(phi2),
                topRadius * cos(theta1),
                topRadius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = topCenter + glm::vec3(
                topRadius * sin(theta2) * cos(phi2),
                topRadius * cos(theta2),
                topRadius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = topCenter + glm::vec3(
                topRadius * sin(theta2) * cos(phi1),
                topRadius * cos(theta2),
                topRadius * sin(theta2) * sin(phi1)
            );

            glm::vec3 n1 = glm::normalize(v1 - topCenter);
            glm::vec3 n2 = glm::normalize(v2 - topCenter);
            glm::vec3 n3 = glm::normalize(v3 - topCenter);
            glm::vec3 n4 = glm::normalize(v4 - topCenter);

            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ v2, n2, glm::vec2(1.0f, 0.0f) });
            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });

            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });
            model.vertices.push_back({ v4, n4, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
        }
    }

    model.hasTexture = false;
    model.setupBuffers();
}