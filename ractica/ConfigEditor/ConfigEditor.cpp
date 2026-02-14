#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <commdlg.h>
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

// Простая структура для модели
struct ModelData {
    std::vector<float> vertices;
    std::vector<float> normals;
    bool loaded;

    ModelData() : loaded(false) {}
};

// Глобальные переменные
GLFWwindow* window;
int windowWidth = 1200;
int windowHeight = 800;
GameConfig currentConfig;

// Пути
std::string basePath;
std::string modelsPath;
std::string configPath;

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
int selectedPart = 0; // 0-голова, 1-тело, 2-хвост
float previewRotation = 0.0f;
ModelData currentModelData;

// Для UI
bool mousePressed = false;
bool mousePressedLast = false;
double mouseX, mouseY;

// Прототипы
void renderMainMenu();
void renderSnakeEditor();
void render3DPreview();
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void openFileDialog();
void initFreeType();
float getTextWidth(const std::string& text, float scale);
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool centerX = false, bool centerY = false);
bool drawButton(int x, int y, int w, int h, const std::string& text, bool enabled = true);
void drawInfoBox(int x, int y, int w, int h, const std::string& label, const std::string& value);
void drawCube();
void reset2DProjection();
bool loadObjModel(const std::string& filename, ModelData& model);
void saveConfig();
void initPaths();

// Инициализация путей
void initPaths() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    basePath = std::string(currentDir);

    std::cout << "Current directory: " << basePath << std::endl;

    // Ищем корневую папку проекта (поднимаемся пока не найдем папку с models)
    std::string path = basePath;
    bool found = false;

    while (true) {
        std::string testModelsPath = path + "\\models";
        std::string testConfigPath = path + "\\config";
        if (std::filesystem::exists(testModelsPath)) {
            basePath = path;
            found = true;
            std::cout << "Found project root: " << basePath << std::endl;
            break;
        }

        // Поднимаемся на уровень выше
        size_t pos = path.find_last_of("\\");
        if (pos == std::string::npos) break;
        path = path.substr(0, pos);
    }

    if (!found) {
        // Если не нашли, используем текущую директорию
        basePath = currentDir;
        std::cout << "Using current directory as root" << std::endl;
    }

    modelsPath = basePath + "\\models\\";
    configPath = basePath + "\\config\\game.cfg";

    std::cout << "Base path: " << basePath << std::endl;
    std::cout << "Models path: " << modelsPath << std::endl;
    std::cout << "Config path: " << configPath << std::endl;

    // Создаем папки если нет
    if (!std::filesystem::exists(modelsPath)) {
        std::filesystem::create_directories(modelsPath);
    }

    std::string configDir = basePath + "\\config";
    if (!std::filesystem::exists(configDir)) {
        std::filesystem::create_directories(configDir);
    }
}

// Сохранение конфига
void saveConfig() {
    if (ConfigManager::saveGameConfig(configPath, currentConfig)) {
        std::cout << "Config saved to: " << configPath << std::endl;
    }
    else {
        std::cout << "Failed to save config to: " << configPath << std::endl;
    }
}

// Загрузка OBJ файла
bool loadObjModel(const std::string& filename, ModelData& model) {
    std::string subFolder;
    switch (selectedPart) {
    case 0: subFolder = "snake_head"; break;
    case 1: subFolder = "snake_body"; break;
    case 2: subFolder = "snake_tail"; break;
    }

    std::string fullPath = modelsPath + subFolder + "\\" + filename;

    std::cout << "Loading model: " << fullPath << std::endl;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        std::cout << "Failed to open model file" << std::endl;
        return false;
    }

    model.vertices.clear();
    model.normals.clear();

    std::vector<glm::vec3> tempPositions;
    std::vector<glm::vec3> tempNormals;

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            glm::vec3 pos;
            sscanf_s(line.c_str(), "v %f %f %f", &pos.x, &pos.y, &pos.z);
            tempPositions.push_back(pos);
        }
        else if (line.substr(0, 3) == "vn ") {
            glm::vec3 normal;
            sscanf_s(line.c_str(), "vn %f %f %f", &normal.x, &normal.y, &normal.z);
            tempNormals.push_back(normal);
        }
        else if (line.substr(0, 2) == "f ") {
            int v1, v2, v3;
            int n1, n2, n3;
            if (sscanf_s(line.c_str(), "f %d//%d %d//%d %d//%d",
                &v1, &n1, &v2, &n2, &v3, &n3) == 6) {

                if (v1 <= tempPositions.size() && v2 <= tempPositions.size() && v3 <= tempPositions.size() &&
                    n1 <= tempNormals.size() && n2 <= tempNormals.size() && n3 <= tempNormals.size()) {

                    glm::vec3 p1 = tempPositions[v1 - 1];
                    glm::vec3 p2 = tempPositions[v2 - 1];
                    glm::vec3 p3 = tempPositions[v3 - 1];

                    glm::vec3 norm1 = tempNormals[n1 - 1];
                    glm::vec3 norm2 = tempNormals[n2 - 1];
                    glm::vec3 norm3 = tempNormals[n3 - 1];

                    model.vertices.push_back(p1.x); model.vertices.push_back(p1.y); model.vertices.push_back(p1.z);
                    model.vertices.push_back(p2.x); model.vertices.push_back(p2.y); model.vertices.push_back(p2.z);
                    model.vertices.push_back(p3.x); model.vertices.push_back(p3.y); model.vertices.push_back(p3.z);

                    model.normals.push_back(norm1.x); model.normals.push_back(norm1.y); model.normals.push_back(norm1.z);
                    model.normals.push_back(norm2.x); model.normals.push_back(norm2.y); model.normals.push_back(norm2.z);
                    model.normals.push_back(norm3.x); model.normals.push_back(norm3.y); model.normals.push_back(norm3.z);
                }
            }
        }
    }

    file.close();
    model.loaded = (model.vertices.size() > 0);
    std::cout << "Loaded " << model.vertices.size() / 3 << " vertices" << std::endl;
    return model.loaded;
}
// Сброс 2D проекции
void reset2DProjection() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Функция открытия диалога выбора файла
void openFileDialog() {
    std::string subFolder;
    switch (selectedPart) {
    case 0: subFolder = "snake_head"; break;
    case 1: subFolder = "snake_body"; break;
    case 2: subFolder = "snake_tail"; break;
    }

    std::string folderPath = modelsPath + subFolder + "\\";

    // Создаем папку если нет
    if (!std::filesystem::exists(folderPath)) {
        std::filesystem::create_directories(folderPath);
    }

    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "OBJ Files\0*.obj\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = folderPath.c_str();
    ofn.lpstrTitle = "Выберите модель";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1);

        if (!filename.empty()) {
            // Сохраняем имя файла
            if (selectedPart == 0) currentConfig.snakeHeadModel = filename;
            else if (selectedPart == 1) currentConfig.snakeBodyModel = filename;
            else currentConfig.snakeTailModel = filename;

            std::cout << "Selected model: " << filename << std::endl;

            // Загружаем модель для предпросмотра
            loadObjModel(filename, currentModelData);

            // Сохраняем конфиг
            saveConfig();
        }
    }
}

// Функция рисования куба
void drawCube() {
    float vertices[] = {
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f
    };

    float normals[] = {
        0,0,1, 0,0,1, 0,0,1, 0,0,1,
        0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
        0,1,0, 0,1,0, 0,1,0, 0,1,0,
        0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0,
        -1,0,0, -1,0,0, -1,0,0, -1,0,0,
        1,0,0, 1,0,0, 1,0,0, 1,0,0
    };

    GLuint indices[] = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glNormalPointer(GL_FLOAT, 0, normals);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

// Функция рисования загруженной модели
void drawModel() {
    if (currentModelData.loaded && currentModelData.vertices.size() > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, currentModelData.vertices.data());
        glNormalPointer(GL_FLOAT, 0, currentModelData.normals.data());

        glDrawArrays(GL_TRIANGLES, 0, currentModelData.vertices.size() / 3);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    else {
        // Если модель не загружена, рисуем куб
        drawCube();
    }
}

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

float getTextWidth(const std::string& text, float scale) {
    float width = 0;
    for (char c : text) {
        Character ch = Characters[c];
        width += (ch.Advance >> 6) * scale;
    }
    return width;
}

void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool centerX, bool centerY) {
    if (text.empty()) return;

    float textWidth = getTextWidth(text, scale);
    float textHeight = 48 * scale;

    if (centerX) x -= textWidth / 2;
    if (centerY) y -= textHeight / 2;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(textShaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
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

bool drawButton(int x, int y, int w, int h, const std::string& text, bool enabled) {
    bool hover = (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);

    glDisable(GL_DEPTH_TEST);

    if (!enabled) {
        glColor3f(0.3f, 0.3f, 0.3f);
    }
    else if (hover && mousePressed) {
        glColor3f(0.2f, 0.6f, 1.0f);
    }
    else if (hover) {
        glColor3f(0.3f, 0.5f, 0.9f);
    }
    else {
        glColor3f(0.2f, 0.4f, 0.8f);
    }

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    renderText(text, x + w / 2, y + h / 2 + 5, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f), true, true);

    return enabled && hover && mousePressed && !mousePressedLast;
}

void drawInfoBox(int x, int y, int w, int h, const std::string& label, const std::string& value) {
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    renderText(label, x + 5, y + 20, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));
    renderText(value, x + 5, y + 45, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
}

void render3DPreview() {
    int previewX = 650;
    int previewY = 120;
    int previewW = 500;
    int previewH = 400;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    float aspect = (float)previewW / previewH;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glm::vec3 eye(3.0f, 2.0f, 5.0f);
    glm::vec3 center(0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));

    glRotatef(previewRotation, 0.0f, 1.0f, 0.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    GLfloat lightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);

    GLfloat lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    // Устанавливаем цвет материала
    GLfloat matColor[4];
    switch (selectedPart) {
    case 0: matColor[0] = 0.0f; matColor[1] = 1.0f; matColor[2] = 0.0f; break;
    case 1: matColor[0] = 0.0f; matColor[1] = 0.7f; matColor[2] = 0.0f; break;
    case 2: matColor[0] = 0.0f; matColor[1] = 0.5f; matColor[2] = 0.0f; break;
    }
    matColor[3] = 1.0f;

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matColor);

    // Рисуем модель
    drawModel();

    // Сетка
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES);
    for (int i = -2; i <= 2; i++) {
        glVertex3f(i, -0.5f, -2);
        glVertex3f(i, -0.5f, 2);
        glVertex3f(-2, -0.5f, i);
        glVertex3f(2, -0.5f, i);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();

    reset2DProjection();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(previewX, previewY);
    glVertex2f(previewX + previewW, previewY);
    glVertex2f(previewX + previewW, previewY + previewH);
    glVertex2f(previewX, previewY + previewH);
    glEnd();

    renderText("3D Preview", previewX + 10, previewY + 25, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));
}

void renderSnakeEditor() {
    reset2DProjection();

    renderText("SNAKE EDITOR", windowWidth / 2, 50, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int buttonWidth = 120;
    int startX = 100;
    int buttonY = 120;

    if (drawButton(startX, buttonY, buttonWidth, 40, "Head")) {
        selectedPart = 0;
        loadObjModel(currentConfig.snakeHeadModel, currentModelData);
    }
    if (drawButton(startX + 130, buttonY, buttonWidth, 40, "Body")) {
        selectedPart = 1;
        loadObjModel(currentConfig.snakeBodyModel, currentModelData);
    }
    if (drawButton(startX + 260, buttonY, buttonWidth, 40, "Tail")) {
        selectedPart = 2;
        loadObjModel(currentConfig.snakeTailModel, currentModelData);
    }

    std::string currentModel;
    std::string folderName;
    switch (selectedPart) {
    case 0:
        currentModel = currentConfig.snakeHeadModel;
        folderName = "snake_head";
        break;
    case 1:
        currentModel = currentConfig.snakeBodyModel;
        folderName = "snake_body";
        break;
    case 2:
        currentModel = currentConfig.snakeTailModel;
        folderName = "snake_tail";
        break;
    }

    drawInfoBox(50, 180, 250, 60, "Current Model", currentModel);
    renderText("Folder: " + folderName, 50, 260, 0.25f, glm::vec3(0.8f, 0.8f, 0.8f));

    if (drawButton(50, 300, 250, 50, "CHOOSE MODEL")) {
        openFileDialog();
    }

    if (drawButton(50, 550, 100, 40, "Back")) {
        currentMode = MODE_MAIN;
    }

    if (drawButton(170, 550, 100, 40, "Save")) {
        saveConfig();
    }

    previewRotation += 0.5f;
    if (previewRotation > 360) previewRotation = 0;
    render3DPreview();
}

void renderMainMenu() {
    reset2DProjection();

    renderText("SNAKE GAME CONFIG EDITOR", windowWidth / 2, 80, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int buttonWidth = 250;
    int buttonHeight = 50;
    int startY = 200;
    int centerX = windowWidth / 2 - buttonWidth / 2;

    if (drawButton(centerX, startY, buttonWidth, buttonHeight, "1. Snake Editor")) {
        currentMode = MODE_SNAKE_EDITOR;
        switch (selectedPart) {
        case 0: loadObjModel(currentConfig.snakeHeadModel, currentModelData); break;
        case 1: loadObjModel(currentConfig.snakeBodyModel, currentModelData); break;
        case 2: loadObjModel(currentConfig.snakeTailModel, currentModelData); break;
        }
    }

    if (drawButton(centerX, startY + 70, buttonWidth, buttonHeight, "2. Obstacle Editor")) {
        // Coming soon
    }

    if (drawButton(centerX, startY + 140, buttonWidth, buttonHeight, "3. Environment Editor")) {
        // Coming soon
    }

    if (drawButton(centerX, startY + 210, buttonWidth, buttonHeight, "4. Save and Exit")) {
        saveConfig();
        glfwSetWindowShouldClose(window, true);
    }

    std::string gridInfo = "Grid: " + std::to_string(currentConfig.gridWidth) + "x" +
        std::to_string(currentConfig.gridDepth);
    renderText(gridInfo, windowWidth / 2, 600, 0.3f, glm::vec3(0.8f, 0.8f, 1.0f), true, false);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mousePressedLast = mousePressed;
        mousePressed = (action == GLFW_PRESS);
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initFreeType();
    initPaths();

    return true;
}

int main() {
    if (!initOpenGL()) {
        return -1;
    }

    // Загружаем конфиг
    if (!ConfigManager::loadGameConfig(configPath, currentConfig)) {
        std::cout << "Creating new config" << std::endl;
    }

    // Загружаем начальную модель
    loadObjModel(currentConfig.snakeHeadModel, currentModelData);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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