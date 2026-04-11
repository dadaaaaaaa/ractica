#include "../pch.h"
#include "FlowerPrimitive.h"
#include "PrimitiveBase.h"

void FlowerPrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(500);

    // Стебель (зелёный цилиндр)
    const int stemSegments = 8;
    float stemRadius = 0.03f;
    float stemHeight = 0.4f;

    for (int i = 0; i < stemSegments; i++) {
        float angle1 = 2.0f * 3.14159f * i / stemSegments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / stemSegments;

        glm::vec3 p1(cos(angle1) * stemRadius, -0.4f, sin(angle1) * stemRadius);
        glm::vec3 p2(cos(angle2) * stemRadius, -0.4f, sin(angle2) * stemRadius);
        glm::vec3 p3(cos(angle1) * stemRadius, 0.0f, sin(angle1) * stemRadius);
        glm::vec3 p4(cos(angle2) * stemRadius, 0.0f, sin(angle2) * stemRadius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    // Листья (два треугольника)
    // Левый лист
    model.vertices.push_back({ glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ glm::vec3(-0.12f, -0.25f, 0.03f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) });
    model.vertices.push_back({ glm::vec3(-0.08f, -0.15f, 0.03f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) });

    // Правый лист
    model.vertices.push_back({ glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ glm::vec3(0.12f, -0.25f, 0.03f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) });
    model.vertices.push_back({ glm::vec3(0.08f, -0.15f, 0.03f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) });

    // Центр цветка (жёлтая сфера)
    int centerSegments = 12;
    int centerRings = 8;
    float centerRadius = 0.08f;
    glm::vec3 centerPos(0.0f, 0.02f, 0.0f);

    for (int i = 0; i < centerRings; ++i) {
        for (int j = 0; j < centerSegments; ++j) {
            float theta1 = (float)i / centerRings * 3.14159f;
            float theta2 = (float)(i + 1) / centerRings * 3.14159f;
            float phi1 = (float)j / centerSegments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / centerSegments * 2.0f * 3.14159f;

            glm::vec3 v1 = centerPos + glm::vec3(
                centerRadius * sin(theta1) * cos(phi1),
                centerRadius * cos(theta1),
                centerRadius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = centerPos + glm::vec3(
                centerRadius * sin(theta1) * cos(phi2),
                centerRadius * cos(theta1),
                centerRadius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = centerPos + glm::vec3(
                centerRadius * sin(theta2) * cos(phi2),
                centerRadius * cos(theta2),
                centerRadius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = centerPos + glm::vec3(
                centerRadius * sin(theta2) * cos(phi1),
                centerRadius * cos(theta2),
                centerRadius * sin(theta2) * sin(phi1)
            );

            glm::vec3 n1 = glm::normalize(v1 - centerPos);
            glm::vec3 n2 = glm::normalize(v2 - centerPos);
            glm::vec3 n3 = glm::normalize(v3 - centerPos);
            glm::vec3 n4 = glm::normalize(v4 - centerPos);

            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ v2, n2, glm::vec2(1.0f, 0.0f) });
            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });

            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });
            model.vertices.push_back({ v4, n4, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
        }
    }

    // Лепестки (6 штук)
    int petalSegments = 8;
    float petalRadius = 0.12f;

    for (int p = 0; p < 6; p++) {
        float angle = p * 60.0f * 3.14159f / 180.0f;
        float px = cos(angle) * 0.18f;
        float pz = sin(angle) * 0.18f;
        glm::vec3 petalCenter(px, 0.02f, pz);

        for (int i = 0; i < petalSegments / 2; ++i) {
            for (int j = 0; j < petalSegments; ++j) {
                float theta1 = (float)i / (petalSegments / 2) * 3.14159f / 2.0f;
                float theta2 = (float)(i + 1) / (petalSegments / 2) * 3.14159f / 2.0f;
                float phi1 = (float)j / petalSegments * 2.0f * 3.14159f;
                float phi2 = (float)(j + 1) / petalSegments * 2.0f * 3.14159f;

                glm::vec3 v1 = petalCenter + glm::vec3(
                    petalRadius * sin(theta1) * cos(phi1),
                    petalRadius * cos(theta1) * 0.5f,
                    petalRadius * sin(theta1) * sin(phi1)
                );
                glm::vec3 v2 = petalCenter + glm::vec3(
                    petalRadius * sin(theta1) * cos(phi2),
                    petalRadius * cos(theta1) * 0.5f,
                    petalRadius * sin(theta1) * sin(phi2)
                );
                glm::vec3 v3 = petalCenter + glm::vec3(
                    petalRadius * sin(theta2) * cos(phi2),
                    petalRadius * cos(theta2) * 0.5f,
                    petalRadius * sin(theta2) * sin(phi2)
                );
                glm::vec3 v4 = petalCenter + glm::vec3(
                    petalRadius * sin(theta2) * cos(phi1),
                    petalRadius * cos(theta2) * 0.5f,
                    petalRadius * sin(theta2) * sin(phi1)
                );

                glm::vec3 n1 = glm::normalize(v1 - petalCenter);
                glm::vec3 n2 = glm::normalize(v2 - petalCenter);
                glm::vec3 n3 = glm::normalize(v3 - petalCenter);
                glm::vec3 n4 = glm::normalize(v4 - petalCenter);

                model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
                model.vertices.push_back({ v2, n2, glm::vec2(1.0f, 0.0f) });
                model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });

                model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });
                model.vertices.push_back({ v4, n4, glm::vec2(0.0f, 1.0f) });
                model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
            }
        }
    }

    model.hasTexture = false;
    model.setupBuffers();
}