#include "../pch.h"
#include "TreePrimitive.h"
#include "PrimitiveBase.h"

void TreePrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(8000);

    const int segments = 32;  // Количество сегментов для конусов
    const int starSegments = 16;  // Сегменты для звезды

    // ===== ПАРАМЕТРЫ КОНУСОВ =====
    struct ConeParams {
        float bottomRadius;
        float topRadius;
        float height;
        float yBase;  // Y-координата основания
    };

    // Три конуса (нижний, средний, верхний)
    ConeParams cones[3] = {
        {0.7f, 0.0f, 1.2f, 0.0f},   // Нижний ярус
        {0.5f, 0.0f, 1.0f, 1.2f},   // Средний ярус
        {0.35f, 0.0f, 0.9f, 2.2f}    // Верхний ярус
    };

    // ===== СОЗДАНИЕ КОНУСОВ =====
    for (int c = 0; c < 3; c++) {
        float rBottom = cones[c].bottomRadius;
        float rTop = cones[c].topRadius;
        float height = cones[c].height;
        float yBottom = cones[c].yBase;
        float yTop = yBottom + height;

        // Боковая поверхность конуса
        for (int i = 0; i < segments; i++) {
            float angle1 = 2.0f * 3.14159f * i / segments;
            float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

            // Четыре вершины четырёхугольника (два треугольника)
            glm::vec3 p1(cos(angle1) * rBottom, yBottom, sin(angle1) * rBottom);
            glm::vec3 p2(cos(angle2) * rBottom, yBottom, sin(angle2) * rBottom);
            glm::vec3 p3(cos(angle1) * rTop, yTop, sin(angle1) * rTop);
            glm::vec3 p4(cos(angle2) * rTop, yTop, sin(angle2) * rTop);

            // Нормали для боковой поверхности (направлены наружу от центра)
            glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.2f, sin(angle1)));
            glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.2f, sin(angle2)));

            // Первый треугольник
            model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

            // Второй треугольник
            model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
            model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
            model.vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
        }

        // Нижнее основание конуса (закрываем дно)
        glm::vec3 centerBottom(yBottom > 0.05f ? 0.0f : 0.0f, yBottom, 0.0f);
        glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);

        for (int i = 0; i < segments; i++) {
            float angle1 = 2.0f * 3.14159f * i / segments;
            float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

            glm::vec3 p1(cos(angle1) * rBottom, yBottom, sin(angle1) * rBottom);
            glm::vec3 p2(cos(angle2) * rBottom, yBottom, sin(angle2) * rBottom);

            model.vertices.push_back({ centerBottom, bottomNormal, glm::vec2(0.5f, 0.5f) });
            model.vertices.push_back({ p1, bottomNormal, glm::vec2(0.0f, 0.0f) });
            model.vertices.push_back({ p2, bottomNormal, glm::vec2(1.0f, 0.0f) });
        }
    }

    // ===== ЗВЕЗДА НА ВЕРШИНЕ =====
    float starY = 3.1f;  // Высота звезды (над верхним конусом)
    float starRadius = 0.25f;
    float starHeight = 0.15f;

    // Создаём звезду как комбинацию 5 лучей (10 треугольников для верхней части)
    const int starPoints = 5;
    float outerRadius = starRadius;
    float innerRadius = starRadius * 0.4f;

    // Верхняя часть звезды (пирамидки)
    glm::vec3 starCenter(0.0f, starY, 0.0f);
    glm::vec3 starTop(0.0f, starY + starHeight, 0.0f);

    // Создаём 5 лучей звезды
    for (int i = 0; i < starPoints; i++) {
        float angle1 = 2.0f * 3.14159f * i / starPoints;
        float angle2 = 2.0f * 3.14159f * (i + 1) / starPoints;

        // Внешние точки лучей
        glm::vec3 outer1(cos(angle1) * outerRadius, starY, sin(angle1) * outerRadius);
        glm::vec3 outer2(cos(angle2) * outerRadius, starY, sin(angle2) * outerRadius);

        // Внутренние точки (между лучами)
        float midAngle = angle1 + 3.14159f / starPoints;
        glm::vec3 inner(cos(midAngle) * innerRadius, starY, sin(midAngle) * innerRadius);

        // Нормали для верхних граней
        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.8f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.8f, sin(angle2)));
        glm::vec3 normalInner = glm::normalize(glm::vec3(cos(midAngle), 0.8f, sin(midAngle)));

        // Треугольник 1 (луч)
        model.vertices.push_back({ starCenter, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ outer1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ starTop, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });

        // Треугольник 2 (луч)
        model.vertices.push_back({ starCenter, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ starTop, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });
        model.vertices.push_back({ outer2, normal2, glm::vec2(1.0f, 0.0f) });

        // Треугольник 3 (впадина между лучами)
        model.vertices.push_back({ starCenter, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ inner, normalInner, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ outer1, normal1, glm::vec2(0.0f, 0.0f) });

        // Треугольник 4 (впадина между лучами)
        model.vertices.push_back({ starCenter, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ outer2, normal2, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ inner, normalInner, glm::vec2(0.0f, 0.0f) });
    }

    // Нижняя часть звезды (треугольники вниз от центра к внешним точкам)
    glm::vec3 starBottom(0.0f, starY - 0.05f, 0.0f);

    for (int i = 0; i < starPoints; i++) {
        float angle1 = 2.0f * 3.14159f * i / starPoints;
        float angle2 = 2.0f * 3.14159f * (i + 1) / starPoints;

        glm::vec3 outer1(cos(angle1) * outerRadius, starY, sin(angle1) * outerRadius);
        glm::vec3 outer2(cos(angle2) * outerRadius, starY, sin(angle2) * outerRadius);

        float midAngle = angle1 + 3.14159f / starPoints;
        glm::vec3 inner(cos(midAngle) * innerRadius, starY, sin(midAngle) * innerRadius);

        glm::vec3 normalDown(0.0f, -0.5f, 0.0f);

        // Нижние треугольники для лучей
        model.vertices.push_back({ starBottom, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ outer1, normalDown, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ starCenter, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.8f) });

        model.vertices.push_back({ starBottom, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ starCenter, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.8f) });
        model.vertices.push_back({ outer2, normalDown, glm::vec2(1.0f, 1.0f) });

        // Нижние треугольники для впадин
        model.vertices.push_back({ starBottom, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ inner, normalDown, glm::vec2(0.0f, 0.8f) });
        model.vertices.push_back({ outer1, normalDown, glm::vec2(0.0f, 1.0f) });

        model.vertices.push_back({ starBottom, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ outer2, normalDown, glm::vec2(1.0f, 1.0f) });
        model.vertices.push_back({ inner, normalDown, glm::vec2(1.0f, 0.8f) });
    }

    // Добавляем маленький шпиль на самую вершину звезды
    float spikeY = starY + starHeight;
    float spikeHeight = 0.1f;
    float spikeRadius = 0.03f;

    for (int i = 0; i < segments / 2; i++) {
        float angle1 = 2.0f * 3.14159f * i / (segments / 2);
        float angle2 = 2.0f * 3.14159f * (i + 1) / (segments / 2);

        glm::vec3 p1(cos(angle1) * spikeRadius, spikeY, sin(angle1) * spikeRadius);
        glm::vec3 p2(cos(angle2) * spikeRadius, spikeY, sin(angle2) * spikeRadius);
        glm::vec3 top(0.0f, spikeY + spikeHeight, 0.0f);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.5f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.5f, sin(angle2)));

        model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ top, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 1.0f) });
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
    }

    // СТВОЛ ДЕРЕВА (небольшой цилиндр внизу)
    float trunkRadius = 0.2f;
    float trunkHeight = 0.3f;
    float trunkY = 0.0f;

    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, trunkY, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, trunkY, sin(angle2) * trunkRadius);
        glm::vec3 p3(cos(angle1) * trunkRadius, trunkY + trunkHeight, sin(angle1) * trunkRadius);
        glm::vec3 p4(cos(angle2) * trunkRadius, trunkY + trunkHeight, sin(angle2) * trunkRadius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        model.vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        model.vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        model.vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }

    // Верхнее и нижнее основание ствола
    glm::vec3 trunkBottomCenter(0.0f, trunkY, 0.0f);
    glm::vec3 trunkTopCenter(0.0f, trunkY + trunkHeight, 0.0f);
    glm::vec3 trunkNormalBottom(0.0f, -1.0f, 0.0f);
    glm::vec3 trunkNormalTop(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(cos(angle1) * trunkRadius, trunkY, sin(angle1) * trunkRadius);
        glm::vec3 p2(cos(angle2) * trunkRadius, trunkY, sin(angle2) * trunkRadius);
        glm::vec3 p3(cos(angle1) * trunkRadius, trunkY + trunkHeight, sin(angle1) * trunkRadius);
        glm::vec3 p4(cos(angle2) * trunkRadius, trunkY + trunkHeight, sin(angle2) * trunkRadius);

        // Нижнее основание
        model.vertices.push_back({ trunkBottomCenter, trunkNormalBottom, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p1, trunkNormalBottom, glm::vec2(0.0f, 0.0f) });
        model.vertices.push_back({ p2, trunkNormalBottom, glm::vec2(1.0f, 0.0f) });

        // Верхнее основание
        model.vertices.push_back({ trunkTopCenter, trunkNormalTop, glm::vec2(0.5f, 0.5f) });
        model.vertices.push_back({ p4, trunkNormalTop, glm::vec2(1.0f, 0.0f) });
        model.vertices.push_back({ p3, trunkNormalTop, glm::vec2(0.0f, 0.0f) });
    }

    model.hasTexture = false;
    model.setupBuffers();
}