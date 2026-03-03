#pragma once
#include <glm/glm.hpp>

// Структура для птицы с анимацией
struct Bird {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;
    float wingAngle;
    float wingSpeed;
    glm::vec3 direction;

    Bird(glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 col = glm::vec3(1.0f),
        float s = 0.1f,
        float sp = 0.1f)
        : position(pos), color(col), size(s), speed(sp),
        wingAngle(0.0f), wingSpeed(5.0f), direction(0.0f) {
    }
};