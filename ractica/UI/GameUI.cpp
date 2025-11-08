#include "../pch.h"
#include "GameUI.h"
#include "../Core/Constants.h"
#include "../Core/UITypes.h"
#include "../Core/Types.h"
#include "../UI/MenuButton.h"
#include "../Graphics/ShaderManager.h"
#include "../Objects/GameObjects.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>

extern ShaderManager g_shaderManager;
extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;

// ============================================================================
//  ŒÕ—“–” “Œ– » ƒ≈—“–” “Œ–
// ============================================================================

GameUI::GameUI()
    : windowWidth(DEFAULT_WINDOW_WIDTH),
    windowHeight(DEFAULT_WINDOW_HEIGHT),
    mouseX(0), mouseY(0), mousePressed(false),
    uiInitialized(false), fontInitialized(false),
    hasSaveGameFlag(false),
    ft(nullptr), face(nullptr), textVAO(0), textVBO(0), textShader(0),
    fontConfig(), buttonConfig() {
}

GameUI::~GameUI() {
    cleanup();
}

// ============================================================================
// Œ—ÕŒ¬Õ€≈ Ã≈“Œƒ€ »Õ»÷»¿À»«¿÷»» » Œ◊»—“ »
// ============================================================================

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

    unsigned int fontSize = getScaledFontSize();
    std::cout << "Loading font with scaled size: " << fontSize << std::endl;

    if (!loadFreeTypeFont(fontConfig.fontPath, fontSize)) {
        std::cerr << "Failed to load font with FreeType" << std::endl;
        return;
    }

    setupTextBuffers();
    compileTextShaders();
    fontInitialized = true;
    std::cout << "FreeType font initialized successfully with size: " << fontSize << std::endl;
}

// ============================================================================
// Ã≈“Œƒ€ –¿¡Œ“€ — Œ ÕŒÃ » Ã¿—ÿ“¿¡»–Œ¬¿Õ»≈Ã
// ============================================================================

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

int GameUI::getScaledX(int x) const {
    return static_cast<int>(x * (static_cast<float>(windowWidth) / SCALE_BASE_WIDTH));
}

int GameUI::getScaledY(int y) const {
    return static_cast<int>(y * (static_cast<float>(windowHeight) / SCALE_BASE_HEIGHT));
}

int GameUI::getScaledWidth(int width) const {
    return static_cast<int>(width * (static_cast<float>(windowWidth) / SCALE_BASE_WIDTH));
}

int GameUI::getScaledHeight(int height) const {
    return static_cast<int>(height * (static_cast<float>(windowHeight) / SCALE_BASE_HEIGHT));
}

unsigned int GameUI::getScaledFontSize() const {
    return static_cast<unsigned int>(DEFAULT_FONT_SIZE * (static_cast<float>(windowHeight) / SCALE_BASE_HEIGHT));
}

// ============================================================================
// —»—“≈Ã¿  ÕŒœŒ  Ã≈Õﬁ
// ============================================================================

void GameUI::updateSaveGameInfo(bool hasSaveGame) {
    hasSaveGameFlag = hasSaveGame;
    recreateButtons();
}

void GameUI::recreateButtons() {
    int centerX = getScaledX(SCALE_BASE_WIDTH / 2 - BUTTON_WIDTH / 2);
    int buttonYStart = getScaledY(MENU_START_Y + 100);
    int backButtonX = windowWidth - getScaledX(BACK_BUTTON_X_OFFSET);
    int backButtonY = getScaledY(BACK_BUTTON_Y);

    mainMenuButtons.clear();

    if (hasSaveGameFlag) {
        mainMenuButtons.push_back(MenuButton("CONTINUE", centerX, buttonYStart, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
        mainMenuButtons.push_back(MenuButton("NEW GAME", centerX, buttonYStart - getScaledY(BUTTON_SPACING), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    }
    else {
        mainMenuButtons.push_back(MenuButton("PLAY", centerX, buttonYStart, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    }

    int firstOffset = hasSaveGameFlag ? 160 : 80;
    int secondOffset = hasSaveGameFlag ? 240 : 160;
    int thirdOffset = hasSaveGameFlag ? 320 : 240;
    int fourthOffset = hasSaveGameFlag ? 400 : 320;

    mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - getScaledY(firstOffset), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, buttonYStart - getScaledY(secondOffset), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, buttonYStart - getScaledY(thirdOffset), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    mainMenuButtons.push_back(MenuButton("EXIT", centerX, buttonYStart - getScaledY(fourthOffset), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));

    pauseMenuButtons.clear();
    pauseMenuButtons.push_back(MenuButton("RESUME", centerX, buttonYStart, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    pauseMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - getScaledY(BUTTON_SPACING), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    pauseMenuButtons.push_back(MenuButton("MAIN MENU", centerX, buttonYStart - getScaledY(BUTTON_SPACING * 2), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));

    gameOverButtons.clear();
    gameOverButtons.push_back(MenuButton("RESTART", centerX, buttonYStart, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
    gameOverButtons.push_back(MenuButton("MAIN MENU", centerX, buttonYStart - getScaledY(BUTTON_SPACING), getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));

    settingsButtons.clear();
    settingsButtons.push_back(MenuButton("BACK", backButtonX, backButtonY, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));

    controlsButtons.clear();
    controlsButtons.push_back(MenuButton("BACK", backButtonX, backButtonY, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));

    highScoresButtons.clear();
    highScoresButtons.push_back(MenuButton("BACK", backButtonX, backButtonY, getScaledWidth(BUTTON_WIDTH), getScaledHeight(BUTTON_HEIGHT)));
}

// ============================================================================
// —»—“≈Ã¿ ÿ–»‘“Œ¬ » “≈ —“¿
// ============================================================================

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

    glUseProgram(textShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(textShader, "textColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float startX = x;
    for (const char& c : text) {
        auto it = characters.find(c);
        if (it == characters.end()) continue;

        TextCharacter ch = it->second;
        float xpos = startX + ch.bearing.x;
        float ypos = y - (ch.size.y - ch.bearing.y);
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

// ============================================================================
// Ã≈“Œƒ€ Œ“–»—Œ¬ » Ã≈Õﬁ
// ============================================================================

void GameUI::drawMainMenu() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(COLOR_MAIN_MENU_BG.r, COLOR_MAIN_MENU_BG.g, COLOR_MAIN_MENU_BG.b, 1.0f);

    drawCenteredText(TITLE_Y, "3D SNAKE GAME", COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    for (auto& button : mainMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawPauseMenu() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(0, 0, windowWidth, windowHeight, COLOR_PAUSE_OVERLAY, OVERLAY_ALPHA);
    drawCenteredText(700, "PAUSED", COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    for (auto& button : pauseMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glDisable(GL_BLEND);
}

void GameUI::drawSettingsMenu(float gameSpeed, const std::string& playerName, float speedMultiplier, const std::string& speedDisplayText) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(COLOR_SETTINGS_BG.r, COLOR_SETTINGS_BG.g, COLOR_SETTINGS_BG.b, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    drawCenteredText(550, "SETTINGS", COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);
    drawCenteredText(450, "SPEED", COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    float multiplierY = getScaledY(SETTINGS_SPEED_Y);
    drawCenteredText(SETTINGS_SPEED_Y, speedDisplayText, COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    float centerX = windowWidth / 2.0f;
    bool minusHover = isSpeedDecreaseButtonClicked(mouseX, mouseY);
    bool plusHover = isSpeedIncreaseButtonClicked(mouseX, mouseY);

    drawText(centerX - getScaledX(120), multiplierY, "-",
        minusHover ? COLOR_TEXT_RED.r : 0.7f, minusHover ? COLOR_TEXT_RED.g : 0.3f, 0.0f);
    drawText(centerX + getScaledX(100), multiplierY, "+",
        0.0f, plusHover ? COLOR_TEXT_GREEN.r : 0.7f, 0.0f);

    drawCenteredText(250, "Use +/- buttons or keyboard", 0.7f, 0.7f, 0.7f);

    for (auto& button : settingsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void GameUI::drawGameOver(int score) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(0, 0, windowWidth, windowHeight, COLOR_GAME_OVER_OVERLAY, OVERLAY_ALPHA);
    drawCenteredText(700, "GAME OVER", COLOR_TEXT_RED.r, COLOR_TEXT_RED.g, COLOR_TEXT_RED.b);

    std::string scoreText = "Final Score: " + std::to_string(score);
    drawCenteredText(600, scoreText, COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    for (auto& button : gameOverButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glDisable(GL_BLEND);
}

void GameUI::drawHighScoresMenu(const std::vector<HighScore>& highScores) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(COLOR_HIGHSCORES_BG.r, COLOR_HIGHSCORES_BG.g, COLOR_HIGHSCORES_BG.b, 1.0f);

    drawCenteredText(550, "HIGH SCORES", COLOR_TEXT_YELLOW.r, COLOR_TEXT_YELLOW.g, COLOR_TEXT_YELLOW.b);

    const int rankX = getScaledX(HIGH_SCORES_RANK_X);
    const int playerX = getScaledX(HIGH_SCORES_PLAYER_X);
    const int scoreX = getScaledX(HIGH_SCORES_SCORE_X);
    const int dateX = getScaledX(HIGH_SCORES_DATE_X);
    const int speedX = getScaledX(HIGH_SCORES_SPEED_X);
    const int lengthX = getScaledX(HIGH_SCORES_LENGTH_X);
    const int timeX = getScaledX(HIGH_SCORES_TIME_X);
    const int headerY = getScaledY(HIGH_SCORES_HEADER_Y);

    drawText(rankX, headerY, "RANK", 0.8f, 0.8f, 1.0f);
    drawText(playerX, headerY, "PLAYER", 0.8f, 0.8f, 1.0f);
    drawText(scoreX, headerY, "SCORE", 0.8f, 0.8f, 1.0f);
    drawText(dateX, headerY, "DATE", 0.8f, 0.8f, 1.0f);
    drawText(speedX, headerY, "SPEED", 0.8f, 0.8f, 1.0f);
    drawText(lengthX, headerY, "LENGTH", 0.8f, 0.8f, 1.0f);
    drawText(timeX, headerY, "TIME", 0.8f, 0.8f, 1.0f);

    int lineY = headerY - getScaledY(5);
    drawQuad(rankX - getScaledX(10), lineY, windowWidth - rankX * 2, 2,
        glm::vec3(0.5f, 0.5f, 0.8f), 0.8f);

    int yPos = getScaledY(450);
    for (size_t i = 0; i < highScores.size() && i < HIGH_SCORES_MAX_DISPLAY; i++) {
        const auto& hs = highScores[i];

        glm::vec3 textColor;
        if (i == 0) textColor = COLOR_GOLD;
        else if (i == 1) textColor = COLOR_SILVER;
        else if (i == 2) textColor = COLOR_BRONZE;
        else textColor = COLOR_TEXT_GREEN;

        std::string rankStr = std::to_string(i + 1) + ".";
        drawText(rankX, yPos, rankStr, textColor.r, textColor.g, textColor.b);

        std::string playerName = hs.playerName.substr(0, HIGH_SCORES_MAX_NAME_LENGTH);
        if (hs.playerName.length() > HIGH_SCORES_MAX_NAME_LENGTH) playerName += "..";
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

        int minutes = hs.gameDuration / 60;
        int secs = hs.gameDuration % 60;
        std::stringstream timeSS;
        timeSS << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << secs;
        std::string timeStr = timeSS.str();
        float timeWidth = getTextWidth(timeStr);
        drawText(timeX + getScaledX(40) - timeWidth, yPos, timeStr,
            textColor.r, textColor.g, textColor.b);

        yPos -= getScaledY(HIGH_SCORES_ROW_SPACING);
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
    glClearColor(COLOR_CONTROLS_BG.r, COLOR_CONTROLS_BG.g, COLOR_CONTROLS_BG.b, 1.0f);

    drawCenteredText(550, "CONTROLS", COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    int yPos = getScaledY(CONTROLS_START_Y);
    drawCenteredText(yPos, "LEFT/RIGHT - Turn snake", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);
    yPos -= getScaledY(CONTROLS_SPACING);
    drawCenteredText(yPos, "Q/E - Rotate camera", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);
    yPos -= getScaledY(CONTROLS_SPACING);
    drawCenteredText(yPos, "Mouse Wheel - Zoom", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);
    yPos -= getScaledY(CONTROLS_SPACING);
    drawCenteredText(yPos, "ESC - Pause/Menu", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);
    yPos -= getScaledY(CONTROLS_SPACING);
    drawCenteredText(yPos, "P - Toggle pause", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);
    yPos -= getScaledY(CONTROLS_SPACING);
    drawCenteredText(yPos, "R - Restart game", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);
    yPos -= getScaledY(CONTROLS_SPACING);
    drawCenteredText(yPos, "D - Debug info", COLOR_TEXT_GREEN.r, COLOR_TEXT_GREEN.g, COLOR_TEXT_GREEN.b);

    for (auto& button : controlsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawInGameUI(int score, float speedMultiplier, const std::string& speedDisplayText) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::string scoreText = "Score: " + std::to_string(score);
    drawText(getScaledX(20), getScaledY(SCORE_DISPLAY_Y), scoreText,
        COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    std::string speedText = "Speed: " + speedDisplayText;
    drawText(getScaledX(20), getScaledY(SPEED_DISPLAY_Y), speedText,
        COLOR_TEXT_WHITE.r, COLOR_TEXT_WHITE.g, COLOR_TEXT_WHITE.b);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// ============================================================================
// ¡¿«Œ¬€≈ Ã≈“Œƒ€ Œ“–»—Œ¬ »
// ============================================================================

void GameUI::drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha) {
    g_shaderManager.useUIShader();
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    g_shaderManager.setUIProjectionMatrix(projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));
    g_shaderManager.setUIModelMatrix(model);

    g_shaderManager.setUIColor(color);
    g_shaderManager.setUIAlpha(alpha);

    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void GameUI::drawButtonWithText(const MenuButton& button) {
    glm::vec3 bgColor = button.hovered ? buttonConfig.hoverColor : buttonConfig.normalColor;
    drawQuad(button.x, button.y, button.width, button.height, bgColor, buttonConfig.alpha);

    if (fontInitialized && !characters.empty()) {
        float textWidth = getTextWidth(button.text);
        float textX = button.x + (button.width - textWidth) / 2;
        float textY = button.y + (button.height / 4) + getScaledY(8);

        drawText(textX, textY, button.text,
            buttonConfig.textColor.r, buttonConfig.textColor.g, buttonConfig.textColor.b);
    }
}

// ============================================================================
// Œ¡–¿¡Œ“ ¿ ¬¬Œƒ¿
// ============================================================================

void GameUI::setMousePosition(double x, double y) {
    mouseX = x;
    mouseY = windowHeight - y;
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

// ============================================================================
// ¬—œŒÃŒ√¿“≈À‹Õ€≈ Ã≈“Œƒ€
// ============================================================================

bool GameUI::isSpeedIncreaseButtonClicked(double mouseX, double mouseY) {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX + getScaledX(SETTINGS_SPEED_BUTTON_OFFSET);
    float buttonY = getScaledY(SETTINGS_SPEED_Y);
    return (mouseX >= buttonX - getScaledX(SETTINGS_SPEED_BUTTON_SIZE) &&
        mouseX <= buttonX + getScaledX(SETTINGS_SPEED_BUTTON_SIZE) &&
        mouseY >= buttonY - getScaledY(SETTINGS_SPEED_BUTTON_SIZE) &&
        mouseY <= buttonY + getScaledY(SETTINGS_SPEED_BUTTON_SIZE));
}

bool GameUI::isSpeedDecreaseButtonClicked(double mouseX, double mouseY) {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX - getScaledX(SETTINGS_SPEED_BUTTON_OFFSET + 20);
    float buttonY = getScaledY(SETTINGS_SPEED_Y);
    return (mouseX >= buttonX - getScaledX(SETTINGS_SPEED_BUTTON_SIZE) &&
        mouseX <= buttonX + getScaledX(SETTINGS_SPEED_BUTTON_SIZE) &&
        mouseY >= buttonY - getScaledY(SETTINGS_SPEED_BUTTON_SIZE) &&
        mouseY <= buttonY + getScaledY(SETTINGS_SPEED_BUTTON_SIZE));
}

// ============================================================================
//  ŒÕ‘»√”–¿÷»ﬂ
// ============================================================================

void GameUI::setFontConfig(const FontConfig& config) {
    fontConfig = config;
    if (fontInitialized) {
        cleanupFreeType();
        initWindowsFont();
    }
}

void GameUI::setButtonConfig(const ButtonConfig& config) {
    buttonConfig = config;
}