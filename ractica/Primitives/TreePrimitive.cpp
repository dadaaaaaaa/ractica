#include "../pch.h"
#include "TreePrimitive.h"
#include "PrimitiveBase.h"

void TreePrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(6000);

    // ===== ПЕРВЫЙ СТВОЛ (от пола до сферы) =====
    const int segments = 16;
    float trunkRadius = 0.25f;
    float trunk1Height = 1.0f;  // Высота первого ствола (до сферы)

    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, 0.0f, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, 0.0f, sin(angle2) * trunkRadius);
        glm::vec3 p3(cos(angle1) * trunkRadius, trunk1Height, sin(angle1) * trunkRadius);
        glm::vec3 p4(cos(angle2) * trunkRadius, trunk1Height, sin(angle2) * trunkRadius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    // Нижняя крышка первого ствола
    glm::vec3 centerBottom(0.0f, 0.0f, 0.0f);
    glm::vec3 trunkNormalBottom(0.0f, -1.0f, 0.0f);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, 0.0f, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, 0.0f, sin(angle2) * trunkRadius);

        model.vertices.push_back({ centerBottom, trunkNormalBottom, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p1, trunkNormalBottom, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p2, trunkNormalBottom, glm::vec2(1.0f, 0.0f) });
    }

    // Верхняя крышка первого ствола
    glm::vec3 centerTop1(0.0f, trunk1Height, 0.0f);
    glm::vec3 trunkNormalTop(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, trunk1Height, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, trunk1Height, sin(angle2) * trunkRadius);

        model.vertices.push_back({ centerTop1, trunkNormalTop, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p2, trunkNormalTop, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p1, trunkNormalTop, glm::vec2(0.0f, 0.0f) });
    }

    // ===== СФЕРА (КРОНА) =====
    const int crownSegments = 32;
    const int crownRings = 24;
    float crownRadius = 0.8f;
    float sphereYOffset = trunk1Height + crownRadius;  // Сфера стоит на первом стволе
    glm::vec3 crownCenter(0.0f, sphereYOffset, 0.0f);

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

    // ===== ВТОРОЙ СТВОЛ (от сферы до куба) =====
    float trunk2Height = 0.6f;  // Высота второго ствола (от сферы до куба)
    float trunk2Radius = 0.18f;  // Немного тоньше первого ствола
    float trunk2YStart = sphereYOffset + crownRadius;  // Начинается от верха сферы
    float trunk2YEnd = trunk2YStart + trunk2Height;

    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunk2Radius, trunk2YStart, sin(angle1) * trunk2Radius);
        glm::vec3 p2(cos(angle2) * trunk2Radius, trunk2YStart, sin(angle2) * trunk2Radius);
        glm::vec3 p3(cos(angle1) * trunk2Radius, trunk2YEnd, sin(angle1) * trunk2Radius);
        glm::vec3 p4(cos(angle2) * trunk2Radius, trunk2YEnd, sin(angle2) * trunk2Radius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    // Нижняя крышка второго ствола
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunk2Radius, trunk2YStart, sin(angle1) * trunk2Radius);
        glm::vec3 p2(cos(angle2) * trunk2Radius, trunk2YStart, sin(angle2) * trunk2Radius);

        model.vertices.push_back({ glm::vec3(0.0f, trunk2YStart, 0.0f), trunkNormalBottom, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p1, trunkNormalBottom, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p2, trunkNormalBottom, glm::vec2(1.0f, 0.0f) });
    }

    // Верхняя крышка второго ствола
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunk2Radius, trunk2YEnd, sin(angle1) * trunk2Radius);
        glm::vec3 p2(cos(angle2) * trunk2Radius, trunk2YEnd, sin(angle2) * trunk2Radius);

        model.vertices.push_back({ glm::vec3(0.0f, trunk2YEnd, 0.0f), trunkNormalTop, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p2, trunkNormalTop, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p1, trunkNormalTop, glm::vec2(0.0f, 0.0f) });
    }

    // ===== БОЛЬШОЙ КУБ (на втором стволе) =====
    float cubeSize = 0.7f;
    float cubeYOffset = trunk2YEnd + cubeSize / 2.0f;  // Куб на вершине второго ствола

    glm::vec3 cubeMin(-cubeSize / 2.0f, cubeYOffset - cubeSize / 2.0f, -cubeSize / 2.0f);
    glm::vec3 cubeMax(cubeSize / 2.0f, cubeYOffset + cubeSize / 2.0f, cubeSize / 2.0f);

    glm::vec3 v[8] = {
        glm::vec3(cubeMin.x, cubeMin.y, cubeMin.z),
        glm::vec3(cubeMax.x, cubeMin.y, cubeMin.z),
        glm::vec3(cubeMax.x, cubeMin.y, cubeMax.z),
        glm::vec3(cubeMin.x, cubeMin.y, cubeMax.z),
        glm::vec3(cubeMin.x, cubeMax.y, cubeMin.z),
        glm::vec3(cubeMax.x, cubeMax.y, cubeMin.z),
        glm::vec3(cubeMax.x, cubeMax.y, cubeMax.z),
        glm::vec3(cubeMin.x, cubeMax.y, cubeMax.z)
    };

    glm::vec3 cubeNormalFront(0.0f, 0.0f, -1.0f);
    glm::vec3 cubeNormalBack(0.0f, 0.0f, 1.0f);
    glm::vec3 cubeNormalLeft(-1.0f, 0.0f, 0.0f);
    glm::vec3 cubeNormalRight(1.0f, 0.0f, 0.0f);
    glm::vec3 cubeNormalTop(0.0f, 1.0f, 0.0f);
    glm::vec3 cubeNormalBottom(0.0f, -1.0f, 0.0f);

    glm::vec2 tex00(0.0f, 0.0f);
    glm::vec2 tex10(1.0f, 0.0f);
    glm::vec2 tex01(0.0f, 1.0f);
    glm::vec2 tex11(1.0f, 1.0f);

    // Передняя грань
    model.vertices.push_back({ v[0], cubeNormalFront, tex00 });
    model.vertices.push_back({ v[1], cubeNormalFront, tex10 });
    model.vertices.push_back({ v[5], cubeNormalFront, tex11 });
    model.vertices.push_back({ v[5], cubeNormalFront, tex11 });
    model.vertices.push_back({ v[4], cubeNormalFront, tex01 });
    model.vertices.push_back({ v[0], cubeNormalFront, tex00 });

    // Задняя грань
    model.vertices.push_back({ v[2], cubeNormalBack, tex00 });
    model.vertices.push_back({ v[3], cubeNormalBack, tex10 });
    model.vertices.push_back({ v[7], cubeNormalBack, tex11 });
    model.vertices.push_back({ v[7], cubeNormalBack, tex11 });
    model.vertices.push_back({ v[6], cubeNormalBack, tex01 });
    model.vertices.push_back({ v[2], cubeNormalBack, tex00 });

    // Левая грань
    model.vertices.push_back({ v[3], cubeNormalLeft, tex00 });
    model.vertices.push_back({ v[0], cubeNormalLeft, tex10 });
    model.vertices.push_back({ v[4], cubeNormalLeft, tex11 });
    model.vertices.push_back({ v[4], cubeNormalLeft, tex11 });
    model.vertices.push_back({ v[7], cubeNormalLeft, tex01 });
    model.vertices.push_back({ v[3], cubeNormalLeft, tex00 });

    // Правая грань
    model.vertices.push_back({ v[1], cubeNormalRight, tex00 });
    model.vertices.push_back({ v[2], cubeNormalRight, tex10 });
    model.vertices.push_back({ v[6], cubeNormalRight, tex11 });
    model.vertices.push_back({ v[6], cubeNormalRight, tex11 });
    model.vertices.push_back({ v[5], cubeNormalRight, tex01 });
    model.vertices.push_back({ v[1], cubeNormalRight, tex00 });

    // Верхняя грань
    model.vertices.push_back({ v[4], cubeNormalTop, tex00 });
    model.vertices.push_back({ v[5], cubeNormalTop, tex10 });
    model.vertices.push_back({ v[6], cubeNormalTop, tex11 });
    model.vertices.push_back({ v[6], cubeNormalTop, tex11 });
    model.vertices.push_back({ v[7], cubeNormalTop, tex01 });
    model.vertices.push_back({ v[4], cubeNormalTop, tex00 });

    // Нижняя грань
    model.vertices.push_back({ v[1], cubeNormalBottom, tex00 });
    model.vertices.push_back({ v[0], cubeNormalBottom, tex10 });
    model.vertices.push_back({ v[3], cubeNormalBottom, tex11 });
    model.vertices.push_back({ v[3], cubeNormalBottom, tex11 });
    model.vertices.push_back({ v[2], cubeNormalBottom, tex01 });
    model.vertices.push_back({ v[1], cubeNormalBottom, tex00 });

    model.hasTexture = false;
    model.setupBuffers();
}