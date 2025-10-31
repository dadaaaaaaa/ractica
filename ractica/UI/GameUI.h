#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "MenuButton.h"
#include <vector>
#include <string>
#include <map>
#include <windows.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// Предварительное объявление вместо инклюда
class GameObjects;

// Структура для символов FreeType
struct TextCharacter {
    GLuint textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class GameUI {
private:
    // UI элементы
    std::vector<MenuButton> mainMenuButtons;
    std::vector<MenuButton> pauseMenuButtons;
    std::vector<MenuButton> settingsButtons;
    std::vector<MenuButton> gameOverButtons;
    std::vector<MenuButton> controlsButtons;
    std::vector<MenuButton> highScoresButtons;
    // FreeType шрифт
    std::map<GLchar, TextCharacter> characters;
    GLuint textVAO, textVBO, textShader;
    FT_Library ft;
    FT_Face face;

    // Состояние UI
    int windowWidth;
    int windowHeight;
    double mouseX, mouseY;
    bool mousePressed;
    bool uiInitialized = false;
    bool fontInitialized = false;

    // FreeType методы
    void compileTextShaders();
    void setupTextBuffers();
    bool loadFreeTypeFont(const std::string& fontPath = "C:/Windows/Fonts/arial.ttf", 
                         unsigned int fontSize = 24);
    void cleanupFreeType();

public:
    GameUI();
    ~GameUI();
    void drawSettingsMenu(float gameSpeed, const std::string& playerName, float speedMultiplier, const std::string& speedDisplayText);
    bool isSpeedIncreaseButtonClicked(double mouseX, double mouseY);
    bool isSpeedDecreaseButtonClicked(double mouseX, double mouseY);
    void drawRect(float x, float y, float width, float height, const glm::vec4& color);
    void debugTextRendering();
    const float SPEED_BUTTON_WIDTH = 40.0f;
    const float SPEED_BUTTON_HEIGHT = 40.0f;
    const float SPEED_DISPLAY_X = 400.0f;
    const float SPEED_DISPLAY_Y = 350.0f;
    // Геттеры
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }
    bool isMousePressed() const { return mousePressed; }
    bool isUIInitialized() const { return uiInitialized; }

    // Сеттеры
    void setMousePosition(double x, double y);
    void setMousePressed(bool pressed) { mousePressed = pressed; }
    void setWindowSize(int width, int height);

    // Инициализация
    void initUI();
    void initWindowsFont();

    // Отрисовка UI
    void drawTextTest();
    void drawMainMenu();
    void drawPauseMenu();
    void drawHighScoresMenu(const std::vector<HighScore>& highScores);
    void drawControlsMenu();
    void drawGameOver(int score);
    void drawButton(const MenuButton& button);
    void drawButtonWithText(const MenuButton& button);
    void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha = 1.0f);
    void drawTestQuad(float x, float y, float width, float height, const glm::vec3& color);

    // Текст с FreeType
    void drawText(float x, float y, const std::string& text, float r, float g, float b);
    void drawCenteredText(float y, const std::string& text, float r, float g, float b);
    float getTextWidth(const std::string& text);
    bool ensureFontInitialized();
    void cleanup();

    // Обработка ввода
    void handleMouseClick(GameObjects& objects);
};