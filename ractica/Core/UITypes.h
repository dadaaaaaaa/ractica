#pragma once
#include <glm/glm.hpp>
#include <string>
#include <map>

// Структура для символа шрифта
struct TextCharacter {
    unsigned int textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

// Типы меню
enum class MenuType {
    MAIN_MENU,
    PAUSE_MENU,
    SETTINGS_MENU,
    HIGH_SCORES_MENU,
    CONTROLS_MENU,
    GAME_OVER_MENU
};

// Конфигурация шрифта
struct FontConfig {
    std::string fontPath;
    unsigned int fontSize;
    glm::vec3 defaultColor;

    FontConfig(const std::string& path = "C:/Windows/Fonts/arial.ttf",
        unsigned int size = 24,
        const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f))
        : fontPath(path), fontSize(size), defaultColor(color) {
    }
};

// Конфигурация кнопки
struct ButtonConfig {
    glm::vec3 normalColor;
    glm::vec3 hoverColor;
    glm::vec3 textColor;
    float alpha;

    ButtonConfig(const glm::vec3& normal = glm::vec3(0.1f, 0.3f, 0.1f),
        const glm::vec3& hover = glm::vec3(0.2f, 0.6f, 0.2f),
        const glm::vec3& text = glm::vec3(1.0f, 1.0f, 1.0f),
        float a = 0.9f)
        : normalColor(normal), hoverColor(hover), textColor(text), alpha(a) {
    }
};

// Конфигурация меню
struct MenuConfig {
    glm::vec3 backgroundColor;
    float backgroundAlpha;
    std::string title;
    glm::vec3 titleColor;

    MenuConfig(const glm::vec3& bgColor = glm::vec3(0.1f, 0.2f, 0.3f),
        float bgAlpha = 1.0f,
        const std::string& menuTitle = "",
        const glm::vec3& titleCol = glm::vec3(1.0f, 1.0f, 1.0f))
        : backgroundColor(bgColor), backgroundAlpha(bgAlpha),
        title(menuTitle), titleColor(titleCol) {
    }
};