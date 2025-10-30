#pragma once
#include <string>

// Структура для кнопки меню
struct MenuButton {
    std::string text;
    float x, y, width, height;
    bool hovered;

    MenuButton(const std::string& t, float xPos, float yPos, float w, float h);
    bool contains(float mouseX, float mouseY) const;
};