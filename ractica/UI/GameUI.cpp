#include "../pch.h"
#include "GameUI.h"
#include "../Graphics/ShaderManager.h"
#include "../Objects/GameObjects.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern GLuint uiVAO, uiVBO;

GameUI::GameUI()
    : windowWidth(1200),
    windowHeight(800),
    mouseX(0),
    mouseY(0),
    mousePressed(false),
    uiInitialized(false),
    textVAO(0), textVBO(0), textShader(0),
    ft(nullptr), face(nullptr),
    fontInitialized(false) {
}

GameUI::~GameUI() {
    cleanup();
}

void GameUI::cleanup() {
    cleanupFreeType();
}

void GameUI::cleanupFreeType() {
    // Очистка FreeType
    if (face) FT_Done_Face(face);
    if (ft) FT_Done_FreeType(ft);

    // Очистка текстур
    for (auto& character : characters) {
        glDeleteTextures(1, &character.second.textureID);
    }
    characters.clear();

    // Очистка буферов
    if (textVAO) glDeleteVertexArrays(1, &textVAO);
    if (textVBO) glDeleteBuffers(1, &textVBO);
    if (textShader) glDeleteProgram(textShader);

    textVAO = 0;
    textVBO = 0;
    textShader = 0;
    ft = nullptr;
    face = nullptr;
    fontInitialized = false;
}

void GameUI::setWindowSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    // Пересоздаем кнопки с новыми размерами
    if (uiInitialized) {
        int centerX = windowWidth / 2 - 100;
        int buttonYStart = windowHeight / 2 + 100; // Начинаем от центра экрана

        mainMenuButtons.clear();
        mainMenuButtons.push_back(MenuButton("PLAY", centerX, buttonYStart, 200, 50));
        mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - 80, 200, 50));
        mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, buttonYStart - 160, 200, 50));
        mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, buttonYStart - 240, 200, 50));
        mainMenuButtons.push_back(MenuButton("EXIT", centerX, buttonYStart - 320, 200, 50));

        pauseMenuButtons.clear();
        pauseMenuButtons.push_back(MenuButton("RESUME", centerX, buttonYStart, 200, 50));
        pauseMenuButtons.push_back(MenuButton("SETTINGS", centerX, buttonYStart - 80, 200, 50));
        pauseMenuButtons.push_back(MenuButton("MAIN MENU", centerX, buttonYStart - 160, 200, 50));

        gameOverButtons.clear();
        gameOverButtons.push_back(MenuButton("RESTART", centerX, buttonYStart, 150, 50));
        gameOverButtons.push_back(MenuButton("MAIN MENU", centerX, buttonYStart - 80, 150, 50));

        settingsButtons.clear();
        settingsButtons.push_back(MenuButton("BACK", centerX, 150, 200, 50));
    }
}

void GameUI::setMousePosition(double x, double y) {
    mouseX = x;
    mouseY = windowHeight - y; // Инвертируем Y для координат OpenGL
}

void GameUI::initUI() {
    std::cout << "Initializing UI..." << std::endl;

    // Создаем VAO и VBO для 2D прямоугольников
    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
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

    // Создаем кнопки
    int centerX = windowWidth / 2 - 100;
    mainMenuButtons.clear();
    mainMenuButtons.push_back(MenuButton("PLAY", centerX, 450, 200, 50));
    mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, 370, 200, 50));
    mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, 290, 200, 50));
    mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, 210, 200, 50));
    mainMenuButtons.push_back(MenuButton("EXIT", centerX, 130, 200, 50));

    pauseMenuButtons.clear();
    pauseMenuButtons.push_back(MenuButton("RESUME", centerX, 450, 200, 50));
    pauseMenuButtons.push_back(MenuButton("SETTINGS", centerX, 370, 200, 50));
    pauseMenuButtons.push_back(MenuButton("MAIN MENU", centerX, 290, 200, 50));

    gameOverButtons.clear();
    gameOverButtons.push_back(MenuButton("RESTART", centerX, 450, 150, 50));
    gameOverButtons.push_back(MenuButton("MAIN MENU", centerX, 370, 150, 50));

    settingsButtons.clear();
    settingsButtons.push_back(MenuButton("BACK", centerX, 150, 200, 50));

    initWindowsFont();

    uiInitialized = true;
    std::cout << "UI initialized successfully" << std::endl;
}

void GameUI::initWindowsFont() {
    std::cout << "Initializing FreeType font..." << std::endl;

    // Инициализация FreeType
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR: Could not init FreeType Library" << std::endl;
        return;
    }

    // Загрузка шрифта Arial
    if (!loadFreeTypeFont("C:/Windows/Fonts/arial.ttf", 24)) {
        std::cerr << "Failed to load font with FreeType" << std::endl;
        return;
    }

    setupTextBuffers();
    compileTextShaders();

    fontInitialized = true;
    std::cout << "FreeType font initialized successfully" << std::endl;
}

bool GameUI::loadFreeTypeFont(const std::string& fontPath, unsigned int fontSize) {
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Загрузка первых 128 символов ASCII
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR: Failed to load Glyph: " << c << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

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

    // Компиляция вершинного шейдера
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Проверка ошибок вершинного шейдера
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Компиляция фрагментного шейдера
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Проверка ошибок фрагментного шейдера
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Создание шейдерной программы
    textShader = glCreateProgram();
    glAttachShader(textShader, vertexShader);
    glAttachShader(textShader, fragmentShader);
    glLinkProgram(textShader);

    // Проверка ошибок линковки
    glGetProgramiv(textShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(textShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Удаление шейдеров
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

float GameUI::getTextWidth(const std::string& text) {
    if (!fontInitialized || characters.empty()) {
        return text.length() * 10.0f; // Fallback width
    }

    float width = 0.0f;
    for (const char& c : text) {
        auto it = characters.find(c);
        if (it != characters.end()) {
            TextCharacter ch = it->second;
            width += (ch.advance >> 6); // bitshift by 6 to get value in pixels
        }
    }
    return width;
}

void GameUI::drawText(float x, float y, const std::string& text, float r, float g, float b) {
    if (!fontInitialized || characters.empty()) {
        return;
    }

    // Активируем соответствующий render state
    glUseProgram(textShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(textShader, "textColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // Включаем blending для текста
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Итерация по всем символам
    float startX = x;
    for (const char& c : text) {
        auto it = characters.find(c);
        if (it == characters.end()) continue;

        TextCharacter ch = it->second;

        float xpos = startX + ch.bearing.x;
        float ypos = y - (ch.size.y - ch.bearing.y);

        float w = ch.size.x;
        float h = ch.size.y;

        // Обновляем VBO для каждого символа
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // Рендерим текстуру глифа на quadrilateral
        glBindTexture(GL_TEXTURE_2D, ch.textureID);

        // Обновляем содержимое VBO памяти
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Рендерим quadrilateral
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Теперь смещаем позицию для следующего глифа
        startX += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

void GameUI::drawCenteredText(float y, const std::string& text, float r, float g, float b) {
    float textWidth = getTextWidth(text);
    float x = (windowWidth - textWidth) / 2;
    drawText(x, y, text, r, g, b);
}

bool GameUI::ensureFontInitialized() {
    if (!fontInitialized) {
        initWindowsFont();
    }
    return fontInitialized;
}

// Остальные методы остаются без изменений...
void GameUI::drawMainMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    drawCenteredText(550, "3D SNAKE GAME", 1.0f, 1.0f, 1.0f);

    for (auto& button : mainMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawPauseMenu() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Полупрозрачный фон
    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);

    // Заголовок
    drawCenteredText(500, "PAUSED", 1.0f, 1.0f, 1.0f);

    for (auto& button : pauseMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }

    glDisable(GL_BLEND);
}

void GameUI::drawSettingsMenu(float gameSpeed, const std::string& playerName) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    drawCenteredText(550, "SETTINGS", 1.0f, 1.0f, 1.0f);

    // Настройки
    std::string speedText = "Game Speed: " + std::to_string(gameSpeed).substr(0, 4);
    drawCenteredText(400, speedText, 1.0f, 1.0f, 0.0f);
    drawCenteredText(350, "Press + to increase, - to decrease", 0.8f, 0.8f, 0.8f);

    std::string nameText = "Player Name: " + playerName;
    drawCenteredText(250, nameText, 0.0f, 1.0f, 1.0f);

    for (auto& button : settingsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButtonWithText(button);
    }
}

void GameUI::drawGameOver(int score) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);

    drawCenteredText(450, "GAME OVER", 1.0f, 0.0f, 0.0f);

    std::string scoreText = "Final Score: " + std::to_string(score);
    drawCenteredText(350, scoreText, 1.0f, 1.0f, 1.0f);

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

    int yPos = 450;
    for (size_t i = 0; i < highScores.size() && i < 10; i++) {
        std::string scoreText = std::to_string(i + 1) + ". " + highScores[i].playerName +
            " - " + std::to_string(highScores[i].score) +
            " (" + highScores[i].date + ")";
        drawCenteredText(yPos, scoreText, 1.0f, 1.0f, 0.0f);
        yPos -= 40;
    }

    drawCenteredText(100, "Press B to go back", 1.0f, 1.0f, 1.0f);
}

void GameUI::drawControlsMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

    drawCenteredText(550, "CONTROLS", 1.0f, 1.0f, 1.0f);

    int yPos = 450;
    drawCenteredText(yPos, "LEFT/RIGHT - Turn snake", 0.0f, 1.0f, 0.0f); yPos -= 40;
    drawCenteredText(yPos, "Q/E - Rotate camera", 0.0f, 1.0f, 0.0f); yPos -= 40;
    drawCenteredText(yPos, "Mouse Wheel - Zoom", 0.0f, 1.0f, 0.0f); yPos -= 40;
    drawCenteredText(yPos, "ESC - Pause/Menu", 0.0f, 1.0f, 0.0f); yPos -= 40;
    drawCenteredText(yPos, "P - Toggle pause", 0.0f, 1.0f, 0.0f); yPos -= 40;
    drawCenteredText(yPos, "R - Restart game", 0.0f, 1.0f, 0.0f); yPos -= 40;
    drawCenteredText(yPos, "D - Debug info", 0.0f, 1.0f, 0.0f); yPos -= 40;

    drawCenteredText(100, "Press B to go back", 1.0f, 1.0f, 1.0f);
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

void GameUI::drawButton(const MenuButton& button) {
    // Яркие контрастные цвета для отладки
    glm::vec3 bgColor;
    if (button.hovered) {
        bgColor = glm::vec3(1.0f, 0.0f, 0.0f); // Красный при наведении
    }
    else {
        bgColor = glm::vec3(0.0f, 0.0f, 1.0f); // Синий обычный
    }

    // Рисуем ОСНОВНУЮ кнопку - большой прямоугольник
    drawQuad(button.x, button.y, button.width, button.height, bgColor, 1.0f);

    // Белая рамка для видимости границ
    drawQuad(button.x - 2, button.y - 2, button.width + 4, 2, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // верх
    drawQuad(button.x - 2, button.y + button.height, button.width + 4, 2, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // низ
    drawQuad(button.x - 2, button.y, 2, button.height, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // лево
    drawQuad(button.x + button.width, button.y, 2, button.height, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // право
}

void GameUI::drawButtonWithText(const MenuButton& button) {
    // Рисуем фон кнопки
    glm::vec3 bgColor = button.hovered ? glm::vec3(0.2f, 0.6f, 0.2f) : glm::vec3(0.1f, 0.3f, 0.1f);
    drawQuad(button.x, button.y, button.width, button.height, bgColor, 1.0f);

    // Вычисляем позицию для текста (по центру кнопки)
    float textWidth = getTextWidth(button.text);
    float textX = button.x + (button.width - textWidth) / 2;
    float textY = button.y + (button.height / 2) + 8; // Центрируем по вертикали

    // Рисуем текст на кнопке белым цветом для контраста
    drawText(textX, textY, button.text, 1.0f, 1.0f, 1.0f);

    // Отладочная информация
    static int debugCount = 0;
    if (debugCount < 10) {
        std::cout << "Button: " << button.text << " at (" << textX << ", " << textY << ")" << std::endl;
        debugCount++;
    }
}

void GameUI::handleMouseClick(GameObjects& objects) {
    mousePressed = true;
    std::cout << "Mouse clicked at: (" << mouseX << ", " << mouseY << ")" << std::endl;
    std::cout << "Game state: " << objects.getGameState() << std::endl;

    switch (objects.getGameState()) {
    case MAIN_MENU:
        for (size_t i = 0; i < mainMenuButtons.size(); i++) {
            if (mainMenuButtons[i].contains(mouseX, mouseY)) {
                std::cout << "Main menu button clicked: " << i << " - " << mainMenuButtons[i].text << std::endl;
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
                    objects.setGameState(CONTROLS);
                    break;
                case 4: /* Exit handled in main */ break;
                }
                return;
            }
        }
        break;

    case PAUSED:
        for (size_t i = 0; i < pauseMenuButtons.size(); i++) {
            if (pauseMenuButtons[i].contains(mouseX, mouseY)) {
                std::cout << "Pause menu button clicked: " << i << " - " << pauseMenuButtons[i].text << std::endl;
                switch (i) {
                case 0: // RESUME
                    objects.setGameState(PLAYING);
                    break;
                case 1: // SETTINGS
                    objects.setPreviousState(PAUSED);
                    objects.setGameState(SETTINGS);
                    break;
                case 2: // MAIN MENU
                    objects.setGameState(MAIN_MENU);
                    break;
                }
                return;
            }
        }
        break;

    case SETTINGS:
        for (size_t i = 0; i < settingsButtons.size(); i++) {
            if (settingsButtons[i].contains(mouseX, mouseY)) {
                std::cout << "Settings menu button clicked: " << i << " - " << settingsButtons[i].text << std::endl;
                switch (i) {
                case 0: // BACK
                    objects.setGameState(objects.getPreviousState());
                    std::cout << "Returning to previous state: " << objects.getPreviousState() << std::endl;
                    break;
                }
                return;
            }
        }
        break;

    case GAME_OVER:
        std::cout << "Checking game over buttons..." << std::endl;
        for (size_t i = 0; i < gameOverButtons.size(); i++) {
            std::cout << "Game over button " << i << ": " << gameOverButtons[i].text
                << " at (" << gameOverButtons[i].x << ", " << gameOverButtons[i].y << ")"
                << " size (" << gameOverButtons[i].width << "x" << gameOverButtons[i].height << ")" << std::endl;
            if (gameOverButtons[i].contains(mouseX, mouseY)) {
                std::cout << "Game over button clicked: " << i << " - " << gameOverButtons[i].text << std::endl;
                switch (i) {
                case 0: // RESTART
                    objects.initGame();
                    objects.setGameState(PLAYING);
                    std::cout << "Restarting game..." << std::endl;
                    break;
                case 1: // MAIN MENU
                    objects.setGameState(MAIN_MENU);
                    std::cout << "Returning to main menu..." << std::endl;
                    break;
                }
                return;
            }
        }
        std::cout << "No game over button clicked" << std::endl;
        break;
    }
}

void GameUI::drawTestQuad(float x, float y, float width, float height, const glm::vec3& color) {
    std::cout << "TEST QUAD: (" << x << "," << y << ") size (" << width << "x" << height << ")" << std::endl;

    g_shaderManager.useUIShader();

    // Проекция с Y вниз (как в UI)
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f);
    g_shaderManager.setUIProjection(projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));
    g_shaderManager.setUIModel(model);

    g_shaderManager.setUIColor(color);

    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void GameUI::drawTextTest() {
    std::cout << "=== TEXT RENDERING TEST ===" << std::endl;

    // Тест в разных местах экрана
    drawText(100, 100, "TOP LEFT TEST", 1.0f, 0.0f, 0.0f);
    drawText(windowWidth - 300, 100, "TOP RIGHT TEST", 0.0f, 1.0f, 0.0f);
    drawText(100, windowHeight - 50, "BOTTOM LEFT TEST", 0.0f, 0.0f, 1.0f);
    drawText(windowWidth / 2 - 100, windowHeight / 2, "CENTER TEST", 1.0f, 1.0f, 0.0f);

    std::cout << "=== END TEXT TEST ===" << std::endl;
}

void GameUI::debugTextRendering() {
    // Метод для отладки
    drawTextTest();
}