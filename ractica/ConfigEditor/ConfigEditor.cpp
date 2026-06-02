#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/freeglut.h>
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
// Добавить после существующих includes
// Добавить после других includes:
#include "../Graphics/Model.h"
#include "../Primitives/PrimitiveBase.h"
#include "../Primitives/ApplePrimitive.h"
#include "../Primitives/TreePrimitive.h"
#include "../Primitives/CloudPrimitive.h"
#include "../Primitives/BirdPrimitive.h"
#include "../Primitives/FlowerPrimitive.h"
#include "../Primitives/FencePrimitive.h"
#include "../Primitives/SnakeHeadPrimitive.h"
#include "../Primitives/SnakeBodyPrimitive.h"
#include "../Primitives/SnakeTailPrimitive.h"
#include <functional>
#include "../Graphics/ShadowMapper.h"
static std::map<const Model*, std::pair<glm::vec3, float>> g_boundsCache;
static bool g_forceBoundsRecalc = true;
// Глобальные переменные для теней в редакторе
static ShadowMapper g_previewStaticShadow;
static ShadowMapper g_previewDynamicShadow;
static ShadowMapper g_previewFoodShadow;
static bool g_previewShadowsInitialized = false;
// После существующих глобальных переменных добавить:
// Модели для предпросмотра (как в GameRenderer)
Model g_previewSnakeHeadModel;
Model g_previewSnakeBodyModel;
Model g_previewSnakeTailModel;
Model g_previewAppleModel;
Model g_previewTreeModel;
Model g_previewCloudModel;
Model g_previewBirdModel;
Model g_previewFlowerModel;
Model g_previewFenceModel;
Model g_previewFloorModel;
GameConfig currentConfig;
static std::vector<DebugRay> g_debugRaysFromShadowMapper;
static bool g_rayDebugEnabled = true;
static bool g_use3DPreviewForFloor = false;
// После существующих глобальных переменных добавить:
static bool g_previewShadowsDirty = true;  // Флаг для пересчёта теней
static int g_lastShadowTraceMode = -1;     // Последний использованный режим
static bool g_lastShadowMapEnabled = false; // Последнее состояние теней
static int g_lastShadowStrideX = 0;         // Последний stride X
static int g_lastShadowStrideZ = 0;         // Последний stride Z
static int g_lastShadowSubdivisionSize = 0; // Последний размер подразбиения
void renderShadowPreview3D();
//=============================================================================
// ОПРЕДЕЛЕНИЯ СТРУКТУР
//=============================================================================
// Добавить после функции loadFBXModel:
// Добавить после других функций:
// Функция инициализации света (вызвать ОДИН раз в initOpenGL())
// Добавить после initElements()
// Флаг для режима предпросмотра

struct DebugRay3D {
    glm::vec3 origin;
    glm::vec3 direction;
    glm::vec3 hitPoint;
    bool hit;
    float distance;
    std::string objectType; // "tree", "snake", "apple"
};

static std::vector<DebugRay3D> g_debugRays3D;
enum PreviewMode {
    PREVIEW_NORMAL,
    PREVIEW_SHADOWS,
    PREVIEW_LIGHT
};
static PreviewMode g_currentPreviewMode = PREVIEW_NORMAL;
// Добавить после других функций
float getModelBottomOffset(const Model& model) {
    if (model.vertices.empty()) return 0.0f;
    float minY = FLT_MAX;
    for (const auto& v : model.vertices) {
        minY = std::min(minY, v.position.y);
    }
    return -minY;
}
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    // Включаем GL_COLOR_MATERIAL для автоматического затенения
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Начальные настройки (потом обновятся в updateLightPosition)
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
}
// Добавить после функций rayIntersectsSphere
void clearBoundsCache() {
    g_boundsCache.clear();
    g_forceBoundsRecalc = true;
    std::cout << "Bounds cache cleared" << std::endl;
}
// Функция обновления позиции света - должна вызываться с единичной ModelView матрицей
// Функция обновления позиции света - как в GameRenderer
void updateLightPosition() {
    switch (currentConfig.lightType) {
    case 0: { // Directional
        GLfloat light0_position[] = { -currentConfig.lightDir.x, -currentConfig.lightDir.y, -currentConfig.lightDir.z, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        break;
    }
    case 1: { // Point
        GLfloat light0_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y, currentConfig.lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        break;
    }
    case 2: { // Spot
        GLfloat light0_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y, currentConfig.lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        GLfloat spot_direction[] = { 0.0f, -1.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
        break;
    }
    }
}

bool convertModelDataToModel(const ModelData& modelData, Model& outModel) {
    if (modelData.vertices.empty()) {
        return false;
    }

    outModel.vertices.clear();
    outModel.hasTexture = false;
    outModel.textureID = 0;

    size_t vertexCount = modelData.vertices.size() / 3;
    bool hasNormals = !modelData.normals.empty();
    bool hasTexCoords = !modelData.texCoords.empty();

    for (size_t i = 0; i < vertexCount; i++) {
        Vertex vertex;

        vertex.position.x = modelData.vertices[i * 3];
        vertex.position.y = modelData.vertices[i * 3 + 1];
        vertex.position.z = modelData.vertices[i * 3 + 2];

        if (hasNormals && i * 3 + 2 < modelData.normals.size()) {
            vertex.normal.x = modelData.normals[i * 3];
            vertex.normal.y = modelData.normals[i * 3 + 1];
            vertex.normal.z = modelData.normals[i * 3 + 2];
        }
        else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (hasTexCoords && i * 2 + 1 < modelData.texCoords.size()) {
            vertex.texCoords.x = modelData.texCoords[i * 2];
            vertex.texCoords.y = modelData.texCoords[i * 2 + 1];
        }
        else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        outModel.vertices.push_back(vertex);
    }

    if (!modelData.materials.empty() && modelData.materials[0].textureID != 0) {
        outModel.textureID = modelData.materials[0].textureID;
        outModel.hasTexture = true;
    }

    outModel.computeNormals();
    outModel.setupBuffers();

    return true;
}


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
    bool useTextureColor;
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

// Управление предпросмотром
float previewCameraDistance = 6.0f;
float previewCameraPitch = 25.0f;
bool previewAutoRotate = true;
float previewRotationAngle = 0.0f;
float previewLastRotateTime = 0.0f;
bool previewMouseRotating = false;
double previewLastMouseX = 0, previewLastMouseY = 0;
int previewHoverX = 0, previewHoverY = 0, previewHoverW = 0, previewHoverH = 0;
bool previewHovered = false;

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

//=============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
//=============================================================================
void setupFixedPipelineLighting();
void renderMainMenu();
void renderSnakeEditor();
void renderGroundSkyEditor();
void renderObstaclesEditor();
void renderEnvironmentEditor();
void renderGridEditor();
void renderShadowEditor();
void renderLightEditor();
void renderShadowPreview();
// ========== ПРОТОТИПЫ ФУНКЦИЙ ДЛЯ ОТЛАДКИ ЛУЧЕЙ ==========
void clearDebugRays3D();
void addDebugRay3D(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& hitPoint, bool hit, float distance,
    const std::string& objectType);
void drawDebugRay3D(const DebugRay3D& ray);
void generateRaysForModel(const glm::vec3& modelCenter, float modelRadius,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType,
    std::function<bool(const glm::vec3&, float&, glm::vec3&)> intersectFunc,
    const std::string& objectType);
void generateRaysForTree(const glm::vec3& treePos, float treeScale, float treeOffset,
    const Model& treeModel,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType);
void generateRaysForSnakeSegment(const glm::vec3& pos, const Model& model,
    float scale, float offset, float rotation,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType, const std::string& segmentName);
void generateRaysForApple(const glm::vec3& applePos, float appleScale, float appleOffset,
    const Model& appleModel,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType);
// Вместо старого объявления, замените на:
void renderModelPreview(ModelData& model, const char* title, float x, float y, float w, float h,
    float& rotation, bool& autoRotate, float& lastTime, float scale, bool isFloor,
    const glm::vec3& customColor = glm::vec3(1.0f));
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
bool loadFBXModel(const std::string& filename, Model& outModel, const std::string& subFolder);
void drawModel(ModelData& model, const glm::vec3& customColor = glm::vec3(1.0f));
void reset2DProjection();
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void windowSizeCallback(GLFWwindow* window, int width, int height);
bool initOpenGL();
std::string truncateFilename(const std::string& filename, int maxLen = 30);
GLuint loadTextureFromFile(const std::string& path);  // Добавить прототип
//=============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
//=============================================================================
// Функция для загрузки модели с fallback на примитив
// ========== ИСПРАВЛЕННАЯ loadModelWithFallback ==========
//=============================================================================
// РЕДАКТОР ТЕНЕЙ
//=============================================================================
// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ВЫЧИСЛЕНИЯ BOUNDING BOX ==========

struct ModelBounds {
    glm::vec3 center;
    float radius;
    float minY;
    float maxY;
};

ModelBounds computeModelBounds(const Model& model) {
    ModelBounds bounds;
    bounds.center = glm::vec3(0.0f);
    bounds.radius = 0.5f;
    bounds.minY = 0.0f;
    bounds.maxY = 0.0f;

    if (model.vertices.empty()) return bounds;

    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;

    for (const auto& vert : model.vertices) {
        minX = std::min(minX, vert.position.x);
        maxX = std::max(maxX, vert.position.x);
        minY = std::min(minY, vert.position.y);
        maxY = std::max(maxY, vert.position.y);
        minZ = std::min(minZ, vert.position.z);
        maxZ = std::max(maxZ, vert.position.z);
    }

    bounds.center = glm::vec3((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
    bounds.radius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
    bounds.minY = minY;
    bounds.maxY = maxY;

    return bounds;
}

float getModelBottomOffsetLocal(const Model& model) {
    if (model.vertices.empty()) return 0.0f;
    float minY = FLT_MAX;
    for (const auto& vert : model.vertices) {
        minY = std::min(minY, vert.position.y);
    }
    return -minY;
}

// Функция для поворота сегмента змейки (как в GameRenderer)

// ========== ИСПРАВЛЕННАЯ ФУНКЦИЯ ДЛЯ ОТЛАДОЧНЫХ ЛУЧЕЙ ==========

void drawDebugRayFromShadowMapper(const DebugRay& ray) {
    glm::vec3 endPoint = ray.hitPoint;

    // УБИРАЕМ ЛЮБЫЕ СМЕЩЕНИЯ!
    // Не добавляем 0.05f, не меняем координаты

    // Луч: красный если попал в модель (тень), зелёный если не попал (свет)
    if (ray.hit) {
        glColor3f(1.0f, 0.2f, 0.2f);  // Красный - попал в объект (тень)
    }
    else {
        glColor3f(1.0f, 1.0f, 0.2f);  // Зелёный - достиг света (нет тени)
    }

    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
    glVertex3f(endPoint.x, endPoint.y, endPoint.z);
    glEnd();
    glLineWidth(1.0f);
}
void renderShadowEditor() {
    g_currentPreviewMode = PREVIEW_SHADOWS;
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "НАСТРОЙКИ ТЕНЕЙ", 1.0f, 1.0f, 0.0f);

    int startX = (int)(windowWidth * 0.03f);
    int startY = (int)(windowHeight * 0.12f);
    int sliderWidth = (int)(windowWidth * 0.25f);

    drawText((float)startX, (float)(startY - 30), "РЕЖИМ ТРАССИРОВКИ", 1.0f, 1.0f, 0.0f);

    const char* modes[] = { "CENTER (1 луч, бинарный)",
                            "CORNERS (4 луча, градиент)",
                            "CENTER_SUBDIVIDED (адаптивный, бинарный)",
                            "CORNERS_SUBDIVIDED (адаптивный, градиент)" };
    int modeBtnWidth = (int)(windowWidth * 0.35f);
    int modeBtnHeight = 35;

    for (int i = 0; i < 4; i++) {
        int modeX = startX + (i % 2) * (modeBtnWidth + 10);
        int modeY = startY + (i / 2) * (modeBtnHeight + 5);
        std::string btnText = std::string(modes[i]) + (currentConfig.shadowTraceMode == i ? " ✓" : "");
        if (drawButton(modeX, modeY, modeBtnWidth, modeBtnHeight, btnText.c_str())) {
            if (currentConfig.shadowTraceMode != i) {
                currentConfig.shadowTraceMode = i;
                g_previewShadowsInitialized = false;
                std::cout << "Shadow mode changed to " << i << ", marking shadows as DIRTY" << std::endl;
            }
        }
    }

    startY += modeBtnHeight * 2 + 30;

    if (currentConfig.shadowTraceMode == 2 || currentConfig.shadowTraceMode == 3) {
        drawText((float)startX, (float)(startY - 30), "РАЗМЕР ПОДКЛЕТОК", 1.0f, 1.0f, 0.0f);
        char subdivText[50];
        sprintf_s(subdivText, "%d x %d", currentConfig.shadowSubdivisionSize, currentConfig.shadowSubdivisionSize);
        drawText((float)(startX + sliderWidth + 100), (float)(startY - 20), subdivText, 1.0f, 1.0f, 0.0f);

        int oldSubdiv = currentConfig.shadowSubdivisionSize;
        drawIntSlider(startX, startY, sliderWidth, &currentConfig.shadowSubdivisionSize, 2, 20, "Подклетки");
        if (oldSubdiv != currentConfig.shadowSubdivisionSize) {
            g_previewShadowsInitialized = false;
        }

        if (drawButton(startX + sliderWidth + 20, startY - 10, 150, 35, "СБРОСИТЬ (10x10)")) {
            if (currentConfig.shadowSubdivisionSize != 10) {
                currentConfig.shadowSubdivisionSize = 10;
                g_previewShadowsInitialized = false;
            }
        }
        startY += 80;
    }

    drawText((float)startX, (float)(startY - 30), "ШАГ ТЕНЕВОЙ СЕТКИ", 1.0f, 1.0f, 0.0f);
    char strideText[100];
    sprintf_s(strideText, "Stride X: %d, Stride Z: %d", currentConfig.shadowStrideX, currentConfig.shadowStrideZ);
    drawText((float)(startX + sliderWidth + 100), (float)(startY - 20), strideText, 1.0f, 1.0f, 0.0f);

    int oldStrideX = currentConfig.shadowStrideX;
    int oldStrideZ = currentConfig.shadowStrideZ;
    drawIntSlider(startX, startY, sliderWidth, &currentConfig.shadowStrideX, 1, 8, "Страйд X");
    drawIntSlider(startX, startY + 45, sliderWidth, &currentConfig.shadowStrideZ, 1, 8, "Страйд Z");
    if (oldStrideX != currentConfig.shadowStrideX || oldStrideZ != currentConfig.shadowStrideZ) {
        g_previewShadowsInitialized = false;
    }

    if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ (2x2)")) {
        if (currentConfig.shadowStrideX != 2 || currentConfig.shadowStrideZ != 2) {
            currentConfig.shadowStrideX = 2;
            currentConfig.shadowStrideZ = 2;
            g_previewShadowsInitialized = false;
        }
    }

    startY += 110;

    drawText((float)startX, (float)(startY - 30), "ДОПОЛНИТЕЛЬНЫЕ НАСТРОЙКИ", 1.0f, 1.0f, 0.0f);

    std::string shadowBtnText = std::string("Тени: ") + (currentConfig.shadowMapEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX, startY, 150, 40, shadowBtnText.c_str())) {
        bool newState = !currentConfig.shadowMapEnabled;
        if (currentConfig.shadowMapEnabled != newState) {
            currentConfig.shadowMapEnabled = newState;
            g_previewShadowsInitialized = false;
        }
    }

    std::string ambientBtnText = std::string("Ambient: ") + (currentConfig.ambientEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX + 170, startY, 150, 40, ambientBtnText.c_str())) {
        currentConfig.ambientEnabled = !currentConfig.ambientEnabled;
        setupFixedPipelineLighting();
        updateLightPosition();
    }

    std::string specularBtnText = std::string("Specular: ") + (currentConfig.specularEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX + 340, startY, 150, 40, specularBtnText.c_str())) {
        currentConfig.specularEnabled = !currentConfig.specularEnabled;
        setupFixedPipelineLighting();
        updateLightPosition();
    }

    // Кнопка для отладки лучей
    std::string debugRaysBtnText = std::string("Отладка лучей: ") + (g_rayDebugEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawButton(startX, startY + 60, 200, 40, debugRaysBtnText.c_str())) {
        g_rayDebugEnabled = !g_rayDebugEnabled;
        g_previewShadowsInitialized = false;  // Пересчитать при изменении
        if (!g_rayDebugEnabled) {
            g_debugRaysFromShadowMapper.clear();
        }
    }

    // 3D предпросмотр теней
    renderShadowPreview3D();

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.92f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}
bool loadModelWithFallback(const std::string& filename, Model& targetModel,
    const std::string& subFolder,
    std::function<void(Model&)> createPrimitive) {

    if (!filename.empty()) {
        if (loadFBXModel(filename, targetModel, subFolder)) {
            return true;
        }
    }

    if (createPrimitive) {
        createPrimitive(targetModel);
        targetModel.computeNormals();  // ВАЖНО!
        targetModel.setupBuffers();     // ВАЖНО!
        return true;
    }
    return false;
}
std::string extractFilename(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) return path.substr(pos + 1);
    return path;
}

std::string truncateFilename(const std::string& filename, int maxLen) {
    if (filename.length() <= maxLen) return filename;
    return "..." + filename.substr(filename.length() - (maxLen - 3));
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
    // Очищаем текущую модель предпросмотра
    previewModel.vertices.clear();
    previewModel.normals.clear();
    previewModel.texCoords.clear();
    previewModel.materials.clear();
    previewModel.loaded = false;

    switch (currentMode) {
    case MODE_SNAKE_EDITOR: {
        if (selectedPart >= 0 && selectedPart < (int)snakeElements.size()) {
            std::string folder = (selectedPart == 0) ? "snake_head" :
                (selectedPart == 1) ? "snake_body" : "snake_tail";
            std::string filename = snakeElements[selectedPart]->modelFile;

            bool loaded = false;
            if (!filename.empty()) {
                Model tempModel;
                if (loadFBXModel(filename, tempModel, folder)) {
                    // Конвертируем Model в ModelData для предпросмотра
                    for (const auto& v : tempModel.vertices) {
                        previewModel.vertices.push_back(v.position.x);
                        previewModel.vertices.push_back(v.position.y);
                        previewModel.vertices.push_back(v.position.z);
                        previewModel.normals.push_back(v.normal.x);
                        previewModel.normals.push_back(v.normal.y);
                        previewModel.normals.push_back(v.normal.z);
                        previewModel.texCoords.push_back(v.texCoords.x);
                        previewModel.texCoords.push_back(v.texCoords.y);
                    }

                    // Сохраняем информацию о текстуре
                    if (tempModel.hasTexture && tempModel.textureID != 0) {
                        Material mat;
                        mat.textureID = tempModel.textureID;
                        previewModel.materials.push_back(mat);
                        std::cout << "Texture loaded with ID: " << tempModel.textureID << std::endl;
                    }

                    previewModel.loaded = true;
                    loaded = true;
                    std::cout << "Loaded FBX model: " << filename << " with " << previewModel.vertices.size() / 3 << " vertices" << std::endl;
                }
            }

            if (!loaded) {
                Model tempModel;
                if (selectedPart == 0) SnakeHeadPrimitive::create(tempModel);
                else if (selectedPart == 1) SnakeBodyPrimitive::create(tempModel);
                else SnakeTailPrimitive::create(tempModel);

                tempModel.computeNormals();
                tempModel.setupBuffers();

                for (const auto& v : tempModel.vertices) {
                    previewModel.vertices.push_back(v.position.x);
                    previewModel.vertices.push_back(v.position.y);
                    previewModel.vertices.push_back(v.position.z);
                    previewModel.normals.push_back(v.normal.x);
                    previewModel.normals.push_back(v.normal.y);
                    previewModel.normals.push_back(v.normal.z);
                    previewModel.texCoords.push_back(v.texCoords.x);
                    previewModel.texCoords.push_back(v.texCoords.y);
                }
                previewModel.loaded = true;
                std::cout << "Created primitive for: " << snakeElements[selectedPart]->name << std::endl;
            }
        }
        break;
    }
    case MODE_GROUND_SKY_EDITOR: {
        if (selectedGroundSky == 0 && !groundSkyElements.empty()) {
            VisualElement* ground = groundSkyElements[0];
            std::string filename = ground->modelFile;
            bool loaded = false;

            std::cout << "=== LOADING FLOOR MODEL ===" << std::endl;
            std::cout << "Filename: " << filename << std::endl;
            std::cout << "Texture file: " << ground->textureFile << std::endl;

            if (!filename.empty()) {
                Model tempModel;
                // Пробуем загрузить модель пола
                if (loadFBXModel(filename, tempModel, "floor")) {
                    std::cout << "FBX model loaded successfully, vertices: " << tempModel.vertices.size() << std::endl;

                    // Конвертируем Model в ModelData для предпросмотра
                    for (const auto& v : tempModel.vertices) {
                        previewModel.vertices.push_back(v.position.x);
                        previewModel.vertices.push_back(v.position.y);
                        previewModel.vertices.push_back(v.position.z);
                        previewModel.normals.push_back(v.normal.x);
                        previewModel.normals.push_back(v.normal.y);
                        previewModel.normals.push_back(v.normal.z);
                        previewModel.texCoords.push_back(v.texCoords.x);
                        previewModel.texCoords.push_back(v.texCoords.y);
                    }

                    // Сначала проверяем, есть ли текстура в модели
                    bool textureFound = false;
                    if (tempModel.hasTexture && tempModel.textureID != 0) {
                        Material mat;
                        mat.textureID = tempModel.textureID;
                        previewModel.materials.push_back(mat);
                        textureFound = true;
                        std::cout << "Floor texture from model loaded with ID: " << tempModel.textureID << std::endl;
                    }

                    // Если текстуры нет в модели, пробуем найти JPG/PNG по имени модели
                    if (!textureFound && !filename.empty()) {
                        // Получаем имя файла без расширения
                        std::string baseName = filename;
                        size_t dotPos = baseName.find_last_of(".");
                        if (dotPos != std::string::npos) {
                            baseName = baseName.substr(0, dotPos);
                        }

                        std::cout << "Searching for texture by name: " << baseName << std::endl;

                        // Расширенный поиск текстуры
                        std::vector<std::string> searchPaths = {
                            // Основные пути
                            g_texturesPath + baseName + ".jpg",
                            g_texturesPath + baseName + ".png",
                            g_texturesPath + baseName + ".jpeg",
                            g_texturesPath + baseName + ".tga",
                            g_texturesPath + baseName + ".bmp",
                            // Путь с подпапкой floor
                            g_texturesPath + "floor\\" + baseName + ".jpg",
                            g_texturesPath + "floor\\" + baseName + ".png",
                            g_texturesPath + "floor\\" + baseName + ".jpeg",
                            // Путь с подпапкой textures
                            g_assetsPath + "textures\\" + baseName + ".jpg",
                            g_assetsPath + "textures\\" + baseName + ".png",
                            // Путь рядом с моделью
                            g_modelsPath + "floor\\" + baseName + ".jpg",
                            g_modelsPath + "floor\\" + baseName + ".png",
                            // Путь с сохранением оригинального имени (с расширением)
                            g_texturesPath + filename + ".jpg",
                            g_texturesPath + filename + ".png",
                            g_modelsPath + "floor\\" + filename + ".jpg",
                            g_modelsPath + "floor\\" + filename + ".png"
                        };

                        for (const auto& texPath : searchPaths) {
                            if (std::filesystem::exists(texPath)) {
                                GLuint texID = loadTextureFromFile(texPath);
                                if (texID != 0) {
                                    Material mat;
                                    mat.textureID = texID;
                                    previewModel.materials.push_back(mat);
                                    textureFound = true;
                                    std::cout << "Floor texture loaded from: " << texPath << std::endl;
                                    break;
                                }
                            }
                        }

                        if (!textureFound) {
                            std::cout << "No texture found for: " << baseName << std::endl;
                            std::cout << "Searched paths:" << std::endl;
                            for (const auto& path : searchPaths) {
                                std::cout << "  - " << path << std::endl;
                            }
                        }
                    }

                    // Если всё ещё нет текстуры, проверяем отдельно загруженную текстуру
                    if (!textureFound && !ground->textureFile.empty() && floorTexture.id != 0) {
                        Material mat;
                        mat.textureID = floorTexture.id;
                        previewModel.materials.push_back(mat);
                        textureFound = true;
                        std::cout << "Floor texture from separate file loaded with ID: " << floorTexture.id << std::endl;
                    }

                    previewModel.loaded = true;
                    loaded = true;
                    std::cout << "Floor model loaded and converted to previewModel" << std::endl;
                }
                else {
                    std::cout << "Failed to load FBX model: " << filename << std::endl;
                }
            }
            else {
                std::cout << "Filename is empty, will use primitive" << std::endl;
            }

            if (!loaded) {
                // Создаём простой квадрат для пола (примитив)
                std::cout << "Creating floor primitive" << std::endl;
                Model tempModel;
                Vertex v1, v2, v3, v4;
                v1.position = glm::vec3(-0.5f, 0.0f, -0.5f);
                v2.position = glm::vec3(0.5f, 0.0f, -0.5f);
                v3.position = glm::vec3(0.5f, 0.0f, 0.5f);
                v4.position = glm::vec3(-0.5f, 0.0f, 0.5f);
                v1.normal = v2.normal = v3.normal = v4.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                tempModel.vertices = { v1, v2, v3, v1, v3, v4 };
                tempModel.computeNormals();
                tempModel.setupBuffers();

                for (const auto& v : tempModel.vertices) {
                    previewModel.vertices.push_back(v.position.x);
                    previewModel.vertices.push_back(v.position.y);
                    previewModel.vertices.push_back(v.position.z);
                    previewModel.normals.push_back(v.normal.x);
                    previewModel.normals.push_back(v.normal.y);
                    previewModel.normals.push_back(v.normal.z);
                }
                previewModel.loaded = true;
                std::cout << "Floor primitive created" << std::endl;
            }

            std::cout << "Final previewModel state: loaded=" << previewModel.loaded
                << ", vertices=" << previewModel.vertices.size()
                << ", textures=" << previewModel.materials.size() << std::endl;
        }
        break;
    }
    case MODE_OBSTACLES_EDITOR: {
        if (selectedObstacle >= 0 && selectedObstacle < (int)obstaclesElements.size()) {
            std::string filename = obstaclesElements[selectedObstacle]->modelFile;
            bool loaded = false;

            if (!filename.empty()) {
                Model tempModel;
                if (loadFBXModel(filename, tempModel, "obstacles")) {
                    for (const auto& v : tempModel.vertices) {
                        previewModel.vertices.push_back(v.position.x);
                        previewModel.vertices.push_back(v.position.y);
                        previewModel.vertices.push_back(v.position.z);
                        previewModel.normals.push_back(v.normal.x);
                        previewModel.normals.push_back(v.normal.y);
                        previewModel.normals.push_back(v.normal.z);
                        previewModel.texCoords.push_back(v.texCoords.x);
                        previewModel.texCoords.push_back(v.texCoords.y);
                    }

                    if (tempModel.hasTexture && tempModel.textureID != 0) {
                        Material mat;
                        mat.textureID = tempModel.textureID;
                        previewModel.materials.push_back(mat);
                        std::cout << "Texture loaded for obstacle with ID: " << tempModel.textureID << std::endl;
                    }

                    previewModel.loaded = true;
                    loaded = true;
                    std::cout << "Loaded obstacle model: " << filename << std::endl;
                }
            }

            if (!loaded) {
                Model tempModel;
                if (selectedObstacle == 0) {
                    TreePrimitive::create(tempModel);
                }
                else if (selectedObstacle == 1) {
                    Vertex v;
                    v.position = glm::vec3(0.0f, 0.0f, 0.0f);
                    v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            for (int k = -1; k <= 1; k++) {
                                v.position = glm::vec3(i * 0.3f, j * 0.3f + 0.3f, k * 0.3f);
                                tempModel.vertices.push_back(v);
                            }
                        }
                    }
                }
                else if (selectedObstacle == 2) {
                    FencePrimitive::create(tempModel);
                }
                else {
                    ApplePrimitive::create(tempModel);
                }
                tempModel.computeNormals();
                tempModel.setupBuffers();

                for (const auto& v : tempModel.vertices) {
                    previewModel.vertices.push_back(v.position.x);
                    previewModel.vertices.push_back(v.position.y);
                    previewModel.vertices.push_back(v.position.z);
                    previewModel.normals.push_back(v.normal.x);
                    previewModel.normals.push_back(v.normal.y);
                    previewModel.normals.push_back(v.normal.z);
                }
                previewModel.loaded = true;
                std::cout << "Created primitive for obstacle: " << obstaclesElements[selectedObstacle]->name << std::endl;
            }
        }
        break;
    }
    case MODE_ENVIRONMENT_EDITOR: {
        if (selectedEnv >= 0 && selectedEnv < (int)environmentElements.size()) {
            std::string folder = (selectedEnv == 0) ? "flowers" :
                (selectedEnv == 1) ? "birds" : "clouds";
            std::string filename = environmentElements[selectedEnv]->modelFile;
            bool loaded = false;

            if (!filename.empty()) {
                Model tempModel;
                if (loadFBXModel(filename, tempModel, folder)) {
                    for (const auto& v : tempModel.vertices) {
                        previewModel.vertices.push_back(v.position.x);
                        previewModel.vertices.push_back(v.position.y);
                        previewModel.vertices.push_back(v.position.z);
                        previewModel.normals.push_back(v.normal.x);
                        previewModel.normals.push_back(v.normal.y);
                        previewModel.normals.push_back(v.normal.z);
                        previewModel.texCoords.push_back(v.texCoords.x);
                        previewModel.texCoords.push_back(v.texCoords.y);
                    }

                    if (tempModel.hasTexture && tempModel.textureID != 0) {
                        Material mat;
                        mat.textureID = tempModel.textureID;
                        previewModel.materials.push_back(mat);
                        std::cout << "Texture loaded for environment with ID: " << tempModel.textureID << std::endl;
                    }

                    previewModel.loaded = true;
                    loaded = true;
                    std::cout << "Loaded environment model: " << filename << std::endl;
                }
            }

            if (!loaded) {
                Model tempModel;
                if (selectedEnv == 0) {
                    FlowerPrimitive::create(tempModel);
                }
                else if (selectedEnv == 1) {
                    BirdPrimitive::create(tempModel);
                }
                else {
                    CloudPrimitive::create(tempModel);
                }
                tempModel.computeNormals();
                tempModel.setupBuffers();

                for (const auto& v : tempModel.vertices) {
                    previewModel.vertices.push_back(v.position.x);
                    previewModel.vertices.push_back(v.position.y);
                    previewModel.vertices.push_back(v.position.z);
                    previewModel.normals.push_back(v.normal.x);
                    previewModel.normals.push_back(v.normal.y);
                    previewModel.normals.push_back(v.normal.z);
                    previewModel.texCoords.push_back(v.texCoords.x);
                    previewModel.texCoords.push_back(v.texCoords.y);
                }
                previewModel.loaded = true;
                std::cout << "Created primitive for environment: " << environmentElements[selectedEnv]->name << std::endl;
            }
        }
        break;
    }
    default: break;
    }
}
void renderModelPreviewFromModel(Model& model, const char* title, float x, float y, float w, float h,
    float& rotation, bool& autoRotate, float& lastTime, float scale, bool isFloor) {

    previewHoverX = (int)x;
    previewHoverY = (int)y;
    previewHoverW = (int)w;
    previewHoverH = (int)h;

    previewHovered = (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);

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

    float radPitch = glm::radians(previewCameraPitch);
    float radYaw = glm::radians(previewRotationAngle);

    glm::vec3 center(0.0f, 0.0f, 0.0f);
    float camX = sin(radYaw) * cos(radPitch) * previewCameraDistance;
    float camY = sin(radPitch) * previewCameraDistance;
    float camZ = cos(radYaw) * cos(radPitch) * previewCameraDistance;
    glm::vec3 eye(camX, camY + 1.0f, camZ);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));

    // Пол
    float worldWidth = currentConfig.gridWidth * currentConfig.cellSize;
    float worldDepth = currentConfig.gridDepth * currentConfig.cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    glDisable(GL_LIGHTING);
    glColor3f(currentConfig.floorColor.r, currentConfig.floorColor.g, currentConfig.floorColor.b);
    glBegin(GL_QUADS);
    glVertex3f(-offsetX, -0.02f, -offsetZ);
    glVertex3f(offsetX, -0.02f, -offsetZ);
    glVertex3f(offsetX, -0.02f, offsetZ);
    glVertex3f(-offsetX, -0.02f, offsetZ);
    glEnd();

    if (currentConfig.gridEnabled) {
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
        glLineWidth(currentConfig.gridLineWidth);
        glBegin(GL_LINES);
        for (int i = -currentConfig.gridWidth / 2; i <= currentConfig.gridWidth / 2; i++) {
            float xPos = (float)i * currentConfig.cellSize;
            glVertex3f(xPos, -0.01f, -offsetZ);
            glVertex3f(xPos, -0.01f, offsetZ);
        }
        for (int i = -currentConfig.gridDepth / 2; i <= currentConfig.gridDepth / 2; i++) {
            float zPos = (float)i * currentConfig.cellSize;
            glVertex3f(-offsetX, -0.01f, zPos);
            glVertex3f(offsetX, -0.01f, zPos);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    // Модель
    if (model.isCompiled || !model.vertices.empty()) {
        glPushMatrix();

        if (isFloor) {
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glScalef(0.01f, 0.01f, 0.01f);
        }
        else {
            float minY = FLT_MAX;
            for (const auto& v : model.vertices) {
                minY = std::min(minY, v.position.y);
            }
            glTranslatef(0.0f, -minY + 0.05f, 0.0f);
            glScalef(scale, scale, scale);
        }

        if (model.hasTexture && model.textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, model.textureID);
        }

        model.draw();

        if (model.hasTexture && model.textureID != 0) {
            glDisable(GL_TEXTURE_2D);
        }
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    reset2DProjection();

    // UI Рамка и кнопки...
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    drawText(x + 10, y + 25, title, 1.0f, 1.0f, 0.0f);

    char controlsInfo[200];
    sprintf_s(controlsInfo, "Управление: ЛКМ+перетаскивание - вращение | Колёсико - Zoom | Пробел - стоп/старт авто");
    drawText(x + 10, y + h - 25, controlsInfo, 0.6f, 0.6f, 0.8f);

    if (!isFloor) {
        char gridInfo[100];
        sprintf_s(gridInfo, "Сетка: %dx%d | Ячейка: %.2f",
            currentConfig.gridWidth, currentConfig.gridDepth, currentConfig.cellSize);
        drawText(x + 10, y + h - 50, gridInfo, 0.7f, 0.7f, 0.7f);

        char scaleText[50];
        sprintf_s(scaleText, "Масштаб: %.2f | Дист: %.1f", scale, previewCameraDistance);
        drawText(x + w - 150, y + 25, scaleText, 1.0f, 1.0f, 0.0f);

        char rotateStatus[30];
        sprintf_s(rotateStatus, "Автовращ: %s", previewAutoRotate ? "ВКЛ" : "ВЫКЛ");
        drawText(x + w - 100, y + h - 25, rotateStatus,
            previewAutoRotate ? 0.0f : 1.0f,
            previewAutoRotate ? 1.0f : 0.5f,
            previewAutoRotate ? 0.0f : 0.5f);
    }

    int arrowY = (int)(y + h + 25);
    int arrowCenterX = (int)(x + w / 2);

    if (drawButton(arrowCenterX - 70, arrowY, 60, 35, "<-")) {
        previewRotationAngle -= 15.0f;
        previewAutoRotate = false;
        previewLastRotateTime = (float)glfwGetTime();
    }
    if (drawButton(arrowCenterX + 10, arrowY, 60, 35, "->")) {
        previewRotationAngle += 15.0f;
        previewAutoRotate = false;
        previewLastRotateTime = (float)glfwGetTime();
    }

    const char* autoRotateBtnText = previewAutoRotate ? "СТОП" : "СТАРТ";
    if (drawButton(arrowCenterX - 135, arrowY, 55, 35, autoRotateBtnText)) {
        previewAutoRotate = !previewAutoRotate;
        if (previewAutoRotate) {
            previewLastRotateTime = (float)glfwGetTime();
        }
    }

    float currentTime = (float)glfwGetTime();
    if (!previewAutoRotate && (currentTime - previewLastRotateTime >= autoRotateDelay)) {
        previewAutoRotate = true;
    }
    if (previewAutoRotate) {
        previewRotationAngle += rotationSpeed;
    }
    if (previewRotationAngle >= 360) previewRotationAngle -= 360;

    rotation = previewRotationAngle;
    autoRotate = previewAutoRotate;
    lastTime = previewLastRotateTime;
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
    head->useTextureColor = true;
    snakeElements.push_back(head);

    VisualElement* body = new VisualElement("ТЕЛО");
    body->modelFile = currentConfig.snakeBodyModel;
    body->color = currentConfig.snakeBodyColor;
    body->scale = currentConfig.snakeBodyScale;
    head->useTextureColor = true;
    snakeElements.push_back(body);

    VisualElement* tail = new VisualElement("ХВОСТ");
    tail->modelFile = currentConfig.snakeTailModel;
    tail->color = currentConfig.snakeTailColor;
    tail->scale = currentConfig.snakeTailScale;
    head->useTextureColor = true;
    snakeElements.push_back(tail);

    // Пол и небо
    VisualElement* ground = new VisualElement("ПОЛ");
    ground->modelFile = currentConfig.floorModel;
    ground->textureFile = currentConfig.floorTexture;
    ground->color = currentConfig.floorColor;
    head->useTextureColor = true;
    groundSkyElements.push_back(ground);

    VisualElement* sky = new VisualElement("НЕБО");
    sky->textureFile = "";
    sky->color = currentConfig.skyColor;
    head->useTextureColor = true;
    groundSkyElements.push_back(sky);

    // Преграды
    VisualElement* tree = new VisualElement("ДЕРЕВО");
    tree->modelFile = currentConfig.treeModel;
    tree->color = glm::vec3(0.1f, 0.4f, 0.1f);
    tree->scale = 1.5f;
    head->useTextureColor = true;
    obstaclesElements.push_back(tree);

    VisualElement* rock = new VisualElement("КАМЕНЬ");
    rock->modelFile = currentConfig.rockModel;
    rock->color = glm::vec3(0.5f, 0.5f, 0.5f);
    rock->scale = 1.2f;
    head->useTextureColor = true;
    obstaclesElements.push_back(rock);

    VisualElement* fence = new VisualElement("ЗАБОР");
    fence->modelFile = currentConfig.fenceModel;
    fence->color = glm::vec3(0.6f, 0.4f, 0.2f);
    fence->scale = 1.0f;
    head->useTextureColor = true;
    obstaclesElements.push_back(fence);

    VisualElement* apple = new VisualElement("ЯБЛОКО");
    apple->modelFile = currentConfig.appleModel;
    apple->color = glm::vec3(1.0f, 0.0f, 0.0f);
    apple->scale = 0.8f;
    apple->count = currentConfig.initialFoodCount;
    head->useTextureColor = true;
    obstaclesElements.push_back(apple);

    // Окружение
    VisualElement* flower = new VisualElement("ЦВЕТОК");
    flower->modelFile = currentConfig.flowerModel;
    flower->color = glm::vec3(1.0f, 0.0f, 1.0f);
    flower->scale = 0.7f;
    flower->count = currentConfig.flowerCount;
    head->useTextureColor = true;
    environmentElements.push_back(flower);

    VisualElement* bird = new VisualElement("ПТИЦА");
    bird->modelFile = currentConfig.birdModel;
    bird->color = glm::vec3(0.5f, 0.5f, 0.5f);
    bird->scale = 0.6f;
    bird->count = currentConfig.birdCount;
    head->useTextureColor = true;
    environmentElements.push_back(bird);

    VisualElement* cloud = new VisualElement("ОБЛАКО");
    cloud->modelFile = currentConfig.cloudModel;
    cloud->color = glm::vec3(1.0f, 1.0f, 1.0f);
    cloud->scale = 1.5f;
    cloud->count = currentConfig.cloudCount;
    head->useTextureColor = true;
    environmentElements.push_back(cloud);
}

//=============================================================================
// ШРИФТЫ (FreeType)
//=============================================================================

void initFreeType() {

    if (FT_Init_FreeType(&g_ft)) {
        return;
    }

    std::string fontPath = "C:/Windows/Fonts/arial.ttf";
    if (FT_New_Face(g_ft, fontPath.c_str(), 0, &g_face)) {
        return;
    }

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

    // Заглавные русские буквы А-Я
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

    // Ё
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

    // Строчные русские буквы а-я
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

    // ё
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

    float startX = x;

    // Находим максимальную высоту символа для выравнивания
    int maxHeight = 0;
    for (unsigned char c : text) {
        auto it = g_characters.find(c);
        if (it != g_characters.end()) {
            maxHeight = std::max(maxHeight, it->second.Size.y);
        }
    }
    if (maxHeight == 0) maxHeight = 20;

    for (unsigned char c : text) {
        auto it = g_characters.find(c);
        if (it == g_characters.end()) {
            it = g_characters.find(' ');
            if (it == g_characters.end()) continue;
        }
        Character& ch = it->second;

        float xpos = startX + (float)ch.Bearing.x;
        // Выравниваем по нижней части символа
        // y - это верхняя граница текста, рисуем символ от y до y+maxHeight
        float ypos = y + (maxHeight - ch.Size.y);
        float w = (float)ch.Size.x;
        float h = (float)ch.Size.y;

        // Для символов типа ,._ немного опускаем
        if (c == '.' || c == ',' || c == '_' || c == ';' || c == ':') {
            ypos = y + (maxHeight - ch.Size.y) + 3;  // Опускаем на 3 пикселя
        }

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

    float textWidth = getTextWidth(text);
    float textHeight = 15.0f;
    float textX = (float)x + ((float)w - textWidth) / 2.0f;
    float textY = (float)y + ((float)h - textHeight) / 2.0f + 3.0f;
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

    drawSlider(x + 70, y + 5, 120, &color.r, 0.0f, 1.0f, "R");
    drawSlider(x + 70, y + 40, 120, &color.g, 0.0f, 1.0f, "G");
    drawSlider(x + 70, y + 75, 120, &color.b, 0.0f, 1.0f, "B");
}

//=============================================================================
// ЗАГРУЗКА МОДЕЛЕЙ
//=============================================================================

GLuint loadTextureFromFile(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::cout << "Texture file not found: " << path << std::endl;
        return 0;
    }

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
        std::cout << "Texture loaded successfully: " << path << " (" << width << "x" << height << ")" << std::endl;
        return textureID;
    }
    std::cout << "Failed to load texture: " << path << " - " << stbi_failure_reason() << std::endl;
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

// ========== ИСПРАВЛЕННАЯ ЗАГРУЗКА FBX МОДЕЛИ С ВЫЧИСЛЕНИЕМ НОРМАЛЕЙ ==========
bool loadFBXModel(const std::string& filename, Model& outModel, const std::string& subFolder) {
    if (filename.empty()) {
        std::cout << "loadFBXModel: filename is empty" << std::endl;
        return false;
    }

    std::string fullPath = g_modelsPath + subFolder + "\\" + filename;
    if (!std::filesystem::exists(fullPath)) {
        std::cout << "loadFBXModel: file not found - " << fullPath << std::endl;
        return false;
    }

    std::cout << "loadFBXModel: loading - " << fullPath << std::endl;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(fullPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "loadFBXModel: failed to load - " << importer.GetErrorString() << std::endl;
        return false;
    }

    outModel.vertices.clear();
    outModel.hasTexture = false;
    outModel.textureID = 0;

    // Загружаем меши
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "Mesh " << i << " has " << mesh->mNumFaces << " faces" << std::endl;

        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                unsigned int vertexIdx = face.mIndices[k];

                Vertex vertex;

                vertex.position.x = mesh->mVertices[vertexIdx].x;
                vertex.position.y = mesh->mVertices[vertexIdx].y;
                vertex.position.z = mesh->mVertices[vertexIdx].z;

                if (mesh->HasNormals()) {
                    vertex.normal.x = mesh->mNormals[vertexIdx].x;
                    vertex.normal.y = mesh->mNormals[vertexIdx].y;
                    vertex.normal.z = mesh->mNormals[vertexIdx].z;
                }
                else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                if (mesh->HasTextureCoords(0)) {
                    vertex.texCoords.x = mesh->mTextureCoords[0][vertexIdx].x;
                    vertex.texCoords.y = mesh->mTextureCoords[0][vertexIdx].y;
                }
                else {
                    vertex.texCoords = glm::vec2(0.0f, 0.0f);
                }

                outModel.vertices.push_back(vertex);
            }
        }
    }

    if (outModel.vertices.empty()) {
        std::cout << "loadFBXModel: no vertices loaded" << std::endl;
        return false;
    }

    std::cout << "loadFBXModel: loaded " << outModel.vertices.size() << " vertices" << std::endl;

    // ========== РАСШИРЕННЫЙ ПОИСК ТЕКСТУР ==========
    std::cout << "Searching for textures in FBX file..." << std::endl;

    // Проверяем все материалы
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];

        // Проверяем разные типы текстур
        aiTextureType textureTypes[] = {
            aiTextureType_DIFFUSE,
            aiTextureType_SPECULAR,
            aiTextureType_AMBIENT,
            aiTextureType_EMISSIVE,
            aiTextureType_HEIGHT,
            aiTextureType_NORMALS,
            aiTextureType_SHININESS,
            aiTextureType_OPACITY,
            aiTextureType_DISPLACEMENT,
            aiTextureType_LIGHTMAP,
            aiTextureType_REFLECTION,
            aiTextureType_BASE_COLOR,
            aiTextureType_NORMAL_CAMERA,
            aiTextureType_EMISSION_COLOR,
            aiTextureType_METALNESS,
            aiTextureType_DIFFUSE_ROUGHNESS,
            aiTextureType_AMBIENT_OCCLUSION
        };

        for (int typeIdx = 0; typeIdx < sizeof(textureTypes) / sizeof(textureTypes[0]); typeIdx++) {
            for (unsigned int texIdx = 0; texIdx < mat->GetTextureCount(textureTypes[typeIdx]); texIdx++) {
                aiString texturePath;
                if (mat->GetTexture(textureTypes[typeIdx], texIdx, &texturePath) == AI_SUCCESS) {
                    std::string texFile = texturePath.C_Str();
                    std::cout << "Found texture reference: " << texFile << " (type: " << typeIdx << ")" << std::endl;

                    // Очищаем путь от папок и обратных слэшей
                    size_t pos = texFile.find_last_of("\\/");
                    if (pos != std::string::npos) texFile = texFile.substr(pos + 1);

                    // Убираем возможные относительные пути
                    if (texFile.find("..") != std::string::npos) {
                        pos = texFile.find_last_of("\\/");
                        if (pos != std::string::npos) texFile = texFile.substr(pos + 1);
                    }

                    std::cout << "Cleaned texture name: " << texFile << std::endl;

                    // Расширенный поиск файла текстуры
                    std::vector<std::string> searchPaths = {
                        g_texturesPath + texFile,
                        g_texturesPath + subFolder + "\\" + texFile,
                        g_texturesPath + "models\\" + subFolder + "\\" + texFile,
                        g_modelsPath + subFolder + "\\" + texFile,
                        g_assetsPath + texFile,
                        g_assetsPath + "textures\\" + texFile,
                        "assets\\textures\\" + texFile,
                        // Поиск без расширения, пробуем добавить расширения
                        g_texturesPath + texFile + ".png",
                        g_texturesPath + texFile + ".jpg",
                        g_texturesPath + texFile + ".jpeg",
                        g_texturesPath + texFile + ".tga",
                        g_texturesPath + texFile + ".bmp",
                        g_texturesPath + subFolder + "\\" + texFile + ".png",
                        g_texturesPath + subFolder + "\\" + texFile + ".jpg",
                    };

                    bool textureLoaded = false;
                    for (const auto& fullTexPath : searchPaths) {
                        if (std::filesystem::exists(fullTexPath)) {
                            GLuint texID = loadTextureFromFile(fullTexPath);
                            if (texID != 0) {
                                outModel.textureID = texID;
                                outModel.hasTexture = true;
                                std::cout << "SUCCESS: Loaded texture from: " << fullTexPath << std::endl;
                                textureLoaded = true;
                                break;
                            }
                        }
                    }

                    if (!textureLoaded) {
                        std::cout << "WARNING: Could not find texture file for: " << texFile << std::endl;
                        std::cout << "Searched in:" << std::endl;
                        for (const auto& path : searchPaths) {
                            std::cout << "  - " << path << std::endl;
                        }
                    }

                    if (outModel.hasTexture) break;
                }
            }
            if (outModel.hasTexture) break;
        }
        if (outModel.hasTexture) break;
    }

    if (!outModel.hasTexture) {
        std::cout << "No textures found in FBX file" << std::endl;
    }

    outModel.computeNormals();
    outModel.setupBuffers();

    return true;
}
void initShadowModels() {
    std::cout << "\n========== INIT SHADOW MODELS ==========" << std::endl;

    // Загружаем модели для теней из currentConfig
    if (!currentConfig.snakeHeadModel.empty()) {
        loadFBXModel(currentConfig.snakeHeadModel, g_previewSnakeHeadModel, "snake_head");
    }
    if (g_previewSnakeHeadModel.vertices.empty()) {
        std::cout << "Creating snake head primitive" << std::endl;
        SnakeHeadPrimitive::create(g_previewSnakeHeadModel);
        g_previewSnakeHeadModel.computeNormals();
        g_previewSnakeHeadModel.setupBuffers();
    }

    if (!currentConfig.snakeBodyModel.empty()) {
        loadFBXModel(currentConfig.snakeBodyModel, g_previewSnakeBodyModel, "snake_body");
    }
    if (g_previewSnakeBodyModel.vertices.empty()) {
        std::cout << "Creating snake body primitive" << std::endl;
        SnakeBodyPrimitive::create(g_previewSnakeBodyModel);
        g_previewSnakeBodyModel.computeNormals();
        g_previewSnakeBodyModel.setupBuffers();
    }

    if (!currentConfig.snakeTailModel.empty()) {
        loadFBXModel(currentConfig.snakeTailModel, g_previewSnakeTailModel, "snake_tail");
    }
    if (g_previewSnakeTailModel.vertices.empty()) {
        std::cout << "Creating snake tail primitive" << std::endl;
        SnakeTailPrimitive::create(g_previewSnakeTailModel);
        g_previewSnakeTailModel.computeNormals();
        g_previewSnakeTailModel.setupBuffers();
    }

    if (!currentConfig.treeModel.empty()) {
        loadFBXModel(currentConfig.treeModel, g_previewTreeModel, "obstacles");
    }
    if (g_previewTreeModel.vertices.empty()) {
        std::cout << "Creating tree primitive" << std::endl;
        TreePrimitive::create(g_previewTreeModel);
        g_previewTreeModel.computeNormals();
        g_previewTreeModel.setupBuffers();
    }

    if (!currentConfig.appleModel.empty()) {
        loadFBXModel(currentConfig.appleModel, g_previewAppleModel, "food");
    }
    if (g_previewAppleModel.vertices.empty()) {
        std::cout << "Creating apple primitive" << std::endl;
        ApplePrimitive::create(g_previewAppleModel);
        g_previewAppleModel.computeNormals();
        g_previewAppleModel.setupBuffers();
    }

    std::cout << "========================================\n" << std::endl;
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

// Измените функцию drawModel - добавьте параметр цвета:
void drawModel(ModelData& model, const glm::vec3& customColor, bool useTextureColor = true) {
    if (!model.loaded || model.vertices.empty()) return;

    // Решение: использовать текстуру или цвет
    bool hasTexture = (!model.materials.empty() && model.materials[0].textureID != 0);

    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, model.materials[0].textureID);

        if (useTextureColor) {
            // Смешиваем цвет с текстурой (умножаем)
            glColor3f(customColor.r, customColor.g, customColor.b);
        }
        else {
            // Только текстура, без цвета
            glColor3f(1.0f, 1.0f, 1.0f);
        }
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(customColor.r, customColor.g, customColor.b);
    }

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < model.vertices.size() / 3; i++) {
        if (!model.normals.empty() && i * 3 + 2 < model.normals.size()) {
            glNormal3f(model.normals[i * 3], model.normals[i * 3 + 1], model.normals[i * 3 + 2]);
        }

        if (hasTexture && !model.texCoords.empty() && i * 2 + 1 < model.texCoords.size()) {
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
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//=============================================================================
// ПРЕДПРОСМОТР МОДЕЛИ
//=============================================================================
void renderModelPreview(ModelData& model, const char* title, float x, float y, float w, float h,
    float& rotation, bool& autoRotate, float& lastTime, float scale, bool isFloor,
    const glm::vec3& customColor, bool useTextureColor = true) {

    previewHoverX = (int)x;
    previewHoverY = (int)y;
    previewHoverW = (int)w;
    previewHoverH = (int)h;

    previewHovered = (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);

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

    float radPitch = glm::radians(previewCameraPitch);
    float radYaw = glm::radians(previewRotationAngle);

    glm::vec3 center(0.0f, 0.0f, 0.0f);
    float camX = sin(radYaw) * cos(radPitch) * previewCameraDistance;
    float camY = sin(radPitch) * previewCameraDistance;
    float camZ = cos(radYaw) * cos(radPitch) * previewCameraDistance;
    glm::vec3 eye(camX, camY + 1.0f, camZ);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));

    // Отключаем освещение для предпросмотра
    glDisable(GL_LIGHTING);

    float worldWidth = currentConfig.gridWidth * currentConfig.cellSize;
    float worldDepth = currentConfig.gridDepth * currentConfig.cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    // Пол
    glColor3f(currentConfig.floorColor.r, currentConfig.floorColor.g, currentConfig.floorColor.b);
    glBegin(GL_QUADS);
    glVertex3f(-offsetX, -0.02f, -offsetZ);
    glVertex3f(offsetX, -0.02f, -offsetZ);
    glVertex3f(offsetX, -0.02f, offsetZ);
    glVertex3f(-offsetX, -0.02f, offsetZ);
    glEnd();

    // Сетка
    if (currentConfig.gridEnabled) {
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
        glLineWidth(currentConfig.gridLineWidth);
        glBegin(GL_LINES);
        for (int i = -currentConfig.gridWidth / 2; i <= currentConfig.gridWidth / 2; i++) {
            float xPos = (float)i * currentConfig.cellSize;
            glVertex3f(xPos, -0.01f, -offsetZ);
            glVertex3f(xPos, -0.01f, offsetZ);
        }
        for (int i = -currentConfig.gridDepth / 2; i <= currentConfig.gridDepth / 2; i++) {
            float zPos = (float)i * currentConfig.cellSize;
            glVertex3f(-offsetX, -0.01f, zPos);
            glVertex3f(offsetX, -0.01f, zPos);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    // Модель
    if (model.loaded && !model.vertices.empty()) {
        glPushMatrix();

        if (isFloor) {
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glScalef(0.01f, 0.01f, 0.01f);
        }
        else {
            float minY = FLT_MAX;
            for (size_t i = 0; i < model.vertices.size() / 3; i++) {
                float vy = model.vertices[i * 3 + 1];
                minY = std::min(minY, vy);
            }
            glTranslatef(0.0f, -minY + 0.05f, 0.0f);
            glScalef(scale, scale, scale);
        }

        // Рисуем модель с учётом флага использования цвета текстуры
        bool hasTexture = (!model.materials.empty() && model.materials[0].textureID != 0);

        if (hasTexture) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, model.materials[0].textureID);

            if (useTextureColor) {
                glColor3f(customColor.r, customColor.g, customColor.b);
            }
            else {
                glColor3f(1.0f, 1.0f, 1.0f);
            }
        }
        else {
            glDisable(GL_TEXTURE_2D);
            glColor3f(customColor.r, customColor.g, customColor.b);
        }

        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            if (!model.normals.empty() && i * 3 + 2 < model.normals.size()) {
                glNormal3f(model.normals[i * 3], model.normals[i * 3 + 1], model.normals[i * 3 + 2]);
            }

            if (hasTexture && !model.texCoords.empty() && i * 2 + 1 < model.texCoords.size()) {
                glTexCoord2f(model.texCoords[i * 2], model.texCoords[i * 2 + 1]);
            }

            glVertex3f(model.vertices[i * 3], model.vertices[i * 3 + 1], model.vertices[i * 3 + 2]);
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        glPopMatrix();
    }

    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    reset2DProjection();

    // UI Рамка и кнопки...
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    drawText(x + 10, y + 25, title, 1.0f, 1.0f, 0.0f);

    char controlsInfo[200];
    sprintf_s(controlsInfo, "Управление: ЛКМ+перетаскивание - вращение | Колёсико - Zoom | Пробел - стоп/старт авто");
    drawText(x + 10, y + h - 25, controlsInfo, 0.6f, 0.6f, 0.8f);

    if (!isFloor) {
        char gridInfo[100];
        sprintf_s(gridInfo, "Сетка: %dx%d | Ячейка: %.2f",
            currentConfig.gridWidth, currentConfig.gridDepth, currentConfig.cellSize);
        drawText(x + 10, y + h - 50, gridInfo, 0.7f, 0.7f, 0.7f);

        char scaleText[50];
        sprintf_s(scaleText, "Масштаб: %.2f | Дист: %.1f", scale, previewCameraDistance);
        drawText(x + w - 150, y + 25, scaleText, 1.0f, 1.0f, 0.0f);

        char rotateStatus[30];
        sprintf_s(rotateStatus, "Автовращ: %s", previewAutoRotate ? "ВКЛ" : "ВЫКЛ");
        drawText(x + w - 100, y + h - 25, rotateStatus,
            previewAutoRotate ? 0.0f : 1.0f,
            previewAutoRotate ? 1.0f : 0.5f,
            previewAutoRotate ? 0.0f : 0.5f);

        char colorInfo[100];
        sprintf_s(colorInfo, "Цвет: R=%.2f G=%.2f B=%.2f", customColor.r, customColor.g, customColor.b);
        drawText(x + w - 250, y + 25, colorInfo, customColor.r, customColor.g, customColor.b);

        // Информация о текстуре
        bool hasTexture = (!model.materials.empty() && model.materials[0].textureID != 0);
        if (hasTexture) {
            char texInfo[100];
            sprintf_s(texInfo, "Текстура: %s цвет", useTextureColor ? "использует" : "игнорирует");
            drawText(x + 10, y + h - 75, texInfo, useTextureColor ? 0.0f : 1.0f,
                useTextureColor ? 1.0f : 0.5f, 0.0f);
        }
    }

    int arrowY = (int)(y + h + 25);
    int arrowCenterX = (int)(x + w / 2);

    if (drawButton(arrowCenterX - 70, arrowY, 60, 35, "<-")) {
        previewRotationAngle -= 15.0f;
        previewAutoRotate = false;
        previewLastRotateTime = (float)glfwGetTime();
    }
    if (drawButton(arrowCenterX + 10, arrowY, 60, 35, "->")) {
        previewRotationAngle += 15.0f;
        previewAutoRotate = false;
        previewLastRotateTime = (float)glfwGetTime();
    }

    const char* autoRotateBtnText = previewAutoRotate ? "СТОП" : "СТАРТ";
    if (drawButton(arrowCenterX - 135, arrowY, 55, 35, autoRotateBtnText)) {
        previewAutoRotate = !previewAutoRotate;
        if (previewAutoRotate) {
            previewLastRotateTime = (float)glfwGetTime();
        }
    }

    float currentTime = (float)glfwGetTime();
    if (!previewAutoRotate && (currentTime - previewLastRotateTime >= autoRotateDelay)) {
        previewAutoRotate = true;
    }
    if (previewAutoRotate) {
        previewRotationAngle += rotationSpeed;
    }
    if (previewRotationAngle >= 360) previewRotationAngle -= 360;

    rotation = previewRotationAngle;
    autoRotate = previewAutoRotate;
    lastTime = previewLastRotateTime;
}

//=============================================================================
// РЕДАКТОР ЗМЕЙКИ
//=============================================================================

void renderSnakeEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "РЕДАКТОР ЗМЕЙКИ", 1.0f, 1.0f, 0.0f);

    int listX = (int)(windowWidth * 0.03f);
    int listY = (int)(windowHeight * 0.12f);
    int listWidth = (int)(windowWidth * 0.12f);
    int listHeight = (int)(windowHeight * 0.05f);

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

        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);

        std::string displayFile = truncateFilename(el->modelFile, 30);
        drawText((float)(editX + 80), (float)(modelY + 20), displayFile.c_str(), 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            std::string folder = (selectedPart == 0) ? "snake_head" :
                (selectedPart == 1) ? "snake_body" : "snake_tail";
            openFileDialog(el->modelFile, folder);
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedPart == 0) el->modelFile = "snake_head.fbx";
            else if (selectedPart == 1) el->modelFile = "snake_body.fbx";
            else el->modelFile = "snake_tail.fbx";
            updatePreviewForCurrentMode();
        }

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

        int texColorY = colorY + 80;
        // Кнопка для переключения режима текстуры
        bool hasTexture = (previewModel.loaded && !previewModel.materials.empty() && previewModel.materials[0].textureID != 0);
        if (hasTexture) {
            drawText((float)editX, (float)texColorY, "Режим текстуры:", 1.0f, 1.0f, 1.0f);

            std::string texBtnText = std::string("Цвет для текстуры: ") +
                (el->useTextureColor ? "ВКЛ" : "ВЫКЛ");
            if (drawButton(editX, texColorY + 20, 200, 35, texBtnText.c_str())) {
                el->useTextureColor = !el->useTextureColor;
            }

            if (drawButton(editX + 220, texColorY + 20, 100, 35, "СБРОСИТЬ")) {
                el->useTextureColor = true;
            }
        }

        int scaleY = hasTexture ? texColorY + 80 : texColorY;
        drawText((float)editX, (float)scaleY, "Масштаб:", 1.0f, 1.0f, 1.0f);

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        drawText((float)(editX + 100), (float)scaleY, scaleText, 1.0f, 1.0f, 0.0f);

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            el->scale = 0.8f;
        }

        if (drawButton(editX, scaleY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            if (selectedPart == 0) {
                el->modelFile = "snake_head.fbx";
                el->color = glm::vec3(0.0f, 1.0f, 0.0f);
                el->scale = 0.8f;
                el->useTextureColor = true;
            }
            else if (selectedPart == 1) {
                el->modelFile = "snake_body.fbx";
                el->color = glm::vec3(0.0f, 0.7f, 0.0f);
                el->scale = 0.8f;
                el->useTextureColor = true;
            }
            else {
                el->modelFile = "snake_tail.fbx";
                el->color = glm::vec3(0.0f, 0.5f, 0.0f);
                el->scale = 0.8f;
                el->useTextureColor = true;
            }
            updatePreviewForCurrentMode();
        }
    }

    if (selectedPart >= 0 && selectedPart < (int)snakeElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            snakeElements[selectedPart]->scale, false,
            snakeElements[selectedPart]->color,
            snakeElements[selectedPart]->useTextureColor);
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ПОЛА И НЕБА
//=============================================================================
void renderFloorPreview3D() {
    int previewX = (int)(windowWidth * 0.46f);
    int previewY = (int)(windowHeight * 0.12f);
    int previewW = (int)(windowWidth * 0.36f);
    int previewH = (int)(windowHeight * 0.5f);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glScissor(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    float aspect = (float)previewW / previewH;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Камера
    static float camDistance = 8.0f;
    static float camAngle = 45.0f;
    static float camHeight = 30.0f;

    float radAngle = glm::radians(camAngle);
    float radHeight = glm::radians(camHeight);

    float worldWidth = currentConfig.gridWidth * currentConfig.cellSize;
    float worldDepth = currentConfig.gridDepth * currentConfig.cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;
    glm::vec3 center(0.0f, 0.0f, 0.0f);

    glm::vec3 eye(
        sin(radAngle) * cos(radHeight) * camDistance,
        sin(radHeight) * camDistance,
        cos(radAngle) * cos(radHeight) * camDistance
    );
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);

    // Включаем освещение
    setupFixedPipelineLighting();
    updateLightPosition();

    VisualElement* ground = groundSkyElements[0];
    glm::vec3 floorColor = ground->color;

    // Рисуем пол
    float cellSize = currentConfig.cellSize;

    glDisable(GL_TEXTURE_2D);
    glColor3f(floorColor.r, floorColor.g, floorColor.b);
    glBegin(GL_QUADS);
    for (int z = 0; z < currentConfig.gridDepth; z++) {
        for (int x = 0; x < currentConfig.gridWidth; x++) {
            float posX = x * cellSize - offsetX;
            float posZ = z * cellSize - offsetZ;
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(posX, -0.02f, posZ);
            glVertex3f(posX + cellSize, -0.02f, posZ);
            glVertex3f(posX + cellSize, -0.02f, posZ + cellSize);
            glVertex3f(posX, -0.02f, posZ + cellSize);
        }
    }
    glEnd();

    // Сетка
    if (currentConfig.gridEnabled) {
        glDisable(GL_LIGHTING);
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
        glLineWidth(currentConfig.gridLineWidth);
        glBegin(GL_LINES);
        for (int i = 0; i <= currentConfig.gridWidth; i++) {
            float x = i * cellSize - offsetX;
            glVertex3f(x, 0.01f, -offsetZ);
            glVertex3f(x, 0.01f, offsetZ);
        }
        for (int i = 0; i <= currentConfig.gridDepth; i++) {
            float z = i * cellSize - offsetZ;
            glVertex3f(-offsetX, 0.01f, z);
            glVertex3f(offsetX, 0.01f, z);
        }
        glEnd();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }

    // Если есть модель пола - рисуем её
    if (previewModel.loaded && !previewModel.vertices.empty()) {
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.01f, 0.01f, 0.01f);

        if (previewModel.materials.empty() || previewModel.materials[0].textureID == 0) {
            glColor3f(floorColor.r, floorColor.g, floorColor.b);
        }
        else {
            glColor3f(1.0f, 1.0f, 1.0f);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, previewModel.materials[0].textureID);
        }

        drawModel(previewModel, currentConfig.floorColor, obstaclesElements[selectedObstacle]->useTextureColor);
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    reset2DProjection();

    // UI
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)previewX, (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)(previewY + previewH));
    glVertex2f((float)previewX, (float)(previewY + previewH));
    glEnd();

    drawText((float)(previewX + 10), (float)(previewY + 25), "3D ПРЕДПРОСМОТР ПОЛА", 1.0f, 1.0f, 0.0f);
    drawText((float)(previewX + 10), (float)(previewY + 50), "Цвет пола применяется в реальном времени", 0.7f, 0.7f, 0.9f);

    char colorInfo[100];
    sprintf_s(colorInfo, "Текущий цвет: R=%.2f G=%.2f B=%.2f",
        floorColor.r, floorColor.g, floorColor.b);
    drawText((float)(previewX + 10), (float)(previewY + 75), colorInfo, 1.0f, 1.0f, 0.5f);
}
void renderGroundSkyEditor() {
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "ПОЛ И НЕБО", 1.0f, 1.0f, 0.0f);

    int listX = (int)(windowWidth * 0.03f);
    int listY = (int)(windowHeight * 0.12f);
    int listWidth = (int)(windowWidth * 0.12f);
    int listHeight = (int)(windowHeight * 0.05f);

    for (size_t i = 0; i < groundSkyElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", groundSkyElements[i]->name.c_str());
        if (drawButton(listX, listY + (int)i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedGroundSky = (int)i;
            updatePreviewForCurrentMode();
        }
    }

    static bool useTileMode = true;  // Режим плиток (всегда включён)

    if (selectedGroundSky == 0) {
        VisualElement* ground = groundSkyElements[0];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), ground->name, 1.0f, 1.0f, 0.0f);

        // Модель
        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);

        std::string displayFile = truncateFilename(ground->modelFile, 30);
        drawText((float)(editX + 80), (float)(modelY + 20), displayFile.c_str(), 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ МОДЕЛЬ")) {
            openFileDialog(ground->modelFile, "floor");
            updatePreviewForCurrentMode();
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ МОДЕЛЬ")) {
            ground->modelFile = "";
            updatePreviewForCurrentMode();
        }

        // Текстура
        int textureY = modelY + 90;
        drawText((float)editX, (float)textureY, "Текстура:", 1.0f, 1.0f, 1.0f);

        if (floorTexture.id != 0) {
            drawText((float)(editX + 100), (float)textureY, truncateFilename(ground->textureFile, 20).c_str(), 0.0f, 1.0f, 0.0f);
        }
        else if (!previewModel.materials.empty() && previewModel.materials[0].textureID != 0) {
            drawText((float)(editX + 100), (float)textureY, "Текстура из модели", 0.0f, 1.0f, 0.0f);
        }
        else {
            drawText((float)(editX + 100), (float)textureY, "Нет текстуры", 1.0f, 0.5f, 0.0f);
        }

        if (drawButton(editX, textureY + 20, buttonWidth, 30, "ЗАГРУЗИТЬ ТЕКСТУРУ")) {
            openTextureFileDialog(ground->textureFile, floorTexture);
            // ВАЖНО: после загрузки текстуры, добавляем её в previewModel
            if (floorTexture.id != 0) {
                previewModel.materials.clear();
                Material mat;
                mat.textureID = floorTexture.id;
                previewModel.materials.push_back(mat);
                std::cout << "Texture loaded and added to previewModel with ID: " << floorTexture.id << std::endl;
            }
            updatePreviewForCurrentMode();
        }

        if (drawButton(editX + buttonSpacing, textureY + 20, buttonWidth, 30, "СБРОСИТЬ ТЕКСТУРУ")) {
            ground->textureFile = "";
            if (floorTexture.id != 0) {
                glDeleteTextures(1, &floorTexture.id);
                floorTexture.id = 0;
            }
            previewModel.materials.clear();
            updatePreviewForCurrentMode();
        }

        // Цвет пола (влияет на текстуру)
        int colorY = textureY + 80;
        drawText((float)editX, (float)colorY, "Цвет пола:", 1.0f, 1.0f, 1.0f);

        glColor3f(ground->color.r, ground->color.g, ground->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 100), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY + 12));
        glVertex2f((float)(editX + 100), (float)(colorY + 12));
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 100), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY + 12));
        glVertex2f((float)(editX + 100), (float)(colorY + 12));
        glEnd();

        drawSlider(editX, colorY + 15, 180, &ground->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX, colorY + 45, 180, &ground->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX, colorY + 75, 180, &ground->color.b, 0.0f, 1.0f, "B");

        currentConfig.floorColor = ground->color;

        if (drawButton(editX + 200, colorY + 30, 80, 30, "СБРОСИТЬ ЦВЕТ")) {
            ground->color = glm::vec3(0.3f, 0.6f, 0.2f);
            currentConfig.floorColor = ground->color;
        }

        // Кнопка для переключения режима текстуры
        bool hasTexture = (previewModel.loaded && !previewModel.materials.empty() && previewModel.materials[0].textureID != 0) ||
            (floorTexture.id != 0);
        if (hasTexture) {
            int texColorY = colorY + 100;
            drawText((float)editX, (float)texColorY, "Режим текстуры:", 1.0f, 1.0f, 1.0f);

            std::string texBtnText = std::string("Цвет для текстуры: ") +
                (ground->useTextureColor ? "ВКЛ" : "ВЫКЛ");
            if (drawButton(editX, texColorY + 20, 200, 35, texBtnText.c_str())) {
                ground->useTextureColor = !ground->useTextureColor;
            }

            if (drawButton(editX + 220, texColorY + 20, 100, 35, "СБРОСИТЬ")) {
                ground->useTextureColor = true;
            }
        }

        if (drawButton(editX, (int)(windowHeight * 0.75f), 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            ground->modelFile = "";
            ground->textureFile = "";
            ground->color = glm::vec3(0.3f, 0.6f, 0.2f);
            ground->useTextureColor = true;
            currentConfig.floorColor = ground->color;
            if (floorTexture.id != 0) {
                glDeleteTextures(1, &floorTexture.id);
                floorTexture.id = 0;
            }
            previewModel.materials.clear();
            updatePreviewForCurrentMode();
        }

        // ========== 3D ПРЕДПРОСМОТР ПОЛА С ТАЙЛИНГОМ ==========
        int previewX = (int)(windowWidth * 0.46f);
        int previewY = (int)(windowHeight * 0.12f);
        int previewW = (int)(windowWidth * 0.36f);
        int previewH = (int)(windowHeight * 0.5f);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
        glScissor(previewX, windowHeight - previewY - previewH, previewW, previewH);
        glEnable(GL_SCISSOR_TEST);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        float aspect = (float)previewW / previewH;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glLoadMatrixf(glm::value_ptr(projection));

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        float radPitch = glm::radians(previewCameraPitch);
        float radYaw = glm::radians(previewRotationAngle);

        glm::vec3 center(0.0f, 0.0f, 0.0f);
        float camX = sin(radYaw) * cos(radPitch) * previewCameraDistance;
        float camY = sin(radPitch) * previewCameraDistance;
        float camZ = cos(radYaw) * cos(radPitch) * previewCameraDistance;
        glm::vec3 eye(camX, camY + 1.0f, camZ);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(eye, center, up);
        glLoadMatrixf(glm::value_ptr(view));

        glDisable(GL_LIGHTING);

        float worldWidth = currentConfig.gridWidth * currentConfig.cellSize;
        float worldDepth = currentConfig.gridDepth * currentConfig.cellSize;
        float offsetX = worldWidth / 2.0f;
        float offsetZ = worldDepth / 2.0f;
        float floorHeight = -0.05f;

        // ========== ОТРИСОВКА ПЛИТОЧНОГО ПОЛА С АВТОМАТИЧЕСКИМ РАСЧЁТОМ ==========
        if (useTileMode && previewModel.loaded && !previewModel.vertices.empty()) {
            // ВЫЧИСЛЯЕМ BOUNDING BOX МОДЕЛИ
            float minX_m = FLT_MAX, maxX_m = -FLT_MAX;
            float minY_m = FLT_MAX, maxY_m = -FLT_MAX;
            float minZ_m = FLT_MAX, maxZ_m = -FLT_MAX;

            for (size_t i = 0; i < previewModel.vertices.size() / 3; i++) {

                float vx = previewModel.vertices[i * 3];
                float vy = previewModel.vertices[i * 3 + 1];
                float vz = previewModel.vertices[i * 3 + 2];
                minX_m = std::min(minX_m, vx);
                maxX_m = std::max(maxX_m, vx);
                minY_m = std::min(minY_m, vy);
                maxY_m = std::max(maxY_m, vy);
                minZ_m = std::min(minZ_m, vz);
                maxZ_m = std::max(maxZ_m, vz);
            }

            float modelWidth = (maxX_m - minX_m);
            float modelDepth = (maxZ_m - minZ_m);
            float modelHeight = (maxY_m - minY_m);

            // АВТОМАТИЧЕСКИЙ РАСЧЁТ РАЗМЕРА ПЛИТКИ
            // Целевой размер плитки - чтобы помещалось целое количество по ширине и глубине
            int desiredTilesPerRow = 5;  // Хотим 10 моделей в ряду
            float targetTileSize = worldWidth / desiredTilesPerRow;

            // Вычисляем масштаб для модели, чтобы её ширина/глубина стала targetTileSize
            float scaleToFit = targetTileSize / std::max(modelWidth, modelDepth);

            // Фактический размер плитки после масштабирования
            float actualTileWidth = modelWidth * scaleToFit;
            float actualTileDepth = modelDepth * scaleToFit;

            // Вычисляем, сколько целых плиток помещается по ширине и глубине
            int tilesX = std::max(1, (int)(worldWidth / actualTileWidth));
            int tilesZ = std::max(1, (int)(worldDepth / actualTileDepth));

            // Корректируем размер плитки, чтобы целое количество точно заполнило поле
            float finalTileWidth = worldWidth / tilesX;
            float finalTileDepth = worldDepth / tilesZ;

            // Финальный масштаб модели
            float finalScale = finalTileWidth / std::max(modelWidth, modelDepth);

            // Информация для отладки
            std::cout << "Model bounds: W=" << modelWidth << " D=" << modelDepth << " H=" << modelHeight << std::endl;
            std::cout << "World size: W=" << worldWidth << " D=" << worldDepth << std::endl;
            std::cout << "Tiles: " << tilesX << " x " << tilesZ << std::endl;
            std::cout << "Final tile size: " << finalTileWidth << " x " << finalTileDepth << std::endl;

            // Начальная позиция (центрируем плитки)
            float startX = -offsetX;
            float startZ = -offsetZ;

            // Определяем, использовать ли цвет с текстурой
            bool hasTexture = (!previewModel.materials.empty() && previewModel.materials[0].textureID != 0) ||
                (floorTexture.id != 0);
            GLuint textureID = 0;
            if (!previewModel.materials.empty() && previewModel.materials[0].textureID != 0) {
                textureID = previewModel.materials[0].textureID;
                std::cout << "Using texture from model, ID: " << textureID << std::endl;
            }
            else if (floorTexture.id != 0) {
                textureID = floorTexture.id;
                std::cout << "Using separate texture, ID: " << textureID << std::endl;
            }
            else {
                std::cout << "No texture available" << std::endl;
            }

            // Рисуем плитки
            for (int ix = 0; ix < tilesX; ix++) {
                for (int iz = 0; iz < tilesZ; iz++) {
                    float posX = startX + ix * finalTileWidth + finalTileWidth / 2.0f;
                    float posZ = startZ + iz * finalTileDepth + finalTileDepth / 2.0f;

                    glPushMatrix();
                    // Позиционируем так, чтобы верх модели был на Y=0
                    // minY_m - это самая нижняя точка модели (отрицательное значение или 0)
                    // maxY_m - самая верхняя точка
                    glTranslatef(posX, floorHeight - (minY_m * finalScale), posZ);
                    if (previewModel.loaded && !previewModel.vertices.empty()) {
                        glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Поворот вокруг X
                    }
                    glScalef(finalScale, finalScale, finalScale);

                    if (hasTexture && textureID != 0) {
                        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, textureID);
                        if (ground->useTextureColor) {
                            glColor3f(ground->color.r, ground->color.g, ground->color.b);
                        }
                        else {
                            glColor3f(1.0f, 1.0f, 1.0f);
                        }
                    }
                    else {
                        glDisable(GL_TEXTURE_2D);
                        glColor3f(ground->color.r, ground->color.g, ground->color.b);
                    }

                    glBegin(GL_TRIANGLES);
                    for (size_t i = 0; i < previewModel.vertices.size() / 3; i++) {
                        if (hasTexture && textureID != 0 && !previewModel.texCoords.empty() && i * 2 + 1 < previewModel.texCoords.size()) {
                            glTexCoord2f(previewModel.texCoords[i * 2], previewModel.texCoords[i * 2 + 1]);
                        }
                        glVertex3f(previewModel.vertices[i * 3],
                            previewModel.vertices[i * 3 + 1],
                            previewModel.vertices[i * 3 + 2]);
                    }
                    glEnd();

                    glPopMatrix();
                }
            }

            if (hasTexture && textureID != 0) {
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D);
            }

            drawText((float)(previewX + 10), (float)(previewY + 25), "3D ПЛИТКИ (АВТОМАТИЧЕСКИЙ ТАЙЛИНГ)", 1.0f, 1.0f, 0.0f);

            char tileInfo[200];
            sprintf_s(tileInfo, "Плитки: %d x %d = %d | Размер: %.2fм x %.2fм",
                tilesX, tilesZ, tilesX * tilesZ, finalTileWidth, finalTileDepth);
            drawText((float)(previewX + 10), (float)(previewY + 50), tileInfo, 0.0f, 1.0f, 0.0f);

            char modelInfo[100];
            sprintf_s(modelInfo, "Модель: %.2fм | Масштаб: %.2fx",
                std::max(modelWidth, modelDepth), finalScale);
            drawText((float)(previewX + 10), (float)(previewY + 75), modelInfo, 0.7f, 0.7f, 0.7f);

            char colorInfo[150];
            sprintf_s(colorInfo, "Цвет: R=%.2f G=%.2f B=%.2f | Влияет на текстуру: %s | Текстура: %s",
                ground->color.r, ground->color.g, ground->color.b,
                ground->useTextureColor ? "ДА" : "НЕТ",
                hasTexture ? "ЕСТЬ" : "НЕТ");
            drawText((float)(previewX + 10), (float)(previewY + 100), colorInfo, 1.0f, 1.0f, 0.5f);
        }
        else if (previewModel.loaded && !previewModel.vertices.empty()) {
            // Если модель загружена, но режим выключен - показываем одну модель на весь пол
            glPushMatrix();

            float minY = FLT_MAX, maxY = -FLT_MAX;
            for (size_t i = 0; i < previewModel.vertices.size() / 3; i++) {
                float vy = previewModel.vertices[i * 3 + 1];
                minY = std::min(minY, vy);
                maxY = std::max(maxY, vy);
            }
            float modelHeight = maxY - minY;

            glTranslatef(0.0f, -minY + floorHeight, 0.0f);

            float maxDim = std::max(worldWidth, worldDepth);
            float scale = maxDim / 2.0f;
            glScalef(scale, scale, scale);

            bool hasTexture = (!previewModel.materials.empty() && previewModel.materials[0].textureID != 0) ||
                (floorTexture.id != 0);
            GLuint textureID = 0;
            if (!previewModel.materials.empty() && previewModel.materials[0].textureID != 0) {
                textureID = previewModel.materials[0].textureID;
            }
            else if (floorTexture.id != 0) {
                textureID = floorTexture.id;
            }

            if (hasTexture && textureID != 0) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, textureID);
                if (ground->useTextureColor) {
                    glColor3f(ground->color.r, ground->color.g, ground->color.b);
                }
                else {
                    glColor3f(1.0f, 1.0f, 1.0f);
                }
            }
            else {
                glDisable(GL_TEXTURE_2D);
                glColor3f(ground->color.r, ground->color.g, ground->color.b);
            }

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < previewModel.vertices.size() / 3; i++) {
                if (hasTexture && textureID != 0 && !previewModel.texCoords.empty() && i * 2 + 1 < previewModel.texCoords.size()) {
                    glTexCoord2f(previewModel.texCoords[i * 2], previewModel.texCoords[i * 2 + 1]);
                }
                glVertex3f(previewModel.vertices[i * 3],
                    previewModel.vertices[i * 3 + 1],
                    previewModel.vertices[i * 3 + 2]);
            }
            glEnd();

            glPopMatrix();

            drawText((float)(previewX + 10), (float)(previewY + 25), "3D МОДЕЛЬ (ВЕСЬ ПОЛ)", 1.0f, 1.0f, 0.0f);
        }
        else {
            // Базовый пол с текстурой (если есть)
            bool hasTexture = (floorTexture.id != 0);

            if (hasTexture) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, floorTexture.id);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                if (ground->useTextureColor) {
                    glColor3f(ground->color.r, ground->color.g, ground->color.b);
                }
                else {
                    glColor3f(1.0f, 1.0f, 1.0f);
                }
            }
            else {
                glDisable(GL_TEXTURE_2D);
                glColor3f(ground->color.r, ground->color.g, ground->color.b);
            }

            // Повторяем текстуру каждую ячейку
            float texRepeatX = worldWidth / currentConfig.cellSize;
            float texRepeatZ = worldDepth / currentConfig.cellSize;

            glBegin(GL_QUADS);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(-offsetX, floorHeight, -offsetZ);
            glTexCoord2f(texRepeatX, 0.0f);
            glVertex3f(offsetX, floorHeight, -offsetZ);
            glTexCoord2f(texRepeatX, texRepeatZ);
            glVertex3f(offsetX, floorHeight, offsetZ);
            glTexCoord2f(0.0f, texRepeatZ);
            glVertex3f(-offsetX, floorHeight, offsetZ);
            glEnd();

            if (hasTexture) {
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D);
            }

            drawText((float)(previewX + 10), (float)(previewY + 25), "БАЗОВЫЙ ПОЛ", 1.0f, 1.0f, 0.0f);

            if (!previewModel.loaded || previewModel.vertices.empty()) {
                drawText((float)(previewX + 10), (float)(previewY + 50), "ЗАГРУЗИТЕ 3D МОДЕЛЬ ДЛЯ ПЛИТОК", 1.0f, 0.7f, 0.0f);
            }
        }

        // Сетка
        if (currentConfig.gridEnabled) {
            glDisable(GL_TEXTURE_2D);
            glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
            glLineWidth(currentConfig.gridLineWidth);
            glBegin(GL_LINES);
            for (int i = -currentConfig.gridWidth / 2; i <= currentConfig.gridWidth / 2; i++) {
                float xPos = (float)i * currentConfig.cellSize;
                glVertex3f(xPos, floorHeight + 0.02f, -offsetZ);
                glVertex3f(xPos, floorHeight + 0.02f, offsetZ);
            }
            for (int i = -currentConfig.gridDepth / 2; i <= currentConfig.gridDepth / 2; i++) {
                float zPos = (float)i * currentConfig.cellSize;
                glVertex3f(-offsetX, floorHeight + 0.02f, zPos);
                glVertex3f(offsetX, floorHeight + 0.02f, zPos);
            }
            glEnd();
            glLineWidth(1.0f);
        }

        glEnable(GL_LIGHTING);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glDisable(GL_SCISSOR_TEST);
        reset2DProjection();

        // UI рамка
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)previewX, (float)previewY);
        glVertex2f((float)(previewX + previewW), (float)previewY);
        glVertex2f((float)(previewX + previewW), (float)(previewY + previewH));
        glVertex2f((float)previewX, (float)(previewY + previewH));
        glEnd();

        // Кнопки управления вращением
        int arrowY = (int)(previewY + previewH + 15);
        int arrowCenterX = (int)(previewX + previewW / 2);

        if (drawButton(arrowCenterX - 70, arrowY, 60, 35, "<-")) {
            previewRotationAngle -= 15.0f;
        }
        if (drawButton(arrowCenterX + 10, arrowY, 60, 35, "->")) {
            previewRotationAngle += 15.0f;
        }
        if (drawButton(arrowCenterX - 135, arrowY, 55, 35, "СБРОС")) {
            previewRotationAngle = 0.0f;
            previewCameraPitch = 25.0f;
            previewCameraDistance = 6.0f;
        }
    }
    else {
        // Небо
        VisualElement* sky = groundSkyElements[1];

        int editX = (int)(windowWidth * 0.18f);
        int editY = (int)(windowHeight * 0.12f);
        int buttonWidth = 90;
        int buttonSpacing = 100;

        drawText((float)editX, (float)(editY - 20), sky->name, 1.0f, 1.0f, 0.0f);

        int textureY = editY;
        drawText((float)editX, (float)textureY, "Текстура:", 1.0f, 1.0f, 1.0f);
        drawText((float)(editX + 100), (float)textureY, truncateFilename(sky->textureFile, 20).c_str(), 0.0f, 1.0f, 0.0f);

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

        int colorY = textureY + 70;
        drawText((float)editX, (float)colorY, "Цвет неба:", 1.0f, 1.0f, 1.0f);

        glColor3f(sky->color.r, sky->color.g, sky->color.b);
        glBegin(GL_QUADS);
        glVertex2f((float)(editX + 100), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY + 12));
        glVertex2f((float)(editX + 100), (float)(colorY + 12));
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(editX + 100), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY - 12));
        glVertex2f((float)(editX + 150), (float)(colorY + 12));
        glVertex2f((float)(editX + 100), (float)(colorY + 12));
        glEnd();

        drawSlider(editX, colorY + 15, 180, &sky->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX, colorY + 45, 180, &sky->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX, colorY + 75, 180, &sky->color.b, 0.0f, 1.0f, "B");

        if (drawButton(editX + 200, colorY + 30, 80, 30, "СБРОСИТЬ")) {
            sky->color = glm::vec3(0.53f, 0.81f, 0.92f);
        }

        if (drawButton(editX, colorY + 110, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            sky->textureFile = "";
            sky->color = glm::vec3(0.53f, 0.81f, 0.92f);
            if (skyTexture.id != 0) {
                glDeleteTextures(1, &skyTexture.id);
                skyTexture.id = 0;
            }
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f((float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f));
        glVertex2f((float)(windowWidth * 0.46f + windowWidth * 0.36f), (float)(windowHeight * 0.12f));
        glVertex2f((float)(windowWidth * 0.46f + windowWidth * 0.36f), (float)(windowHeight * 0.12f + windowHeight * 0.5f));
        glVertex2f((float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f + windowHeight * 0.5f));
        glEnd();

        drawCenteredText((float)(windowHeight * 0.37f), "НЕТ ПРЕДПРОСМОТРА ДЛЯ НЕБА", 1.0f, 1.0f, 0.0f);
    }

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.92f), 150, 50, "НАЗАД")) {
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
    int listWidth = (int)(windowWidth * 0.12f);
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

        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);

        std::string displayFile = truncateFilename(el->modelFile, 30);
        drawText((float)(editX + 80), (float)(modelY + 20), displayFile.c_str(), 0.0f, 1.0f, 0.0f);

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

        int texColorY = colorY + 80;
        // Кнопка для переключения режима текстуры
        bool hasTexture = (previewModel.loaded && !previewModel.materials.empty() && previewModel.materials[0].textureID != 0);
        if (hasTexture) {
            drawText((float)editX, (float)texColorY, "Режим текстуры:", 1.0f, 1.0f, 1.0f);

            std::string texBtnText = std::string("Цвет для текстуры: ") +
                (el->useTextureColor ? "ВКЛ" : "ВЫКЛ");
            if (drawButton(editX, texColorY + 20, 200, 35, texBtnText.c_str())) {
                el->useTextureColor = !el->useTextureColor;
            }

            if (drawButton(editX + 220, texColorY + 20, 100, 35, "СБРОСИТЬ")) {
                el->useTextureColor = true;
            }
        }

        int scaleY = hasTexture ? texColorY + 80 : texColorY;
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

        int countY = scaleY + 70;
        drawText((float)editX, (float)countY, "Количество:", 1.0f, 1.0f, 1.0f);

        char countText[20];
        sprintf_s(countText, "%d", el->count);
        drawText((float)(editX + 120), (float)countY, countText, 1.0f, 1.0f, 0.0f);

        drawIntSlider(editX, countY + 20, 250, &el->count, 0, 1000, "");

        if (drawButton(editX + 260, countY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->count = 10;
            else if (selectedObstacle == 1) el->count = 5;
            else if (selectedObstacle == 2) el->count = 8;
            else el->count = 10;
        }

        if (drawButton(editX, countY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            if (selectedObstacle == 0) {
                el->modelFile = "tree.fbx";
                el->color = glm::vec3(0.1f, 0.4f, 0.1f);
                el->scale = 1.5f;
                el->count = 10;
                el->useTextureColor = true;
            }
            else if (selectedObstacle == 1) {
                el->modelFile = "rock.fbx";
                el->color = glm::vec3(0.5f, 0.5f, 0.5f);
                el->scale = 1.2f;
                el->count = 5;
                el->useTextureColor = true;
            }
            else if (selectedObstacle == 2) {
                el->modelFile = "fence.fbx";
                el->color = glm::vec3(0.6f, 0.4f, 0.2f);
                el->scale = 1.0f;
                el->count = 8;
                el->useTextureColor = true;
            }
            else {
                el->modelFile = "apple.fbx";
                el->color = glm::vec3(1.0f, 0.0f, 0.0f);
                el->scale = 0.8f;
                el->count = 10;
                el->useTextureColor = true;
            }
            updatePreviewForCurrentMode();
        }
    }

    if (selectedObstacle >= 0 && selectedObstacle < (int)obstaclesElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            obstaclesElements[selectedObstacle]->scale, false,
            obstaclesElements[selectedObstacle]->color,
            obstaclesElements[selectedObstacle]->useTextureColor);
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
    int listWidth = (int)(windowWidth * 0.12f);
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

        int modelY = editY;
        drawText((float)editX, (float)(modelY + 20), "Модель:", 1.0f, 1.0f, 1.0f);

        std::string displayFile = truncateFilename(el->modelFile, 30);
        drawText((float)(editX + 80), (float)(modelY + 20), displayFile.c_str(), 0.0f, 1.0f, 0.0f);

        if (drawButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(el->modelFile, folder);
        }

        if (drawButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->modelFile = "flower.fbx";
            else if (selectedEnv == 1) el->modelFile = "bird.fbx";
            else el->modelFile = "cloud.fbx";
            updatePreviewForCurrentMode();
        }

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

        int texColorY = colorY + 80;
        // Кнопка для переключения режима текстуры
        bool hasTexture = (previewModel.loaded && !previewModel.materials.empty() && previewModel.materials[0].textureID != 0);
        if (hasTexture) {
            drawText((float)editX, (float)texColorY, "Режим текстуры:", 1.0f, 1.0f, 1.0f);

            std::string texBtnText = std::string("Цвет для текстуры: ") +
                (el->useTextureColor ? "ВКЛ" : "ВЫКЛ");
            if (drawButton(editX, texColorY + 20, 200, 35, texBtnText.c_str())) {
                el->useTextureColor = !el->useTextureColor;
            }

            if (drawButton(editX + 220, texColorY + 20, 100, 35, "СБРОСИТЬ")) {
                el->useTextureColor = true;
            }
        }

        int scaleY = hasTexture ? texColorY + 80 : texColorY;
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

        if (drawButton(editX, countY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            if (selectedEnv == 0) {
                el->modelFile = "flower.fbx";
                el->color = glm::vec3(1.0f, 0.0f, 1.0f);
                el->scale = 0.7f;
                el->count = 25;
                el->useTextureColor = true;
            }
            else if (selectedEnv == 1) {
                el->modelFile = "bird.fbx";
                el->color = glm::vec3(0.5f, 0.5f, 0.5f);
                el->scale = 0.6f;
                el->count = 15;
                el->useTextureColor = true;
            }
            else {
                el->modelFile = "cloud.fbx";
                el->color = glm::vec3(1.0f, 1.0f, 1.0f);
                el->scale = 1.5f;
                el->count = 20;
                el->useTextureColor = true;
            }
            updatePreviewForCurrentMode();
        }
    }

    if (selectedEnv >= 0 && selectedEnv < (int)environmentElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            (float)(windowWidth * 0.46f), (float)(windowHeight * 0.12f),
            (float)(windowWidth * 0.36f), (float)(windowHeight * 0.5f),
            previewRotation, autoRotate, lastRotationTime,
            environmentElements[selectedEnv]->scale, false,
            environmentElements[selectedEnv]->color,
            environmentElements[selectedEnv]->useTextureColor);
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

    startY += 110;

    // Размер сетки
    drawText((float)startX, (float)(startY - 30), "РАЗМЕР СЕТКИ", 1.0f, 1.0f, 0.0f);
    drawIntSlider(startX, startY, sliderWidth, &currentConfig.gridWidth, 5, 200, "Ширина");
    drawIntSlider(startX, startY + 50, sliderWidth, &currentConfig.gridDepth, 5, 200, "Глубина");
    if (drawButton(startX + sliderWidth + 20, startY + 10, 100, 30, "СБРОСИТЬ")) {
        currentConfig.gridWidth = 120;
        currentConfig.gridDepth = 120;
    }

    startY += 130;

    // Размер ячейки
    drawText((float)startX, (float)(startY - 30), "РАЗМЕР ЯЧЕЙКИ", 1.0f, 1.0f, 0.0f);
    drawSlider(startX, startY, sliderWidth, &currentConfig.cellSize, 0.05f, 1.0f, "Размер");
    if (drawButton(startX + sliderWidth + 20, startY - 10, 100, 30, "СБРОСИТЬ")) {
        currentConfig.cellSize = 0.1f;
    }

    startY += 80;

    // Цвет сетки
    drawColorPicker(startX, startY, "Цвет сетки:", currentConfig.gridColor);
    if (drawButton(startX + sliderWidth + 20, startY + 50, 100, 30, "СБРОСИТЬ")) {
        currentConfig.gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    }

    // Предпросмотр
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
    glm::vec3 eye(10.0f, 8.0f, 12.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
    glLoadMatrixf(glm::value_ptr(view));

    float worldWidth = currentConfig.gridWidth * currentConfig.cellSize;
    float worldDepth = currentConfig.gridDepth * currentConfig.cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    glDisable(GL_LIGHTING);
    glColor3f(currentConfig.floorColor.r, currentConfig.floorColor.g, currentConfig.floorColor.b);
    glBegin(GL_QUADS);
    glVertex3f(-offsetX, -0.5f, -offsetZ);
    glVertex3f(offsetX, -0.5f, -offsetZ);
    glVertex3f(offsetX, -0.5f, offsetZ);
    glVertex3f(-offsetX, -0.5f, offsetZ);
    glEnd();

    if (currentConfig.gridEnabled) {
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
        glLineWidth(currentConfig.gridLineWidth);
        glBegin(GL_LINES);

        for (int i = -currentConfig.gridWidth / 2; i <= currentConfig.gridWidth / 2; i++) {
            float x = (float)i * currentConfig.cellSize;
            glVertex3f(x, -0.4f, -offsetZ);
            glVertex3f(x, -0.4f, offsetZ);
        }
        for (int i = -currentConfig.gridDepth / 2; i <= currentConfig.gridDepth / 2; i++) {
            float z = (float)i * currentConfig.cellSize;
            glVertex3f(-offsetX, -0.4f, z);
            glVertex3f(offsetX, -0.4f, z);
        }
        glEnd();
        glLineWidth(1.0f);
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
    glVertex2f((float)previewX, (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)(previewY + previewH));
    glVertex2f((float)previewX, (float)(previewY + previewH));
    glEnd();

    char infoText[100];
    sprintf_s(infoText, "ПРЕДПРОСМОТР: %dx%d | Ячейка: %.2f",
        currentConfig.gridWidth, currentConfig.gridDepth, currentConfig.cellSize);
    drawText((float)(previewX + 10), (float)(previewY + 25), infoText, 1.0f, 1.0f, 0.0f);

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР СВЕТА
//=============================================================================
void setupFixedPipelineLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    // Глобальная ambient
    if (currentConfig.ambientEnabled) {
        GLfloat global_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    }
    else {
        GLfloat global_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    }

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    switch (currentConfig.lightType) {
    case 0: { // Directional
        GLfloat light0_ambient[] = { currentConfig.ambientEnabled ? 0.3f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.3f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.3f : 0.0f, 1.0f };
        GLfloat light0_diffuse[] = { currentConfig.lightColor.r, currentConfig.lightColor.g,
                                     currentConfig.lightColor.b, 1.0f };
        GLfloat light0_specular[] = { currentConfig.specularEnabled ? 0.5f : 0.0f,
                                      currentConfig.specularEnabled ? 0.5f : 0.0f,
                                      currentConfig.specularEnabled ? 0.5f : 0.0f, 1.0f };
        GLfloat light0_position[] = { -currentConfig.lightDir.x, -currentConfig.lightDir.y,
                                      -currentConfig.lightDir.z, 0.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        glDisable(GL_LIGHT1);
        break;
    }
    case 1: { // Point
        GLfloat light0_ambient[] = { currentConfig.ambientEnabled ? 0.2f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.2f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.2f : 0.0f, 1.0f };
        GLfloat light0_diffuse[] = { currentConfig.lightColor.r * 0.9f,
                                     currentConfig.lightColor.g * 0.9f,
                                     currentConfig.lightColor.b * 0.9f, 1.0f };
        GLfloat light0_specular[] = { currentConfig.specularEnabled ? 0.4f : 0.0f,
                                      currentConfig.specularEnabled ? 0.4f : 0.0f,
                                      currentConfig.specularEnabled ? 0.4f : 0.0f, 1.0f };
        GLfloat light0_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y,
                                      currentConfig.lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.8f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.07f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.02f);
        glDisable(GL_LIGHT1);
        break;
    }
    case 2: { // Spot
        GLfloat light0_ambient[] = { currentConfig.ambientEnabled ? 0.15f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.15f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.15f : 0.0f, 1.0f };
        GLfloat light0_diffuse[] = { currentConfig.lightColor.r * 1.5f,
                                     currentConfig.lightColor.g * 1.5f,
                                     currentConfig.lightColor.b * 1.5f, 1.0f };
        GLfloat light0_specular[] = { currentConfig.specularEnabled ? 0.7f : 0.0f,
                                      currentConfig.specularEnabled ? 0.7f : 0.0f,
                                      currentConfig.specularEnabled ? 0.7f : 0.0f, 1.0f };
        GLfloat light0_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y,
                                      currentConfig.lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

        GLfloat spot_direction[] = { 0.0f, -1.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
        glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f);
        glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 2.0f);

        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.03f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

        GLfloat light1_ambient[] = { currentConfig.ambientEnabled ? 0.2f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.2f : 0.0f,
                                     currentConfig.ambientEnabled ? 0.2f : 0.0f, 1.0f };
        GLfloat light1_diffuse[] = { 0.25f, 0.25f, 0.25f, 1.0f };
        GLfloat light1_position[] = { 0.0f, 5.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
        glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
        break;
    }
    }
}
void renderLightEditor() {
    g_currentPreviewMode = PREVIEW_LIGHT;
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.05f), "НАСТРОЙКИ СВЕТА", 1.0f, 1.0f, 0.0f);

    int startX = (int)(windowWidth * 0.05f);
    int startY = (int)(windowHeight * 0.12f);
    int sliderWidth = (int)(windowWidth * 0.25f);

    drawText((float)startX, (float)(startY - 30), "ТИП ИСТОЧНИКА СВЕТА", 1.0f, 1.0f, 0.0f);

    bool lightChanged = false;

    if (drawButton(startX, startY, 180, 40, "Направленный")) {
        currentConfig.lightType = 0;
        lightChanged = true;
    }
    if (drawButton(startX + 200, startY, 180, 40, "Точечный")) {
        currentConfig.lightType = 1;
        lightChanged = true;
    }
    if (drawButton(startX + 400, startY, 180, 40, "Прожектор")) {
        currentConfig.lightType = 2;
        lightChanged = true;
    }

    startY += 60;

    drawColorPicker(startX, startY, "Цвет света:", currentConfig.lightColor);
    if (drawButton(startX + sliderWidth + 20, startY + 50, 100, 30, "СБРОСИТЬ")) {
        currentConfig.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        lightChanged = true;
    }

    startY += 120;

    if (currentConfig.lightType == 0) {
        drawText((float)startX, (float)(startY - 30), "НАПРАВЛЕНИЕ СВЕТА", 1.0f, 1.0f, 0.0f);
        if (drawSlider(startX, startY, sliderWidth, &currentConfig.lightDir.x, -1.0f, 1.0f, "X")) lightChanged = true;
        if (drawSlider(startX, startY + 50, sliderWidth, &currentConfig.lightDir.y, -1.0f, 1.0f, "Y")) lightChanged = true;
        if (drawSlider(startX, startY + 100, sliderWidth, &currentConfig.lightDir.z, -1.0f, 1.0f, "Z")) lightChanged = true;
        if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ")) {
            currentConfig.lightDir = glm::vec3(-1.0f, -1.0f, 0.5f);
            currentConfig.lightDir = glm::normalize(currentConfig.lightDir);
            lightChanged = true;
        }
    }
    else {
        drawText((float)startX, (float)(startY - 30), "ПОЗИЦИЯ СВЕТА", 1.0f, 1.0f, 0.0f);
        if (drawSlider(startX, startY, sliderWidth, &currentConfig.lightPos.x, -10.0f, 10.0f, "X")) lightChanged = true;
        if (drawSlider(startX, startY + 50, sliderWidth, &currentConfig.lightPos.y, 0.0f, 15.0f, "Y")) lightChanged = true;
        if (drawSlider(startX, startY + 100, sliderWidth, &currentConfig.lightPos.z, -10.0f, 10.0f, "Z")) lightChanged = true;
        if (drawButton(startX + sliderWidth + 20, startY, 150, 35, "СБРОСИТЬ")) {
            currentConfig.lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
            lightChanged = true;
        }
    }

    if (lightChanged) {
        setupFixedPipelineLighting();
        updateLightPosition();
        g_previewShadowsDirty = true;
        g_forceBoundsRecalc = true;
    }

    // 3D предпросмотр с источником света
    renderShadowPreview3D();

    if (drawButton((int)(windowWidth * 0.03f), (int)(windowHeight * 0.9f), 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ТЕНЕЙ
//=============================================================================
// Функция для вычисления bounding sphere модели (как в GameRenderer::rayIntersectsModel)
// ========== ФУНКЦИИ ТОЧНОЙ ГЕОМЕТРИИ ИЗ GAMERENDERER ==========
// ========== ФУНКЦИИ ТОЧНОЙ ГЕОМЕТРИИ ИЗ GAMERENDERER ==========

// ========== СТРУКТУРА ЛУЧА ==========


// ========== ПРОВЕРКА ПЕРЕСЕЧЕНИЯ ЛУЧА СО СФЕРОЙ ==========
// ========== ПРОВЕРКА ПЕРЕСЕЧЕНИЯ ЛУЧА СО СФЕРОЙ (как в GameRenderer) ==========
// ИЗ GameRenderer.cpp - точная копия
bool rayIntersectsSphere(const Ray& ray, const glm::vec3& center, float radius, float& tHit) {
    glm::vec3 oc = ray.origin - center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float sqrtD = sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    if (t1 > 0.01f) {
        tHit = t1;
        return true;
    }
    if (t2 > 0.01f) {
        tHit = t2;
        return true;
    }

    return false;
}
// Функция пересечения луча со сферой (уже есть в файле, но продублируем для уверенности)
bool rayIntersectsSphereEditor(const Ray& ray, const glm::vec3& center, float radius, float& tHit) {
    glm::vec3 oc = ray.origin - center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float sqrtD = sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    if (t1 > 0.01f) {
        tHit = t1;
        return true;
    }
    if (t2 > 0.01f) {
        tHit = t2;
        return true;
    }

    return false;
}float calculateSegmentRotationEditor(const std::vector<glm::vec3>& snakePositions, size_t index) {
    if (snakePositions.size() <= 1) return 0.0f;

    if (index == 0) {
        if (snakePositions[0].x > snakePositions[1].x) return 90.0f;
        if (snakePositions[0].x < snakePositions[1].x) return -90.0f;
        if (snakePositions[0].z > snakePositions[1].z) return 0.0f;
        if (snakePositions[0].z < snakePositions[1].z) return 180.0f;
    }
    else {
        if (snakePositions[index].x > snakePositions[index - 1].x) return 90.0f;
        if (snakePositions[index].x < snakePositions[index - 1].x) return -90.0f;
        if (snakePositions[index].z > snakePositions[index - 1].z) return 0.0f;
        if (snakePositions[index].z < snakePositions[index - 1].z) return 180.0f;
    }

    return 0.0f;
}
// ========== ПРОВЕРКА ПЕРЕСЕЧЕНИЯ ЛУЧА С МОДЕЛЬЮ (ТОЧНАЯ ГЕОМЕТРИЯ) ==========
// ========== ПРОВЕРКА ПЕРЕСЕЧЕНИЯ ЛУЧА С МОДЕЛЬЮ (ТОЧНАЯ ГЕОМЕТРИЯ, как в GameRenderer) ==========
// ========== ПРОВЕРКА ПЕРЕСЕЧЕНИЯ ЛУЧА С МОДЕЛЬЮ (ТОЧНАЯ ГЕОМЕТРИЯ) ==========
// ИЗ GameRenderer.cpp - точная копия с кэшем
// Добавить после других функций в ConfigEditor.cpp
bool rayIntersectsModel(const Ray& ray, const Model& model,
    const glm::mat4& transform,
    float& hitDistance, glm::vec3& hitPoint) {

    if (model.vertices.empty()) {
        return false;
    }

    static int callCount = 0;
    callCount++;
    bool shouldLog = (callCount <= 20);

    if (shouldLog) {
        std::cout << "[rayIntersectsModel #" << callCount << "] Model has "
            << model.vertices.size() << " vertices" << std::endl;
        std::cout << "  Ray origin: (" << ray.origin.x << "," << ray.origin.y << "," << ray.origin.z << ")" << std::endl;
        std::cout << "  Ray direction: (" << ray.direction.x << "," << ray.direction.y << "," << ray.direction.z << ")" << std::endl;
    }

    float closestHit = 1000.0f;
    bool hit = false;
    const float EPSILON = 0.000001f;

    for (size_t i = 0; i < model.vertices.size(); i += 3) {
        if (i + 2 >= model.vertices.size()) break;

        glm::vec3 v0 = glm::vec3(transform * glm::vec4(model.vertices[i].position, 1.0f));
        glm::vec3 v1 = glm::vec3(transform * glm::vec4(model.vertices[i + 1].position, 1.0f));
        glm::vec3 v2 = glm::vec3(transform * glm::vec4(model.vertices[i + 2].position, 1.0f));

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) continue;

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f) continue;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f) continue;

        float t = f * glm::dot(edge2, q);

        if (t > EPSILON && t < closestHit) {
            closestHit = t;
            hitPoint = ray.pointAt(t);
            hit = true;

            if (shouldLog) {
                std::cout << "  HIT at distance " << t << std::endl;
            }
            // Не возвращаем сразу - ищем ближайшее
        }
    }

    if (shouldLog) {
        std::cout << "  Result: " << (hit ? "HIT" : "MISS") << std::endl;
    }

    if (hit) {
        hitDistance = closestHit;
    }

    return hit;
}
// В ConfigEditor.cpp, добавьте эту функцию для отладки деревьев:

HitInfo intersectTreesOnlyEditor(const Ray& ray,
    const std::vector<glm::vec3>& treePositions,
    const Model& treeModel, float treeScale, float treeOffset,
    float offsetX, float offsetZ, float floorHeight, float cellSize) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;
    float maxDistance = 100.0f;
    float halfWidth = 120 * cellSize / 2.0f;
    float halfDepth = 120 * cellSize / 2.0f;

    // Вычисляем bounding box модели дерева
    static float treeMinY = FLT_MAX, treeMaxY = -FLT_MAX;
    static float treeRadius = 0.0f;
    static bool boundsComputed = false;

    if (!boundsComputed && !treeModel.vertices.empty()) {
        for (const auto& vert : treeModel.vertices) {
            treeMinY = std::min(treeMinY, vert.position.y);
            treeMaxY = std::max(treeMaxY, vert.position.y);
        }
        treeRadius = (treeMaxY - treeMinY) * 0.5f;
        boundsComputed = true;
        std::cout << "\n========== TREE BOUNDS ==========" << std::endl;
        std::cout << "Tree minY: " << treeMinY << ", maxY: " << treeMaxY << std::endl;
        std::cout << "Tree radius (local): " << treeRadius << std::endl;
        std::cout << "Tree offset: " << treeOffset << ", scale: " << treeScale << std::endl;
        std::cout << "=================================" << std::endl;
    }

    // ДИАГНОСТИКА ДЛЯ ДЕРЕВЬЕВ
    static int treeRayCounter = 0;
    bool shouldLogTree = (treeRayCounter++ < 50);

    if (shouldLogTree && !treeModel.vertices.empty()) {
        std::cout << "\n[TREE RAY #" << treeRayCounter << "] Ray origin: ("
            << ray.origin.x << ", " << ray.origin.y << ", " << ray.origin.z << ")" << std::endl;
        std::cout << "  Ray direction: (" << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << ")" << std::endl;
    }

    // Проверяем деревья
    for (const auto& treePos : treePositions) {
        float x = treePos.x;
        float z = treePos.z;

        // Вычисляем позицию дерева
        float bottomOffset = treeOffset * treeScale;
        float centerY = floorHeight + bottomOffset + treeRadius * treeScale;

        glm::vec3 treeCenter(x, centerY, z);
        float boundingRadius = treeRadius * treeScale * 1.2f;

        if (shouldLogTree) {
            std::cout << "  Tree at (" << x << ", " << centerY << ", " << z
                << ") radius=" << boundingRadius << std::endl;
        }

        float tSphere;
        if (rayIntersectsSphereEditor(ray, treeCenter, boundingRadius, tSphere)) {
            if (shouldLogTree) {
                std::cout << "    SPHERE HIT at distance: " << tSphere << std::endl;
            }

            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(x, floorHeight + bottomOffset, z));
            transform = glm::scale(transform, glm::vec3(treeScale));

            float hitDist;
            glm::vec3 hitPt;
            if (rayIntersectsModel(ray, treeModel, transform, hitDist, hitPt)) {
                if (shouldLogTree) {
                    std::cout << "    MODEL HIT at distance: " << hitDist
                        << ", point=(" << hitPt.x << "," << hitPt.y << "," << hitPt.z << ")" << std::endl;
                }
                if (hitDist > 0.01f && hitDist < closestHit.distance) {
                    closestHit.hit = true;
                    closestHit.distance = hitDist;
                    closestHit.point = hitPt;
                    return closestHit;  // Возвращаем сразу при первом попадании
                }
            }
            else if (shouldLogTree) {
                std::cout << "    MODEL MISS (no triangle intersection)" << std::endl;
            }
        }
        else if (shouldLogTree) {
            std::cout << "    SPHERE MISS" << std::endl;
        }
    }

    // Если не нашли деревья, проверяем пол
    if (!closestHit.hit) {
        float tGround = -ray.origin.y / ray.direction.y;
        if (tGround > 0.01f && tGround < maxDistance) {
            glm::vec3 hitPoint = ray.pointAt(tGround);
            if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
                closestHit.hit = true;
                closestHit.distance = tGround;
                closestHit.point = hitPoint;
                if (shouldLogTree) {
                    std::cout << "  GROUND HIT at distance: " << tGround
                        << ", point=(" << hitPoint.x << "," << hitPoint.y << "," << hitPoint.z << ")" << std::endl;
                }
            }
        }
    }

    return closestHit;
}

// Аналогично для intersectFoodOnlyEditor:

HitInfo intersectFoodOnlyEditor(const Ray& ray,
    const std::vector<glm::vec3>& applePositions,
    const Model& appleModel, float appleScale, float appleOffset,
    float offsetX, float offsetZ, float floorHeight, float cellSize) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;
    float maxDistance = 100.0f;
    float halfWidth = 120 * cellSize / 2.0f;
    float halfDepth = 120 * cellSize / 2.0f;

    // Вычисляем bounding box модели яблока
    static float appleMinY = FLT_MAX, appleMaxY = -FLT_MAX;
    static float appleRadius = 0.0f;
    static bool boundsComputed = false;

    if (!boundsComputed && !appleModel.vertices.empty()) {
        for (const auto& vert : appleModel.vertices) {
            appleMinY = std::min(appleMinY, vert.position.y);
            appleMaxY = std::max(appleMaxY, vert.position.y);
        }
        appleRadius = (appleMaxY - appleMinY) * 0.5f;
        boundsComputed = true;
        std::cout << "\n========== APPLE BOUNDS ==========" << std::endl;
        std::cout << "Apple minY: " << appleMinY << ", maxY: " << appleMaxY << std::endl;
        std::cout << "Apple radius (local): " << appleRadius << std::endl;
        std::cout << "Apple offset: " << appleOffset << ", scale: " << appleScale << std::endl;
        std::cout << "==================================" << std::endl;
    }

    static int appleRayCounter = 0;
    bool shouldLogApple = (appleRayCounter++ < 30);

    // Проверяем яблоки
    for (const auto& applePos : applePositions) {
        float centerX = applePos.x;
        float centerZ = applePos.z;

        float worldBottomOffset = appleOffset * appleScale;
        float centerY = floorHeight + worldBottomOffset + appleRadius * appleScale;

        glm::vec3 center(centerX, centerY, centerZ);
        float boundingRadius = appleRadius * appleScale * 1.2f;

        if (shouldLogApple) {
            std::cout << "[APPLE CHECK] Apple at (" << centerX << ", " << centerY << ", " << centerZ
                << ") radius=" << boundingRadius << std::endl;
        }

        float tSphere;
        if (rayIntersectsSphereEditor(ray, center, boundingRadius, tSphere)) {
            if (shouldLogApple) {
                std::cout << "  SPHERE HIT at distance: " << tSphere << std::endl;
            }

            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, center);
            transform = glm::scale(transform, glm::vec3(appleScale));

            float hitDist;
            glm::vec3 hitPt;
            if (rayIntersectsModel(ray, appleModel, transform, hitDist, hitPt)) {
                if (shouldLogApple) {
                    std::cout << "  MODEL HIT at distance: " << hitDist
                        << ", point=(" << hitPt.x << "," << hitPt.y << "," << hitPt.z << ")" << std::endl;
                }
                if (hitDist > 0.01f && hitDist < closestHit.distance) {
                    closestHit.hit = true;
                    closestHit.distance = hitDist;
                    closestHit.point = hitPt;
                    return closestHit;
                }
            }
            else if (shouldLogApple) {
                std::cout << "  MODEL MISS" << std::endl;
            }
        }
    }

    // Если не нашли яблоки, проверяем пол
    if (!closestHit.hit) {
        float tGround = -ray.origin.y / ray.direction.y;
        if (tGround > 0.01f && tGround < maxDistance) {
            glm::vec3 hitPoint = ray.pointAt(tGround);
            if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
                closestHit.hit = true;
                closestHit.distance = tGround;
                closestHit.point = hitPoint;
            }
        }
    }

    return closestHit;
}
HitInfo intersectSnakeOnlyEditor(const Ray& ray,
    const std::vector<glm::vec3>& snakePositions,
    const Model& snakeHeadModel, const Model& snakeBodyModel, const Model& snakeTailModel,
    float headScale, float bodyScale, float tailScale,
    float headOffset, float bodyOffset, float tailOffset,
    float offsetX, float offsetZ) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;

    if (snakeHeadModel.vertices.empty()) return closestHit;

    for (size_t i = 0; i < snakePositions.size(); i++) {
        const glm::vec3& pos = snakePositions[i];

        float x = pos.x;
        float z = pos.z;
        float y = pos.y;

        float scale;
        const Model* model = nullptr;
        float offset;

        if (i == 0) {
            scale = headScale;
            model = &snakeHeadModel;
            offset = headOffset;
        }
        else if (i == snakePositions.size() - 1) {
            scale = tailScale;
            model = &snakeTailModel;
            offset = tailOffset;
        }
        else {
            scale = bodyScale;
            model = &snakeBodyModel;
            offset = bodyOffset;
        }

        float segmentRadius = scale * 0.6f;
        glm::vec3 center(x, y + offset * scale, z);

        float tSphere;
        if (rayIntersectsSphereEditor(ray, center, segmentRadius, tSphere)) {
            if (!model->vertices.empty()) {
                float rotationAngle = calculateSegmentRotationEditor(snakePositions, i);
                glm::mat4 transform = glm::mat4(1.0f);
                transform = glm::translate(transform, glm::vec3(x, y + offset * scale, z));
                transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                transform = glm::scale(transform, glm::vec3(scale));

                float hitDist;
                glm::vec3 hitPt;
                if (rayIntersectsModel(ray, *model, transform, hitDist, hitPt)) {
                    if (hitDist > 0.01f && hitDist < closestHit.distance) {
                        closestHit.hit = true;
                        closestHit.distance = hitDist;
                        closestHit.point = hitPt;
                    }
                }
            }
        }
    }

    return closestHit;
}

// ========== ВЫЧИСЛЕНИЕ ПОВОРОТА СЕГМЕНТА ЗМЕЙКИ ==========
// ========== ВЫЧИСЛЕНИЕ ПОВОРОТА СЕГМЕНТА ЗМЕЙКИ (как в GameRenderer) ==========
// ИЗ GameRenderer.cpp - точная копия
float calculateSegmentRotation(const std::vector<glm::vec3>& snakePositions, size_t index) {
    if (snakePositions.size() <= 1) return 0.0f;

    if (index == 0) {
        if (snakePositions[0].x > snakePositions[1].x) return 90.0f;
        if (snakePositions[0].x < snakePositions[1].x) return -90.0f;
        if (snakePositions[0].z > snakePositions[1].z) return 0.0f;
        if (snakePositions[0].z < snakePositions[1].z) return 180.0f;
    }
    else {
        if (snakePositions[index].x > snakePositions[index - 1].x) return 90.0f;
        if (snakePositions[index].x < snakePositions[index - 1].x) return -90.0f;
        if (snakePositions[index].z > snakePositions[index - 1].z) return 0.0f;
        if (snakePositions[index].z < snakePositions[index - 1].z) return 180.0f;
    }

    return 0.0f;
}
// ИЗ GameRenderer.cpp - точная копия
void setupDirectionalLight() {
    GLfloat light0_ambient[] = {
        currentConfig.ambientEnabled ? 0.3f : 0.0f,
        currentConfig.ambientEnabled ? 0.3f : 0.0f,
        currentConfig.ambientEnabled ? 0.3f : 0.0f,
        1.0f
    };

    GLfloat light0_diffuse[] = { currentConfig.lightColor.r, currentConfig.lightColor.g, currentConfig.lightColor.b, 1.0f };

    GLfloat light0_specular[] = {
        currentConfig.specularEnabled ? 0.5f : 0.0f,
        currentConfig.specularEnabled ? 0.5f : 0.0f,
        currentConfig.specularEnabled ? 0.5f : 0.0f,
        1.0f
    };

    GLfloat light0_position[] = { -currentConfig.lightDir.x, -currentConfig.lightDir.y, -currentConfig.lightDir.z, 0.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    glDisable(GL_LIGHT1);
}

void setupPointLight() {
    GLfloat light0_ambient[] = {
        currentConfig.ambientEnabled ? 0.2f : 0.0f,
        currentConfig.ambientEnabled ? 0.2f : 0.0f,
        currentConfig.ambientEnabled ? 0.2f : 0.0f,
        1.0f
    };

    GLfloat light0_diffuse[] = { currentConfig.lightColor.r * 0.9f, currentConfig.lightColor.g * 0.9f, currentConfig.lightColor.b * 0.9f, 1.0f };

    GLfloat light0_specular[] = {
        currentConfig.specularEnabled ? 0.4f : 0.0f,
        currentConfig.specularEnabled ? 0.4f : 0.0f,
        currentConfig.specularEnabled ? 0.4f : 0.0f,
        1.0f
    };

    GLfloat light0_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y, currentConfig.lightPos.z, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.07f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.02f);

    glDisable(GL_LIGHT1);
}

void setupSpotLight() {
    GLfloat light0_ambient[] = {
        currentConfig.ambientEnabled ? 0.15f : 0.0f,
        currentConfig.ambientEnabled ? 0.15f : 0.0f,
        currentConfig.ambientEnabled ? 0.15f : 0.0f,
        1.0f
    };

    GLfloat light0_diffuse[] = { currentConfig.lightColor.r * 1.5f, currentConfig.lightColor.g * 1.5f, currentConfig.lightColor.b * 1.5f, 1.0f };

    GLfloat light0_specular[] = {
        currentConfig.specularEnabled ? 0.7f : 0.0f,
        currentConfig.specularEnabled ? 0.7f : 0.0f,
        currentConfig.specularEnabled ? 0.7f : 0.0f,
        1.0f
    };

    GLfloat light0_position[] = { currentConfig.lightPos.x, currentConfig.lightPos.y, currentConfig.lightPos.z, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    GLfloat spot_direction[] = { 0.0f, -1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 2.0f);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.03f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

    GLfloat light1_ambient[] = {
        currentConfig.ambientEnabled ? 0.2f : 0.0f,
        currentConfig.ambientEnabled ? 0.2f : 0.0f,
        currentConfig.ambientEnabled ? 0.2f : 0.0f,
        1.0f
    };
    GLfloat light1_diffuse[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat light1_position[] = { 0.0f, 5.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
}
// ИЗ GameRenderer.cpp - точная копия

static void getModelBoundingSphere(const Model& model, float scale, glm::vec3& center, float& radius) {
    if (model.vertices.empty()) {
        center = glm::vec3(0.0f);
        radius = 0.5f;
        return;
    }

    float minX = model.vertices[0].position.x;
    float maxX = minX, minY = minX, maxY = minX, minZ = minX, maxZ = minX;
    for (const auto& v : model.vertices) {
        minX = std::min(minX, v.position.x);
        maxX = std::max(maxX, v.position.x);
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
        minZ = std::min(minZ, v.position.z);
        maxZ = std::max(maxZ, v.position.z);
    }

    center = glm::vec3((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f) * scale;
    radius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f * scale;
}
// ========== ФУНКЦИИ ДЛЯ ВИЗУАЛИЗАЦИИ ЛУЧЕЙ В РЕДАКТОРЕ ТЕНЕЙ ==========


// Очистка лучей
void clearDebugRays3D() {
    g_debugRays3D.clear();
}

// Добавление луча
void addDebugRay3D(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& hitPoint, bool hit, float distance,
    const std::string& objectType) {
    if (!g_rayDebugEnabled) return;
    DebugRay3D ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.hitPoint = hitPoint;
    ray.hit = hit;
    ray.distance = distance;
    ray.objectType = objectType;
    g_debugRays3D.push_back(ray);
}

// Рисование луча в 3D
void drawDebugRay3D(const DebugRay3D& ray) {
    glm::vec3 endPoint;
    if (ray.hit) {
        endPoint = ray.hitPoint;
    }
    else {
        endPoint = ray.origin + ray.direction * ray.distance;
    }

    // Луч: красный если попал в модель (тень), жёлтый если не попал (свет)
    if (ray.hit) {
        glColor3f(1.0f, 0.2f, 0.2f);  // Красный - попал в объект (тень)
    }
    else {
        glColor3f(1.0f, 1.0f, 0.2f);  // Жёлтый - достиг света (нет тени)
    }

    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
    glVertex3f(endPoint.x, endPoint.y, endPoint.z);
    glEnd();
    glLineWidth(1.0f);
}

// Генерация тестовых лучей для модели
void generateRaysForModel(const glm::vec3& modelCenter, float modelRadius,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType,
    std::function<bool(const glm::vec3&, float&, glm::vec3&)> intersectFunc,
    const std::string& objectType) {

    // Генерируем 10 случайных точек вокруг модели
    std::vector<glm::vec3> testPoints;

    // Используем равномерное распределение вокруг сферы
    for (int i = 0; i < 10; i++) {
        // Используем золотое сечение для равномерного распределения точек на сфере
        float theta = 2.0f * 3.14159f * (i * 0.618033988749895f);
        float phi = acos(1.0f - 2.0f * ((i + 0.5f) / 10.0f));

        float x = sin(phi) * cos(theta);
        float y = sin(phi) * sin(theta);
        float z = cos(phi);

        glm::vec3 offset = glm::vec3(x, y, z) * (modelRadius + 0.1f);
        glm::vec3 point = modelCenter + offset;
        point.y = std::max(point.y, 0.05f);  // Не ниже пола

        testPoints.push_back(point);
    }

    // Для каждой точки трассируем луч к свету
    for (const auto& point : testPoints) {
        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;
        float maxDistance = 100.0f;

        switch (lightType) {
        case LightType::Directional:
            rayOrigin = point - lightDir * 50.0f;
            rayDirection = lightDir;
            maxDistance = glm::distance(rayOrigin, point);
            break;
        case LightType::Points:
        case LightType::Spot:
            rayOrigin = lightPos;
            rayDirection = glm::normalize(point - lightPos);
            maxDistance = glm::distance(lightPos, point);
            break;
        }

        float hitDist;
        glm::vec3 hitPoint;
        bool hit = intersectFunc(point, hitDist, hitPoint);

        // Определяем конечную точку луча
        glm::vec3 endPoint;
        if (hit) {
            endPoint = hitPoint;
        }
        else {
            endPoint = point;
        }

        addDebugRay3D(rayOrigin, rayDirection, endPoint, hit,
            hit ? hitDist : maxDistance, objectType);
    }
}

// Версия для деревьев (с учётом смещения)
void generateRaysForTree(const glm::vec3& treePos, float treeScale, float treeOffset,
    const Model& treeModel,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType) {
    if (treeModel.vertices.empty()) return;

    // Вычисляем bounding box модели
    float minY = FLT_MAX, maxY = -FLT_MAX;
    for (const auto& v : treeModel.vertices) {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }
    float modelHeight = (maxY - minY) * treeScale;
    glm::vec3 worldCenter = treePos + glm::vec3(0.0f, treeOffset * treeScale + modelHeight * 0.5f, 0.0f);
    float worldRadius = modelHeight * 0.6f;

    auto intersectFunc = [&](const glm::vec3& point, float& hitDist, glm::vec3& hitPoint) -> bool {
        Ray ray;
        ray.origin = point;
        if (lightType == LightType::Directional) {
            ray.direction = -lightDir;
        }
        else {
            ray.direction = glm::normalize(lightPos - point);
        }

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(treePos.x, treePos.y + treeOffset * treeScale, treePos.z));
        transform = glm::scale(transform, glm::vec3(treeScale));

        return rayIntersectsModel(ray, treeModel, transform, hitDist, hitPoint);
        };

    generateRaysForModel(worldCenter, worldRadius, lightDir, lightPos, lightType, intersectFunc, "tree");
}

// Версия для змейки
void generateRaysForSnakeSegment(const glm::vec3& pos, const Model& model,
    float scale, float offset, float rotation,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType, const std::string& segmentName) {
    if (model.vertices.empty()) return;

    // Вычисляем bounding box
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;
    for (const auto& v : model.vertices) {
        minX = std::min(minX, v.position.x);
        maxX = std::max(maxX, v.position.x);
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
        minZ = std::min(minZ, v.position.z);
        maxZ = std::max(maxZ, v.position.z);
    }

    float modelWidth = (maxX - minX) * scale;
    float modelHeight = (maxY - minY) * scale;
    float modelDepth = (maxZ - minZ) * scale;

    glm::vec3 worldCenter = pos + glm::vec3(0.0f, offset * scale + modelHeight * 0.5f, 0.0f);
    float worldRadius = std::max({ modelWidth, modelHeight, modelDepth }) * 0.6f;

    auto intersectFunc = [&](const glm::vec3& point, float& hitDist, glm::vec3& hitPoint) -> bool {
        Ray ray;
        ray.origin = point;
        if (lightType == LightType::Directional) {
            ray.direction = -lightDir;
        }
        else {
            ray.direction = glm::normalize(lightPos - point);
        }

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(pos.x, pos.y + offset * scale, pos.z));
        if (rotation != 0.0f) transform = glm::rotate(transform, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(scale));

        return rayIntersectsModel(ray, model, transform, hitDist, hitPoint);
        };

    generateRaysForModel(worldCenter, worldRadius, lightDir, lightPos, lightType, intersectFunc, segmentName);
}

// Версия для яблок
void generateRaysForApple(const glm::vec3& applePos, float appleScale, float appleOffset,
    const Model& appleModel,
    const glm::vec3& lightDir, const glm::vec3& lightPos,
    LightType lightType) {
    if (appleModel.vertices.empty()) return;

    // Вычисляем bounding box
    float minY = FLT_MAX, maxY = -FLT_MAX;
    for (const auto& v : appleModel.vertices) {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }
    float modelHeight = (maxY - minY) * appleScale;
    glm::vec3 worldCenter = applePos + glm::vec3(0.0f, appleOffset * appleScale + modelHeight * 0.5f, 0.0f);
    float worldRadius = modelHeight * 0.6f;

    auto intersectFunc = [&](const glm::vec3& point, float& hitDist, glm::vec3& hitPoint) -> bool {
        Ray ray;
        ray.origin = point;
        if (lightType == LightType::Directional) {
            ray.direction = -lightDir;
        }
        else {
            ray.direction = glm::normalize(lightPos - point);
        }

        glm::vec3 center(applePos.x, applePos.y + appleOffset * appleScale, applePos.z);
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, center);
        transform = glm::scale(transform, glm::vec3(appleScale));

        return rayIntersectsModel(ray, appleModel, transform, hitDist, hitPoint);
        };

    generateRaysForModel(worldCenter, worldRadius, lightDir, lightPos, lightType, intersectFunc, "apple");
}
// ========== ФУНКЦИЯ ПЕРЕСЕЧЕНИЯ ЛУЧА С МОДЕЛЬЮ ДЛЯ РЕДАКТОРА ==========
bool rayIntersectsModelEditor(const Ray& ray, const Model& model,
    const glm::mat4& transform,
    float& hitDistance, glm::vec3& hitPoint,
    bool skipSphereCheck = false) {

    if (model.vertices.empty()) return false;

    const float EPSILON = 0.000001f;
    float closestHit = 1000.0f;
    bool hit = false;

    for (size_t i = 0; i < model.vertices.size(); i += 3) {
        if (i + 2 >= model.vertices.size()) break;

        glm::vec3 v0 = glm::vec3(transform * glm::vec4(model.vertices[i].position, 1.0f));
        glm::vec3 v1 = glm::vec3(transform * glm::vec4(model.vertices[i + 1].position, 1.0f));
        glm::vec3 v2 = glm::vec3(transform * glm::vec4(model.vertices[i + 2].position, 1.0f));

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) continue;

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f) continue;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f) continue;

        float t = f * glm::dot(edge2, q);

        if (t > EPSILON && t < closestHit) {
            closestHit = t;
            hitPoint = ray.pointAt(t);
            hit = true;
        }
    }

    if (hit) {
        hitDistance = closestHit;
    }

    return hit;
}
// ИСПРАВЛЕННАЯ ФУНКЦИЯ - свет НЕ зависит от камеры
// ========== ИСПРАВЛЕННАЯ ФУНКЦИЯ - свет НЕ зависит от камеры ==========
void renderShadowPreview3D() {
    int previewX = (int)(windowWidth * 0.55f);
    int previewY = (int)(windowHeight * 0.12f);
    int previewW = (int)(windowWidth * 0.42f);
    int previewH = (int)(windowHeight * 0.65f);

    previewHoverX = previewX;
    previewHoverY = previewY;
    previewHoverW = previewW;
    previewHoverH = previewH;

    previewHovered = (mouseX >= previewX && mouseX <= previewX + previewW &&
        mouseY >= previewY && mouseY <= previewY + previewH);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glScissor(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    float aspect = (float)previewW / previewH;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Управление камерой
    static float camX = 0.0f;
    static float camY = 2.5f;
    static float camZ = 8.0f;
    static float camYaw = 0.0f;
    static float camPitch = 25.0f;
    static float camDistance = 8.0f;

    const float moveSpeed = 5.0f;

    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);
    lastTime = currentTime;
    if (deltaTime > 0.033f) deltaTime = 0.033f;

    if (previewHovered) {
        camDistance = previewCameraDistance;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camX += sin(glm::radians(camYaw)) * moveSpeed * deltaTime;
        camZ += cos(glm::radians(camYaw)) * moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camX -= sin(glm::radians(camYaw)) * moveSpeed * deltaTime;
        camZ -= cos(glm::radians(camYaw)) * moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camX -= cos(glm::radians(camYaw)) * moveSpeed * deltaTime;
        camZ += sin(glm::radians(camYaw)) * moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camX += cos(glm::radians(camYaw)) * moveSpeed * deltaTime;
        camZ -= sin(glm::radians(camYaw)) * moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camY -= moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camY += moveSpeed * deltaTime;
    }

    static double lastMouseX = previewX + previewW / 2, lastMouseY = previewY + previewH / 2;
    static bool mouseWasPressed = false;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && previewHovered) {
        double mouseXpos, mouseYpos;
        glfwGetCursorPos(window, &mouseXpos, &mouseYpos);

        if (mouseWasPressed) {
            double deltaX = mouseXpos - lastMouseX;
            double deltaY = mouseYpos - lastMouseY;
            camYaw += (float)deltaX * 0.5f;
            camPitch += (float)deltaY * 0.5f;
            if (camPitch > 89.0f) camPitch = 89.0f;
            if (camPitch < -89.0f) camPitch = -89.0f;
        }

        lastMouseX = mouseXpos;
        lastMouseY = mouseYpos;
        mouseWasPressed = true;
    }
    else {
        mouseWasPressed = false;
    }

    float radPitch = glm::radians(camPitch);
    float radYaw = glm::radians(camYaw);

    glm::vec3 center(camX, camY, camZ);
    glm::vec3 eye(
        camX + sin(radYaw) * cos(radPitch) * camDistance,
        camY + sin(radPitch) * camDistance,
        camZ + cos(radYaw) * cos(radPitch) * camDistance
    );

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);

    updateLightPosition();

    static bool modelsInitialized = false;
    if (!modelsInitialized) {
        initShadowModels();
        modelsInitialized = true;
    }

    float cellSize = currentConfig.cellSize;
    float floorHeight = 0.0f;
    float worldWidth = currentConfig.gridWidth * cellSize;
    float worldDepth = currentConfig.gridDepth * cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    LightType lightType = static_cast<LightType>(currentConfig.lightType);
    glm::vec3 lightDir = glm::normalize(currentConfig.lightDir);
    glm::vec3 lightPos = currentConfig.lightPos;

    bool isSubdivided = (currentConfig.shadowTraceMode == 2 || currentConfig.shadowTraceMode == 3);
    bool useGradient = (currentConfig.shadowTraceMode == 1 || currentConfig.shadowTraceMode == 3);
    int subDivSize = currentConfig.shadowSubdivisionSize;

    // Получаем bounding box и offset моделей
    static ModelBounds treeBounds, headBounds, bodyBounds, tailBounds, appleBounds;
    static float treeBottomOffset = 0.0f, headBottomOffset = 0.0f, bodyBottomOffset = 0.0f, tailBottomOffset = 0.0f, appleBottomOffset = 0.0f;
    static bool radiiComputed = false;

    if (!radiiComputed) {
        if (!g_previewTreeModel.vertices.empty()) {
            treeBounds = computeModelBounds(g_previewTreeModel);
            treeBottomOffset = getModelBottomOffsetLocal(g_previewTreeModel);
        }
        if (!g_previewSnakeHeadModel.vertices.empty()) {
            headBounds = computeModelBounds(g_previewSnakeHeadModel);
            headBottomOffset = getModelBottomOffsetLocal(g_previewSnakeHeadModel);
        }
        if (!g_previewSnakeBodyModel.vertices.empty()) {
            bodyBounds = computeModelBounds(g_previewSnakeBodyModel);
            bodyBottomOffset = getModelBottomOffsetLocal(g_previewSnakeBodyModel);
        }
        if (!g_previewSnakeTailModel.vertices.empty()) {
            tailBounds = computeModelBounds(g_previewSnakeTailModel);
            tailBottomOffset = getModelBottomOffsetLocal(g_previewSnakeTailModel);
        }
        if (!g_previewAppleModel.vertices.empty()) {
            appleBounds = computeModelBounds(g_previewAppleModel);
            appleBottomOffset = getModelBottomOffsetLocal(g_previewAppleModel);
        }
        radiiComputed = true;

        std::cout << "\n========== MODEL BOUNDS ==========" << std::endl;
        std::cout << "Tree: minY=" << treeBounds.minY << ", maxY=" << treeBounds.maxY << ", offset=" << treeBottomOffset << std::endl;
        std::cout << "Apple: minY=" << appleBounds.minY << ", maxY=" << appleBounds.maxY << ", offset=" << appleBottomOffset << std::endl;
        std::cout << "Head: minY=" << headBounds.minY << ", maxY=" << headBounds.maxY << ", offset=" << headBottomOffset << std::endl;
        std::cout << "Body: minY=" << bodyBounds.minY << ", maxY=" << bodyBounds.maxY << ", offset=" << bodyBottomOffset << std::endl;
        std::cout << "Tail: minY=" << tailBounds.minY << ", maxY=" << tailBounds.maxY << ", offset=" << tailBottomOffset << std::endl;
        std::cout << "==================================" << std::endl;
    }

    // Масштабы из конфига (уменьшаем модели)
    float headScale = cellSize * currentConfig.snakeHeadScale;
    float bodyScale = cellSize * currentConfig.snakeBodyScale;
    float tailScale = cellSize * currentConfig.snakeTailScale;
    float treeScale = cellSize * 1.2f;
    float appleScale = cellSize * 0.6f;

    // Цвета из конфига
    glm::vec3 treeColor = currentConfig.treeColor;
    glm::vec3 appleColor = currentConfig.appleColor;
    glm::vec3 headColor = currentConfig.snakeHeadColor;
    glm::vec3 bodyColor = currentConfig.snakeBodyColor;
    glm::vec3 tailColor = currentConfig.snakeTailColor;

    // Позиции объектов
    glm::vec3 headPos(0.0f, floorHeight, 0.0f);
    glm::vec3 bodyPos(0.0f, floorHeight, -0.2f);
    glm::vec3 tailPos(0.0f, floorHeight, -0.4f);
    std::vector<glm::vec3> snakePositions = { headPos, bodyPos, tailPos };

    std::vector<glm::vec3> treePositions = {
        glm::vec3(0.0f, floorHeight, 1.0f)
    };

    std::vector<glm::vec3> applePositions = {
        glm::vec3(0.5f, floorHeight, 0.3f)    };

    bool needsRecalc = !g_previewShadowsInitialized ||
        g_lastShadowTraceMode != currentConfig.shadowTraceMode ||
        g_lastShadowMapEnabled != currentConfig.shadowMapEnabled ||
        g_lastShadowStrideX != currentConfig.shadowStrideX ||
        g_lastShadowStrideZ != currentConfig.shadowStrideZ ||
        g_forceBoundsRecalc;

    if (needsRecalc && currentConfig.shadowMapEnabled) {
        std::cout << "\n========== RECALCULATING SHADOWS ==========" << std::endl;

        g_previewStaticShadow = ShadowMapper(
            currentConfig.gridWidth, currentConfig.gridDepth,
            cellSize, floorHeight,
            lightDir, currentConfig.lightColor,
            lightType, lightPos,
            currentConfig.shadowStrideX, currentConfig.shadowStrideZ
        );

        g_previewDynamicShadow = ShadowMapper(
            currentConfig.gridWidth, currentConfig.gridDepth,
            cellSize, floorHeight,
            lightDir, currentConfig.lightColor,
            lightType, lightPos,
            currentConfig.shadowStrideX, currentConfig.shadowStrideZ
        );

        g_previewFoodShadow = ShadowMapper(
            currentConfig.gridWidth, currentConfig.gridDepth,
            cellSize, floorHeight,
            lightDir, currentConfig.lightColor,
            lightType, lightPos,
            currentConfig.shadowStrideX, currentConfig.shadowStrideZ
        );

        g_previewStaticShadow.setUseSpheres(false);
        g_previewDynamicShadow.setUseSpheres(false);
        g_previewFoodShadow.setUseSpheres(false);

        ShadowMapper::ShadowTraceMode traceMode;
        switch (currentConfig.shadowTraceMode) {
        case 0: traceMode = ShadowMapper::TRACE_CENTER; break;
        case 1: traceMode = ShadowMapper::TRACE_CORNERS; break;
        case 2: traceMode = ShadowMapper::TRACE_CENTER_SUBDIVIDED; break;
        case 3: traceMode = ShadowMapper::TRACE_CORNERS_SUBDIVIDED; break;
        default: traceMode = ShadowMapper::TRACE_CORNERS_SUBDIVIDED; break;
        }

        g_previewStaticShadow.setShadowTraceMode(traceMode);
        g_previewDynamicShadow.setShadowTraceMode(traceMode);
        g_previewFoodShadow.setShadowTraceMode(traceMode);

        if (g_rayDebugEnabled) {
            g_previewStaticShadow.enableDebugRays(true);
            g_previewDynamicShadow.enableDebugRays(true);
            g_previewFoodShadow.enableDebugRays(true);
        }

        // Универсальная функция пересечения
        auto universalIntersect = [&](const Ray& ray, float& hitDist, glm::vec3& hitPoint,
            bool trees, bool snake, bool food) -> bool {
                float closestDist = 1000.0f;
                bool hit = false;

                // Проверка деревьев
                if (trees && !g_previewTreeModel.vertices.empty()) {
                    for (const auto& treePos : treePositions) {
                        float treeHeight = (treeBounds.maxY - treeBounds.minY) * treeScale;
                        float yPos = treePos.y + treeBottomOffset * treeScale;
                        glm::vec3 treeCenter(treePos.x, yPos + treeHeight * 0.5f, treePos.z);
                        float boundingRadius = treeHeight * 0.6f;

                        float tSphere;
                        if (rayIntersectsSphereEditor(ray, treeCenter, boundingRadius, tSphere)) {
                            glm::mat4 transform = glm::mat4(1.0f);
                            transform = glm::translate(transform, glm::vec3(treePos.x, yPos, treePos.z));
                            transform = glm::scale(transform, glm::vec3(treeScale));

                            float tempDist;
                            glm::vec3 tempPoint;
                            if (rayIntersectsModel(ray, g_previewTreeModel, transform, tempDist, tempPoint)) {
                                if (tempDist > 0.01f && tempDist < closestDist) {
                                    closestDist = tempDist;
                                    hitPoint = tempPoint;
                                    hit = true;
                                }
                            }
                        }
                    }
                }

                // Проверка змейки
                if (snake && !g_previewSnakeHeadModel.vertices.empty()) {
                    for (size_t i = 0; i < snakePositions.size(); i++) {
                        const auto& segPos = snakePositions[i];
                        float scale, bottomOffset;
                        const Model* model;

                        if (i == 0) {
                            scale = headScale;
                            bottomOffset = headBottomOffset;
                            model = &g_previewSnakeHeadModel;
                        }
                        else if (i == snakePositions.size() - 1) {
                            scale = tailScale;
                            bottomOffset = tailBottomOffset;
                            model = &g_previewSnakeTailModel;
                        }
                        else {
                            scale = bodyScale;
                            bottomOffset = bodyBottomOffset;
                            model = &g_previewSnakeBodyModel;
                        }

                        float segHeight = (bodyBounds.maxY - bodyBounds.minY) * scale;
                        float yPos = segPos.y + bottomOffset * scale;
                        glm::vec3 segCenter(segPos.x, yPos + segHeight * 0.5f, segPos.z);
                        float boundingRadius = segHeight * 10.6f;  // Увеличиваем радиус для змейки

                        float tSphere;
                        if (rayIntersectsSphereEditor(ray, segCenter, boundingRadius, tSphere)) {
                            float rotationAngle = calculateSegmentRotationEditor(snakePositions, i);
                            glm::mat4 transform = glm::mat4(1.0f);
                            transform = glm::translate(transform, glm::vec3(segPos.x, yPos, segPos.z));
                            transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                            transform = glm::scale(transform, glm::vec3(scale));

                            float tempDist;
                            glm::vec3 tempPoint;
                            if (rayIntersectsModel(ray, *model, transform, tempDist, tempPoint)) {
                                if (tempDist > 0.01f && tempDist < closestDist) {
                                    closestDist = tempDist;
                                    hitPoint = tempPoint;
                                    hit = true;
                                }
                            }
                        }
                    }
                }

                // Проверка яблок
                if (food && !g_previewAppleModel.vertices.empty()) {
                    for (const auto& applePos : applePositions) {
                        float appleHeight = (appleBounds.maxY - appleBounds.minY) * appleScale;
                        float yPos = applePos.y + appleBottomOffset * appleScale;
                        glm::vec3 appleCenter(applePos.x, yPos + appleHeight * 0.5f, applePos.z);
                        float boundingRadius = appleHeight * 0.5f;

                        float tSphere;
                        if (rayIntersectsSphereEditor(ray, appleCenter, boundingRadius, tSphere)) {
                            glm::mat4 transform = glm::mat4(1.0f);
                            transform = glm::translate(transform, glm::vec3(applePos.x, yPos, applePos.z));
                            transform = glm::scale(transform, glm::vec3(appleScale));

                            float tempDist;
                            glm::vec3 tempPoint;
                            if (rayIntersectsModel(ray, g_previewAppleModel, transform, tempDist, tempPoint)) {
                                if (tempDist > 0.01f && tempDist < closestDist) {
                                    closestDist = tempDist;
                                    hitPoint = tempPoint;
                                    hit = true;
                                }
                            }
                        }
                    }
                }

                // Проверка пола
                if (!hit && fabs(ray.direction.y) > 0.0001f) {
                    float tGround = -ray.origin.y / ray.direction.y;
                    if (tGround > 0.01f && tGround < closestDist) {
                        glm::vec3 hitPt = ray.pointAt(tGround);
                        float worldWidth_local = currentConfig.gridWidth * cellSize;
                        float worldDepth_local = currentConfig.gridDepth * cellSize;
                        float offsetX_local = worldWidth_local / 2.0f;
                        float offsetZ_local = worldDepth_local / 2.0f;
                        if (abs(hitPt.x) <= offsetX_local && abs(hitPt.z) <= offsetZ_local) {
                            closestDist = tGround;
                            hitPoint = hitPt;
                            hit = true;
                        }
                    }
                }

                if (hit) hitDist = closestDist;
                return hit;
            };

        g_previewStaticShadow.setIntersectCallback(
            [&](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
                return universalIntersect(ray, hitDist, hitPoint, true, false, false);
            }
        );

        g_previewDynamicShadow.setIntersectCallback(
            [&](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
                return universalIntersect(ray, hitDist, hitPoint, false, true, false);
            }
        );

        g_previewFoodShadow.setIntersectCallback(
            [&](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
                return universalIntersect(ray, hitDist, hitPoint, false, false, true);
            }
        );

        g_previewStaticShadow.computeShadows();
        g_previewDynamicShadow.computeShadows();
        g_previewFoodShadow.computeShadows();

        if (g_rayDebugEnabled) {
            g_debugRaysFromShadowMapper.clear();

            const auto& staticRays = g_previewStaticShadow.getDebugRays();
            const auto& dynamicRays = g_previewDynamicShadow.getDebugRays();
            const auto& foodRays = g_previewFoodShadow.getDebugRays();

            // Сохраняем ВСЕ лучи (и hit и miss)
            for (const auto& ray : staticRays) {
                g_debugRaysFromShadowMapper.push_back(ray);
            }
            for (const auto& ray : dynamicRays) {
                g_debugRaysFromShadowMapper.push_back(ray);
            }
            for (const auto& ray : foodRays) {
                g_debugRaysFromShadowMapper.push_back(ray);
            }

            g_previewStaticShadow.enableDebugRays(false);
            g_previewDynamicShadow.enableDebugRays(false);
            g_previewFoodShadow.enableDebugRays(false);

            std::cout << "Total rays: " << g_debugRaysFromShadowMapper.size()
                << " (hit=" << std::count_if(g_debugRaysFromShadowMapper.begin(), g_debugRaysFromShadowMapper.end(),
                    [](const DebugRay& r) { return r.hit; })
                << ", miss=" << std::count_if(g_debugRaysFromShadowMapper.begin(), g_debugRaysFromShadowMapper.end(),
                    [](const DebugRay& r) { return !r.hit; }) << ")" << std::endl;
        }

        g_previewShadowsInitialized = true;
        g_forceBoundsRecalc = false;

        g_lastShadowTraceMode = currentConfig.shadowTraceMode;
        g_lastShadowMapEnabled = currentConfig.shadowMapEnabled;
        g_lastShadowStrideX = currentConfig.shadowStrideX;
        g_lastShadowStrideZ = currentConfig.shadowStrideZ;
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    setupFixedPipelineLighting();

    glm::vec3 darkColor = currentConfig.floorColor * 0.15f;
    glm::vec3 lightColor = currentConfig.floorColor;

    // Отрисовка пола с тенями
    if (currentConfig.shadowMapEnabled && g_previewShadowsInitialized) {
        if (isSubdivided && subDivSize > 1) {
            float subCellSizeX = cellSize / subDivSize;
            float subCellSizeZ = cellSize / subDivSize;

            for (int z = 0; z < currentConfig.gridDepth; z++) {
                for (int x = 0; x < currentConfig.gridWidth; x++) {
                    float posX = x * cellSize - offsetX;
                    float posZ = z * cellSize - offsetZ;

                    for (int subZ = 0; subZ < subDivSize; subZ++) {
                        for (int subX = 0; subX < subDivSize; subX++) {
                            float subPosX = posX + subX * subCellSizeX;
                            float subPosZ = posZ + subZ * subCellSizeZ;
                            float subCenterX = subPosX + subCellSizeX / 2.0f;
                            float subCenterZ = subPosZ + subCellSizeZ / 2.0f;
                            glm::vec3 subCenter(subCenterX, 0.05f, subCenterZ);

                            float staticShadow = g_previewStaticShadow.getShadowAtPointWithSubdivision(subCenter, nullptr);
                            float snakeShadow = g_previewDynamicShadow.getShadowAtPointWithSubdivision(subCenter, nullptr);
                            float foodShadow = g_previewFoodShadow.getShadowAtPointWithSubdivision(subCenter, nullptr);

                            float shadowValue = std::min({ staticShadow, snakeShadow, foodShadow });

                            glm::vec3 finalColor;
                            if (useGradient) {
                                finalColor = glm::mix(darkColor, lightColor, shadowValue);
                            }
                            else {
                                finalColor = (shadowValue >= 0.5f) ? lightColor : darkColor;
                            }

                            glColor3f(finalColor.r, finalColor.g, finalColor.b);
                            glBegin(GL_QUADS);
                            glNormal3f(0.0f, 1.0f, 0.0f);
                            glVertex3f(subPosX, -0.02f, subPosZ);
                            glVertex3f(subPosX + subCellSizeX, -0.02f, subPosZ);
                            glVertex3f(subPosX + subCellSizeX, -0.02f, subPosZ + subCellSizeZ);
                            glVertex3f(subPosX, -0.02f, subPosZ + subCellSizeZ);
                            glEnd();
                        }
                    }
                }
            }
        }
        else {
            for (int z = 0; z < currentConfig.gridDepth; z++) {
                for (int x = 0; x < currentConfig.gridWidth; x++) {
                    float posX = x * cellSize - offsetX;
                    float posZ = z * cellSize - offsetZ;

                    float staticShadow = g_previewStaticShadow.getShadowAtCell(x, z);
                    float snakeShadow = g_previewDynamicShadow.getShadowAtCell(x, z);
                    float foodShadow = g_previewFoodShadow.getShadowAtCell(x, z);
                    float shadowValue = std::min({ staticShadow, snakeShadow, foodShadow });

                    glm::vec3 finalColor;
                    if (useGradient) {
                        finalColor = glm::mix(darkColor, lightColor, shadowValue);
                    }
                    else {
                        finalColor = (shadowValue >= 0.5f) ? lightColor : darkColor;
                    }

                    glColor3f(finalColor.r, finalColor.g, finalColor.b);
                    glBegin(GL_QUADS);
                    glNormal3f(0.0f, 1.0f, 0.0f);
                    glVertex3f(posX, -0.02f, posZ);
                    glVertex3f(posX + cellSize, -0.02f, posZ);
                    glVertex3f(posX + cellSize, -0.02f, posZ + cellSize);
                    glVertex3f(posX, -0.02f, posZ + cellSize);
                    glEnd();
                }
            }
        }
    }
    else {
        for (int z = 0; z < currentConfig.gridDepth; z++) {
            for (int x = 0; x < currentConfig.gridWidth; x++) {
                float posX = x * cellSize - offsetX;
                float posZ = z * cellSize - offsetZ;
                glColor3f(lightColor.r, lightColor.g, lightColor.b);
                glBegin(GL_QUADS);
                glNormal3f(0.0f, 1.0f, 0.0f);
                glVertex3f(posX, -0.02f, posZ);
                glVertex3f(posX + cellSize, -0.02f, posZ);
                glVertex3f(posX + cellSize, -0.02f, posZ + cellSize);
                glVertex3f(posX, -0.02f, posZ + cellSize);
                glEnd();
            }
        }
    }

    // Сетка
    if (currentConfig.gridEnabled) {
        glDisable(GL_LIGHTING);
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);
        glLineWidth(currentConfig.gridLineWidth);
        glBegin(GL_LINES);
        for (int i = 0; i <= currentConfig.gridWidth; i++) {
            float x = i * cellSize - offsetX;
            glVertex3f(x, 0.01f, -offsetZ);
            glVertex3f(x, 0.01f, offsetZ);
        }
        for (int i = 0; i <= currentConfig.gridDepth; i++) {
            float z = i * cellSize - offsetZ;
            glVertex3f(-offsetX, 0.01f, z);
            glVertex3f(offsetX, 0.01f, z);
        }
        glEnd();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }

    // Функция отрисовки модели
    auto drawModel3D = [&](Model& model, const glm::vec3& pos, float bottomOffset,
        float scale, float rotation, const glm::vec3& color) {
            if (model.vertices.empty()) return;

            glPushMatrix();
            float yPos = pos.y + bottomOffset * scale;
            glTranslatef(pos.x, yPos, pos.z);
            if (rotation != 0.0f) glRotatef(rotation, 0.0f, 1.0f, 0.0f);
            glScalef(scale, scale, scale);

            if (model.hasTexture && model.textureID != 0) {
                glColor3f(1.0f, 1.0f, 1.0f);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, model.textureID);
            }
            else {
                glDisable(GL_TEXTURE_2D);
                glColor3f(color.r, color.g, color.b);
            }

            model.draw();
            glPopMatrix();
        };

    // Рисуем змейку
    drawModel3D(g_previewSnakeHeadModel, headPos, headBottomOffset, headScale,
        calculateSegmentRotationEditor(snakePositions, 0), headColor);
    drawModel3D(g_previewSnakeBodyModel, bodyPos, bodyBottomOffset, bodyScale,
        calculateSegmentRotationEditor(snakePositions, 1), bodyColor);
    drawModel3D(g_previewSnakeTailModel, tailPos, tailBottomOffset, tailScale,
        calculateSegmentRotationEditor(snakePositions, 2), tailColor);

    // Рисуем деревья
    for (const auto& treePos : treePositions) {
        drawModel3D(g_previewTreeModel, treePos, treeBottomOffset, treeScale, 0.0f, treeColor);
    }

    // Рисуем яблоки
    for (const auto& applePos : applePositions) {
        drawModel3D(g_previewAppleModel, applePos, appleBottomOffset, appleScale, 0.0f, appleColor);
    }

    

    // Отладочные лучи
// Отладочные лучи
// Отладочные лучи с удалением дублирующихся жёлтых лучей
    if (g_rayDebugEnabled && currentConfig.shadowMapEnabled && !g_debugRaysFromShadowMapper.empty()) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // Разделяем лучи на попавшие и не попавшие
        std::vector<const DebugRay*> hitRays;
        std::vector<const DebugRay*> missRays;

        for (const auto& ray : g_debugRaysFromShadowMapper) {
            if (ray.hit) {
                hitRays.push_back(&ray);
            }
            else {
                missRays.push_back(&ray);
            }
        }

        std::cout << "Hit rays: " << hitRays.size() << ", Miss rays: " << missRays.size() << std::endl;

        // Вектор для хранения уникальных лучей (используем флаг)
        std::vector<bool> shouldDraw(g_debugRaysFromShadowMapper.size(), false);

        // Флаг для отметки жёлтых лучей, которые нужно удалить
        std::vector<bool> removeRay(g_debugRaysFromShadowMapper.size(), false);

        // Помечаем все красные лучи для отрисовки
        for (size_t i = 0; i < g_debugRaysFromShadowMapper.size(); i++) {
            if (g_debugRaysFromShadowMapper[i].hit) {
                shouldDraw[i] = true;
            }
        }

        // Функция для получения координат клетки из точки
        float worldWidth = currentConfig.gridWidth * currentConfig.cellSize;
        float worldDepth = currentConfig.gridDepth * currentConfig.cellSize;
        float offsetX_local = worldWidth / 2.0f;
        float offsetZ_local = worldDepth / 2.0f;

        auto getCellCoords = [&](const glm::vec3& point) -> std::pair<int, int> {
            int cellX = (int)((point.x + offsetX_local) / currentConfig.cellSize);
            int cellZ = (int)((point.z + offsetZ_local) / currentConfig.cellSize);
            return { cellX, cellZ };
            };

        // Функция для проверки, совпадает ли жёлтый луч с красным (проходит через ту же точку)
        auto isSameHitPoint = [](const DebugRay& hitRay, const DebugRay& missRay, float epsilon = 0.05f) -> bool {
            // Если жёлтый луч заканчивается очень близко к точке попадания красного луча
            float distToHitPoint = glm::distance(missRay.hitPoint, hitRay.hitPoint);
            return distToHitPoint < epsilon;
            };

        // Функция для проверки, лежит ли жёлтый луч внутри красного (проходит через ту же траекторию)
        auto isInsideRedRay = [](const DebugRay& hitRay, const DebugRay& missRay, float epsilon = 0.05f) -> bool {
            // Проверяем, лежит ли жёлтый луч на том же луче, что и красный
            // Направления должны быть коллинеарны
            float dot = glm::dot(hitRay.direction, missRay.direction);
            if (fabs(fabs(dot) - 1.0f) > epsilon) return false;

            // Проверяем, лежит ли точка попадания жёлтого луча на пути красного луча
            glm::vec3 toMiss = missRay.hitPoint - hitRay.origin;
            float proj = glm::dot(toMiss, hitRay.direction);

            // Если проекция положительная и точка находится между origin и hitPoint красного луча
            if (proj > 0.01f && proj < hitRay.distance - epsilon) {
                // Проверяем расстояние от точки до луча
                glm::vec3 perp = toMiss - hitRay.direction * proj;
                if (glm::length(perp) < epsilon) {
                    return true;
                }
            }

            return false;
            };

        // Функция для увеличения длины жёлтого луча (чтобы он выходил за пол)
        auto extendMissRay = [&](const DebugRay& ray) -> DebugRay {
            DebugRay extendedRay = ray;
            // Увеличиваем длину луча на 20% или минимум на 0.2f
            float extension = std::max(ray.distance * 0.2f, 0.2f);
            extendedRay.distance = ray.distance + extension;
            // Вычисляем новую точку попадания
            extendedRay.hitPoint = ray.origin + ray.direction * extendedRay.distance;
            return extendedRay;
            };

        // Для каждого красного луча проверяем жёлтые лучи на совпадение
        for (size_t hitIdx = 0; hitIdx < g_debugRaysFromShadowMapper.size(); hitIdx++) {
            if (!g_debugRaysFromShadowMapper[hitIdx].hit) continue;

            const auto& hitRay = g_debugRaysFromShadowMapper[hitIdx];
            auto [hitCellX, hitCellZ] = getCellCoords(hitRay.hitPoint);

            // Перебираем все жёлтые лучи
            for (size_t missIdx = 0; missIdx < g_debugRaysFromShadowMapper.size(); missIdx++) {
                if (g_debugRaysFromShadowMapper[missIdx].hit) continue;

                // Если жёлтый луч уже отмечен на удаление, пропускаем
                if (removeRay[missIdx]) continue;

                const auto& missRay = g_debugRaysFromShadowMapper[missIdx];
                auto [missCellX, missCellZ] = getCellCoords(missRay.hitPoint);

                // Вычисляем разницу в клетках
                int dx = abs(hitCellX - missCellX);
                int dz = abs(hitCellZ - missCellZ);

                // ПРОВЕРКА 1: если жёлтый луч совпадает с красным (проходит через ту же точку или траекторию)
                if (isSameHitPoint(hitRay, missRay) || isInsideRedRay(hitRay, missRay)) {
                    removeRay[missIdx] = true;  // Помечаем на удаление
                    std::cout << "Removing duplicate yellow ray near red ray at cell ("
                        << hitCellX << "," << hitCellZ << ")" << std::endl;
                    continue;
                }

                // ПРОВЕРКА 2: если жёлтый луч в радиусе 2 клеток - оставляем
                if (dx <= 2 && dz <= 2) {
                    shouldDraw[missIdx] = true;
                }
            }
        }

        // Подсчитываем количество лучей для отрисовки
        int redCount = 0, yellowCount = 0;
        for (size_t i = 0; i < shouldDraw.size(); i++) {
            if (shouldDraw[i] && !removeRay[i]) {
                if (g_debugRaysFromShadowMapper[i].hit) redCount++;
                else yellowCount++;
            }
        }

        std::cout << "Drawing " << (redCount + yellowCount) << " rays ("
            << redCount << " red, " << yellowCount << " yellow)" << std::endl;

        std::cout << "Removed " << std::count(removeRay.begin(), removeRay.end(), true)
            << " duplicate yellow rays" << std::endl;

        // Рисуем все отобранные лучи (не удалённые)
        for (size_t i = 0; i < g_debugRaysFromShadowMapper.size(); i++) {
            if (shouldDraw[i] && !removeRay[i]) {
                if (!g_debugRaysFromShadowMapper[i].hit) {
                    // Для жёлтых лучей - увеличиваем длину, чтобы они выходили за пол
                    DebugRay extendedRay = extendMissRay(g_debugRaysFromShadowMapper[i]);
                    drawDebugRayFromShadowMapper(extendedRay);
                }
                else {
                    drawDebugRayFromShadowMapper(g_debugRaysFromShadowMapper[i]);
                }
            }
        }

        glEnable(GL_LIGHTING);
    }

    // Источник света
    if (currentConfig.lightType != 0) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glPushMatrix();
        glTranslatef(currentConfig.lightPos.x, currentConfig.lightPos.y, currentConfig.lightPos.z);
        glColor3f(1.0f, 0.8f, 0.2f);
        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.3f, 16, 16);
        gluDeleteQuadric(quad);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    glDisable(GL_COLOR_MATERIAL);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    reset2DProjection();

    // UI
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)previewX, (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)previewY);
    glVertex2f((float)(previewX + previewW), (float)(previewY + previewH));
    glVertex2f((float)previewX, (float)(previewY + previewH));
    glEnd();

    drawText((float)(previewX + 10), (float)(previewY + 25), "ПРЕДПРОСМОТР ТЕНЕЙ", 1.0f, 1.0f, 0.0f);
    drawText((float)(previewX + 10), (float)(previewY + 50),
        "Управление: WASD - движение, ЛКМ+мышь - вращение, КОЛЁСИКО - зум, Q/E - вверх/вниз",
        0.7f, 0.7f, 0.9f);

    const char* modeName = "";
    switch (currentConfig.shadowTraceMode) {
    case 0: modeName = "CENTER"; break;
    case 1: modeName = "CORNERS"; break;
    case 2: modeName = "CENTER_SUBDIV"; break;
    case 3: modeName = "CORNERS_SUBDIV"; break;
    }

    char infoText[300];
    sprintf_s(infoText, "Тени: %s | Режим: %s | %s | Подклетки: %dx%d | Stride: %dx%d | Zoom: %.1f",
        currentConfig.shadowMapEnabled ? "ВКЛ" : "ВЫКЛ",
        modeName,
        useGradient ? "градиент" : "бинарный",
        isSubdivided ? subDivSize : 1,
        isSubdivided ? subDivSize : 1,
        currentConfig.shadowStrideX, currentConfig.shadowStrideZ,
        camDistance);
    drawText((float)(previewX + 10), (float)(previewY + 75), infoText, 0.7f, 0.7f, 0.7f);

    if (g_rayDebugEnabled && currentConfig.shadowMapEnabled) {
        int hitCount = 0, missCount = 0;
        for (const auto& ray : g_debugRaysFromShadowMapper) {
            if (ray.hit) hitCount++;
            else missCount++;
        }
        char rayCountText[200];
        sprintf_s(rayCountText, "Лучей: %d (красные=%d тень, зелёные=%d свет)",
            (int)g_debugRaysFromShadowMapper.size(), hitCount, missCount);
        drawText((float)(previewX + 10), (float)(previewY + previewH - 30), rayCountText, 0.8f, 0.8f, 0.8f);
    }
}


//=============================================================================
// ГЛАВНОЕ МЕНЮ
//=============================================================================

void renderMainMenu() {
    g_currentPreviewMode = PREVIEW_NORMAL;
    reset2DProjection();
    drawCenteredText((float)(windowHeight * 0.08f), "РЕДАКТОР КОНФИГУРАЦИИ ИГРЫ ЗМЕЙКА", 1.0f, 1.0f, 0.0f);

    int buttonWidth = (int)(windowWidth * 0.32f);
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
    sprintf_s(gridInfo, "Сетка: %dx%d  Размер ячейки: %.2f",
        currentConfig.gridWidth, currentConfig.gridDepth, currentConfig.cellSize);
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
    // ЗМЕЯ
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

    // ПОЛ И НЕБО
    if (groundSkyElements.size() >= 2) {
        currentConfig.floorModel = groundSkyElements[0]->modelFile;
        currentConfig.floorTexture = groundSkyElements[0]->textureFile;
        currentConfig.floorColor = groundSkyElements[0]->color;
        currentConfig.skyColor = groundSkyElements[1]->color;
    }

    // ПРЕГРАДЫ - СОХРАНЯЕМ ВСЁ (модели, цвета, масштабы, количество)
    if (obstaclesElements.size() >= 4) {
        // Дерево
        currentConfig.treeModel = obstaclesElements[0]->modelFile;
        currentConfig.treeColor = obstaclesElements[0]->color;
        currentConfig.treeScale = obstaclesElements[0]->scale;
        currentConfig.obstacleCount = obstaclesElements[0]->count;  // ← ДОБАВЛЕНО!


        // Камень
        currentConfig.rockModel = obstaclesElements[1]->modelFile;
        currentConfig.rockColor = obstaclesElements[1]->color;
        currentConfig.rockScale = obstaclesElements[1]->scale;

        // Забор
        currentConfig.fenceModel = obstaclesElements[2]->modelFile;
        currentConfig.fenceColor = obstaclesElements[2]->color;
        currentConfig.fenceScale = obstaclesElements[2]->scale;

        // Яблоко (еда)
        currentConfig.appleModel = obstaclesElements[3]->modelFile;
        currentConfig.appleColor = obstaclesElements[3]->color;
        currentConfig.appleScale = obstaclesElements[3]->scale;
        currentConfig.initialFoodCount = obstaclesElements[3]->count;
    }

    // ОКРУЖЕНИЕ - СОХРАНЯЕМ ВСЁ
    if (environmentElements.size() >= 3) {
        // Цветы
        currentConfig.flowerModel = environmentElements[0]->modelFile;
        currentConfig.flowerColor = environmentElements[0]->color;
        currentConfig.flowerScale = environmentElements[0]->scale;
        currentConfig.flowerCount = environmentElements[0]->count;

        // Птицы
        currentConfig.birdModel = environmentElements[1]->modelFile;
        currentConfig.birdColor = environmentElements[1]->color;
        currentConfig.birdScale = environmentElements[1]->scale;
        currentConfig.birdCount = environmentElements[1]->count;

        // Облака
        currentConfig.cloudModel = environmentElements[2]->modelFile;
        currentConfig.cloudColor = environmentElements[2]->color;
        currentConfig.cloudScale = environmentElements[2]->scale;
        currentConfig.cloudCount = environmentElements[2]->count;
    }

    // Сохраняем в файл
    ConfigManager::saveGameConfig(g_configPath, currentConfig);

    std::cout << "Configuration saved to: " << g_configPath << std::endl;
    std::cout << "Tree count saved: " << currentConfig.obstacleCount << std::endl;  // Для отладки
}

//=============================================================================
// КОЛБЭКИ
//=============================================================================

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mousePressedLast = mousePressed;
        mousePressed = (action == GLFW_PRESS);

        if (action == GLFW_PRESS && previewHovered) {
            previewMouseRotating = true;
        }
        if (action == GLFW_RELEASE) {
            previewMouseRotating = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;

    if (previewMouseRotating && previewHovered) {
        double deltaX = xpos - previewLastMouseX;
        double deltaY = ypos - previewLastMouseY;

        if (deltaX != 0) {
            previewRotationAngle += (float)deltaX * 0.5f;
            previewAutoRotate = false;
            previewLastRotateTime = (float)glfwGetTime();
        }
        if (deltaY != 0) {
            previewCameraPitch += (float)deltaY * 0.3f;
            if (previewCameraPitch > 85.0f) previewCameraPitch = 85.0f;
            if (previewCameraPitch < 5.0f) previewCameraPitch = 5.0f;
            previewAutoRotate = false;
            previewLastRotateTime = (float)glfwGetTime();
        }
    }

    previewLastMouseX = xpos;
    previewLastMouseY = ypos;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (previewHovered) {
        previewCameraDistance -= (float)yoffset;
        if (previewCameraDistance < 2.0f) previewCameraDistance = 2.0f;
        if (previewCameraDistance > 20.0f) previewCameraDistance = 20.0f;

        previewAutoRotate = false;
        previewLastRotateTime = (float)glfwGetTime();
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            if (currentMode != MODE_MAIN) {
                currentMode = MODE_MAIN;
            }
            else {
                glfwSetWindowShouldClose(window, true);
            }
        }

        if (key == GLFW_KEY_SPACE && previewHovered) {
            previewAutoRotate = !previewAutoRotate;
            if (previewAutoRotate) {
                previewLastRotateTime = (float)glfwGetTime();
            }
        }

        if (key == GLFW_KEY_R && previewHovered) {
            previewCameraDistance = 6.0f;
            previewCameraPitch = 25.0f;
            previewRotationAngle = 0.0f;
            previewAutoRotate = true;
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
    glfwSetScrollCallback(window, scrollCallback);

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

    // Инициализируем освещение
    setupFixedPipelineLighting();

    // Очищаем кэш границ моделей
    g_boundsCache.clear();
    g_forceBoundsRecalc = true;

    return true;
}

//=============================================================================
// MAIN
//=============================================================================

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);


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

        if (!mousePressed) mousePressedLast = false;
    }

    cleanupFreeType();
    glfwDestroyWindow(window);
    glfwTerminate();


    return 0;
}