#include "../pch.h"
#include "GameUI.h"
#include "../Objects/GameObjects.h"
#include <iostream>
#include <glm/glm.hpp>

extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;

// Вспомогательная функция для форматирования времени
static std::string formatGameTime(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    if (minutes > 0) {
        return std::to_string(minutes) + "m " + std::to_string(secs) + "s";
    }
    return std::to_string(secs) + "s";
}

GameUI::GameUI()
    : windowWidth(1200), windowHeight(800),
    mouseX(0), mouseY(0), mousePressed(false),
    uiInitialized(false), fontInitialized(false),
    ft(nullptr), face(nullptr), textVAO(0), textVBO(0) {
}

GameUI::~GameUI() {
    cleanup();
}

void GameUI::cleanup() {
    cleanupFreeType();
}

void GameUI::cleanupFreeType() {
    if (face) FT_Done_Face(face);
    if (ft) FT_Done_FreeType(ft);

    for (auto& character : characters) {
        glDeleteTextures(1, &character.second.textureID);
    }
    characters.clear();

    if (textVAO) glDeleteVertexArrays(1, &textVAO);
    if (textVBO) glDeleteBuffers(1, &textVBO);

    textVAO = textVBO = 0;
    ft = nullptr;
    face = nullptr;
    fontInitialized = false;
}

void GameUI::setWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    std::cout << "Window size changed to: " << width << "x" << height << std::endl;

    if (uiInitialized) {
        recreateButtons();

        if (fontInitialized) {
            std::cout << "Reinitializing font for new window size..." << std::endl;
            cleanupFreeType();
            initWindowsFont();
        }
    }
}

void GameUI::updateSaveGameInfo(bool hasSaveGame) {
    hasSaveGameFlag = hasSaveGame;
    recreateButtons();
}

void GameUI::recreateButtons() {
    int centerX = getScaledX(600 - 100);
    int buttonYStart = getScaledY(400 + 100);
    int backButtonX = windowWidth - getScaledX(220);
    int backButtonY = getScaledY(50);

    bool hasSave = hasSaveGameFlag;

    mainMenuButtons.clear();

    if (hasSave) {
        mainMenuButtons.push_back(MenuButton("CONTINUE", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
        mainMenuButtons.push_back(MenuButton("NEW GAME", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));
    }
    else {
        mainMenuButtons.push_back(MenuButton("PLAY", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    }

    int firstOffset = hasSave ? 160 : 80;
    int secondOffset = hasSave ? 240 : 160;
    int thirdOffset = hasSave ? 320 : 240;
    int fourthOffset = hasSave ? 400 : 320;

    mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - getScaledY(firstOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, buttonYStart - getScaledY(secondOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, buttonYStart - getScaledY(thirdOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("EXIT", centerX, buttonYStart - getScaledY(fourthOffset), getScaledWidth(200), getScaledHeight(50)));

    pauseMenuButtons.clear();
    pauseMenuButtons.push_back(MenuButton("RESUME", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    pauseMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));
    pauseMenuButtons.push_back(MenuButton("MAIN MENU", centerX, buttonYStart - getScaledY(160), getScaledWidth(200), getScaledHeight(50)));

    gameOverButtons.clear();
    gameOverButtons.push_back(MenuButton("RESTART", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    gameOverButtons.push_back(MenuButton("MAIN MENU", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));

    settingsButtons.clear();
    settingsButtons.push_back(MenuButton("BACK", backButtonX, backButtonY, getScaledWidth(200), getScaledHeight(50)));

    controlsButtons.clear();
    controlsButtons.push_back(MenuButton("BACK", backButtonX, backButtonY, getScaledWidth(200), getScaledHeight(50)));

    highScoresButtons.clear();
    highScoresButtons.push_back(MenuButton("BACK", backButtonX, backButtonY, getScaledWidth(200), getScaledHeight(50)));
}

void GameUI::setMousePosition(double x, double y) {
    mouseX = x;
    mouseY = windowHeight - y;
}

void GameUI::initUI() {
    std::cout << "Initializing UI..." << std::endl;

    // Создаем простой VAO/VBO для квадратов (без шейдеров, будем использовать glBegin/glEnd)
    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initWindowsFont();
    recreateButtons();

    uiInitialized = true;
    std::cout << "UI initialized successfully" << std::endl;
}

void GameUI::initWindowsFont() {
    std::cout << "Initializing FreeType font..." << std::endl;

    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR: Could not init FreeType Library" << std::endl;
        return;
    }

    unsigned int fontSize = getScaledFontSize();
    std::cout << "Loading font with scaled size: " << fontSize << " (base: 24)" << std::endl;

    if (!loadFreeTypeFont("C:/Windows/Fonts/arial.ttf", fontSize)) {
        std::cerr << "Failed to load font with FreeType" << std::endl;
        return;
    }

    setupTextBuffers();
    fontInitialized = true;
    std::cout << "FreeType font initialized successfully with size: " << fontSize << std::endl;
}

bool GameUI::loadFreeTypeFont(const std::string& fontPath, unsigned int fontSize) {
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
            face->glyph->bitmap.width, face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        TextCharacter character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        characters.insert(std::pair<GLchar, TextCharacter>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void GameUI::setupTextBuffers() {
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

float GameUI::getTextWidth(const std::string& text) const {
    if (!fontInitialized || characters.empty()) {
        return text.length() * getScaledWidth(10);
    }

    float width = 0.0f;
    for (const char& c : text) {
        auto it = characters.find(c);
        if (it != characters.end()) {
            width += (it->second.advance >> 6);
        }
    }
    return width;
}

void GameUI::drawText(float x, float y, const std::string& text, float r, float g, float b) {
    if (!fontInitialized || characters.empty()) return;

    float scaledX = x;
    float scaledY = y;

    // Настройка ортографической проекции для текста
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(r, g, b);

    glBindVertexArray(textVAO);

    float startX = scaledX;
    for (const char& c : text) {
        auto it = characters.find(c);
        if (it == characters.end()) continue;

        TextCharacter ch = it->second;
        float xpos = startX + ch.bearing.x;
        float ypos = scaledY - (ch.size.y - ch.bearing.y);
        float w = ch.size.x, h = ch.size.y;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        startX += (ch.advance >> 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void GameUI::drawCenteredText(float y, const std::string& text, float r, float g, float b) {
    float textWidth = getTextWidth(text);
    float x = (windowWidth - textWidth) / 2;
    drawText(x, getScaledY(static_cast<int>(y)), text, r, g, b);
}

bool GameUI::ensureFontInitialized() {
    if (!fontInitialized) {
        initWindowsFont();
    }
    return fontInitialized;
}

void GameUI::drawMainMenu() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    drawCenteredText(600, "3D SNAKE GAME", 1.0f, 1.0f, 1.0f);
    for (auto& button : mainMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawPauseMenu() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);
    drawCenteredText(700, "PAUSED", 1.0f, 1.0f, 1.0f);

    for (auto& button : pauseMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glDisable(GL_BLEND);
}

void GameUI::drawSettingsMenu(float gameSpeed, const std::string& playerName,
    float speedMultiplier, const std::string& speedDisplayText,
    bool isNameInputActive, bool doubleBufferingEnabled) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.3f, 0.2f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    drawCenteredText(550, "SETTINGS", 1.0f, 1.0f, 1.0f);
    drawNameInputField(playerName, isNameInputActive);
    drawCenteredText(300, "SPEED", 1.0f, 1.0f, 1.0f);

    float multiplierY = getScaledY(250);
    drawCenteredText(250, speedDisplayText, 1.0f, 1.0f, 1.0f);

    float centerX = windowWidth / 2.0f;
    bool minusHover = isSpeedDecreaseButtonClicked(mouseX, mouseY);
    bool plusHover = isSpeedIncreaseButtonClicked(mouseX, mouseY);

    drawText(centerX - getScaledX(120), multiplierY, "-",
        minusHover ? 1.0f : 0.7f, 0.3f, 0.0f);
    drawText(centerX + getScaledX(100), multiplierY, "+",
        0.0f, plusHover ? 1.0f : 0.7f, 0.0f);
    drawCenteredText(20, "Click on name field to change player name", 0.7f, 0.7f, 0.7f);

    for (auto& button : settingsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

bool GameUI::isSpeedIncreaseButtonClicked(double mouseX, double mouseY) const {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX + getScaledX(100);
    float buttonY = getScaledY(250);
    return (mouseX >= buttonX - getScaledX(20) && mouseX <= buttonX + getScaledX(20) &&
        mouseY >= buttonY - getScaledY(20) && mouseY <= buttonY + getScaledY(20));
}

bool GameUI::isSpeedDecreaseButtonClicked(double mouseX, double mouseY)const {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX - getScaledX(120);
    float buttonY = getScaledY(250);
    return (mouseX >= buttonX - getScaledX(20) && mouseX <= buttonX + getScaledX(20) &&
        mouseY >= buttonY - getScaledY(20) && mouseY <= buttonY + getScaledY(20));
}

void GameUI::drawGameOver(int score) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);
    drawCenteredText(700, "GAME OVER", 1.0f, 0.0f, 0.0f);

    std::string scoreText = "Final Score: " + std::to_string(score);
    drawCenteredText(600, scoreText, 1.0f, 1.0f, 1.0f);

    for (auto& button : gameOverButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glDisable(GL_BLEND);
}

void GameUI::drawHighScoresMenu(const std::vector<HighScore>& highScores) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.15f, 0.05f, 0.25f, 1.0f);

    drawCenteredText(550, "HIGH SCORES", 1.0f, 1.0f, 0.0f);

    const int rankX = getScaledX(200);
    const int playerX = getScaledX(300);
    const int scoreX = getScaledX(500);
    const int dateX = getScaledX(600);
    const int speedX = getScaledX(750);
    const int lengthX = getScaledX(850);
    const int timeX = getScaledX(950);

    int headerY = getScaledY(500);
    drawText(rankX, headerY, "RANK", 0.8f, 0.8f, 1.0f);
    drawText(playerX, headerY, "PLAYER", 0.8f, 0.8f, 1.0f);
    drawText(scoreX, headerY, "SCORE", 0.8f, 0.8f, 1.0f);
    drawText(dateX, headerY, "DATE", 0.8f, 0.8f, 1.0f);
    drawText(speedX, headerY, "SPEED", 0.8f, 0.8f, 1.0f);
    drawText(lengthX, headerY, "LENGTH", 0.8f, 0.8f, 1.0f);
    drawText(timeX, headerY, "TIME", 0.8f, 0.8f, 1.0f);

    int yPos = getScaledY(450);
    for (size_t i = 0; i < highScores.size() && i < 10; i++) {
        const auto& hs = highScores[i];

        glm::vec3 textColor;
        if (i == 0) textColor = glm::vec3(1.0f, 0.8f, 0.0f);
        else if (i == 1) textColor = glm::vec3(0.7f, 0.7f, 0.7f);
        else if (i == 2) textColor = glm::vec3(0.8f, 0.5f, 0.2f);
        else textColor = glm::vec3(0.0f, 1.0f, 0.0f);

        std::string rankStr = std::to_string(i + 1) + ".";
        drawText(rankX, yPos, rankStr, textColor.r, textColor.g, textColor.b);

        std::string playerName = hs.playerName.substr(0, 10);
        if (hs.playerName.length() > 10) playerName += "..";
        drawText(playerX, yPos, playerName, textColor.r, textColor.g, textColor.b);

        std::string scoreStr = std::to_string(hs.score);
        float scoreWidth = getTextWidth(scoreStr);
        drawText(scoreX + getScaledX(50) - scoreWidth, yPos, scoreStr,
            textColor.r, textColor.g, textColor.b);

        drawText(dateX, yPos, hs.date, textColor.r, textColor.g, textColor.b);

        std::string speedStr = std::to_string(hs.gameSpeed).substr(0, 3) + "x";
        float speedWidth = getTextWidth(speedStr);
        drawText(speedX + getScaledX(30) - speedWidth, yPos, speedStr,
            textColor.r, textColor.g, textColor.b);

        std::string lengthStr = std::to_string(hs.snakeLength);
        float lengthWidth = getTextWidth(lengthStr);
        drawText(lengthX + getScaledX(40) - lengthWidth, yPos, lengthStr,
            textColor.r, textColor.g, textColor.b);

        std::string timeStr = formatGameTime(hs.gameDuration);
        float timeWidth = getTextWidth(timeStr);
        drawText(timeX + getScaledX(40) - timeWidth, yPos, timeStr,
            textColor.r, textColor.g, textColor.b);

        yPos -= getScaledY(35);
    }

    if (highScores.empty()) {
        drawCenteredText(400, "No high scores yet! Play the game to set records!",
            1.0f, 0.5f, 0.5f);
    }

    for (auto& button : highScoresButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawControlsMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

    drawCenteredText(550, "CONTROLS", 1.0f, 1.0f, 1.0f);

    int yPos = getScaledY(450);
    drawCenteredText(yPos, "LEFT/RIGHT - Turn snake", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "Q/E - Rotate camera", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "Mouse Wheel - Zoom", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "ESC - Pause/Menu", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "P - Toggle pause", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "R - Restart game", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);

    for (auto& button : controlsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha) {
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Настройка ортографической проекции
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4f(color.r, color.g, color.b, alpha);

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

void GameUI::drawButtonWithText(const MenuButton& button) {
    glm::vec3 bgColor = button.hovered ? glm::vec3(0.2f, 0.6f, 0.2f) : glm::vec3(0.1f, 0.3f, 0.1f);
    drawQuad(button.x, button.y, button.width, button.height, bgColor, 1.0f);

    if (fontInitialized && !characters.empty()) {
        float textWidth = getTextWidth(button.text);
        float textX = button.x + (button.width - textWidth) / 2;
        float textY = button.y + (button.height / 4) + getScaledY(8);
        drawText(textX, textY, button.text, 1.0f, 1.0f, 1.0f);
    }
}

bool GameUI::isNameFieldClicked(double mouseX, double mouseY) const {
    float centerX = windowWidth / 2.0f;
    float fieldX = centerX - getScaledX(150);
    float fieldY = getScaledY(400);
    float fieldWidth = getScaledWidth(300);
    float fieldHeight = getScaledHeight(40);

    return (mouseX >= fieldX && mouseX <= fieldX + fieldWidth &&
        mouseY >= fieldY && mouseY <= fieldY + fieldHeight);
}

void GameUI::drawNameInputField(const std::string& playerName, bool isActive) {
    float centerX = windowWidth / 2.0f;
    float fieldX = centerX - getScaledX(150);
    float fieldY = getScaledY(400);
    float fieldWidth = getScaledWidth(300);
    float fieldHeight = getScaledHeight(40);

    glm::vec3 fieldColor = isActive ? glm::vec3(0.3f, 0.5f, 0.3f) : glm::vec3(0.2f, 0.2f, 0.2f);
    drawQuad(fieldX, fieldY, fieldWidth, fieldHeight, fieldColor, 1.0f);

    // Рисуем рамку
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINE_LOOP);
    glVertex2f(fieldX, fieldY);
    glVertex2f(fieldX + fieldWidth, fieldY);
    glVertex2f(fieldX + fieldWidth, fieldY + fieldHeight);
    glVertex2f(fieldX, fieldY + fieldHeight);
    glEnd();

    std::string displayName = playerName;
    if (isActive) {
        displayName += "|";
    }
    if (displayName.empty()) {
        displayName = "Click to enter name...";
        drawText(fieldX + getScaledX(10), fieldY + getScaledY(12), displayName, 0.7f, 0.7f, 0.7f);
    }
    else {
        drawText(fieldX + getScaledX(10), fieldY + getScaledY(12), displayName, 1.0f, 1.0f, 1.0f);
    }

    drawCenteredText(450, "PLAYER NAME", 0.8f, 0.8f, 1.0f);

    std::string lengthInfo = "(max 15 characters)";
    float infoX = fieldX + fieldWidth - getTextWidth(lengthInfo);
    float infoY = fieldY - getScaledY(25);
    drawText(infoX, infoY, lengthInfo, 0.6f, 0.6f, 0.6f);

    if (!playerName.empty()) {
        std::string currentLength = std::to_string(playerName.length()) + "/15";
        float lengthX = fieldX + getScaledX(5);
        float lengthY = fieldY - getScaledY(25);
        drawText(lengthX, lengthY, currentLength, 0.6f, 0.6f, 0.6f);
    }

    glEnable(GL_LIGHTING);
}

void GameUI::handleMouseClick(GameObjects& objects) {
    mousePressed = true;

    switch (objects.getGameState()) {
    case MAIN_MENU:
        for (size_t i = 0; i < mainMenuButtons.size(); i++) {
            if (mainMenuButtons[i].contains(mouseX, mouseY)) {
                bool hasSave = objects.hasSaveGame();

                if (hasSave) {
                    switch (i) {
                    case 0:
                        objects.setGameState(PLAYING);
                        objects.loadGame();
                        break;
                    case 1:
                        objects.initGame();
                        objects.setGameState(PLAYING);
                        objects.deleteSaveGame();
                        break;
                    case 2:
                        objects.setPreviousState(MAIN_MENU);
                        objects.setGameState(SETTINGS);
                        break;
                    case 3:
                        objects.setPreviousState(MAIN_MENU);
                        objects.setGameState(HIGH_SCORES);
                        break;
                    case 4:
                        objects.setPreviousState(MAIN_MENU);
                        objects.setGameState(CONTROLS);
                        break;
                    case 5:
                        objects.saveGame();
                        objects.saveSettings();
                        g_shouldExitGame = true;
                        break;
                    }
                }
                else {
                    switch (i) {
                    case 0:
                        objects.setGameState(PLAYING);
                        objects.initGame();
                        break;
                    case 1:
                        objects.setPreviousState(MAIN_MENU);
                        objects.setGameState(SETTINGS);
                        break;
                    case 2:
                        objects.setPreviousState(MAIN_MENU);
                        objects.setGameState(HIGH_SCORES);
                        break;
                    case 3:
                        objects.setPreviousState(MAIN_MENU);
                        objects.setGameState(CONTROLS);
                        break;
                    case 4:
                        objects.saveSettings();
                        g_shouldExitGame = true;
                        break;
                    }
                }
                return;
            }
        }
        break;

    case PAUSED:
        for (size_t i = 0; i < pauseMenuButtons.size(); i++) {
            if (pauseMenuButtons[i].contains(mouseX, mouseY)) {
                switch (i) {
                case 0:
                    objects.setGameState(PLAYING);
                    break;
                case 1:
                    objects.setPreviousState(PAUSED);
                    objects.setGameState(SETTINGS);
                    break;
                case 2:
                    objects.saveGame();
                    objects.setGameState(MAIN_MENU);
                    break;
                }
                return;
            }
        }
        break;

    case SETTINGS:
        for (auto& button : settingsButtons) {
            if (button.contains(mouseX, mouseY)) {
                objects.setGameState(objects.getPreviousState());
                return;
            }
        }
        break;

    case HIGH_SCORES:
        for (auto& button : highScoresButtons) {
            if (button.contains(mouseX, mouseY)) {
                objects.setGameState(objects.getPreviousState());
                return;
            }
        }
        break;

    case CONTROLS:
        for (auto& button : controlsButtons) {
            if (button.contains(mouseX, mouseY)) {
                objects.setGameState(objects.getPreviousState());
                return;
            }
        }
        break;

    case GAME_OVER:
        for (size_t i = 0; i < gameOverButtons.size(); i++) {
            if (gameOverButtons[i].contains(mouseX, mouseY)) {
                switch (i) {
                case 0:
                    objects.initGame();
                    objects.setGameState(PLAYING);
                    objects.deleteSaveGame();
                    break;
                case 1:
                    objects.setGameState(MAIN_MENU);
                    objects.deleteSaveGame();
                    break;
                }
                return;
            }
        }
        break;
    }
}