#include "../pch.h"
#include "BirdPrimitive.h"
#include "PrimitiveBase.h"

void BirdPrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(1000);

    // Тело птицы (вытянутая сфера - эллипсоид)
    const int segments = 16;
    const int rings = 12;
    float bodyRadiusX = 0.3f;
    float bodyRadiusY = 0.2f;
    float bodyRadiusZ = 0.25f;

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                bodyRadiusX * sin(theta1) * cos(phi1),
                bodyRadiusY * cos(theta1),
                bodyRadiusZ * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                bodyRadiusX * sin(theta1) * cos(phi2),
                bodyRadiusY * cos(theta1),
                bodyRadiusZ * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                bodyRadiusX * sin(theta2) * cos(phi2),
                bodyRadiusY * cos(theta2),
                bodyRadiusZ * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                bodyRadiusX * sin(theta2) * cos(phi1),
                bodyRadiusY * cos(theta2),
                bodyRadiusZ * sin(theta2) * sin(phi1)
            );

            glm::vec3 n1 = glm::normalize(v1);
            glm::vec3 n2 = glm::normalize(v2);
            glm::vec3 n3 = glm::normalize(v3);
            glm::vec3 n4 = glm::normalize(v4);

            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ v2, n2, glm::vec2(1.0f, 0.0f) });
            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });

            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });
            model.vertices.push_back({ v4, n4, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
        }
    }

    // Голова (маленькая сфера, смещённая вперёд)
    float headRadius = 0.18f;
    glm::vec3 headCenter(0.35f, 0.15f, 0.0f);
    int headSegments = 12;
    int headRings = 8;

    for (int i = 0; i < headRings; ++i) {
        for (int j = 0; j < headSegments; ++j) {
            float theta1 = (float)i / headRings * 3.14159f;
            float theta2 = (float)(i + 1) / headRings * 3.14159f;
            float phi1 = (float)j / headSegments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / headSegments * 2.0f * 3.14159f;

            glm::vec3 v1 = headCenter + glm::vec3(
                headRadius * sin(theta1) * cos(phi1),
                headRadius * cos(theta1),
                headRadius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = headCenter + glm::vec3(
                headRadius * sin(theta1) * cos(phi2),
                headRadius * cos(theta1),
                headRadius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = headCenter + glm::vec3(
                headRadius * sin(theta2) * cos(phi2),
                headRadius * cos(theta2),
                headRadius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = headCenter + glm::vec3(
                headRadius * sin(theta2) * cos(phi1),
                headRadius * cos(theta2),
                headRadius * sin(theta2) * sin(phi1)
            );

            glm::vec3 n1 = glm::normalize(v1 - headCenter);
            glm::vec3 n2 = glm::normalize(v2 - headCenter);
            glm::vec3 n3 = glm::normalize(v3 - headCenter);
            glm::vec3 n4 = glm::normalize(v4 - headCenter);

            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ v2, n2, glm::vec2(1.0f, 0.0f) });
            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });

            model.vertices.push_back({ v3, n3, glm::vec2(1.0f, 1.0f) });
            model.vertices.push_back({ v4, n4, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ v1, n1, glm::vec2(0.0f, 0.0f) });
        }
    }

    // Клюв (конус - два треугольника)
    glm::vec3 beakTip(0.6f, 0.12f, 0.0f);
    glm::vec3 beakBase1(0.45f, 0.18f, -0.06f);
    glm::vec3 beakBase2(0.45f, 0.18f, 0.06f);
    glm::vec3 beakBase3(0.45f, 0.06f, 0.0f);

    glm::vec3 beakNormal = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));

    model.vertices.push_back({ beakTip, beakNormal, glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ beakBase1, beakNormal, glm::vec2(0.0f, 0.0f) });
    model.vertices.push_back({ beakBase2, beakNormal, glm::vec2(1.0f, 0.0f) });

    model.vertices.push_back({ beakTip, beakNormal, glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ beakBase3, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });
    model.vertices.push_back({ beakBase1, beakNormal, glm::vec2(0.0f, 0.0f) });

    model.vertices.push_back({ beakTip, beakNormal, glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ beakBase2, beakNormal, glm::vec2(1.0f, 0.0f) });
    model.vertices.push_back({ beakBase3, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });

    // Хвост (конус)
    glm::vec3 tailTip(-0.45f, 0.0f, 0.0f);
    glm::vec3 tailBase1(-0.3f, 0.05f, -0.1f);
    glm::vec3 tailBase2(-0.3f, 0.05f, 0.1f);
    glm::vec3 tailBase3(-0.3f, -0.05f, 0.0f);

    model.vertices.push_back({ tailTip, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ tailBase1, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) });
    model.vertices.push_back({ tailBase2, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) });

    model.vertices.push_back({ tailTip, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ tailBase3, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });
    model.vertices.push_back({ tailBase1, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) });

    model.vertices.push_back({ tailTip, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ tailBase2, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) });
    model.vertices.push_back({ tailBase3, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });

    // Крылья
    float wingOffset = 0.25f;

    // Левое крыло
    model.vertices.push_back({ glm::vec3(0.1f, 0.0f, 0.25f), glm::vec3(0.0f, 0.5f, 0.5f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ glm::vec3(0.4f, 0.15f, 0.4f), glm::vec3(0.0f, 0.5f, 0.5f), glm::vec2(1.0f, 0.0f) });
    model.vertices.push_back({ glm::vec3(0.4f, -0.1f, 0.35f), glm::vec3(0.0f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f) });

    // Правое крыло
    model.vertices.push_back({ glm::vec3(0.1f, 0.0f, -0.25f), glm::vec3(0.0f, 0.5f, -0.5f), glm::vec2(0.5f, 0.5f) });
    model.vertices.push_back({ glm::vec3(0.4f, 0.15f, -0.4f), glm::vec3(0.0f, 0.5f, -0.5f), glm::vec2(1.0f, 0.0f) });
    model.vertices.push_back({ glm::vec3(0.4f, -0.1f, -0.35f), glm::vec3(0.0f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f) });

    model.hasTexture = false;
    model.setupBuffers();
}