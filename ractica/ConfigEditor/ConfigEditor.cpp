#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>                    // <-- ДОБАВЬТЕ ЭТОТ INCLUDE
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ConfigManager.h"
#include "../Shared/ConfigTypes.h"

// Структура для символа
struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

// Глобальные переменные
GLFWwindow* window;
int windowWidth = 900;
int windowHeight = 700;
GameConfig currentConfig;
std::vector<std::string> availableModels;

// FreeType
FT_Library ft;
FT_Face face;
std::map<char, Character> Characters;    // <-- ТЕПЕРЬ РАБОТАЕТ
GLuint textVAO, textVBO;

// Состояния редактора
enum EditorMode {
    MODE_MAIN,
    MODE_SNAKE_EDITOR
};
EditorMode currentMode = MODE_MAIN;

// Для редактора змейки
int selectedPart = 0; // 0-голова, 1-тело, 2-хвост

// Для UI
bool mousePressed = false;
double mouseX, mouseY;

// Прототипы
void renderMainMenu();
void renderSnakeEditor();
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void loadAvailableModels();
void initFreeType();
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
bool drawButton(int x, int y, int w, int h, const std::string& text);
void drawInfoBox(int x, int y, int w, int h, const std::string& label, const std::string& value);

// Инициализация FreeType
void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not init FreeType" << std::endl;
        return;
    }

    // Загружаем шрифт (путь к вашему шрифту)
    if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face)) {
        std::cerr << "Could not load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 24);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Загружаем символы ASCII
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph" << std::endl;
            continue;
        }

        unsigned int texture;
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

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    // Создаем VAO/VBO для текста
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Рендеринг текста
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(0); // Используем фиксированный pipeline для простоты

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (char c : text) {
        Character ch = Characters[c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

// Инициализация OpenGL
bool initOpenGL() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    window = glfwCreateWindow(windowWidth, windowHeight, "Snake Game Config Editor", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Инициализируем FreeType после OpenGL
    initFreeType();

    return true;
}

// Загрузка доступных моделей
void loadAvailableModels() {
    availableModels.clear();
    availableModels.push_back("snake_head.obj");
    availableModels.push_back("snake_head_alt.obj");
    availableModels.push_back("snake_body.obj");
    availableModels.push_back("snake_body_alt.obj");
    availableModels.push_back("snake_tail.obj");
    availableModels.push_back("snake_tail_alt.obj");
}

// Рисование кнопки
bool drawButton(int x, int y, int w, int h, const std::string& text) {
    bool hover = (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);

    if (hover) glColor3f(0.3f, 0.5f, 0.8f);
    else glColor3f(0.2f, 0.3f, 0.5f);

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    // Рендерим текст с FreeType
    renderText(text, x + 10, y + h / 2 - 10, 0.5f, glm::vec3(1.0f));

    return hover && mousePressed;
}

// Рисование информационного бокса
void drawInfoBox(int x, int y, int w, int h, const std::string& label, const std::string& value) {
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    renderText(label, x + 5, y + 20, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
    renderText(value, x + 5, y + 45, 0.4f, glm::vec3(1.0f));
}

// Главное меню
void renderMainMenu() {
    renderText("=== SNAKE GAME CONFIG EDITOR ===", 50, 50, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f));

    if (drawButton(50, 120, 250, 50, "1. Snake Editor")) {
        currentMode = MODE_SNAKE_EDITOR;
    }

    if (drawButton(50, 190, 250, 50, "2. Obstacle Editor")) {
        renderText("Coming soon...", 50, 260, 0.5f, glm::vec3(1.0f, 0.5f, 0.0f));
    }

    if (drawButton(50, 260, 250, 50, "3. Environment Editor")) {
        renderText("Coming soon...", 50, 330, 0.5f, glm::vec3(1.0f, 0.5f, 0.0f));
    }

    if (drawButton(50, 330, 250, 50, "4. Save and Exit")) {
        ConfigManager::saveGameConfig("../config/game.cfg", currentConfig);
        glfwSetWindowShouldClose(window, true);
    }

    drawInfoBox(400, 120, 300, 80, "Current Config", "Loaded");

    std::string gridInfo = "Grid: " + std::to_string(currentConfig.gridWidth) + "x" +
        std::to_string(currentConfig.gridDepth);
    renderText(gridInfo, 405, 230, 0.4f, glm::vec3(0.8f, 0.8f, 1.0f));
}

// Редактор змейки
void renderSnakeEditor() {
    renderText("=== SNAKE EDITOR ===", 50, 50, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f));

    // Кнопки выбора части змейки
    if (drawButton(50, 100, 120, 40, "Head")) selectedPart = 0;
    if (drawButton(180, 100, 120, 40, "Body")) selectedPart = 1;
    if (drawButton(310, 100, 120, 40, "Tail")) selectedPart = 2;

    // Отображение текущих настроек
    std::string partName;
    std::string currentModel;
    glm::vec3 currentColor;
    float currentScale;

    switch (selectedPart) {
    case 0:
        partName = "HEAD";
        currentModel = currentConfig.snakeHeadModel;
        currentColor = currentConfig.snakeHeadColor;
        currentScale = currentConfig.snakeHeadScale;
        break;
    case 1:
        partName = "BODY";
        currentModel = currentConfig.snakeBodyModel;
        currentColor = currentConfig.snakeBodyColor;
        currentScale = currentConfig.snakeBodyScale;
        break;
    case 2:
        partName = "TAIL";
        currentModel = currentConfig.snakeTailModel;
        currentColor = currentConfig.snakeTailColor;
        currentScale = currentConfig.snakeTailScale;
        break;
    }

    drawInfoBox(50, 170, 300, 70, partName + " Model", currentModel);

    // Список доступных моделей
    int yPos = 260;
    renderText("Available Models:", 50, yPos, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
    yPos += 30;

    for (size_t i = 0; i < availableModels.size() && i < 5; i++) {
        std::string buttonText = availableModels[i];
        if (buttonText == currentModel) {
            // Текущая модель подсвечена
            if (drawButton(50, yPos, 250, 30, "✓ " + buttonText)) {
                if (selectedPart == 0) currentConfig.snakeHeadModel = buttonText;
                else if (selectedPart == 1) currentConfig.snakeBodyModel = buttonText;
                else currentConfig.snakeTailModel = buttonText;
            }
        }
        else {
            if (drawButton(50, yPos, 250, 30, buttonText)) {
                if (selectedPart == 0) currentConfig.snakeHeadModel = buttonText;
                else if (selectedPart == 1) currentConfig.snakeBodyModel = buttonText;
                else currentConfig.snakeTailModel = buttonText;
            }
        }
        yPos += 40;
    }

    // Отображение цвета
    renderText("Color (RGB):", 350, 170, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));

    std::string rText = "R: " + std::to_string(currentColor.r).substr(0, 4);
    renderText(rText, 350, 200, 0.5f, glm::vec3(currentColor.r, 0, 0));

    std::string gText = "G: " + std::to_string(currentColor.g).substr(0, 4);
    renderText(gText, 350, 230, 0.5f, glm::vec3(0, currentColor.g, 0));

    std::string bText = "B: " + std::to_string(currentColor.b).substr(0, 4);
    renderText(bText, 350, 260, 0.5f, glm::vec3(0, 0, currentColor.b));

    // Отображение размера
    std::string scaleText = "Scale: " + std::to_string(currentScale).substr(0, 4);
    renderText(scaleText, 350, 300, 0.5f, glm::vec3(1.0f));

    // Кнопки навигации
    if (drawButton(50, 500, 120, 40, "Back")) {
        currentMode = MODE_MAIN;
    }

    if (drawButton(200, 500, 120, 40, "Save")) {
        ConfigManager::saveGameConfig("../config/game.cfg", currentConfig);
        renderText("Saved!", 350, 515, 0.5f, glm::vec3(0, 1, 0));
    }
}

// Колбэки
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mousePressed = (action == GLFW_PRESS);
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

// Главная функция
int main() {
    if (!initOpenGL()) {
        return -1;
    }

    ConfigManager::loadGameConfig("../config/game.cfg", currentConfig);
    loadAvailableModels();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        // 2D проекция для UI
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Рендеринг
        switch (currentMode) {
        case MODE_MAIN: renderMainMenu(); break;
        case MODE_SNAKE_EDITOR: renderSnakeEditor(); break;
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}