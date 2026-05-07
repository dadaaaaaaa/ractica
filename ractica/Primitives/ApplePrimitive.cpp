#include "../pch.h"
#include "ApplePrimitive.h"
#include "PrimitiveBase.h"

void ApplePrimitive::create(Model& model) {
    model.vertices.clear();

    // Создаём сферу с правильными нормалями
    const int segments = 20;
    const int rings = 20;
    float radius = 0.5f;

    for (int i = 0; i < rings; ++i) {
        float theta1 = (float)i / rings * 3.14159f;
        float theta2 = (float)(i + 1) / rings * 3.14159f;

        for (int j = 0; j < segments; ++j) {
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            // Вершины
            glm::vec3 v1(
                radius * sin(theta1) * cos(phi1),
                radius * cos(theta1) * 1.1f,  // Вытянутость по Y
                radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2(
                radius * sin(theta1) * cos(phi2),
                radius * cos(theta1) * 1.1f,
                radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3(
                radius * sin(theta2) * cos(phi2),
                radius * cos(theta2) * 1.1f,
                radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4(
                radius * sin(theta2) * cos(phi1),
                radius * cos(theta2) * 1.1f,
                radius * sin(theta2) * sin(phi1)
            );

            // Нормали (ВАЖНО: нормализуем)
            glm::vec3 n1 = glm::normalize(v1);
            glm::vec3 n2 = glm::normalize(v2);
            glm::vec3 n3 = glm::normalize(v3);
            glm::vec3 n4 = glm::normalize(v4);

            // Треугольник 1
            model.vertices.push_back({ v1, n1, glm::vec2(0,0) });
            model.vertices.push_back({ v2, n2, glm::vec2(1,0) });
            model.vertices.push_back({ v3, n3, glm::vec2(1,1) });

            // Треугольник 2
            model.vertices.push_back({ v3, n3, glm::vec2(1,1) });
            model.vertices.push_back({ v4, n4, glm::vec2(0,1) });
            model.vertices.push_back({ v1, n1, glm::vec2(0,0) });
        }
    }

    model.hasTexture = false;
    model.setupBuffers();

}