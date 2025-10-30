#pragma once
#include <glm/glm.hpp>

// Структура для спрайта (облака)
struct Sprite {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;

    Sprite(glm::vec3 pos, glm::vec3 col, float s, float sp);
};