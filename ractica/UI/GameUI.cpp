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
void GameUI::updateSaveGameInfo(bool hasSaveGame) {
    hasSaveGameFlag = hasSaveGame;
    recreateButtons();
}
void GameUI::recreateButtons() {
    int centerX = getScaledX(600 - 100);
    int buttonYStart = getScaledY(400 + 100);
    int backButtonX = windowWidth - getScaledX(220);
    int backButtonY = getScaledY(50);

    // Используем сохраненную информацию о сохранениях
    bool hasSave = hasSaveGameFlag;

    mainMenuButtons.clear();

    if (hasSave) {
        mainMenuButtons.push_back(MenuButton("CONTINUE", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
        mainMenuButtons.push_back(MenuButton("NEW GAME", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));
    }
    else {
        mainMenuButtons.push_back(MenuButton("PLAY", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    }

    // Используем правильные отступы в зависимости от наличия сохранений
    int firstOffset = hasSave ? 160 : 80;
    int secondOffset = hasSave ? 240 : 160;
    int thirdOffset = hasSave ? 320 : 240;
    int fourthOffset = hasSave ? 400 : 320;

    mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - getScaledY(firstOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, buttonYStart - getScaledY(secondOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, buttonYStart - getScaledY(thirdOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("EXIT", centerX, buttonYStart - getScaledY(fourthOffset), getScaledWidth(200), getScaledHeight(50)));

    // Остальные меню без изменений
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

float GameUI::getTextWidth(const std::string& text) const{
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

void GameUI::drawSettingsMenu(float gameSpeed, const std::string& playerName,
    float speedMultiplier, const std::string& speedDisplayText,
    bool isNameInputActive, bool doubleBufferingEnabled) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.3f, 0.2f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    drawCenteredText(550, "SETTINGS", 1.0f, 1.0f, 1.0f);

    // Поле ввода имени
    drawNameInputField(playerName, isNameInputActive);

    // Настройка скорости
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

bool GameUI::isSpeedIncreaseButtonClicked(double mouseX, double mouseY) {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX + getScaledX(100);
    float buttonY = getScaledY(250); // Обновили Y-координату
    return (mouseX >= buttonX - getScaledX(20) && mouseX <= buttonX + getScaledX(20) &&
        mouseY >= buttonY - getScaledY(20) && mouseY <= buttonY + getScaledY(20));
}

bool GameUI::isSpeedDecreaseButtonClicked(double mouseX, double mouseY) {
    float centerX = windowWidth / 2.0f;
    float buttonX = centerX - getScaledX(120);
    float buttonY = getScaledY(250); // Обновили Y-координату
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

// ЗАМЕНИТЬ существующую функцию drawHighScoresMenu:
void GameUI::drawHighScoresMenu(const std::vector<HighScore>& highScores) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.15f, 0.05f, 0.25f, 1.0f); // Темно-фиолетовый фон

    // Заголовок
    drawCenteredText(550, "HIGH SCORES", 1.0f, 1.0f, 0.0f);

    // Определяем позиции колонок
    const int rankX = getScaledX(200);
    const int playerX = getScaledX(300);
    const int scoreX = getScaledX(500);
    const int dateX = getScaledX(600);
    const int speedX = getScaledX(750);
    const int lengthX = getScaledX(850);
    const int timeX = getScaledX(950);

    // Заголовки колонок
    int headerY = getScaledY(500);
    drawText(rankX, headerY, "RANK", 0.8f, 0.8f, 1.0f);
    drawText(playerX, headerY, "PLAYER", 0.8f, 0.8f, 1.0f);
    drawText(scoreX, headerY, "SCORE", 0.8f, 0.8f, 1.0f);
    drawText(dateX, headerY, "DATE", 0.8f, 0.8f, 1.0f);
    drawText(speedX, headerY, "SPEED", 0.8f, 0.8f, 1.0f);
    drawText(lengthX, headerY, "LENGTH", 0.8f, 0.8f, 1.0f);
    drawText(timeX, headerY, "TIME", 0.8f, 0.8f, 1.0f);

    // Разделительная линия под заголовком
    int lineY = headerY - getScaledY(5);
    drawQuad(rankX - getScaledX(10), lineY, windowWidth - rankX * 2, 2,
        glm::vec3(0.5f, 0.5f, 0.8f), 0.8f);

    // Данные таблицы
    int yPos = getScaledY(450);
    for (size_t i = 0; i < highScores.size() && i < 10; i++) {
        const auto& hs = highScores[i];

        // Цвета для разных позиций
        glm::vec3 textColor;
        if (i == 0) textColor = glm::vec3(1.0f, 0.8f, 0.0f);      // Золотой
        else if (i == 1) textColor = glm::vec3(0.7f, 0.7f, 0.7f); // Серебряный
        else if (i == 2) textColor = glm::vec3(0.8f, 0.5f, 0.2f); // Бронзовый
        else textColor = glm::vec3(0.0f, 1.0f, 0.0f);             // Зеленый

        // Ранг
        std::string rankStr = std::to_string(i + 1) + ".";
        drawText(rankX, yPos, rankStr, textColor.r, textColor.g, textColor.b);

        // Имя игрока (обрезаем если слишком длинное)
        std::string playerName = hs.playerName.substr(0, 10);
        if (hs.playerName.length() > 10) playerName += "..";
        drawText(playerX, yPos, playerName, textColor.r, textColor.g, textColor.b);

        // Счет
        std::string scoreStr = std::to_string(hs.score);
        float scoreWidth = getTextWidth(scoreStr);
        drawText(scoreX + getScaledX(50) - scoreWidth, yPos, scoreStr,
            textColor.r, textColor.g, textColor.b);

        // Дата
        drawText(dateX, yPos, hs.date, textColor.r, textColor.g, textColor.b);

        // Скорость
        std::string speedStr = std::to_string(hs.gameSpeed).substr(0, 3) + "x";
        float speedWidth = getTextWidth(speedStr);
        drawText(speedX + getScaledX(30) - speedWidth, yPos, speedStr,
            textColor.r, textColor.g, textColor.b);

        // Длина змейки
        std::string lengthStr = std::to_string(hs.snakeLength);
        float lengthWidth = getTextWidth(lengthStr);
        drawText(lengthX + getScaledX(40) - lengthWidth, yPos, lengthStr,
            textColor.r, textColor.g, textColor.b);

        // Время игры
        std::string timeStr = formatGameTime(hs.gameDuration);
        float timeWidth = getTextWidth(timeStr);
        drawText(timeX + getScaledX(40) - timeWidth, yPos, timeStr,
            textColor.r, textColor.g, textColor.b);

        yPos -= getScaledY(35);
    }

    // Подпись если таблица пустая
    if (highScores.empty()) {
        drawCenteredText(400, "No high scores yet! Play the game to set records!",
            1.0f, 0.5f, 0.5f);
    }

    // Кнопка назад
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
                bool hasSave = objects.hasSaveGame();

                if (hasSave) {
                    switch (i) {
                    case 0:
                        objects.setGameState(PLAYING);
                        objects.loadGame(); // CONTINUE - загружаем сохранение
                        break;
                    case 1:
                        objects.initGame(); // NEW GAME - начинаем новую
                        objects.setGameState(PLAYING);
                        objects.deleteSaveGame(); // Удаляем старое сохранение
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
                        // EXIT - сохраняем игру и выходим
                        objects.saveGame(); // Сохраняем игру при выходе через EXIT
                        objects.saveSettings(); // Сохраняем настройки
                        g_shouldExitGame = true;
                        break;
                    }
                }
                else {
                    switch (i) {
                    case 0:
                        objects.setGameState(PLAYING);
                        objects.initGame(); // PLAY - начинаем новую игру
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
                        // EXIT - сохраняем настройки и выходим (игры нет для сохранения)
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
                    // RESUME - просто продолжаем игру без сохранения
                    objects.setGameState(PLAYING);
                    break;
                case 1:
                    // SETTINGS - переходим в настройки
                    objects.setPreviousState(PAUSED);
                    objects.setGameState(SETTINGS);
                    break;
                case 2:
                    // MAIN MENU - сохраняем игру и возвращаемся в меню
                    objects.saveGame(); // Сохраняем игру при переходе в главное меню
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
                // BACK - возвращаемся в предыдущее состояние
                // Настройки НЕ сохраняем здесь - они сохраняются только при изменении
                objects.setGameState(objects.getPreviousState());
                return;
            }
        }
        break;

    case HIGH_SCORES:
        for (auto& button : highScoresButtons) {
            if (button.contains(mouseX, mouseY)) {
                // BACK - возвращаемся в предыдущее состояние
                objects.setGameState(objects.getPreviousState());
                return;
            }
        }
        break;

    case CONTROLS:
        for (auto& button : controlsButtons) {
            if (button.contains(mouseX, mouseY)) {
                // BACK - возвращаемся в предыдущее состояние
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
                    // RESTART - начинаем новую игру, удаляем сохранение
                    objects.initGame();
                    objects.setGameState(PLAYING);
                    objects.deleteSaveGame(); // Удаляем сохранение завершенной игры
                    break;
                case 1:
                    // MAIN MENU - возвращаемся в меню, удаляем сохранение
                    objects.setGameState(MAIN_MENU);
                    objects.deleteSaveGame(); // Удаляем сохранение завершенной игры
                    break;
                }
                return;
            }
        }
        break;
    }
}
bool GameUI::isNameFieldClicked(double mouseX, double mouseY) const {
    float centerX = windowWidth / 2.0f;
    float fieldX = centerX - getScaledX(150);
    float fieldY = getScaledY(400); // Такая же Y-координата как в drawNameInputField
    float fieldWidth = getScaledWidth(300);
    float fieldHeight = getScaledHeight(40);

    return (mouseX >= fieldX && mouseX <= fieldX + fieldWidth &&
        mouseY >= fieldY && mouseY <= fieldY + fieldHeight);
}

void GameUI::drawNameInputField(const std::string& playerName, bool isActive) {
    float centerX = windowWidth / 2.0f;
    float fieldX = centerX - getScaledX(150);
    float fieldY = getScaledY(400); // Было 350 - ПОДНЯЛИ ВЫШЕ
    float fieldWidth = getScaledWidth(300);
    float fieldHeight = getScaledHeight(40);

    // Рисуем фон поля ввода
    glm::vec3 fieldColor = isActive ? glm::vec3(0.3f, 0.5f, 0.3f) : glm::vec3(0.2f, 0.2f, 0.2f);
    drawQuad(fieldX, fieldY, fieldWidth, fieldHeight, fieldColor, 1.0f);

    // Рисуем рамку
    drawQuad(fieldX - 2, fieldY - 2, fieldWidth + 4, 2, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f); // верх
    drawQuad(fieldX - 2, fieldY + fieldHeight, fieldWidth + 4, 2, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f); // низ
    drawQuad(fieldX - 2, fieldY - 2, 2, fieldHeight + 4, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f); // лево
    drawQuad(fieldX + fieldWidth, fieldY - 2, 2, fieldHeight + 4, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f); // право

    // Рисуем текст имени
    std::string displayName = playerName;
    if (isActive) {
        displayName += "|"; // Курсор
    }
    if (displayName.empty()) {
        displayName = "Click to enter name...";
        drawText(fieldX + getScaledX(10), fieldY + getScaledY(12), displayName, 0.7f, 0.7f, 0.7f);
    }
    else {
        drawText(fieldX + getScaledX(10), fieldY + getScaledY(12), displayName, 1.0f, 1.0f, 1.0f);
    }

    // Подпись и информация о максимальной длине
    drawCenteredText(450, "PLAYER NAME", 0.8f, 0.8f, 1.0f); // Было 320 - ПОДНЯЛИ ВЫШЕ

    // Информация о максимальной длине (под полем ввода)
    std::string lengthInfo = "(max 15 characters)";
    float infoX = fieldX + fieldWidth - getTextWidth(lengthInfo);
    float infoY = fieldY - getScaledY(25);
    drawText(infoX, infoY, lengthInfo, 0.6f, 0.6f, 0.6f);

    // Также показываем текущую длину имени
    if (!playerName.empty()) {
        std::string currentLength = std::to_string(playerName.length()) + "/15";
        float lengthX = fieldX + getScaledX(5);
        float lengthY = fieldY - getScaledY(25);
        drawText(lengthX, lengthY, currentLength, 0.6f, 0.6f, 0.6f);
    }
}