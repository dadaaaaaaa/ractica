#include "../pch.h"
#include "GameUI.h"
#include "../Graphics/ShaderManager.h"
#include "../Objects/GameObjects.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern ShaderManager g_shaderManager;
extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;

GameUI::GameUI()
    : windowWidth(1200), windowHeight(800),
    mouseX(0), mouseY(0), mousePressed(false),
    uiInitialized(false), fontInitialized(false),
    ft(nullptr), face(nullptr), textVAO(0), textVBO(0), textShader(0) {
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
    if (textShader) glDeleteProgram(textShader);

    textVAO = textVBO = textShader = 0;
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

void GameUI::recreateButtons() {
    int centerX = getScaledX(600 - 100);
    int buttonYStart = getScaledY(400 + 100);
    int backButtonX = windowWidth - getScaledX(220);
    int backButtonY = getScaledY(50);

    mainMenuButtons.clear();
    mainMenuButtons.push_back(MenuButton("PLAY", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, buttonYStart - getScaledY(160), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, buttonYStart - getScaledY(240), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("EXIT", centerX, buttonYStart - getScaledY(320), getScaledWidth(200), getScaledHeight(50)));

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

    float vertices[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f
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

    // Размер шрифта масштабируется в зависимости от разрешения
    unsigned int fontSize = getScaledFontSize();
    std::cout << "Loading font with scaled size: " << fontSize << " (base: 24)" << std::endl;

    if (!loadFreeTypeFont("C:/Windows/Fonts/arial.ttf", fontSize)) {
        std::cerr << "Failed to load font with FreeType" << std::endl;
        return;
    }

    setupTextBuffers();
    compileTextShaders();
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

void GameUI::compileTextShaders() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec4 vertex;
        out vec2 TexCoords;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        uniform sampler2D text;
        uniform vec3 textColor;
        void main() {    
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    textShader = glCreateProgram();
    glAttachShader(textShader, vertexShader);
    glAttachShader(textShader, fragmentShader);
    glLinkProgram(textShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

float GameUI::getTextWidth(const std::string& text) {
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

    glUseProgram(textShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(textShader, "textColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void GameUI::drawSettingsMenu(float gameSpeed, const std::string& playerName, float speedMultiplier, const std::string& speedDisplayText) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.3f, 0.2f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    drawCenteredText(550, "SETTINGS", 1.0f, 1.0f, 1.0f);
    drawCenteredText(450, "SPEED", 1.0f, 1.0f, 1.0f);

    float multiplierY = getScaledY(350);
    drawCenteredText(350, speedDisplayText, 1.0f, 1.0f, 1.0f);

    float centerX = windowWidth / 2.0f;
    bool minusHover = isSpeedDecreaseButtonClicked(mouseX, mouseY);
    bool plusHover = isSpeedIncreaseButtonClicked(mouseX, mouseY);

    drawText(centerX - getScaledX(120), multiplierY, "-",
        minusHover ? 1.0f : 0.7f, 0.3f, 0.0f);
    drawText(centerX + getScaledX(100), multiplierY, "+",
        0.0f, plusHover ? 1.0f : 0.7f, 0.0f);

    drawCenteredText(250, "Use +/- buttons or keyboard", 0.7f, 0.7f, 0.7f);

    for (auto& button : settingsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

bool GameUI::isSpeedIncreaseButtonClicked(double mouseX, double mouseY) {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX + getScaledX(100);
    float buttonY = getScaledY(350);
    return (mouseX >= buttonX - getScaledX(20) && mouseX <= buttonX + getScaledX(20) &&
        mouseY >= buttonY - getScaledY(20) && mouseY <= buttonY + getScaledY(20));
}

bool GameUI::isSpeedDecreaseButtonClicked(double mouseX, double mouseY) {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX - getScaledX(120);
    float buttonY = getScaledY(350);
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
    glClearColor(0.2f, 0.1f, 0.1f, 1.0f);

    drawCenteredText(550, "HIGH SCORES", 1.0f, 1.0f, 1.0f);

    int yPos = getScaledY(450);
    for (size_t i = 0; i < highScores.size() && i < 10; i++) {
        std::string scoreText = std::to_string(i + 1) + ". " + highScores[i].playerName +
            " - " + std::to_string(highScores[i].score) + " (" + highScores[i].date + ")";
        drawCenteredText(yPos, scoreText, 1.0f, 1.0f, 0.0f);
        yPos -= getScaledY(40);
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
    drawCenteredText(yPos, "D - Debug info", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);

    for (auto& button : controlsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha) {
    g_shaderManager.useUIShader();
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    g_shaderManager.setUIProjection(projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));
    g_shaderManager.setUIModel(model);

    g_shaderManager.setUIColor(color);
    g_shaderManager.setUIAlpha(alpha);

    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void GameUI::drawButtonWithText(const MenuButton& button) {
    glm::vec3 bgColor = button.hovered ? glm::vec3(0.2f, 0.6f, 0.2f) : glm::vec3(0.1f, 0.3f, 0.1f);
    drawQuad(button.x, button.y, button.width, button.height, bgColor, 1.0f);

    if (fontInitialized && !characters.empty()) {
        float textWidth = getTextWidth(button.text);

        // Позиционируем текст по центру кнопки
        // Координаты кнопки уже масштабированы, поэтому используем их как есть
        float textX = button.x + (button.width - textWidth) / 2;

        // Y-координата: центр кнопки + небольшой отступ для визуального центрирования
        // Отступ также масштабируем
        float textY = button.y + (button.height / 4) + getScaledY(8);

        // Рисуем текст с масштабированными координатами
        drawText(textX, textY, button.text, 1.0f, 1.0f, 1.0f);
    }
}

void GameUI::handleMouseClick(GameObjects& objects) {
    mousePressed = true;

    switch (objects.getGameState()) {
    case MAIN_MENU:
        for (size_t i = 0; i < mainMenuButtons.size(); i++) {
            if (mainMenuButtons[i].contains(mouseX, mouseY)) {
                switch (i) {
                case 0: objects.setGameState(PLAYING); objects.initGame(); break;
                case 1: objects.setPreviousState(MAIN_MENU); objects.setGameState(SETTINGS); break;
                case 2: objects.setPreviousState(MAIN_MENU); objects.setGameState(HIGH_SCORES); break;
                case 3: objects.setPreviousState(MAIN_MENU); objects.setGameState(CONTROLS); break;
                case 4: g_shouldExitGame = true; break;
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

    case PAUSED:
        for (size_t i = 0; i < pauseMenuButtons.size(); i++) {
            if (pauseMenuButtons[i].contains(mouseX, mouseY)) {
                switch (i) {
                case 0: objects.setGameState(PLAYING); break;
                case 1: objects.setPreviousState(PAUSED); objects.setGameState(SETTINGS); break;
                case 2: objects.setGameState(MAIN_MENU); break;
                }
                return;
            }
        }
        break;

    case GAME_OVER:
        for (size_t i = 0; i < gameOverButtons.size(); i++) {
            if (gameOverButtons[i].contains(mouseX, mouseY)) {
                switch (i) {
                case 0: objects.initGame(); objects.setGameState(PLAYING); break;
                case 1: objects.setGameState(MAIN_MENU); break;
                }
                return;
            }
        }
        break;
    }
}