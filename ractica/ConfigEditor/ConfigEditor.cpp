#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
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
int windowWidth = 1024;
int windowHeight = 768;
GameConfig currentConfig;
std::vector<std::string> availableModels;

// FreeType
FT_Library ft;
FT_Face face;
std::map<char, Character> Characters;
GLuint textVAO, textVBO;
GLuint textShaderProgram;

// Состояния редактора
enum EditorMode {
    MODE_MAIN,
    MODE_SNAKE_EDITOR
};
EditorMode currentMode = MODE_MAIN;

// Для редактора змейки
int selectedPart = 0;
int selectedModelIndex = 0;

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
float getTextWidth(const std::string& text, float scale);
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool centerX = false, bool centerY = false);
bool drawButton(int x, int y, int w, int h, const std::string& text);
void drawInfoBox(int x, int y, int w, int h, const std::string& label, const std::string& value);

// Шейдеры для текста
const char* textVertexShader = R"(
#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 TexCoords;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* textFragmentShader = R"(
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

// Компиляция шейдера
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }
    return shader;
}

// Инициализация шейдера для текста
void initTextShader() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, textVertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, textFragmentShader);

    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vertexShader);
    glAttachShader(textShaderProgram, fragmentShader);
    glLinkProgram(textShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// Инициализация FreeType
void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not init FreeType" << std::endl;
        return;
    }

    if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face)) {
        std::cerr << "Could not load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initTextShader();
}

// Получение ширины текста
float getTextWidth(const std::string& text, float scale) {
    float width = 0;
    for (char c : text) {
        Character ch = Characters[c];
        width += (ch.Advance >> 6) * scale;
    }
    return width;
}

// Рендеринг текста с поддержкой центрирования
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool centerX, bool centerY) {
    if (text.empty()) return;

    // Корректировка координат для центрирования
    float textWidth = getTextWidth(text, scale);
    float textHeight = 48 * scale; // Примерная высота

    if (centerX) {
        x -= textWidth / 2;
    }
    if (centerY) {
        y -= textHeight / 2;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(textShaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (char c : text) {
        Character ch = Characters[c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y + (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos - h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos - h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos - h,   1.0f, 0.0f }
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
    glUseProgram(0);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    // Текст по центру кнопки
    renderText(text, x + w / 2, y + h / 2, 0.3f, glm::vec3(1.0f), true, true);

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

    renderText(label, x + 5, y + 20, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));
    renderText(value, x + 5, y + 45, 0.25f, glm::vec3(1.0f));
}

// Главное меню
void renderMainMenu() {
    // Заголовок по центру
    renderText("SNAKE GAME CONFIG EDITOR", windowWidth / 2, 80, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    // Кнопки по центру
    int buttonWidth = 250;
    int buttonHeight = 50;
    int startY = 200;
    int centerX = windowWidth / 2 - buttonWidth / 2;

    if (drawButton(centerX, startY, buttonWidth, buttonHeight, "1. Snake Editor")) {
        currentMode = MODE_SNAKE_EDITOR;
    }

    if (drawButton(centerX, startY + 70, buttonWidth, buttonHeight, "2. Obstacle Editor")) {
        // Coming soon
    }

    if (drawButton(centerX, startY + 140, buttonWidth, buttonHeight, "3. Environment Editor")) {
        // Coming soon
    }

    if (drawButton(centerX, startY + 210, buttonWidth, buttonHeight, "4. Save and Exit")) {
        ConfigManager::saveGameConfig("../config/game.cfg", currentConfig);
        glfwSetWindowShouldClose(window, true);
    }

    // Информация внизу
    std::string gridInfo = "Grid: " + std::to_string(currentConfig.gridWidth) + "x" +
        std::to_string(currentConfig.gridDepth);
    renderText(gridInfo, windowWidth / 2, 600, 0.3f, glm::vec3(0.8f, 0.8f, 1.0f), true, false);
}

// Редактор змейки
void renderSnakeEditor() {
    renderText("SNAKE EDITOR", windowWidth / 2, 50, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    // Кнопки выбора части по центру
    int buttonWidth = 100;
    int startX = windowWidth / 2 - 160;
    int buttonY = 120;

    if (drawButton(startX, buttonY, buttonWidth, 40, "Head")) selectedPart = 0;
    if (drawButton(startX + 110, buttonY, buttonWidth, 40, "Body")) selectedPart = 1;
    if (drawButton(startX + 220, buttonY, buttonWidth, 40, "Tail")) selectedPart = 2;

    // Текущая модель
    std::string currentModel;
    switch (selectedPart) {
    case 0: currentModel = currentConfig.snakeHeadModel; break;
    case 1: currentModel = currentConfig.snakeBodyModel; break;
    case 2: currentModel = currentConfig.snakeTailModel; break;
    }

    drawInfoBox(windowWidth / 2 - 125, 180, 250, 60, "Current Model", currentModel);

    // Список моделей по центру
    renderText("Available Models:", windowWidth / 2, 260, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int modelY = 300;
    int modelWidth = 220;
    int modelX = windowWidth / 2 - modelWidth / 2;

    for (size_t i = 0; i < availableModels.size() && i < 5; i++) {
        if (drawButton(modelX, modelY + i * 35, modelWidth, 30, availableModels[i])) {
            if (selectedPart == 0) currentConfig.snakeHeadModel = availableModels[i];
            else if (selectedPart == 1) currentConfig.snakeBodyModel = availableModels[i];
            else currentConfig.snakeTailModel = availableModels[i];
        }
    }

    // Кнопки навигации внизу
    if (drawButton(windowWidth / 2 - 110, 550, 100, 40, "Back")) {
        currentMode = MODE_MAIN;
    }

    if (drawButton(windowWidth / 2 + 10, 550, 100, 40, "Save")) {
        ConfigManager::saveGameConfig("../config/game.cfg", currentConfig);
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

        // 2D проекция
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

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