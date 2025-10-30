#include "../pch.h"
#include "MenuButton.h"

MenuButton::MenuButton(const std::string& t, float xPos, float yPos, float w, float h)
    : text(t), x(xPos), y(yPos), width(w), height(h), hovered(false) {
}

bool MenuButton::contains(float mouseX, float mouseY) const {
    return mouseX >= x && mouseX <= x + width &&
        mouseY >= y && mouseY <= y + height;
}