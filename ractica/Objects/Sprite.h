#pragma once
#include <glm/glm.hpp>

// Структура для спрайта (облака, цветы)
struct Sprite {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;

    Sprite(glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 col = glm::vec3(1.0f),
        float s = 1.0f,
        float sp = 0.0f)
        : position(pos), color(col), size(s), speed(sp) {
    }
};