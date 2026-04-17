#include "../pch.h"
#include "GameUI.h"
#include "../Objects/GameObjects.h"
#include <iostream>
#include <glm/glm.hpp>
#include <windows.h>
#include <map>

extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;

// Глобальные переменные для шрифта (как в рабочем проекте)
static FT_Library g_ft;
static FT_Face g_face;
static std::map<unsigned char, Character> g_characters;  // используем Character как в рабочем проекте
static GLuint g_textVAO = 0, g_textVBO = 0;
static bool g_fontInitialized = false;



// Вспомогательная функция для форматирования времени
static std::string formatGameTime(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    if (minutes > 0) {
        return std::to_string(minutes) + "м " + std::to_string(secs) + "с";
    }
    return std::to_string(secs) + "с";
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
    if (g_face) FT_Done_Face(g_face);
    if (g_ft) FT_Done_FreeType(g_ft);

    for (auto& character : g_characters) {
        glDeleteTextures(1, &character.second.TextureID);
    }
    g_characters.clear();

    if (g_textVAO) glDeleteVertexArrays(1, &g_textVAO);
    if (g_textVBO) glDeleteBuffers(1, &g_textVBO);

    g_textVAO = g_textVBO = 0;
    fontInitialized = false;
    g_fontInitialized = false;
}

void GameUI::setWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    std::cout << "Размер окна изменён: " << width << "x" << height << std::endl;

    if (uiInitialized) {
        recreateButtons();

        if (fontInitialized || g_fontInitialized) {
            std::cout << "Пересоздаём шрифт..." << std::endl;
            cleanupFreeType();
            initWindowsFont();
        }
    }
}

void GameUI::updateSaveGameInfo(bool hasSaveGame) {
    hasSaveGameFlag = hasSaveGame;
    recreateButtons();
}

// РУССКИЙ ТЕКСТ (UTF-8 строки - сохраните файл в UTF-8)
void GameUI::recreateButtons() {
    int centerX = getScaledX(600 - 100);
    int buttonYStart = getScaledY(400 + 100);
    int backButtonX = windowWidth - getScaledX(220);
    int backButtonY = getScaledY(50);

    bool hasSave = hasSaveGameFlag;

    mainMenuButtons.clear();

    if (hasSave) {
        mainMenuButtons.push_back(MenuButton("ПРОДОЛЖИТЬ", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
        mainMenuButtons.push_back(MenuButton("НОВАЯ ИГРА", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));
    }
    else {
        mainMenuButtons.push_back(MenuButton("ИГРАТЬ", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    }

    int firstOffset = hasSave ? 160 : 80;
    int secondOffset = hasSave ? 240 : 160;
    int thirdOffset = hasSave ? 320 : 240;
    int fourthOffset = hasSave ? 400 : 320;

    mainMenuButtons.push_back(MenuButton("НАСТРОЙКИ", centerX, buttonYStart - getScaledY(firstOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("РЕКОРДЫ", centerX, buttonYStart - getScaledY(secondOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("УПРАВЛЕНИЕ", centerX, buttonYStart - getScaledY(thirdOffset), getScaledWidth(200), getScaledHeight(50)));
    mainMenuButtons.push_back(MenuButton("ВЫХОД", centerX, buttonYStart - getScaledY(fourthOffset), getScaledWidth(200), getScaledHeight(50)));

    pauseMenuButtons.clear();
    pauseMenuButtons.push_back(MenuButton("ПРОДОЛЖИТЬ", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    pauseMenuButtons.push_back(MenuButton("НАСТРОЙКИ", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));
    pauseMenuButtons.push_back(MenuButton("ГЛАВНОЕ МЕНЮ", centerX, buttonYStart - getScaledY(160), getScaledWidth(200), getScaledHeight(50)));

    gameOverButtons.clear();
    gameOverButtons.push_back(MenuButton("ЗАНОВО", centerX, buttonYStart, getScaledWidth(200), getScaledHeight(50)));
    gameOverButtons.push_back(MenuButton("ГЛАВНОЕ МЕНЮ", centerX, buttonYStart - getScaledY(80), getScaledWidth(200), getScaledHeight(50)));

    settingsButtons.clear();
    settingsButtons.push_back(MenuButton("НАЗАД", backButtonX, backButtonY, getScaledWidth(200), getScaledHeight(50)));

    controlsButtons.clear();
    controlsButtons.push_back(MenuButton("НАЗАД", backButtonX, backButtonY, getScaledWidth(200), getScaledHeight(50)));

    highScoresButtons.clear();
    highScoresButtons.push_back(MenuButton("НАЗАД", backButtonX, backButtonY, getScaledWidth(200), getScaledHeight(50)));
}

void GameUI::setMousePosition(double x, double y) {
    mouseX = x;
    mouseY = windowHeight - y;
}

void GameUI::initUI() {
    std::cout << "Инициализация UI..." << std::endl;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    initWindowsFont();
    recreateButtons();

    uiInitialized = true;
    std::cout << "UI успешно инициализирован" << std::endl;
}

void GameUI::initWindowsFont() {
    std::cout << "Инициализация шрифта FreeType..." << std::endl;

    if (FT_Init_FreeType(&g_ft)) {
        std::cerr << "ОШИБКА: Не удалось инициализировать FreeType" << std::endl;
        return;
    }

    // Загружаем шрифт
    std::string fontPath = "C:/Windows/Fonts/arial.ttf";
    if (FT_New_Face(g_ft, fontPath.c_str(), 0, &g_face)) {
        std::cerr << "ОШИБКА: Не удалось загрузить шрифт" << std::endl;
        return;
    }

    unsigned int fontSize = getScaledFontSize();
    FT_Set_Pixel_Sizes(g_face, 0, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Загружаем ASCII символы (32-126)
    for (unsigned char c = 32; c < 127; c++) {
        if (FT_Load_Char(g_face, c, FT_LOAD_RENDER)) continue;

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        // ВАЖНО: используем GL_ALPHA вместо GL_RED для правильной прозрачности
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows),
            glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top),
            (unsigned int)g_face->glyph->advance.x
        };
        g_characters.insert(std::pair<unsigned char, Character>(c, character));
    }

    // Заглавные русские буквы А-Я (Unicode 0x0410-0x042F -> CP1251 0xC0-0xDF)
    for (int i = 0; i < 32; i++) {
        int unicode = 0x0410 + i;
        unsigned char cp1251 = 0xC0 + i;

        if (FT_Load_Char(g_face, unicode, FT_LOAD_RENDER)) continue;

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        // ВАЖНО: используем GL_ALPHA вместо GL_RED
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows),
            glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top),
            (unsigned int)g_face->glyph->advance.x
        };
        g_characters.insert(std::pair<unsigned char, Character>(cp1251, character));
    }

    // Ё (0x0401 -> 0xA8)
    if (FT_Load_Char(g_face, 0x0401, FT_LOAD_RENDER) == 0) {
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows),
            glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top),
            (unsigned int)g_face->glyph->advance.x
        };
        g_characters.insert(std::pair<unsigned char, Character>(0xA8, character));
    }

    // Строчные русские буквы а-я (Unicode 0x0430-0x044F -> CP1251 0xE0-0xFF)
    for (int i = 0; i < 32; i++) {
        int unicode = 0x0430 + i;
        unsigned char cp1251 = 0xE0 + i;

        if (FT_Load_Char(g_face, unicode, FT_LOAD_RENDER)) continue;

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows),
            glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top),
            (unsigned int)g_face->glyph->advance.x
        };
        g_characters.insert(std::pair<unsigned char, Character>(cp1251, character));
    }

    // ё (0x0451 -> 0xB8)
    if (FT_Load_Char(g_face, 0x0451, FT_LOAD_RENDER) == 0) {
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows),
            glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top),
            (unsigned int)g_face->glyph->advance.x
        };
        g_characters.insert(std::pair<unsigned char, Character>(0xB8, character));
    }

    std::cout << "Загружено символов: " << g_characters.size() << std::endl;

    // Создаём VAO/VBO для текста
    glGenVertexArrays(1, &g_textVAO);
    glGenBuffers(1, &g_textVBO);

    glBindVertexArray(g_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), 0);
    glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    g_fontInitialized = true;
    fontInitialized = true;
    std::cout << "Шрифт успешно инициализирован" << std::endl;
}

float GameUI::getTextWidth(const std::string& text) const {
    if (!g_fontInitialized || g_characters.empty()) {
        return text.length() * getScaledWidth(10);
    }

    float width = 0;
    for (unsigned char c : text) {
        auto it = g_characters.find(c);
        if (it != g_characters.end()) {
            width += (it->second.Advance >> 6);
        }
        else {
            auto spaceIt = g_characters.find(' ');
            if (spaceIt != g_characters.end()) {
                width += (spaceIt->second.Advance >> 6);
            }
        }
    }
    return width;
}

void GameUI::drawText(float x, float y, const std::string& text, float r, float g, float b) {
    if (!g_fontInitialized || g_characters.empty() || text.empty()) return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glColor4f(r, g, b, 1.0f);

    glBindVertexArray(g_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_textVBO);

    float startX = x;
    for (unsigned char c : text) {
        auto it = g_characters.find(c);
        if (it == g_characters.end()) {
            it = g_characters.find(' ');
            if (it == g_characters.end()) continue;
        }

        Character& ch = it->second;

        float xpos = startX + ch.Bearing.x;
        float ypos = y - (ch.Size.y - ch.Bearing.y);
        float w = ch.Size.x;
        float h = ch.Size.y;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), 0);
        glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        startX += (ch.Advance >> 6);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

void GameUI::drawCenteredText(float y, const std::string& text, float r, float g, float b) {
    float textWidth = getTextWidth(text);
    float x = (windowWidth - textWidth) / 2;
    drawText(x, getScaledY(static_cast<int>(y)), text, r, g, b);
}

bool GameUI::ensureFontInitialized() {
    if (!g_fontInitialized) {
        initWindowsFont();
    }
    return g_fontInitialized;
}

void GameUI::drawMainMenu() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Фон
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(0.1f, 0.2f, 0.3f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    drawCenteredText(600, "3D ЗМЕЙКА", 1.0f, 1.0f, 1.0f);

    for (auto& button : mainMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawPauseMenu() {
    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);
    drawCenteredText(700, "ПАУЗА", 1.0f, 1.0f, 1.0f);

    for (auto& button : pauseMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawSettingsMenu(float gameSpeed, const std::string& playerName,
    float speedMultiplier, const std::string& speedDisplayText,
    bool isNameInputActive, bool doubleBufferingEnabled) {

    // Фон
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(0.1f, 0.3f, 0.2f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    drawCenteredText(550, "НАСТРОЙКИ", 1.0f, 1.0f, 1.0f);
    drawNameInputField(playerName, isNameInputActive);
    drawCenteredText(300, "СКОРОСТЬ", 1.0f, 1.0f, 1.0f);

    float multiplierY = getScaledY(250);
    drawCenteredText(250, speedDisplayText, 1.0f, 1.0f, 1.0f);

    float centerX = windowWidth / 2.0f;
    bool minusHover = isSpeedDecreaseButtonClicked(mouseX, mouseY);
    bool plusHover = isSpeedIncreaseButtonClicked(mouseX, mouseY);

    drawText(centerX - getScaledX(120), multiplierY, "-",
        minusHover ? 1.0f : 0.7f, 0.3f, 0.0f);
    drawText(centerX + getScaledX(100), multiplierY, "+",
        0.0f, plusHover ? 1.0f : 0.7f, 0.0f);
    drawCenteredText(20, "Нажмите на поле имени, чтобы изменить имя", 0.7f, 0.7f, 0.7f);

    for (auto& button : settingsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
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
    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);
    drawCenteredText(700, "ИГРА ОКОНЧЕНА", 1.0f, 0.0f, 0.0f);

    std::string scoreText = "СЧЁТ: " + std::to_string(score);
    drawCenteredText(600, scoreText, 1.0f, 1.0f, 1.0f);

    for (auto& button : gameOverButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawHighScoresMenu(const std::vector<HighScore>& highScores) {
    // Фон
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.05f, 0.25f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    drawCenteredText(550, "ТАБЛИЦА РЕКОРДОВ", 1.0f, 1.0f, 0.0f);

    const int rankX = getScaledX(200);
    const int playerX = getScaledX(300);
    const int scoreX = getScaledX(500);
    const int dateX = getScaledX(600);
    const int speedX = getScaledX(750);
    const int lengthX = getScaledX(850);
    const int timeX = getScaledX(950);

    int headerY = getScaledY(500);
    drawText(rankX, headerY, "МЕСТО", 0.8f, 0.8f, 1.0f);
    drawText(playerX, headerY, "ИГРОК", 0.8f, 0.8f, 1.0f);
    drawText(scoreX, headerY, "СЧЁТ", 0.8f, 0.8f, 1.0f);
    drawText(dateX, headerY, "ДАТА", 0.8f, 0.8f, 1.0f);
    drawText(speedX, headerY, "СКОРОСТЬ", 0.8f, 0.8f, 1.0f);
    drawText(lengthX, headerY, "ДЛИНА", 0.8f, 0.8f, 1.0f);
    drawText(timeX, headerY, "ВРЕМЯ", 0.8f, 0.8f, 1.0f);

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
        drawCenteredText(400, "Пока нет рекордов! Сыграйте в игру!", 1.0f, 0.5f, 0.5f);
    }

    for (auto& button : highScoresButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawControlsMenu() {
    // Фон
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(0.1f, 0.2f, 0.1f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    drawCenteredText(550, "УПРАВЛЕНИЕ", 1.0f, 1.0f, 1.0f);

    int yPos = getScaledY(450);
    drawCenteredText(yPos, "ВЛЕВО/ВПРАВО - Поворот змейки", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "Q/E - Вращение камеры", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "Колёсико мыши - Приближение/отдаление", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "ESC - Пауза/Меню", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "P - Вкл/Выкл паузы", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);
    drawCenteredText(yPos, "R - Перезапустить игру", 0.0f, 1.0f, 0.0f); yPos -= getScaledY(40);

    for (auto& button : controlsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glColor4f(color.r, color.g, color.b, alpha);

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void GameUI::drawButtonWithText(const MenuButton& button) {
    glm::vec3 bgColor = button.hovered ? glm::vec3(0.2f, 0.6f, 0.2f) : glm::vec3(0.1f, 0.3f, 0.1f);
    drawQuad(button.x, button.y, button.width, button.height, bgColor, 1.0f);

    if (g_fontInitialized && !g_characters.empty()) {
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

    // Рамка
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(fieldX, fieldY);
    glVertex2f(fieldX + fieldWidth, fieldY);
    glVertex2f(fieldX + fieldWidth, fieldY + fieldHeight);
    glVertex2f(fieldX, fieldY + fieldHeight);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    std::string displayName = playerName;
    if (isActive) {
        displayName += "|";
    }
    if (displayName.empty()) {
        displayName = "Нажмите, чтобы ввести имя...";
        drawText(fieldX + getScaledX(10), fieldY + getScaledY(12), displayName, 0.7f, 0.7f, 0.7f);
    }
    else {
        drawText(fieldX + getScaledX(10), fieldY + getScaledY(12), displayName, 1.0f, 1.0f, 1.0f);
    }

    drawCenteredText(450, "ИМЯ ИГРОКА", 0.8f, 0.8f, 1.0f);

    std::string lengthInfo = "(макс. 15 символов)";
    float infoX = fieldX + fieldWidth - getTextWidth(lengthInfo);
    float infoY = fieldY - getScaledY(25);
    drawText(infoX, infoY, lengthInfo, 0.6f, 0.6f, 0.6f);

    if (!playerName.empty()) {
        std::string currentLength = std::to_string(playerName.length()) + "/15";
        float lengthX = fieldX + getScaledX(5);
        float lengthY = fieldY - getScaledY(25);
        drawText(lengthX, lengthY, currentLength, 0.6f, 0.6f, 0.6f);
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