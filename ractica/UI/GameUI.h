#pragma once
#include "../Core/Constants.h"
#include "../Core/UITypes.h"
#include "../Core/Types.h"
#include "../UI/MenuButton.h"
#include "../Graphics/ShaderManager.h"
#include "../Objects/GameObjects.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>

// Предварительные объявления
extern ShaderManager g_shaderManager;
extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;

class GameUI {
private:
    // Основные параметры
    int windowWidth;
    int windowHeight;
    double mouseX;
    double mouseY;
    bool mousePressed;
    bool uiInitialized;
    bool fontInitialized;
    bool hasSaveGameFlag;

    // Система шрифтов
    FT_Library ft;
    FT_Face face;
    std::map<GLchar, TextCharacter> characters;
    GLuint textVAO;
    GLuint textVBO;
    GLuint textShader;

    // Кнопки меню
    std::vector<MenuButton> mainMenuButtons;
    std::vector<MenuButton> pauseMenuButtons;
    std::vector<MenuButton> gameOverButtons;
    std::vector<MenuButton> settingsButtons;
    std::vector<MenuButton> controlsButtons;
    std::vector<MenuButton> highScoresButtons;

    // Конфигурации
    FontConfig fontConfig;
    ButtonConfig buttonConfig;

    // Приватные методы
    void cleanupFreeType();
    void recreateButtons();
    bool loadFreeTypeFont(const std::string& fontPath, unsigned int fontSize);
    void setupTextBuffers();
    void compileTextShaders();
    void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha);
    void drawButtonWithText(const MenuButton& button);
    bool ensureFontInitialized();

    // Вспомогательные методы для позиционирования
    int getScaledX(int x) const;
    int getScaledY(int y) const;
    int getScaledWidth(int width) const;
    int getScaledHeight(int height) const;
    unsigned int getScaledFontSize() const;
    void initWindowsFont();
    unsigned int getScaledFontSize() const;
    bool loadFreeTypeFont(const std::string& fontPath, unsigned int fontSize);
    void setupTextBuffers();
    void compileTextShaders();

    // Вспомогательные методы
    void cleanupFreeType();
    bool ensureFontInitialized();
    float getTextWidth(const std::string& text);

    // Методы для кнопок настроек
    bool isSpeedIncreaseButtonClicked(double mouseX, double mouseY);
    bool isSpeedDecreaseButtonClicked(double mouseX, double mouseY);
public:
    GameUI();
    ~GameUI();
    void drawText(float x, float y, const std::string& text, float r, float g, float b);
    void drawCenteredText(float y, const std::string& text, float r, float g, float b);

    // Методы отрисовки UI элементов
    void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha);
    void drawButtonWithText(const MenuButton& button);

    // Методы обработки ввода
    void handleMouseClick(GameObjects& objects);

    // Конфигурационные методы
    void setFontConfig(const FontConfig& config);
    void setButtonConfig(const ButtonConfig& config);

    // Методы для обновления информации
    void updateSaveGameInfo(bool hasSaveGame);
    // Основные методы
    void initUI();
    void cleanup();
    void setWindowSize(int width, int height);
    void setMousePosition(double x, double y);
    void updateSaveGameInfo(bool hasSaveGame);

    // Методы отрисовки
    void drawMainMenu();
    void drawPauseMenu();
    void drawSettingsMenu(float gameSpeed, const std::string& playerName,
        float speedMultiplier, const std::string& speedDisplayText);
    void drawGameOver(int score);
    void drawHighScoresMenu(const std::vector<HighScore>& highScores);
    void drawControlsMenu();
    void drawInGameUI(int score, float speedMultiplier, const std::string& speedDisplayText);

    // Методы работы с текстом
    void drawText(float x, float y, const std::string& text,
        float r = 1.0f, float g = 1.0f, float b = 1.0f);
    void drawCenteredText(float y, const std::string& text,
        float r = 1.0f, float g = 1.0f, float b = 1.0f);
    float getTextWidth(const std::string& text);

    // Обработка ввода
    void handleMouseClick(GameObjects& objects);
    void setMousePressed(bool pressed) { mousePressed = pressed; }

    // Вспомогательные методы для настроек
    bool isSpeedIncreaseButtonClicked(double mouseX, double mouseY);
    bool isSpeedDecreaseButtonClicked(double mouseX, double mouseY);

    // Геттеры
    bool isUIIinitialized() const { return uiInitialized; }
    bool isFontInitialized() const { return fontInitialized; }
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }

    // Конфигурация
    void setFontConfig(const FontConfig& config);
    void setButtonConfig(const ButtonConfig& config);
};