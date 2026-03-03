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

class GameObjects;  // Forward declaration

// Структура для хранения информации о символах шрифта
struct TextCharacter {
    GLuint textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class GameUI {
private:
    std::vector<MenuButton> mainMenuButtons;
    std::vector<MenuButton> pauseMenuButtons;
    std::vector<MenuButton> settingsButtons;
    std::vector<MenuButton> gameOverButtons;
    std::vector<MenuButton> controlsButtons;
    std::vector<MenuButton> highScoresButtons;

    std::map<GLchar, TextCharacter> characters;
    GLuint textVAO, textVBO, textShader;
    FT_Library ft;
    FT_Face face;
    bool hasSaveGameFlag = false;
    int windowWidth;
    int windowHeight;
    double mouseX, mouseY;
    bool mousePressed;
    bool uiInitialized = false;
    bool fontInitialized = false;

    // Методы для работы с текстом и шрифтами
    void compileTextShaders();
    void setupTextBuffers();
    bool loadFreeTypeFont(const std::string& fontPath = "C:/Windows/Fonts/arial.ttf",
        unsigned int fontSize = 24);
    void cleanupFreeType();

    // Методы масштабирования координат для разных разрешений
    int getScaledX(int x) const {
        return static_cast<int>(x * static_cast<float>(windowWidth) / 1200.0f);
    }
    int getScaledY(int y) const {
        return static_cast<int>(y * static_cast<float>(windowHeight) / 800.0f);
    }
    int getScaledWidth(int width) const {
        return static_cast<int>(width * static_cast<float>(windowWidth) / 1200.0f);
    }
    int getScaledHeight(int height) const {
        return static_cast<int>(height * static_cast<float>(windowHeight) / 800.0f);
    }

    // Масштабирование размера шрифта
    unsigned int getScaledFontSize() const {
        float scale = std::min(
            static_cast<float>(windowWidth) / 1200.0f,
            static_cast<float>(windowHeight) / 800.0f
        );
        return static_cast<unsigned int>(24 * scale);
    }

    // Форматирование времени игры в минуты:секунды
    std::string formatGameTime(int seconds) {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << secs;
        return ss.str();
    }

public:
    GameUI();
    ~GameUI();

    // Основные методы управления UI
    void updateSaveGameInfo(bool hasSaveGame);
    void setMousePosition(double x, double y);
    void setMousePressed(bool pressed) { mousePressed = pressed; }
    void setWindowSize(int width, int height);
    void initUI();
    void initWindowsFont();
    void cleanup();

    // Методы проверки кликов
    bool isNameFieldClicked(double mouseX, double mouseY) const;
    bool isSpeedIncreaseButtonClicked(double mouseX, double mouseY) ;
    bool isSpeedDecreaseButtonClicked(double mouseX, double mouseY) ;

    // Методы отрисовки различных меню
    void drawNameInputField(const std::string& playerName, bool isActive);
    void drawMainMenu();
    void drawPauseMenu();
    void drawGameOver(int score);
    void drawSettingsMenu(float gameSpeed, const std::string& playerName,
        float speedMultiplier, const std::string& speedDisplayText,
        bool isNameInputActive, bool doubleBufferingEnabled);
    void drawHighScoresMenu(const std::vector<HighScore>& highScores);
    void drawControlsMenu();

    // Базовые методы отрисовки
    void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha = 1.0f);
    void drawButtonWithText(const MenuButton& button);
    void drawText(float x, float y, const std::string& text, float r, float g, float b);
    void drawCenteredText(float y, const std::string& text, float r, float g, float b);

    // Вспомогательные методы
    float getTextWidth(const std::string& text) const;
    bool ensureFontInitialized();
    void handleMouseClick(GameObjects& objects);
    void recreateButtons();

    // Геттеры
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }
    bool isMousePressed() const { return mousePressed; }
    bool isUIInitialized() const { return uiInitialized; }
    bool isInitialized() const { return uiInitialized; }
};