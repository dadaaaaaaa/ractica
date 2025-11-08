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

    Bird(glm::vec3 pos, glm::vec3 col, float s, float sp);
};