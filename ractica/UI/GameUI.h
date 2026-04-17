#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>

#include "../Core/Types.h"
#include "../Objects/GameObjects.h"
#include "MenuButton.h"  // <-- ДОБАВИТЬ ЭТУ СТРОКУ

// УДАЛИТЬ определение MenuButton отсюда (оно теперь в MenuButton.h)
// struct MenuButton { ... };
// Структура символа (как в рабочем проекте)
struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};
struct TextCharacter {
    GLuint textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class GameUI {
public:
    GameUI();
    ~GameUI();

    void initUI();
    void cleanup();
    void drawMainMenu();
    void drawPauseMenu();
    void drawSettingsMenu(float gameSpeed, const std::string& playerName,
        float speedMultiplier, const std::string& speedDisplayText,
        bool isNameInputActive, bool doubleBufferingEnabled);
    void drawGameOver(int score);
    void drawHighScoresMenu(const std::vector<HighScore>& highScores);
    void drawControlsMenu();

    void handleMouseClick(GameObjects& objects);
    void setMousePosition(double x, double y);
    void setMousePressed(bool pressed) { mousePressed = pressed; }
    void setWindowSize(int width, int height);
    void updateSaveGameInfo(bool hasSaveGame);

    bool isSpeedIncreaseButtonClicked(double mouseX, double mouseY) const;
    bool isSpeedDecreaseButtonClicked(double mouseX, double mouseY) const;
    bool isNameFieldClicked(double mouseX, double mouseY) const;

    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }

private:
    void recreateButtons();
    void initWindowsFont();
    bool loadFreeTypeFont(const std::string& fontPath, unsigned int fontSize);
    void setupTextBuffers();
    void cleanupFreeType();
    void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha = 1.0f);
    void drawButtonWithText(const MenuButton& button);
    void drawText(float x, float y, const std::string& text, float r, float g, float b);
    void drawCenteredText(float y, const std::string& text, float r, float g, float b);
    float getTextWidth(const std::string& text) const;
    void drawNameInputField(const std::string& playerName, bool isActive);
    bool ensureFontInitialized();

    // Масштабирование для разных разрешений
    int getScaledX(int x) const { return static_cast<int>(x * windowWidth / 1200.0f); }
    int getScaledY(int y) const { return static_cast<int>(y * windowHeight / 800.0f); }
    int getScaledWidth(int w) const { return static_cast<int>(w * windowWidth / 1200.0f); }
    int getScaledHeight(int h) const { return static_cast<int>(h * windowHeight / 800.0f); }
    unsigned int getScaledFontSize() const {
        float scale = std::min(windowWidth / 1200.0f, windowHeight / 800.0f);
        return static_cast<unsigned int>(24 * scale);
    }

    int windowWidth, windowHeight;
    double mouseX, mouseY;
    bool mousePressed;
    bool uiInitialized;
    bool fontInitialized;
    bool hasSaveGameFlag;

    std::vector<MenuButton> mainMenuButtons;
    std::vector<MenuButton> pauseMenuButtons;
    std::vector<MenuButton> gameOverButtons;
    std::vector<MenuButton> settingsButtons;
    std::vector<MenuButton> controlsButtons;
    std::vector<MenuButton> highScoresButtons;

    // FreeType
    FT_Library ft;
    FT_Face face;
    std::map<GLchar, TextCharacter> characters;
    GLuint textVAO, textVBO;
};