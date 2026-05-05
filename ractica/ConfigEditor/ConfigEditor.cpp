#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Windows headers
#include <windows.h>
#include <commdlg.h>

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// STB Image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// STL
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>

#include "ConfigManager.h"
#include "../Shared/ConfigTypes.h"

//=============================================================================
// ОПРЕДЕЛЕНИЯ СТРУКТУР
//=============================================================================

// Структура для символа шрифта
struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint Advance;
};

// Структура для текстуры
struct Texture {
    unsigned int id;
    int width;
    int height;
    std::string path;
    Texture() : id(0), width(0), height(0), path("") {}
};

// Структура для визуального элемента
struct VisualElement {
    std::string name;
    std::string modelFile;
    std::string textureFile;
    glm::vec3 color;
    float scale;
    int count;
    bool enabled;

    VisualElement(const std::string& n = "") : name(n), modelFile(""), textureFile(""),
        color(1.0f, 1.0f, 1.0f), scale(1.0f), count(1), enabled(true) {
    }
};

//=============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
//=============================================================================

std::string g_assetsPath;
std::string g_modelsPath;
std::string g_texturesPath;
std::string g_configPath;

// FreeType
static FT_Library g_ft = nullptr;
static FT_Face g_face = nullptr;
static std::map<unsigned char, Character> g_characters;
static GLuint g_textVAO = 0, g_textVBO = 0;
static bool g_fontInitialized = false;

// GLFW
GLFWwindow* window;
int windowWidth = 1400;
int windowHeight = 900;
GameConfig currentConfig;

// Режимы редактора
enum EditorMode {
    MODE_MAIN,
    MODE_SNAKE_EDITOR,
    MODE_GROUND_SKY_EDITOR,
    MODE_OBSTACLES_EDITOR,
    MODE_ENVIRONMENT_EDITOR,
    MODE_GRID_EDITOR,
    MODE_SHADOW_EDITOR,
    MODE_LIGHT_EDITOR
};
EditorMode currentMode = MODE_MAIN;

// Элементы
std::vector<VisualElement*> snakeElements;
std::vector<VisualElement*> groundSkyElements;
std::vector<VisualElement*> obstaclesElements;
std::vector<VisualElement*> environmentElements;

// Предпросмотр
ModelData previewModel;
float previewRotation = 0.0f;
bool autoRotate = true;
float lastRotationTime = 0.0f;
float autoRotateDelay = 5.0f;
float rotationSpeed = 0.5f;

// Выбранные элементы
int selectedPart = 0;
int selectedObstacle = 0;
int selectedEnv = 0;
int selectedGroundSky = 0;

// Текстуры
Texture floorTexture;
Texture skyTexture;

// UI состояние
bool mousePressed = false;
bool mousePressedLast = false;
double mouseX = 0, mouseY = 0;

//=============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
//=============================================================================

void renderMainMenu();
void renderSnakeEditor();
void renderGroundSkyEditor();
void renderObstaclesEditor();
void renderEnvironmentEditor();
void renderGridEditor();
void renderShadowEditor();
void renderLightEditor();
void renderShadowPreview();
void renderModelPreview(ModelData& model, const char* title, float x, float y, float w, float h,
    float& rotation, bool& autoRotate, float& lastTime, float scale = 1.0f, bool isFloor = false);
void saveConfig();
void initPaths();
void initElements();
void initFreeType();
void cleanupFreeType();
void drawText(float x, float y, const std::string& text, float r, float g, float b);
void drawCenteredText(float y, const std::string& text, float r, float g, float b);
float getTextWidth(const std::string& text);
void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha);
bool drawButton(int x, int y, int w, int h, const char* text, bool enabled = true);
bool drawSlider(int x, int y, int w, float* value, float minVal, float maxVal, const char* label);
bool drawIntSlider(int x, int y, int w, int* value, int minVal, int maxVal, const char* label);
void drawColorPicker(int x, int y, const char* label, glm::vec3& color);
void openFileDialog(std::string& destVar, const std::string& subFolder);
void openTextureFileDialog(std::string& destVar, Texture& texture);
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder);
void drawModel(ModelData& model);
void reset2DProjection();
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void windowSizeCallback(GLFWwindow* window, int width, int height);
bool initOpenGL();

//=============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
//=============================================================================

std::string extractFilename(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) return path.substr(pos + 1);
    return path;
}

GLuint loadTextureFromMemory(unsigned char* data, int width, int height, int channels) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

void clearPreview() {
    previewModel.loaded = false;
    previewModel.vertices.clear();
    previewModel.normals.clear();
    previewModel.texCoords.clear();
    previewModel.materials.clear();
    previewModel.materialIndices.clear();
}

void updatePreviewForCurrentMode() {
    clearPreview();
    switch (currentMode) {
    case MODE_SNAKE_EDITOR:
        if (selectedPart >= 0 && selectedPart < (int)snakeElements.size()) {
            std::string folder = (selectedPart == 0) ? "snake_head" :
                (selectedPart == 1) ? "snake_body" : "snake_tail";
            loadFBXModel(snakeElements[selectedPart]->modelFile, previewModel, folder);
        }
        break;
    case MODE_GROUND_SKY_EDITOR:
        if (selectedGroundSky == 0 && !groundSkyElements.empty()) {
            loadFBXModel(groundSkyElements[0]->modelFile, previewModel, "floor");
        }
        break;
    case MODE_OBSTACLES_EDITOR:
        if (selectedObstacle >= 0 && selectedObstacle < (int)obstaclesElements.size()) {
            loadFBXModel(obstaclesElements[selectedObstacle]->modelFile, previewModel, "obstacles");
        }
        break;
    case MODE_ENVIRONMENT_EDITOR:
        if (selectedEnv >= 0 && selectedEnv < (int)environmentElements.size()) {
            std::string folder = (selectedEnv == 0) ? "flowers" :
                (selectedEnv == 1) ? "birds" : "clouds";
            loadFBXModel(environmentElements[selectedEnv]->modelFile, previewModel, folder);
        }
        break;
    default: break;
    }
}

//=============================================================================
// ИНИЦИАЛИЗАЦИЯ
//=============================================================================

void initPaths() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::string exePath = std::string(currentDir);

    std::string rootPath = exePath;
    size_t pos = rootPath.find("\\Project2");
    if (pos != std::string::npos) rootPath = rootPath.substr(0, pos);
    else if ((pos = rootPath.find("\\x64\\Debug")) != std::string::npos) rootPath = rootPath.substr(0, pos);
    else if ((pos = rootPath.find("\\Debug")) != std::string::npos) rootPath = rootPath.substr(0, pos);

    if (rootPath.find("ractica") == std::string::npos) rootPath += "\\ractica";

    g_assetsPath = rootPath + "\\assets\\";
    g_modelsPath = g_assetsPath + "models\\";
    g_texturesPath = g_assetsPath + "textures\\";
    g_configPath = g_assetsPath + "config\\game.cfg";

    CreateDirectoryA(g_assetsPath.c_str(), NULL);
    CreateDirectoryA((g_assetsPath + "config").c_str(), NULL);
    CreateDirectoryA(g_modelsPath.c_str(), NULL);
    CreateDirectoryA(g_texturesPath.c_str(), NULL);

    std::vector<std::string> modelSubfolders = {
        "snake_head", "snake_body", "snake_tail", "obstacles",
        "food", "clouds", "birds", "flowers", "floor", "fence"
    };
    for (const auto& subfolder : modelSubfolders) {
        std::string path = g_modelsPath + subfolder + "\\";
        CreateDirectoryA(path.c_str(), NULL);
    }
}

// В ConfigEditor.cpp, исправленная функция initElements():

void initElements() {
    // Очищаем существующие элементы
    for (auto* el : snakeElements) delete el;
    for (auto* el : groundSkyElements) delete el;
    for (auto* el : obstaclesElements) delete el;
    for (auto* el : environmentElements) delete el;

    snakeElements.clear();
    groundSkyElements.clear();
    obstaclesElements.clear();
    environmentElements.clear();

    // Змея
    VisualElement* head = new VisualElement("ГОЛОВА");
    head->modelFile = currentConfig.snakeHeadModel;
    head->color = currentConfig.snakeHeadColor;
    head->scale = currentConfig.snakeHeadScale;
    snakeElements.push_back(head);

    VisualElement* body = new VisualElement("ТЕЛО");
    body->modelFile = currentConfig.snakeBodyModel;
    body->color = currentConfig.snakeBodyColor;
    body->scale = currentConfig.snakeBodyScale;
    snakeElements.push_back(body);

    VisualElement* tail = new VisualElement("ХВОСТ");
    tail->modelFile = currentConfig.snakeTailModel;
    tail->color = currentConfig.snakeTailColor;
    tail->scale = currentConfig.snakeTailScale;
    snakeElements.push_back(tail);

    // Пол и небо
    VisualElement* ground = new VisualElement("ПОЛ");
    ground->modelFile = currentConfig.floorModel;
    ground->textureFile = currentConfig.floorTexture;
    ground->color = currentConfig.floorColor;
    groundSkyElements.push_back(ground);

    VisualElement* sky = new VisualElement("НЕБО");
    sky->textureFile = "";
    sky->color = currentConfig.skyColor;
    groundSkyElements.push_back(sky);

    // Преграды (читаем из конфига)
    VisualElement* tree = new VisualElement("ДЕРЕВО");
    tree->modelFile = currentConfig.treeModel;
    tree->color = glm::vec3(0.1f, 0.4f, 0.1f);
    tree->scale = 1.5f;
    obstaclesElements.push_back(tree);

    VisualElement* rock = new VisualElement("КАМЕНЬ");
    rock->modelFile = currentConfig.rockModel;
    rock->color = glm::vec3(0.5f, 0.5f, 0.5f);
    rock->scale = 1.2f;
    obstaclesElements.push_back(rock);

    VisualElement* fence = new VisualElement("ЗАБОР");
    fence->modelFile = currentConfig.fenceModel;
    fence->color = glm::vec3(0.6f, 0.4f, 0.2f);
    fence->scale = 1.0f;
    obstaclesElements.push_back(fence);

    VisualElement* apple = new VisualElement("ЯБЛОКО");
    apple->modelFile = currentConfig.appleModel;
    apple->color = glm::vec3(1.0f, 0.0f, 0.0f);
    apple->scale = 0.8f;
    apple->count = currentConfig.initialFoodCount;
    obstaclesElements.push_back(apple);

    // Окружение
    VisualElement* flower = new VisualElement("ЦВЕТОК");
    flower->modelFile = currentConfig.flowerModel;
    flower->color = glm::vec3(1.0f, 0.0f, 1.0f);
    flower->scale = 0.7f;
    flower->count = currentConfig.flowerCount;
    environmentElements.push_back(flower);

    VisualElement* bird = new VisualElement("ПТИЦА");
    bird->modelFile = currentConfig.birdModel;
    bird->color = glm::vec3(0.5f, 0.5f, 0.5f);
    bird->scale = 0.6f;
    bird->count = currentConfig.birdCount;
    environmentElements.push_back(bird);

    VisualElement* cloud = new VisualElement("ОБЛАКО");
    cloud->modelFile = currentConfig.cloudModel;
    cloud->color = glm::vec3(1.0f, 1.0f, 1.0f);
    cloud->scale = 1.5f;
    cloud->count = currentConfig.cloudCount;
    environmentElements.push_back(cloud);
}
//=============================================================================
// ШРИФТЫ (FreeType)
//=============================================================================

void initFreeType() {
    std::cout << "Инициализация шрифта FreeType..." << std::endl;

    if (FT_Init_FreeType(&g_ft)) {
        std::cerr << "ОШИБКА: Не удалось инициализировать FreeType" << std::endl;
        return;
    }

    std::string fontPath = "C:/Windows/Fonts/arial.ttf";
    if (FT_New_Face(g_ft, fontPath.c_str(), 0, &g_face)) {
        std::cerr << "ОШИБКА: Не удалось загрузить шрифт" << std::endl;
        return;
    }

    // Устанавливаем размер шрифта 20 (как в GameUI)
    FT_Set_Pixel_Sizes(g_face, 0, 20);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Загружаем ASCII символы (32-126)
    for (unsigned char c = 32; c < 127; c++) {
        if (FT_Load_Char(g_face, c, FT_LOAD_RENDER)) continue;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character;
        character.TextureID = texture;
        character.Size = glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows);
        character.Bearing = glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top);
        character.Advance = g_face->glyph->advance.x;
        g_characters.insert(std::pair<unsigned char, Character>(c, character));
    }

    // Заглавные русские буквы А-Я (Unicode 0x0410-0x042F -> CP1251 0xC0-0xDF)
    for (int i = 0; i < 32; i++) {
        int unicode = 0x0410 + i;
        unsigned char cp1251 = 0xC0 + i;

        if (FT_Load_Char(g_face, unicode, FT_LOAD_RENDER)) continue;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character;
        character.TextureID = texture;
        character.Size = glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows);
        character.Bearing = glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top);
        character.Advance = g_face->glyph->advance.x;
        g_characters.insert(std::pair<unsigned char, Character>(cp1251, character));
    }

    // Ё (0x0401 -> 0xA8)
    if (FT_Load_Char(g_face, 0x0401, FT_LOAD_RENDER) == 0) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character;
        character.TextureID = texture;
        character.Size = glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows);
        character.Bearing = glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top);
        character.Advance = g_face->glyph->advance.x;
        g_characters.insert(std::pair<unsigned char, Character>(0xA8, character));
    }

    // Строчные русские буквы а-я (Unicode 0x0430-0x044F -> CP1251 0xE0-0xFF)
    for (int i = 0; i < 32; i++) {
        int unicode = 0x0430 + i;
        unsigned char cp1251 = 0xE0 + i;

        if (FT_Load_Char(g_face, unicode, FT_LOAD_RENDER)) continue;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character;
        character.TextureID = texture;
        character.Size = glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows);
        character.Bearing = glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top);
        character.Advance = g_face->glyph->advance.x;
        g_characters.insert(std::pair<unsigned char, Character>(cp1251, character));
    }

    // ё (0x0451 -> 0xB8)
    if (FT_Load_Char(g_face, 0x0451, FT_LOAD_RENDER) == 0) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows,
            0, GL_ALPHA, GL_UNSIGNED_BYTE, g_face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character;
        character.TextureID = texture;
        character.Size = glm::ivec2(g_face->glyph->bitmap.width, g_face->glyph->bitmap.rows);
        character.Bearing = glm::ivec2(g_face->glyph->bitmap_left, g_face->glyph->bitmap_top);
        character.Advance = g_face->glyph->advance.x;
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
    std::cout << "Шрифт успешно инициализирован" << std::endl;
}

void cleanupFreeType() {
    if (g_face) FT_Done_Face(g_face);
    if (g_ft) FT_Done_FreeType(g_ft);
    for (auto& character : g_characters) {
        glDeleteTextures(1, &character.second.TextureID);
    }
    g_characters.clear();
    if (g_textVAO) glDeleteVertexArrays(1, &g_textVAO);
    if (g_textVBO) glDeleteBuffers(1, &g_textVBO);
    g_textVAO = g_textVBO = 0;
    g_fontInitialized = false;
}

float getTextWidth(const std::string& text) {
    if (!g_fontInitialized || g_characters.empty()) {
        return (float)(text.length() * 10);
    }
    float width = 0;
    for (unsigned char c : text) {
        auto it = g_characters.find(c);
        if (it != g_characters.end()) {
            width += (float)(it->second.Advance >> 6);
        }
        else {
            auto spaceIt = g_characters.find(' ');
            if (spaceIt != g_characters.end()) width += (float)(spaceIt->second.Advance >> 6);
            else width += 10.0f;
        }
    }
    return width;
}

void drawText(float x, float y, const std::string& text, float r, float g, float b) {
    if (!g_fontInitialized || g_characters.empty() || text.empty()) return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)windowWidth, (double)windowHeight, 0.0, -1.0, 1.0);

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

    // Фиксированная высота строки (можно подобрать под размер шрифта)
    const int LINE_HEIGHT = 20;
    // Базовое смещение для выравнивания символов
    const int BASE_OFFSET = 15;

    float startX = x;
    for (unsigned char c : text) {
        auto it = g_characters.find(c);
        if (it == g_characters.end()) {
            it = g_characters.find(' ');
            if (it == g_characters.end()) continue;
        }
        Character& ch = it->second;

        // Выравниваем все символы по одной базовой линии
        float xpos = startX + (float)ch.Bearing.x;
        // Смещаем так, чтобы низ символа был на y + BASE_OFFSET
        float ypos = y + BASE_OFFSET - (float)ch.Size.y;
        float w = (float)ch.Size.x;
        float h = (float)ch.Size.y;

        float vertices[6][4] = {
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f },
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), 0);
        glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        startX += (float)(ch.Advance >> 6);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

void drawCenteredText(float y, const std::string& text, float r, float g, float b) {
    float textWidth = getTextWidth(text);
    float x = ((float)windowWidth - textWidth) / 2.0f;
    drawText(x, y, text, r, g, b);
}

void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Та же система координат: Y=0 вверху, Y=windowHeight внизу
    glOrtho(0.0, (double)windowWidth, (double)windowHeight, 0.0, -1.0, 1.0);

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

//=============================================================================
// UI ЭЛЕМЕНТЫ
//=============================================================================

bool drawButton(int x, int y, int w, int h, const char* text, bool enabled) {
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
    glVertex2f((float)x, (float)y);
    glVertex2f((float)(x + w), (float)y);
    glVertex2f((float)(x + w), (float)(y + h));
    glVertex2f((float)x, (float)(y + h));
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)(x + w), (float)y);
    glVertex2f((float)(x + w), (float)(y + h));
    glVertex2f((float)x, (float)(y + h));
    glEnd();

    // Центрируем текст на кнопке (Y теперь правильный)
    float textWidth = getTextWidth(text);
    float textHeight = 15.0f;
    float textX = (float)x + ((float)w - textWidth) / 2.0f;
    float textY = (float)y + ((float)h - textHeight) / 2.0f;  // Без +7, так как Y=0 вверху
    drawText(textX, textY, text, 1.0f, 1.0f, 1.0f);

    return enabled && hover && mousePressed && !mousePressedLast;
}

bool drawSlider(int x, int y, int w, float* value, float minVal, float maxVal, const char* label) {
    bool hover = (mouseX >= x && mouseX <= x + w && mouseY >= y - 10 && mouseY <= y + 10);
    bool changed = false;

    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)(x + w), (float)y);
    glEnd();

    float thumbX = (float)x + (*value - minVal) / (maxVal - minVal) * (float)w;

    if (hover && mousePressed) {
        *value = minVal + (float)((mouseX - x) / w) * (maxVal - minVal);
        if (*value < minVal) *value = minVal;
        if (*value > maxVal) *value = maxVal;
        glColor3f(1.0f, 1.0f, 0.0f);
        changed = true;
    }
    else if (hover) {
        glColor3f(0.8f, 0.8f, 1.0f);
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    glBegin(GL_QUADS);
    glVertex2f(thumbX - 5, (float)(y - 10));
    glVertex2f(thumbX + 5, (float)(y - 10));
    glVertex2f(thumbX + 5, (float)(y + 10));
    glVertex2f(thumbX - 5, (float)(y + 10));
    glEnd();

    char valueText[100];
    sprintf_s(valueText, "%s: %.2f", label, *value);
    // Текст рисуем справа от слайдера, Y корректируем
    drawText((float)(x + w + 10), (float)(y - 8), valueText, 1.0f, 1.0f, 1.0f);

    return changed;
}

bool drawIntSlider(int x, int y, int w, int* value, int minVal, int maxVal, const char* label) {
    float fval = (float)*value;
    bool changed = drawSlider(x, y, w, &fval, (float)minVal, (float)maxVal, label);
    if (changed) *value = (int)(fval + 0.5f);
    return changed;
}

void drawColorPicker(int x, int y, const char* label, glm::vec3& color) {
    // label рисуем над picker'ом
    drawText((float)x, (float)(y - 22), label, 1.0f, 1.0f, 0.0f);

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)(x + 60), (float)y);
    glVertex2f((float)(x + 60), (float)(y + 35));
    glVertex2f((float)x, (float)(y + 35));
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)(x + 60), (float)y);
    glVertex2f((float)(x + 60), (float)(y + 35));
    glVertex2f((float)x, (float)(y + 35));
    glEnd();

    // Слайдеры под color picker'ом
    drawSlider(x + 70, y + 5, 120, &color.r, 0.0f, 1.0f, "R");
    drawSlider(x + 70, y + 40, 120, &color.g, 0.0f, 1.0f, "G");
    drawSlider(x + 70, y + 75, 120, &color.b, 0.0f, 1.0f, "B");
}

//=============================================================================
// ЗАГРУЗКА МОДЕЛЕЙ
//=============================================================================

GLuint loadTextureFromFile(const std::string& path) {
    if (!std::filesystem::exists(path)) return 0;

    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data) {
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return textureID;
    }
    stbi_image_free(data);
    return 0;
}

bool loadTexture(const std::string& filename, Texture& texture) {
    std::string fullPath = g_texturesPath + filename;
    if (!std::filesystem::exists(fullPath)) return false;
    if (texture.id != 0) glDeleteTextures(1, &texture.id);
    texture.id = loadTextureFromFile(fullPath);
    texture.path = filename;
    return texture.id != 0;
}

bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder) {
    if (filename.empty()) return false;

    std::string fullPath = g_modelsPath + subFolder + "\\" + filename;
    if (!std::filesystem::exists(fullPath)) return false;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(fullPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices);
    if (!scene) return false;

    model.vertices.clear();
    model.normals.clear();
    model.texCoords.clear();
    model.materials.clear();
    model.materialIndices.clear();

    // Загружаем материалы
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material material;
        material.textureID = 0;
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.diffuse = glm::vec3(color.r, color.g, color.b);
        }
        model.materials.push_back(material);
    }
    if (model.materials.empty()) {
        Material defaultMat;
        defaultMat.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        model.materials.push_back(defaultMat);
    }

    // Загружаем меши
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        int materialIndex = (mesh->mMaterialIndex < (int)model.materials.size()) ? mesh->mMaterialIndex : 0;
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            model.materialIndices.push_back(materialIndex);
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                unsigned int vertexIdx = face.mIndices[k];
                model.vertices.push_back(mesh->mVertices[vertexIdx].x);
                model.vertices.push_back(mesh->mVertices[vertexIdx].y);
                model.vertices.push_back(mesh->mVertices[vertexIdx].z);
                if (mesh->HasNormals()) {
                    model.normals.push_back(mesh->mNormals[vertexIdx].x);
                    model.normals.push_back(mesh->mNormals[vertexIdx].y);
                    model.normals.push_back(mesh->mNormals[vertexIdx].z);
                }
                if (mesh->HasTextureCoords(0)) {
                    model.texCoords.push_back(mesh->mTextureCoords[0][vertexIdx].x);
                    model.texCoords.push_back(mesh->mTextureCoords[0][vertexIdx].y);
                }
            }
        }
    }

    if (model.normals.empty()) {
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            model.normals.push_back(0.0f);
            model.normals.push_back(1.0f);
            model.normals.push_back(0.0f);
        }
    }
    if (model.texCoords.empty()) {
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            model.texCoords.push_back(0.0f);
            model.texCoords.push_back(0.0f);
        }
    }

    model.loaded = (model.vertices.size() > 0);
    return model.loaded;
}

void openFileDialog(std::string& destVar, const std::string& subFolder) {
    std::string folderPath = g_modelsPath + subFolder + "\\";
    CreateDirectoryA(folderPath.c_str(), NULL);

    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "FBX Files\0*.fbx\0OBJ Files\0*.obj\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = folderPath.c_str();
    ofn.lpstrTitle = "Выберите модель";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1);
        destVar = filename;
        std::string destPath = folderPath + filename;
        if (fullPath != destPath) CopyFileA(fullPath.c_str(), destPath.c_str(), FALSE);
        updatePreviewForCurrentMode();
    }
}

void openTextureFileDialog(std::string& destVar, Texture& texture) {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = g_texturesPath.c_str();
    ofn.lpstrTitle = "Выберите текстуру";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1);
        destVar = filename;
        loadTexture(filename, texture);
        if (currentMode == MODE_GROUND_SKY_EDITOR && selectedGroundSky == 0) updatePreviewForCurrentMode();
    }
}

//=============================================================================
// ОТРИСОВКА МОДЕЛЕЙ
//=============================================================================

void drawModel(ModelData& model) {
    if (!model.loaded || model.vertices.empty()) return;

    if (!model.materials.empty() && model.materials[0].textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, model.materials[0].textureID);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.8f, 0.8f, 0.8f);
    }

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < model.vertices.size() / 3; i++) {
        if (!model.texCoords.empty()) {
            glTexCoord2f(model.texCoords[i * 2], model.texCoords[i * 2 + 1]);
        }
        glVertex3f(model.vertices[i * 3], model.vertices[i * 3 + 1], model.vertices[i * 3 + 2]);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void reset2DProjection() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Y=0 вверху, Y=windowHeight внизу
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
//=============================================================================
// ПРЕДПРОСМОТР МОДЕЛИ
//=============================================================================

void renderModelPreview(ModelData& model, const char* title, float x, float y, float w, float h,
    float& rotation, bool& autoRotate, float& lastTime, float scale, bool isFloor) {

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport((GLint)x, windowHeight - (GLint)y - (GLint)h, (GLint)w, (GLint)h);
    glScissor((GLint)x, windowHeight - (GLint)y - (GLint)h, (GLint)w, (GLint)h);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    float aspect = w / h;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glm::vec3 eye(3.0f, 2.0f, 5.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);

    // Пол
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-3.0f, -0.5f, -3.0f);
    glVertex3f(3.0f, -0.5f, -3.0f);
    glVertex3f(3.0f, -0.5f, 3.0f);
    glVertex3f(-3.0f, -0.5f, 3.0f);
    glEnd();
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    for (int i = -3; i <= 3; i++) {
        float pos = (float)i * 0.5f;
        glVertex3f(pos, -0.45f, -3.0f);
        glVertex3f(pos, -0.45f, 3.0f);
        glVertex3f(-3.0f, -0.45f, pos);
        glVertex3f(3.0f, -0.45f, pos);
    }
    glEnd();

    // Модель
    if (model.loaded && !model.vertices.empty()) {
        glPushMatrix();
        if (isFloor) {
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glScalef(0.01f, 0.01f, 0.01f);
        }
        else {
            glScalef(scale, scale, scale);
            float minX = model.vertices[0], maxX = model.vertices[0];
            float minY = model.vertices[1], maxY = model.vertices[1];
            float minZ = model.vertices[2], maxZ = model.vertices[2];
            for (size_t i = 0; i < model.vertices.size() / 3; i++) {
                float vx = model.vertices[i * 3];
                float vy = model.vertices[i * 3 + 1];
                float vz = model.vertices[i * 3 + 2];
                minX = std::min(minX, vx); maxX = max(maxX, vx);
                minY = std::min(minY, vy); maxY = max(maxY, vy);
                minZ = std::min(minZ, vz); maxZ = max(maxZ, vz);
            }
            float centerX = (minX + maxX) / 2.0f;
            float centerZ = (minZ + maxZ) / 2.0f;
            glTranslatef(-centerX, -minY + 0.1f, -centerZ);
        }
        drawModel(model);
        glPopMatrix();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    reset2DProjection();

    // Рамка
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    drawText(x + 10, y + 25, title, 1.0f, 1.0f, 0.0f);

    if (!isFloor) {
        char scaleText[50];
        sprintf_s(scaleText, "Scale: %.2f", scale);
        drawText(x + w - 100, y + 25, scaleText, 1.0f, 1.0f, 0.0f);
    }

    int arrowY = (int)(y + h + 25);
    int arrowCenterX = (int)(x + w / 2);
    if (drawButton(arrowCenterX - 70, arrowY, 60, 35, "<-")) {
        rotation -= 15.0f;
        autoRotate = false;
    }
    if (drawButton(arrowCenterX + 10, arrowY, 60, 35, "->")) {
        rotation += 15.0f;
        autoRotate = false;
    }
    float currentTime = (float)glfwGetTime();
    if (!autoRotate && (currentTime - lastTime >= autoRotateDelay)) autoRotate = true;
    if (autoRotate) rotation += rotationSpeed;
    if (rotation >= 360) rotation -= 360;
}

//=============================================================================
// РЕДАКТОР ЗМЕЙКИ
//=============================================================================
std::string truncateFilename(const std::string& filename, int maxLen = 25) {
    if (filename.length() <= maxLen) return filename;
    return "..." + filename.substr(filename.length() - (maxLen - 3));
}
void renderSnakeEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "РЕДАКТОР ЗМЕЙКИ", 1.0f, 1.0f, 0.0f);

    int listX = (int)(windowWidth * 0.03f);
    int listY = (int)(windowHeight * 0.12f);
    int listWidth = (int)(windowWidth * 0.12f);
    int listHeight = (int)(windowHeight * 0.05f);

    // Список частей змеи
    for (size_t i = 0; i < snakeElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", snakeElements[i]->name.c_str());
        if (drawButton(listX, listY + (int)i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedPart = (int)i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedPart >= 0 && selectedPart < (int)snakeElements.size()) {
        VisualElement* el = snakeElements[selectedPart];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), el->name.c_str(), 1.0f, 1.0f, 0.0f);

        // Модель
        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);

        std::string displayFile = truncateFilename(el->modelFile, 30);
        drawText((float)(editX + 80), (float)(modelY + 20), displayFile.c_str(), 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
            openFileDialog(el->modelFile, folder);
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedPart == 0) el->modelFile = "snake_head.fbx";
            else if (selectedPart == 1) el->modelFile = "snake_body.fbx";
            else el->modelFile = "snake_tail.fbx";
            updatePreviewForCurrentMode();
        }

        // Цвет
        int colorY = modelY + 90;
        drawText((float)editX, (float)colorY, "Цвет:", 1.0f, 1.0f, 1.0f);

        glColor3f(el->color.r, el->color.g, el->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &el->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &el->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &el->color.b, 0.0f, 1.0f, "B");

        if (drawButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            if (selectedPart == 0) el->color = glm::vec3(0.0f, 1.0f, 0.0f);
            else if (selectedPart == 1) el->color = glm::vec3(0.0f, 0.7f, 0.0f);
            else el->color = glm::vec3(0.0f, 0.5f, 0.0f);
        }

        // Масштаб
        int scaleY = colorY + 80;
        drawText((float)editX, (float)scaleY, "Масштаб:", 1.0f, 1.0f, 1.0f);

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        drawText((float)(editX + 100), (float)scaleY, scaleText, 1.0f, 1.0f, 0.0f);

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            el->scale = 0.8f;
        }
    }

    if (selectedPart >= 0 && selectedPart < (int)snakeElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            snakeElements[selectedPart]->scale, false);
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ПОЛА И НЕБА
//=============================================================================

void renderGroundSkyEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "ПОЛ И НЕБО", 1.0f, 1.0f, 0.0f);

    int listX = (int)(windowWidth * 0.03f);
    int listY = (int)(windowHeight * 0.12f);
    int listWidth = (int)(windowWidth * 0.1f);
    int listHeight = (int)(windowHeight * 0.05f);

    for (size_t i = 0; i < groundSkyElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", groundSkyElements[i]->name.c_str());
        if (drawButton(listX, listY + (int)i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedGroundSky = (int)i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedGroundSky == 0) {
        // ПОЛ
        VisualElement* ground = groundSkyElements[0];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), ground->name, 1.0f, 1.0f, 0.0f);

        // Модель пола
        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);
        drawText((float)(editX + 80), (float)(modelY + 20), ground->modelFile, 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(ground->modelFile, "floor");
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            ground->modelFile = "";
            updatePreviewForCurrentMode();
        }

        // Текстура пола
        int textureY = modelY + 90;
        drawText((float)editX, (float)textureY, "Текстура:", 1.0f, 1.0f, 1.0f);
        drawText((float)(editX + 100), (float)textureY, ground->textureFile, 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, textureY + 20, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openTextureFileDialog(ground->textureFile, floorTexture);
        }

        if (drawButton(editX + buttonSpacing, textureY + 20, buttonWidth, 30, "СБРОСИТЬ")) {
            ground->textureFile = "";
            if (floorTexture.id != 0) {
                glDeleteTextures(1, &floorTexture.id);
                floorTexture.id = 0;
            }
        }

        // Цвет пола
        int colorY = textureY + 70;
        drawText((float)editX, (float)colorY, "Цвет:", 1.0f, 1.0f, 1.0f);

        glColor3f(ground->color.r, ground->color.g, ground->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &ground->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &ground->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &ground->color.b, 0.0f, 1.0f, "B");

        if (drawButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            ground->color = glm::vec3(0.3f, 0.6f, 0.2f);
        }

        // Кнопка очистить все
        if (drawButton(editX, colorY + 80, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            ground->modelFile = "";
            ground->textureFile = "";
            ground->color = glm::vec3(0.3f, 0.6f, 0.2f);
            if (floorTexture.id != 0) {
                glDeleteTextures(1, &floorTexture.id);
                floorTexture.id = 0;
            }
            updatePreviewForCurrentMode();
        }

        renderModelPreview(previewModel, "ПРЕДПРОСМОТР ПОЛА",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            1.0f, true);
    }
    else {
        // НЕБО
        VisualElement* sky = groundSkyElements[1];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), sky->name, 1.0f, 1.0f, 0.0f);

        // Текстура неба
        int textureY = editY;
        drawText((float)editX, (float)textureY, "Текстура:", 1.0f, 1.0f, 1.0f);
        drawText((float)(editX + 100), (float)textureY, sky->textureFile, 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, textureY + 20, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openTextureFileDialog(sky->textureFile, skyTexture);
        }

        if (drawButton(editX + buttonSpacing, textureY + 20, buttonWidth, 30, "СБРОСИТЬ")) {
            sky->textureFile = "";
            if (skyTexture.id != 0) {
                glDeleteTextures(1, &skyTexture.id);
                skyTexture.id = 0;
            }
        }

        // Цвет неба
        int colorY = textureY + 70;
        drawText((float)editX, (float)colorY, "Цвет:", 1.0f, 1.0f, 1.0f);

        glColor3f(sky->color.r, sky->color.g, sky->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &sky->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &sky->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &sky->color.b, 0.0f, 1.0f, "B");

        if (drawButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            sky->color = glm::vec3(0.53f, 0.81f, 0.92f);
        }

        // Кнопка очистить все
        if (drawButton(editX, colorY + 80, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            sky->textureFile = "";
            sky->color = glm::vec3(0.53f, 0.81f, 0.92f);
            if (skyTexture.id != 0) {
                glDeleteTextures(1, &skyTexture.id);
                skyTexture.id = 0;
            }
        }

        // Для неба показываем сообщение
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f));
        glVertex2f((float)(windowWidth * 0.46f + windowWidth * 0.36f), (float)(windowHeight * 0.12f));
        glVertex2f((float)(windowWidth * 0.46f + windowWidth * 0.36f), (float)(windowHeight * 0.12f + windowHeight * 0.5f));
        glVertex2f((float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f + windowHeight * 0.5f));
        glEnd();

        drawCenteredText((float)(windowHeight * 0.37f), "НЕТ ПРЕДПРОСМОТРА ДЛЯ НЕБА", 1.0f, 1.0f, 0.0f);
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ПРЕГРАД
//=============================================================================

void renderObstaclesEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "ПРЕГРАДЫ", 1.0f, 1.0f, 0.0f);

    int listX = (int)(windowWidth * 0.03f);
    int listY = (int)(windowHeight * 0.12f);
    int listWidth = (int)(windowWidth * 0.1f);
    int listHeight = (int)(windowHeight * 0.05f);

    for (size_t i = 0; i < obstaclesElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", obstaclesElements[i]->name.c_str());
        if (drawButton(listX, listY + (int)i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedObstacle = (int)i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedObstacle >= 0 && selectedObstacle < (int)obstaclesElements.size()) {
        VisualElement* el = obstaclesElements[selectedObstacle];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), el->name, 1.0f, 1.0f, 0.0f);

        // Модель
        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);
        drawText((float)(editX + 80), (float)(modelY + 20), el->modelFile, 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(el->modelFile, "obstacles");
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->modelFile = "tree.fbx";
            else if (selectedObstacle == 1) el->modelFile = "rock.fbx";
            else if (selectedObstacle == 2) el->modelFile = "fence.fbx";
            else el->modelFile = "apple.fbx";
            updatePreviewForCurrentMode();
        }

        // Цвет
        int colorY = modelY + 90;
        drawText((float)editX, (float)colorY, "Цвет:", 1.0f, 1.0f, 1.0f);

        glColor3f(el->color.r, el->color.g, el->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &el->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &el->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &el->color.b, 0.0f, 1.0f, "B");

        if (drawButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->color = glm::vec3(0.1f, 0.4f, 0.1f);
            else if (selectedObstacle == 1) el->color = glm::vec3(0.5f, 0.5f, 0.5f);
            else if (selectedObstacle == 2) el->color = glm::vec3(0.6f, 0.4f, 0.2f);
            else el->color = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        // Масштаб
        int scaleY = colorY + 80;
        drawText((float)editX, (float)scaleY, "Масштаб:", 1.0f, 1.0f, 1.0f);

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        drawText((float)(editX + 100), (float)scaleY, scaleText, 1.0f, 1.0f, 0.0f);

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->scale = 1.5f;
            else if (selectedObstacle == 1) el->scale = 1.2f;
            else if (selectedObstacle == 2) el->scale = 1.0f;
            else el->scale = 0.8f;
        }

        // Количество
        int countY = scaleY + 70;
        drawText((float)editX, (float)countY, "Количество:", 1.0f, 1.0f, 1.0f);

        char countText[20];
        sprintf_s(countText, "%d", el->count);
        drawText((float)(editX + 120), (float)countY, countText, 1.0f, 1.0f, 0.0f);

        drawIntSlider(editX, countY + 20, 250, &el->count, 0, 50, "");

        if (drawButton(editX + 260, countY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->count = 10;
            else if (selectedObstacle == 1) el->count = 5;
            else if (selectedObstacle == 2) el->count = 8;
            else el->count = 10;
        }

        // Кнопка очистить все
        if (drawButton(editX, countY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            if (selectedObstacle == 0) {
                el->modelFile = "tree.fbx";
                el->color = glm::vec3(0.1f, 0.4f, 0.1f);
                el->scale = 1.5f;
                el->count = 10;
            }
            else if (selectedObstacle == 1) {
                el->modelFile = "rock.fbx";
                el->color = glm::vec3(0.5f, 0.5f, 0.5f);
                el->scale = 1.2f;
                el->count = 5;
            }
            else if (selectedObstacle == 2) {
                el->modelFile = "fence.fbx";
                el->color = glm::vec3(0.6f, 0.4f, 0.2f);
                el->scale = 1.0f;
                el->count = 8;
            }
            else {
                el->modelFile = "apple.fbx";
                el->color = glm::vec3(1.0f, 0.0f, 0.0f);
                el->scale = 0.8f;
                el->count = 10;
            }
            updatePreviewForCurrentMode();
        }
    }

    if (selectedObstacle >= 0 && selectedObstacle < (int)obstaclesElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            obstaclesElements[selectedObstacle]->scale, false);
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ОКРУЖЕНИЯ
//=============================================================================

void renderEnvironmentEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "ОКРУЖЕНИЕ", 1.0f, 1.0f, 0.0f);

    int listX = (int)(windowWidth * 0.03f);
    int listY = (int)(windowHeight * 0.12f);
    int listWidth = (int)(windowWidth * 0.1f);
    int listHeight = (int)(windowHeight * 0.05f);

    for (size_t i = 0; i < environmentElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", environmentElements[i]->name.c_str());
        if (drawButton(listX, listY + (int)i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedEnv = (int)i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedEnv >= 0 && selectedEnv < (int)environmentElements.size()) {
        VisualElement* el = environmentElements[selectedEnv];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), el->name, 1.0f, 1.0f, 0.0f);

        std::string folder = (selectedEnv == 0) ? "flowers" : (selectedEnv == 1) ? "birds" : "clouds";

        // Модель
        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);
        drawText((float)(editX + 80), (float)(modelY + 20), el->modelFile, 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(el->modelFile, folder);
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->modelFile = "flower.fbx";
            else if (selectedEnv == 1) el->modelFile = "bird.fbx";
            else el->modelFile = "cloud.fbx";
            updatePreviewForCurrentMode();
        }

        // Цвет
        int colorY = modelY + 90;
        drawText((float)editX, (float)colorY, "Цвет:", 1.0f, 1.0f, 1.0f);

        glColor3f(el->color.r, el->color.g, el->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 60), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY - 15));
        glVertex2f((float)(editX + 110), (float)(colorY + 15));
        glVertex2f((float)(editX + 60), (float)(colorY + 15));
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &el->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &el->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &el->color.b, 0.0f, 1.0f, "B");

        if (drawButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->color = glm::vec3(1.0f, 0.0f, 1.0f);
            else if (selectedEnv == 1) el->color = glm::vec3(0.5f, 0.5f, 0.5f);
            else el->color = glm::vec3(1.0f, 1.0f, 1.0f);
        }

        // Масштаб
        int scaleY = colorY + 80;
        drawText((float)editX, (float)scaleY, "Масштаб:", 1.0f, 1.0f, 1.0f);

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        drawText((float)(editX + 100), (float)scaleY, scaleText, 1.0f, 1.0f, 0.0f);

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->scale = 0.7f;
            else if (selectedEnv == 1) el->scale = 0.6f;
            else el->scale = 1.5f;
        }

        // Количество
        int countY = scaleY + 70;
        drawText((float)editX, (float)countY, "Количество:", 1.0f, 1.0f, 1.0f);

        char countText[20];
        sprintf_s(countText, "%d", el->count);
        drawText((float)(editX + 120), (float)countY, countText, 1.0f, 1.0f, 0.0f);

        drawIntSlider(editX, countY + 20, 250, &el->count, 0, 50, "");

        if (drawButton(editX + 260, countY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->count = 25;
            else if (selectedEnv == 1) el->count = 15;
            else el->count = 20;
        }

        // Кнопка очистить все
        if (drawButton(editX, countY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            if (selectedEnv == 0) {
                el->modelFile = "flower.fbx";
                el->color = glm::vec3(1.0f, 0.0f, 1.0f);
                el->scale = 0.7f;
                el->count = 25;
            }
            else if (selectedEnv == 1) {
                el->modelFile = "bird.fbx";
                el->color = glm::vec3(0.5f, 0.5f, 0.5f);
                el->scale = 0.6f;
                el->count = 15;
            }
            else {
                el->modelFile = "cloud.fbx";
                el->color = glm::vec3(1.0f, 1.0f, 1.0f);
                el->scale = 1.5f;
                el->count = 20;
            }
            updatePreviewForCurrentMode();
        }
    }

    if (selectedEnv >= 0 && selectedEnv < (int)environmentElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            environmentElements[selectedEnv]->scale, false);
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ИГРОВОГО ПОЛЯ
//=============================================================================

void renderGridEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "ИГРОВОЕ ПОЛЕ", 1.0f, 1.0f, 0.0f);

    int startX = (int)(windowWidth * 0.05f);
    int startY = (int)(windowHeight * 0.12f);
    int sliderWidth = (int)(windowWidth * 0.25f);
    int spacing = (int)(windowHeight * 0.08f);

    // Сетка
    drawText((float)startX, (float)(startY - 30), "СЕТКА", 1.0f, 1.0f, 0.0f);
    char gridBtnText[50];
    sprintf_s(gridBtnText, "Сетка: %s", currentConfig.gridEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX, startY, 150, 40, gridBtnText)) {
        currentConfig.gridEnabled = !currentConfig.gridEnabled;
    }
    if (drawButton(startX + 160, startY, 120, 40, "СБРОСИТЬ")) {
        currentConfig.gridEnabled = true;
        currentConfig.gridLineWidth = 1.0f;
        currentConfig.gridWidth = 120;
        currentConfig.gridDepth = 120;
        currentConfig.cellSize = 0.1f;
        currentConfig.gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    }

    if (currentConfig.gridEnabled) {
        drawText((float)startX, (float)(startY + 60), "Толщина линий:", 1.0f, 1.0f, 1.0f);
        drawSlider(startX, startY + 90, sliderWidth, &currentConfig.gridLineWidth, 0.5f, 5.0f, "Толщина");
        if (drawButton(startX + sliderWidth + 20, startY + 80, 100, 30, "СБРОСИТЬ")) {
            currentConfig.gridLineWidth = 1.0f;
        }
    }

    startY += spacing;

    // Размер сетки
    drawText((float)startX, (float)(startY - 30), "РАЗМЕР СЕТКИ", 1.0f, 1.0f, 0.0f);
    drawIntSlider(startX, startY, sliderWidth, &currentConfig.gridWidth, 5, 200, "Ширина");
    drawIntSlider(startX, startY + 50, sliderWidth, &currentConfig.gridDepth, 5, 200, "Глубина");
    if (drawButton(startX + sliderWidth + 20, startY + 10, 100, 30, "СБРОСИТЬ")) {
        currentConfig.gridWidth = 120;
        currentConfig.gridDepth = 120;
    }

    startY += spacing + 30;

    // Размер ячейки
    drawText((float)startX, (float)(startY - 30), "РАЗМЕР ЯЧЕЙКИ", 1.0f, 1.0f, 0.0f);
    drawSlider(startX, startY, sliderWidth, &currentConfig.cellSize, 0.05f, 1.0f, "Размер");
    if (drawButton(startX + sliderWidth + 20, startY - 10, 100, 30, "СБРОСИТЬ")) {
        currentConfig.cellSize = 0.1f;
    }

    startY += 80;

    // Полигоны пола
    drawText((float)startX, (float)(startY - 30), "ПОЛИГОНЫ ПОЛА", 1.0f, 1.0f, 0.0f);
    int totalPolygons = currentConfig.floorPolygonsX * currentConfig.floorPolygonsZ * 2;
    char polyInfo[100];
    sprintf_s(polyInfo, "Всего полигонов: %d", totalPolygons);
    drawText((float)(startX + sliderWidth + 100), (float)(startY - 20), polyInfo, 1.0f, 1.0f, 0.0f);

    drawIntSlider(startX, startY, sliderWidth, &currentConfig.floorPolygonsX, 1, 50, "Полигонов по X");
    drawIntSlider(startX, startY + 45, sliderWidth, &currentConfig.floorPolygonsZ, 1, 50, "Полигонов по Z");

    if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ (8x8)")) {
        currentConfig.floorPolygonsX = 8;
        currentConfig.floorPolygonsZ = 8;
    }

    startY += 110;

    // Подразбиение пола
    drawText((float)startX, (float)(startY - 30), "ПОДРАЗБИЕНИЕ ДЛЯ ТЕНЕЙ", 1.0f, 1.0f, 0.0f);
    char subdivBtnText[50];
    sprintf_s(subdivBtnText, "Подразбиение: %s", currentConfig.floorUseSubdivision ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX, startY, 180, 40, subdivBtnText)) {
        currentConfig.floorUseSubdivision = !currentConfig.floorUseSubdivision;
    }

    if (currentConfig.floorUseSubdivision) {
        char levelText[50];
        sprintf_s(levelText, "Уровень: %d", currentConfig.floorSubdivisionLevel);
        drawText((float)(startX + 200), (float)(startY + 15), levelText, 1.0f, 1.0f, 0.0f);
        drawIntSlider(startX + 280, startY, 150, &currentConfig.floorSubdivisionLevel, 2, 20, "");
    }

    if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ")) {
        currentConfig.floorUseSubdivision = false;
        currentConfig.floorSubdivisionLevel = 10;
    }

    startY += 70;

    // Цвет сетки
    drawColorPicker(startX, startY, "Цвет сетки:", currentConfig.gridColor);
    if (drawButton(startX + sliderWidth + 20, startY + 50, 100, 30, "СБРОСИТЬ")) {
        currentConfig.gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    }

    // Предпросмотр (3D)
    int previewX = (int)(windowWidth * 0.55f);
    int previewY = (int)(windowHeight * 0.12f);
    int previewW = (int)(windowWidth * 0.4f);
    int previewH = (int)(windowHeight * 0.6f);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    float aspect = (float)previewW / previewH;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glm::vec3 eye(10.0f, 8.0f, 15.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
    glLoadMatrixf(glm::value_ptr(view));

    // Пол
    float size = (float)currentConfig.gridWidth * currentConfig.cellSize / 2.0f;
    float depth = (float)currentConfig.gridDepth * currentConfig.cellSize / 2.0f;
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-size, -0.5f, -depth);
    glVertex3f(size, -0.5f, -depth);
    glVertex3f(size, -0.5f, depth);
    glVertex3f(-size, -0.5f, depth);
    glEnd();

    if (currentConfig.gridEnabled) {
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
        glLineWidth(currentConfig.gridLineWidth);
        glBegin(GL_LINES);
        for (int i = -currentConfig.gridWidth / 2; i <= currentConfig.gridWidth / 2; i++) {
            float x = (float)i * currentConfig.cellSize;
            glVertex3f(x, -0.4f, -depth);
            glVertex3f(x, -0.4f, depth);
        }
        for (int i = -currentConfig.gridDepth / 2; i <= currentConfig.gridDepth / 2; i++) {
            float z = (float)i * currentConfig.cellSize;
            glVertex3f(-size, -0.4f, z);
            glVertex3f(size, -0.4f, z);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    reset2DProjection();
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)previewX, (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)(previewY + previewH));
    glVertex2f((float)previewX, (float)(previewY + previewH));
    glEnd();
    drawText((float)(previewX + 10), (float)(previewY + 25), "ПРЕДПРОСМОТР СЕТКИ", 1.0f, 1.0f, 0.0f);

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР СВЕТА
//=============================================================================

void renderLightEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "НАСТРОЙКИ СВЕТА", 1.0f, 1.0f, 0.0f);

    int startX = (int)(windowWidth * 0.05f);
    int startY = (int)(windowHeight * 0.12f);
    int sliderWidth = (int)(windowWidth * 0.25f);

    drawText((float)startX, (float)(startY - 30), "ТИП ИСТОЧНИКА СВЕТА", 1.0f, 1.0f, 0.0f);

    if (drawButton(startX, startY, 180, 40, "Направленный")) {
        currentConfig.lightType = 0;
    }
    if (drawButton(startX + 200, startY, 180, 40, "Точечный")) {
        currentConfig.lightType = 1;
    }
    if (drawButton(startX + 400, startY, 180, 40, "Прожектор")) {
        currentConfig.lightType = 2;
    }

    startY += 60;

    drawColorPicker(startX, startY, "Цвет света:", currentConfig.lightColor);

    startY += 120;

    if (currentConfig.lightType == 0) {
        drawText((float)startX, (float)(startY - 30), "НАПРАВЛЕНИЕ СВЕТА", 1.0f, 1.0f, 0.0f);
        drawSlider(startX, startY, sliderWidth, &currentConfig.lightDir.x, -1.0f, 1.0f, "X");
        drawSlider(startX, startY + 50, sliderWidth, &currentConfig.lightDir.y, -1.0f, 1.0f, "Y");
        drawSlider(startX, startY + 100, sliderWidth, &currentConfig.lightDir.z, -1.0f, 1.0f, "Z");
        if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ")) {
            currentConfig.lightDir = glm::vec3(-1.0f, -1.0f, 0.5f);
            currentConfig.lightDir = glm::normalize(currentConfig.lightDir);
        }
    }
    else {
        drawText((float)startX, (float)(startY - 30), "ПОЗИЦИЯ СВЕТА", 1.0f, 1.0f, 0.0f);
        drawSlider(startX, startY, sliderWidth, &currentConfig.lightPos.x, -10.0f, 10.0f, "X");
        drawSlider(startX, startY + 50, sliderWidth, &currentConfig.lightPos.y, 0.0f, 15.0f, "Y");
        drawSlider(startX, startY + 100, sliderWidth, &currentConfig.lightPos.z, -10.0f, 10.0f, "Z");
        if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ")) {
            currentConfig.lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
        }
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ТЕНЕЙ
//=============================================================================

void renderShadowPreview() {
    int previewX = (int)(windowWidth * 0.6f);
    int previewY = (int)(windowHeight * 0.12f);
    int previewW = (int)(windowWidth * 0.35f);
    int previewH = (int)(windowHeight * 0.55f);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat light_ambient[] = { currentConfig.ambientEnabled ? 0.3f : 0.0f,
                                currentConfig.ambientEnabled ? 0.3f : 0.0f,
                                currentConfig.ambientEnabled ? 0.3f : 0.0f, 1.0f };
    GLfloat light_diffuse[] = { currentConfig.lightColor.r, currentConfig.lightColor.g, currentConfig.lightColor.b, 1.0f };
    GLfloat light_specular[] = { currentConfig.specularEnabled ? 0.5f : 0.0f,
                                 currentConfig.specularEnabled ? 0.5f : 0.0f,
                                 currentConfig.specularEnabled ? 0.5f : 0.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    if (currentConfig.lightType == 0) {
        GLfloat light_position[] = { -currentConfig.lightDir.x, -currentConfig.lightDir.y, -currentConfig.lightDir.z, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    }
    else {
        GLfloat light_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y, currentConfig.lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        if (currentConfig.lightType == 2) {
            GLfloat spot_direction[] = { 0.0f, -1.0f, 0.0f };
            glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
            glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f);
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    float aspect = (float)previewW / previewH;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glm::vec3 eye(5.0f, 4.0f, 8.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
    glLoadMatrixf(glm::value_ptr(view));

    // Пол
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-3.0f, -0.5f, -3.0f);
    glVertex3f(3.0f, -0.5f, -3.0f);
    glVertex3f(3.0f, -0.5f, 3.0f);
    glVertex3f(-3.0f, -0.5f, 3.0f);
    glEnd();

    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES);
    for (int i = -3; i <= 3; i++) {
        glVertex3f((float)i, -0.45f, -3.0f);
        glVertex3f((float)i, -0.45f, 3.0f);
        glVertex3f(-3.0f, -0.45f, (float)i);
        glVertex3f(3.0f, -0.45f, (float)i);
    }
    glEnd();
    glEnable(GL_LIGHTING);

    // Змейка (простая модель)
    glPushMatrix();
    glTranslatef(0.0f, -0.2f, 0.0f);
    glScalef(0.6f, 0.6f, 0.6f);
    glColor3f(0.0f, 1.0f, 0.0f);

    // Голова (сфера из кубов)
    glBegin(GL_QUADS);
    for (int i = 0; i < 6; i++) {
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
    }
    glEnd();

    // Тело (цилиндр из квадов)
    glPushMatrix();
    glTranslatef(-0.8f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.7f, 0.0f);
    glBegin(GL_QUADS);
    for (int i = 0; i < 32; i++) {
        float angle = (float)i * 3.14159f * 2.0f / 32.0f;
        float x1 = 0.4f * cos(angle);
        float z1 = 0.4f * sin(angle);
        float x2 = 0.4f * cos(angle + 3.14159f * 2.0f / 32.0f);
        float z2 = 0.4f * sin(angle + 3.14159f * 2.0f / 32.0f);
        glVertex3f(x1, -0.5f, z1);
        glVertex3f(x2, -0.5f, z2);
        glVertex3f(x2, 0.5f, z2);
        glVertex3f(x1, 0.5f, z1);
    }
    glEnd();
    glPopMatrix();
    glPopMatrix();

    // Деревья
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef((float)i * 1.8f, -0.2f, -1.5f);
        glScalef(0.8f, 1.2f, 0.8f);
        glColor3f(0.5f, 0.3f, 0.1f);
        glBegin(GL_QUADS);
        glVertex3f(-0.25f, -0.5f, -0.25f);
        glVertex3f(0.25f, -0.5f, -0.25f);
        glVertex3f(0.25f, 0.5f, -0.25f);
        glVertex3f(-0.25f, 0.5f, -0.25f);
        glEnd();
        glColor3f(0.2f, 0.6f, 0.2f);
        glTranslatef(0.0f, 0.6f, 0.0f);
        glBegin(GL_TRIANGLES);
        for (int j = 0; j < 12; j++) {
            float angle = (float)j * 3.14159f * 2.0f / 12.0f;
            float x1 = 0.5f * cos(angle);
            float z1 = 0.5f * sin(angle);
            float x2 = 0.5f * cos(angle + 3.14159f * 2.0f / 12.0f);
            float z2 = 0.5f * sin(angle + 3.14159f * 2.0f / 12.0f);
            glVertex3f(0.0f, 0.3f, 0.0f);
            glVertex3f(x1, -0.2f, z1);
            glVertex3f(x2, -0.2f, z2);
        }
        glEnd();
        glPopMatrix();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    reset2DProjection();
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)previewX, (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)(previewY + previewH));
    glVertex2f((float)previewX, (float)(previewY + previewH));
    glEnd();

    drawText((float)(previewX + 10), (float)(previewY + 25), "ПРЕДПРОСМОТР ТЕНЕЙ", 1.0f, 1.0f, 0.0f);
    drawText((float)(previewX + 10), (float)(previewY + 50), currentConfig.getShadowModeName(), 0.8f, 0.8f, 1.0f);
}

void renderShadowEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "НАСТРОЙКИ ТЕНЕЙ", 1.0f, 1.0f, 0.0f);

    int startX = (int)(windowWidth * 0.05f);
    int startY = (int)(windowHeight * 0.12f);
    int sliderWidth = (int)(windowWidth * 0.25f);

    drawText((float)startX, (float)(startY - 30), "РЕЖИМ ТРАССИРОВКИ", 1.0f, 1.0f, 0.0f);

    const char* modes[] = { "CENTER (1 луч, бинарный)", "CORNERS (4 луча, градиент)",
                            "CENTER_SUBDIVIDED (адаптивный, бинарный)", "CORNERS_SUBDIVIDED (адаптивный, градиент)" };
    int modeBtnWidth = (int)(windowWidth * 0.4f);
    int modeBtnHeight = 40;

    for (int i = 0; i < 4; i++) {
        int modeX = startX + (i % 2) * (modeBtnWidth + 10);
        int modeY = startY + (i / 2) * (modeBtnHeight + 5);
        std::string btnText = std::string(modes[i]) + (currentConfig.shadowTraceMode == i ? " ✓" : "");
        if (drawButton(modeX, modeY, modeBtnWidth, modeBtnHeight, btnText.c_str())) {
            currentConfig.shadowTraceMode = i;
        }
    }

    startY += modeBtnHeight * 2 + 30;

    if (currentConfig.shadowTraceMode == 2 || currentConfig.shadowTraceMode == 3) {
        drawText((float)startX, (float)(startY - 30), "РАЗМЕР ПОДКЛЕТОК", 1.0f, 1.0f, 0.0f);
        char subdivText[50];
        sprintf_s(subdivText, "%d x %d", currentConfig.shadowSubdivisionSize, currentConfig.shadowSubdivisionSize);
        drawText((float)(startX + sliderWidth + 100), (float)(startY - 20), subdivText, 1.0f, 1.0f, 0.0f);
        drawIntSlider(startX, startY, sliderWidth, &currentConfig.shadowSubdivisionSize, 2, 20, "Подклетки");
        if (drawButton(startX + sliderWidth + 20, startY - 10, 150, 35, "СБРОСИТЬ (10x10)")) {
            currentConfig.shadowSubdivisionSize = 10;
        }
        startY += 80;
    }

    drawText((float)startX, (float)(startY - 30), "ШАГ ТЕНЕВОЙ СЕТКИ", 1.0f, 1.0f, 0.0f);
    char strideText[100];
    sprintf_s(strideText, "Stride X: %d, Stride Z: %d", currentConfig.shadowStrideX, currentConfig.shadowStrideZ);
    drawText((float)(startX + sliderWidth + 100), (float)(startY - 20), strideText, 1.0f, 1.0f, 0.0f);
    drawIntSlider(startX, startY, sliderWidth, &currentConfig.shadowStrideX, 1, 8, "Страйд X");
    drawIntSlider(startX, startY + 45, sliderWidth, &currentConfig.shadowStrideZ, 1, 8, "Страйд Z");
    if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ (2x2)")) {
        currentConfig.shadowStrideX = 2;
        currentConfig.shadowStrideZ = 2;
    }

    startY += 110;

    drawText((float)startX, (float)(startY - 30), "ДОПОЛНИТЕЛЬНЫЕ НАСТРОЙКИ", 1.0f, 1.0f, 0.0f);

    std::string shadowBtnText = std::string("Тени: ") + (currentConfig.shadowMapEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX, startY, 180, 40, shadowBtnText.c_str())) {
        currentConfig.shadowMapEnabled = !currentConfig.shadowMapEnabled;
    }

    std::string ambientBtnText = std::string("Ambient: ") + (currentConfig.ambientEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX + 200, startY, 180, 40, ambientBtnText.c_str())) {
        currentConfig.ambientEnabled = !currentConfig.ambientEnabled;
    }

    std::string specularBtnText = std::string("Specular: ") + (currentConfig.specularEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX + 400, startY, 180, 40, specularBtnText.c_str())) {
        currentConfig.specularEnabled = !currentConfig.specularEnabled;
    }

    // Информация
    int infoX = (int)(windowWidth * 0.05f);
    int infoY = (int)(windowHeight * 0.7f);
    int infoWidth = (int)(windowWidth * 0.5f);
    int infoHeight = (int)(windowHeight * 0.2f);

    glColor3f(0.2f, 0.2f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f((float)infoX, (float)infoY);
    glVertex2f((float)(infoX + infoWidth), (float)infoY);
    glVertex2f((float)(infoX + infoWidth), (float)(infoY + infoHeight));
    glVertex2f((float)infoX, (float)(infoY + infoHeight));
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)infoX, (float)infoY);
    glVertex2f((float)(infoX + infoWidth), (float)infoY);
    glVertex2f((float)(infoX + infoWidth), (float)(infoY + infoHeight));
    glVertex2f((float)infoX, (float)(infoY + infoHeight));
    glEnd();

    drawText((float)(infoX + 10), (float)(infoY + 25), "ИНФОРМАЦИЯ О РЕЖИМЕ", 1.0f, 1.0f, 0.0f);
    drawText((float)(infoX + 10), (float)(infoY + 55), currentConfig.getShadowModeName(), 0.8f, 0.8f, 1.0f);

    if (currentConfig.shadowTraceMode == 2 || currentConfig.shadowTraceMode == 3) {
        char subdivInfo[200];
        sprintf_s(subdivInfo, "Размер подклеток: %dx%d, Всего лучей на клетку: %d",
            currentConfig.shadowSubdivisionSize, currentConfig.shadowSubdivisionSize,
            currentConfig.shadowSubdivisionSize * currentConfig.shadowSubdivisionSize *
            (currentConfig.useCornerTrace() ? 4 : 1));
        drawText((float)(infoX + 10), (float)(infoY + 85), subdivInfo, 0.7f, 1.0f, 0.7f);
    }

    int shadowCellsX = currentConfig.gridWidth / currentConfig.shadowStrideX;
    int shadowCellsZ = currentConfig.gridDepth / currentConfig.shadowStrideZ;
    char cellsInfo[200];
    sprintf_s(cellsInfo, "Теневая сетка: %dx%d клеток (из %dx%d игровых)",
        shadowCellsX, shadowCellsZ, currentConfig.gridWidth, currentConfig.gridDepth);
    drawText((float)(infoX + 10), (float)(infoY + 115), cellsInfo, 0.7f, 1.0f, 0.7f);

    renderShadowPreview();

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// ГЛАВНОЕ МЕНЮ
//=============================================================================

void renderMainMenu() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.08f), "РЕДАКТОР КОНФИГУРАЦИИ ИГРЫ ЗМЕЙКА", 1.0f, 1.0f, 0.0f);

    int buttonWidth = (int)(windowWidth * 0.28f);
    int buttonHeight = (int)(windowHeight * 0.07f);
    int startY = (int)(windowHeight * 0.2f);
    int centerX = windowWidth / 2 - buttonWidth / 2;
    int spacing = (int)(windowHeight * 0.07f);

    if (drawButton(centerX, startY, buttonWidth, buttonHeight, "1. ЗМЕЯ")) {
        currentMode = MODE_SNAKE_EDITOR;
        selectedPart = 0;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing, buttonWidth, buttonHeight, "2. ПОЛ И НЕБО")) {
        currentMode = MODE_GROUND_SKY_EDITOR;
        selectedGroundSky = 0;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing * 2, buttonWidth, buttonHeight, "3. ПРЕГРАДЫ")) {
        currentMode = MODE_OBSTACLES_EDITOR;
        selectedObstacle = 0;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing * 3, buttonWidth, buttonHeight, "4. ОКРУЖЕНИЕ")) {
        currentMode = MODE_ENVIRONMENT_EDITOR;
        selectedEnv = 0;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing * 4, buttonWidth, buttonHeight, "5. ИГРОВОЕ ПОЛЕ")) {
        currentMode = MODE_GRID_EDITOR;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing * 5, buttonWidth, buttonHeight, "6. НАСТРОЙКИ СВЕТА")) {
        currentMode = MODE_LIGHT_EDITOR;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing * 6, buttonWidth, buttonHeight, "7. НАСТРОЙКИ ТЕНЕЙ")) {
        currentMode = MODE_SHADOW_EDITOR;
        updatePreviewForCurrentMode();
    }
    if (drawButton(centerX, startY + spacing * 7 + 20, buttonWidth, buttonHeight, "СОХРАНИТЬ И ВЫЙТИ")) {
        saveConfig();
        glfwSetWindowShouldClose(window, true);
    }

    char gridInfo[100];
    sprintf_s(gridInfo, "Сетка: %dx%d  Размер ячейки: %.2f  Полигонов пола: %d",
        currentConfig.gridWidth, currentConfig.gridDepth, currentConfig.cellSize,
        currentConfig.floorPolygonsX * currentConfig.floorPolygonsZ * 2);
    drawCenteredText((float)(windowHeight * 0.92f), gridInfo, 0.8f, 0.8f, 1.0f);

    char shadowInfo[100];
    sprintf_s(shadowInfo, "Тени: %s, Режим: %s",
        currentConfig.shadowMapEnabled ? "ВКЛ" : "ВЫКЛ",
        currentConfig.getShadowModeName());
    drawCenteredText((float)(windowHeight * 0.96f), shadowInfo, 0.7f, 0.7f, 0.7f);
}

//=============================================================================
// СОХРАНЕНИЕ КОНФИГА
//=============================================================================

void saveConfig() {
    if (snakeElements.size() >= 3) {
        currentConfig.snakeHeadModel = snakeElements[0]->modelFile;
        currentConfig.snakeHeadColor = snakeElements[0]->color;
        currentConfig.snakeHeadScale = snakeElements[0]->scale;
        currentConfig.snakeBodyModel = snakeElements[1]->modelFile;
        currentConfig.snakeBodyColor = snakeElements[1]->color;
        currentConfig.snakeBodyScale = snakeElements[1]->scale;
        currentConfig.snakeTailModel = snakeElements[2]->modelFile;
        currentConfig.snakeTailColor = snakeElements[2]->color;
        currentConfig.snakeTailScale = snakeElements[2]->scale;
    }
    if (groundSkyElements.size() >= 2) {
        currentConfig.floorModel = groundSkyElements[0]->modelFile;
        currentConfig.floorTexture = groundSkyElements[0]->textureFile;
        currentConfig.floorColor = groundSkyElements[0]->color;
        currentConfig.skyColor = groundSkyElements[1]->color;
    }
    if (obstaclesElements.size() >= 4) {
        currentConfig.treeModel = obstaclesElements[0]->modelFile;
        currentConfig.rockModel = obstaclesElements[1]->modelFile;
        currentConfig.fenceModel = obstaclesElements[2]->modelFile;
        currentConfig.appleModel = obstaclesElements[3]->modelFile;
        currentConfig.initialFoodCount = obstaclesElements[3]->count;
    }
    if (environmentElements.size() >= 3) {
        currentConfig.flowerModel = environmentElements[0]->modelFile;
        currentConfig.flowerCount = environmentElements[0]->count;
        currentConfig.birdModel = environmentElements[1]->modelFile;
        currentConfig.birdCount = environmentElements[1]->count;
        currentConfig.cloudModel = environmentElements[2]->modelFile;
        currentConfig.cloudCount = environmentElements[2]->count;
    }

    ConfigManager::saveGameConfig(g_configPath, currentConfig);
    std::cout << "✓ Config saved to: " << g_configPath << std::endl;
}

//=============================================================================
// КОЛБЭКИ
//=============================================================================

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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        if (currentMode != MODE_MAIN) {
            currentMode = MODE_MAIN;
        }
        else {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

//=============================================================================
// ИНИЦИАЛИЗАЦИЯ OPENGL
//=============================================================================

bool initOpenGL() {
    if (!glfwInit()) {
        std::cerr << "Не удалось инициализировать GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Редактор конфигурации игры Змейка", NULL, NULL);
    if (!window) {
        std::cerr << "Не удалось создать окно" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Не удалось инициализировать GLEW" << std::endl;
        return false;
    }

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initFreeType();
    initPaths();

    ConfigManager::loadGameConfig(g_configPath, currentConfig);
    initElements();

    return true;
}

//=============================================================================
// MAIN
//=============================================================================

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::cout << "\n=========================================" << std::endl;
    std::cout << "=     РЕДАКТОР КОНФИГУРАЦИИ ИГРЫ      =" << std::endl;
    std::cout << "=========================================\n" << std::endl;

    if (!initOpenGL()) return -1;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        switch (currentMode) {
        case MODE_MAIN: renderMainMenu(); break;
        case MODE_SNAKE_EDITOR: renderSnakeEditor(); break;
        case MODE_GROUND_SKY_EDITOR: renderGroundSkyEditor(); break;
        case MODE_OBSTACLES_EDITOR: renderObstaclesEditor(); break;
        case MODE_ENVIRONMENT_EDITOR: renderEnvironmentEditor(); break;
        case MODE_GRID_EDITOR: renderGridEditor(); break;
        case MODE_SHADOW_EDITOR: renderShadowEditor(); break;
        case MODE_LIGHT_EDITOR: renderLightEditor(); break;
        }

        glfwSwapBuffers(window);

        // Обновление состояния кнопки мыши
        if (!mousePressed) mousePressedLast = false;
    }

    cleanupFreeType();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "=         РАБОТА ЗАВЕРШЕНА             =" << std::endl;
    std::cout << "=========================================\n" << std::endl;

    return 0;
}