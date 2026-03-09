#define GLEW_STATIC
#include <GL/glew.h>           // ДОЛЖЕН БЫТЬ ПЕРВЫМ!
#include "../../ractica/includes/GLFW/glfw3.h"

// Правильные include для GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // для perspective, ortho
#include <glm/gtc/type_ptr.hpp>           // для value_ptr

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

// Глобальные пути
std::string g_assetsPath;
std::string g_modelsPath;
std::string g_texturesPath;
std::string g_configPath;

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

// Структура для символа
struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

// Структура для текстуры
struct Texture {
    unsigned int id;
    int width;
    int height;
    std::string path;

    Texture() : id(0), width(0), height(0), path("") {}
};

// Структура для элемента с текстурой и цветом
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

// Глобальные переменные
GLFWwindow* window;
int windowWidth = 1400;
int windowHeight = 900;
GameConfig currentConfig;

// FreeType
FT_Library ft;
FT_Face face;
std::map<unsigned char, Character> Characters;
GLuint textVAO, textVBO;
GLuint textShaderProgram;

// Состояния редактора
enum EditorMode {
    MODE_MAIN,
    MODE_SNAKE_EDITOR,
    MODE_GROUND_SKY_EDITOR,
    MODE_OBSTACLES_EDITOR,
    MODE_ENVIRONMENT_EDITOR,
    MODE_GRID_EDITOR
};
EditorMode currentMode = MODE_MAIN;

// Элементы для каждой категории
std::vector<VisualElement*> snakeElements;
std::vector<VisualElement*> groundSkyElements;
std::vector<VisualElement*> obstaclesElements;
std::vector<VisualElement*> environmentElements;

// Для предпросмотра
ModelData previewModel;
std::string currentPreviewModel;
std::string currentPreviewFolder;
float previewRotation = 0.0f;
bool autoRotate = true;
float lastRotationTime = 0.0f;
float autoRotateDelay = 5.0f;
float rotationSpeed = 0.5f;

// Для редактора змейки
int selectedPart = 0;

// Для редактора препятствий
int selectedObstacle = 0;

// Для редактора окружения
int selectedEnv = 0;

// Для редактора пола и неба
int selectedGroundSky = 0;

// Текстуры
Texture floorTexture;
Texture skyTexture;
std::vector<std::string> availableTextures;

// Для UI
bool mousePressed = false;
bool mousePressedLast = false;
double mouseX, mouseY;

// Прототипы функций
void renderMainMenu();
void renderSnakeEditor();
void renderGroundSkyEditor();
void renderObstaclesEditor();
void renderEnvironmentEditor();
void renderGridEditor();
void renderModelPreview(ModelData& model, const char* title, float x, float y, float w, float h, float& rotation, bool& autoRotate, float& lastTime);
void renderTexturedFloor();
void renderTexturedSky();
void drawGrid();
void drawModel(ModelData& model);
void reset2DProjection();
void saveConfig();
void initPaths();
void initElements();
void loadAvailableTextures();
void openFileDialog(std::string& destVar, const std::string& subFolder);
void openTextureFileDialog(std::string& destVar, Texture& texture);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void windowSizeCallback(GLFWwindow* window, int width, int height);
void initFreeType();
float getTextWidth(const char* text, float scale);
void renderText(const char* text, float x, float y, float scale, glm::vec3 color, bool centerX = false, bool centerY = false);
bool drawButton(int x, int y, int w, int h, const char* text, bool enabled = true);
bool drawSlider(int x, int y, int w, float* value, float minVal, float maxVal, const char* label);
bool drawIntSlider(int x, int y, int w, int* value, int minVal, int maxVal, const char* label);
void drawInfoBox(int x, int y, int w, int h, const char* label, const char* value);
void drawColorPicker(int x, int y, const char* label, glm::vec3& color);
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder = "");
bool loadTexture(const std::string& filename, Texture& texture);
GLuint loadTextureFromFile(const std::string& path);

// Функция для извлечения имени файла из пути
std::string extractFilename(const std::string& path) {
    // Ищем последний слеш или обратный слеш
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

// Функция для загрузки текстуры из памяти
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

// Функция для загрузки встроенной текстуры из FBX
GLuint loadEmbeddedTexture(const aiTexture* texture) {
    if (texture->mHeight == 0) {
        // Сжатая текстура (PNG, JPG и т.д.)
        int width, height, channels;
        unsigned char* data = stbi_load_from_memory(
            reinterpret_cast<unsigned char*>(texture->pcData),
            texture->mWidth,
            &width, &height, &channels, 0
        );

        if (data) {
            GLuint texID = loadTextureFromMemory(data, width, height, channels);
            stbi_image_free(data);
            return texID;
        }
    }
    else {
        // Несжатая текстура (RAW)
        return loadTextureFromMemory(
            reinterpret_cast<unsigned char*>(texture->pcData),
            texture->mWidth,
            texture->mHeight,
            4 // Предполагаем RGBA
        );
    }
    return 0;
}

// Функция для очистки модели предпросмотра
void clearPreview() {
    previewModel.loaded = false;
    previewModel.vertices.clear();
    previewModel.normals.clear();
    previewModel.texCoords.clear();
    previewModel.materials.clear();
    previewModel.materialIndices.clear();
}

// Функция для обновления предпросмотра текущего элемента
void updatePreviewForCurrentMode() {
    // Сначала очищаем предпросмотр
    clearPreview();

    switch (currentMode) {
    case MODE_SNAKE_EDITOR:
        if (selectedPart >= 0 && selectedPart < snakeElements.size()) {
            std::string folder = (selectedPart == 0) ? "snake_head" :
                (selectedPart == 1) ? "snake_body" : "snake_tail";
            loadFBXModel(snakeElements[selectedPart]->modelFile, previewModel, folder);
        }
        break;

    case MODE_GROUND_SKY_EDITOR:
        if (selectedGroundSky == 0 && !groundSkyElements.empty()) {
            // Только для пола загружаем модель, для неба не надо
            loadFBXModel(groundSkyElements[0]->modelFile, previewModel, "floor");
        }
        // Для неба оставляем пустым
        break;

    case MODE_OBSTACLES_EDITOR:
        if (selectedObstacle >= 0 && selectedObstacle < obstaclesElements.size()) {
            loadFBXModel(obstaclesElements[selectedObstacle]->modelFile, previewModel, "obstacles");
        }
        break;

    case MODE_ENVIRONMENT_EDITOR:
        if (selectedEnv >= 0 && selectedEnv < environmentElements.size()) {
            std::string folder = (selectedEnv == 0) ? "flowers" :
                (selectedEnv == 1) ? "birds" : "clouds";
            loadFBXModel(environmentElements[selectedEnv]->modelFile, previewModel, folder);
        }
        break;

    case MODE_GRID_EDITOR:
        // Для редактора сетки модель не нужна
        break;
    }
}

//=============================================================================
// ФУНКЦИИ ДЛЯ РАБОТЫ С РУССКИМ ТЕКСТОМ
//=============================================================================
void renderRussianText(const char* text, float x, float y, float scale, glm::vec3 color, bool centerX = false, bool centerY = false) {
    renderText(text, x, y, scale, color, centerX, centerY);
}

bool drawRussianButton(int x, int y, int w, int h, const char* text, bool enabled = true) {
    return drawButton(x, y, w, h, text, enabled);
}

//=============================================================================
// ИНИЦИАЛИЗАЦИЯ ЭЛЕМЕНТОВ
//=============================================================================
void initElements() {
    // 1. ЗМЕЯ
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

    // 2. ПОЛ И НЕБО
    VisualElement* ground = new VisualElement("ПОЛ");
    ground->modelFile = currentConfig.floorModel;
    ground->textureFile = currentConfig.floorTexture;
    ground->color = currentConfig.floorColor;
    ground->scale = 1.0f;
    groundSkyElements.push_back(ground);

    VisualElement* sky = new VisualElement("НЕБО");
    sky->textureFile = ""; // будет загружаться отдельно
    sky->color = currentConfig.skyColor;
    sky->scale = 1.0f;
    groundSkyElements.push_back(sky);

    // 3. ПРЕГРАДЫ (деревья, камни, забор, яблоки)
    VisualElement* tree = new VisualElement("ДЕРЕВО");
    tree->modelFile = currentConfig.treeModel;
    tree->color = glm::vec3(0.1f, 0.4f, 0.1f);
    tree->scale = 1.5f;
    tree->count = 10;
    obstaclesElements.push_back(tree);

    VisualElement* rock = new VisualElement("КАМЕНЬ");
    rock->modelFile = currentConfig.rockModel;
    rock->color = glm::vec3(0.5f, 0.5f, 0.5f);
    rock->scale = 1.2f;
    rock->count = 5;
    obstaclesElements.push_back(rock);

    VisualElement* fence = new VisualElement("ЗАБОР");
    fence->modelFile = currentConfig.fenceModel;
    fence->color = glm::vec3(0.6f, 0.4f, 0.2f);
    fence->scale = 1.0f;
    fence->count = 8;
    obstaclesElements.push_back(fence);

    VisualElement* apple = new VisualElement("ЯБЛОКО");
    apple->modelFile = currentConfig.appleModel;
    apple->color = glm::vec3(1.0f, 0.0f, 0.0f);
    apple->scale = 0.8f;
    apple->count = currentConfig.initialFoodCount;
    obstaclesElements.push_back(apple);

    // 4. ОКРУЖЕНИЕ (цветы, птицы, облака)
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
// ИНИЦИАЛИЗАЦИЯ ПУТЕЙ
//=============================================================================
void initPaths() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::string exePath = std::string(currentDir);

    std::cout << "\n=========================================" << std::endl;
    std::cout << "===== КОНФИГУРАТОР ПУТЕЙ =====" << std::endl;
    std::cout << "=========================================" << std::endl;

    std::string rootPath = exePath;

    size_t pos = rootPath.find("\\Project2");
    if (pos != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
    }
    else if ((pos = rootPath.find("\\x64\\Debug")) != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
    }
    else if ((pos = rootPath.find("\\Debug")) != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
    }

    if (rootPath.find("ractica") == std::string::npos) {
        rootPath += "\\ractica";
    }

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

//=============================================================================
// ЗАГРУЗКА ТЕКСТУР
//=============================================================================
GLuint loadTextureFromFile(const std::string& path) {
    std::cout << "  Loading texture from file: " << path << std::endl;

    if (!std::filesystem::exists(path)) {
        std::cout << "  ✗ Texture file not found" << std::endl;
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

        std::cout << "  ✓ Texture loaded: " << width << "x" << height << std::endl;
        return textureID;
    }

    std::cout << "  ✗ Failed to load texture: " << stbi_failure_reason() << std::endl;
    stbi_image_free(data);
    glDeleteTextures(1, &textureID);
    return 0;
}

bool loadTexture(const std::string& filename, Texture& texture) {
    std::string fullPath = g_texturesPath + filename;

    if (!std::filesystem::exists(fullPath)) {
        return false;
    }

    if (texture.id != 0) {
        glDeleteTextures(1, &texture.id);
    }

    texture.id = loadTextureFromFile(fullPath);
    texture.path = filename;

    return texture.id != 0;
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

        // Для пола обновляем предпросмотр
        if (currentMode == MODE_GROUND_SKY_EDITOR && selectedGroundSky == 0) {
            updatePreviewForCurrentMode();
        }
    }
}

//=============================================================================
// ЗАГРУЗКА МОДЕЛЕЙ (ИСПРАВЛЕННАЯ)
//=============================================================================
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder) {
    if (filename.empty()) {
        std::cout << "  ✗ Filename is empty" << std::endl;
        return false;
    }

    std::string fullPath = g_modelsPath + subFolder + "\\" + filename;
    std::cout << "\nLoading FBX: " << fullPath << std::endl;

    if (!std::filesystem::exists(fullPath)) {
        std::cout << "  ✗ File not found" << std::endl;
        return false;
    }

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(fullPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes |
        aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "  ✗ Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::cout << "  ✓ Scene loaded" << std::endl;
    std::cout << "    - Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "    - Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "    - Textures: " << scene->mNumTextures << std::endl;

    model.vertices.clear();
    model.normals.clear();
    model.texCoords.clear();
    model.materials.clear();
    model.materialIndices.clear();

    // Получаем базовое имя модели
    std::string baseName = filename;
    size_t dotPos = baseName.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseName = baseName.substr(0, dotPos);
    }

    // Загружаем материалы
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material material;
        material.textureID = 0;

        aiColor3D color(1.0f, 1.0f, 1.0f);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse = glm::vec3(color.r, color.g, color.b);

        // Ищем диффузную текстуру
        aiString texPath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string textureFile = texPath.C_Str();

            // Извлекаем имя файла из URL или пути
            size_t lastSlash = textureFile.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                textureFile = textureFile.substr(lastSlash + 1);
            }

            std::cout << "  Found texture: " << textureFile << std::endl;

            // Ищем текстуру в папке с моделью
            std::string basePath = fullPath.substr(0, fullPath.find_last_of("\\/") + 1);
            std::string texFullPath = basePath + textureFile;

            if (std::filesystem::exists(texFullPath)) {
                material.textureID = loadTextureFromFile(texFullPath);
                if (material.textureID) {
                    std::cout << "  ✓ Texture loaded" << std::endl;
                }
            }
        }

        model.materials.push_back(material);
    }

    if (model.materials.empty()) {
        Material defaultMat;
        defaultMat.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        defaultMat.textureID = 0;
        model.materials.push_back(defaultMat);
    }

    // Загружаем меши с правильной индексацией
    std::cout << "\n  --- Loading Meshes ---" << std::endl;

    int totalVertices = 0;
    int totalTriangles = 0;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "  Mesh " << i << ": " << mesh->mNumVertices << " vertices, "
            << mesh->mNumFaces << " faces" << std::endl;

        int materialIndex = mesh->mMaterialIndex;
        if (materialIndex >= (int)model.materials.size()) {
            materialIndex = 0;
        }

        // Для каждой грани (треугольника)
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];

            // Добавляем индекс материала для этого треугольника
            model.materialIndices.push_back(materialIndex);

            // Для каждой вершины треугольника
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                unsigned int vertexIdx = face.mIndices[k];

                // Вершина
                model.vertices.push_back(mesh->mVertices[vertexIdx].x);
                model.vertices.push_back(mesh->mVertices[vertexIdx].y);
                model.vertices.push_back(mesh->mVertices[vertexIdx].z);

                // Нормаль
                if (mesh->HasNormals()) {
                    model.normals.push_back(mesh->mNormals[vertexIdx].x);
                    model.normals.push_back(mesh->mNormals[vertexIdx].y);
                    model.normals.push_back(mesh->mNormals[vertexIdx].z);
                }

                // Текстурные координаты
                if (mesh->HasTextureCoords(0)) {
                    model.texCoords.push_back(mesh->mTextureCoords[0][vertexIdx].x);
                    model.texCoords.push_back(mesh->mTextureCoords[0][vertexIdx].y);
                }

                totalVertices++;
            }
        }
        totalTriangles += mesh->mNumFaces;
    }

    // Если нормалей нет, генерируем простые
    if (model.normals.empty()) {
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            model.normals.push_back(0.0f);
            model.normals.push_back(1.0f);
            model.normals.push_back(0.0f);
        }
    }

    // Если текстурных координат нет, добавляем нулевые
    if (model.texCoords.empty()) {
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            model.texCoords.push_back(0.0f);
            model.texCoords.push_back(0.0f);
        }
    }

    model.loaded = (model.vertices.size() > 0);

    if (model.loaded) {
        std::cout << "\n  ✓ Model loaded successfully!" << std::endl;
        std::cout << "    Total vertices: " << model.vertices.size() / 3 << std::endl;
        std::cout << "    Total triangles: " << totalTriangles << std::endl;
        std::cout << "    Total materials: " << model.materials.size() << std::endl;

        int texturesLoaded = 0;
        for (const auto& mat : model.materials) {
            if (mat.textureID != 0) texturesLoaded++;
        }
        std::cout << "    Textures loaded: " << texturesLoaded << "/" << model.materials.size() << std::endl;
    }

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
        if (fullPath != destPath) {
            CopyFileA(fullPath.c_str(), destPath.c_str(), FALSE);
        }

        // Обновляем предпросмотр после загрузки
        updatePreviewForCurrentMode();
    }
}

//=============================================================================
// ОТРИСОВКА (ИСПРАВЛЕННАЯ)
//=============================================================================
void drawModel(ModelData& model) {
    if (!model.loaded || model.vertices.empty()) return;

    size_t numVertices = model.vertices.size() / 3;
    if (numVertices == 0) return;

    // Проходим по всем треугольникам
    glBegin(GL_TRIANGLES);

    int lastMaterial = -1;

    for (size_t i = 0; i < numVertices / 3; i++) {
        // Определяем материал для этого треугольника
        int materialIdx = 0;
        if (!model.materialIndices.empty() && i < model.materialIndices.size()) {
            materialIdx = model.materialIndices[i];
        }

        // Если материал изменился, меняем состояние
        if (materialIdx != lastMaterial && materialIdx < (int)model.materials.size()) {
            // Завершаем текущий блок
            glEnd();

            lastMaterial = materialIdx;
            Material& mat = model.materials[materialIdx];

            // Устанавливаем новый материал
            if (mat.textureID != 0) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mat.textureID);
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            else {
                glDisable(GL_TEXTURE_2D);
                glColor3f(mat.diffuse.r, mat.diffuse.g, mat.diffuse.b);
            }

            // Начинаем новый блок
            glBegin(GL_TRIANGLES);
        }

        // Рисуем треугольник
        for (int j = 0; j < 3; j++) {
            size_t idx = i * 3 + j;
            if (idx < numVertices) {
                // Текстурные координаты
                if (!model.texCoords.empty() && idx < model.texCoords.size() / 2) {
                    glTexCoord2f(model.texCoords[idx * 2], model.texCoords[idx * 2 + 1]);
                }

                // Нормаль
                if (!model.normals.empty() && idx < model.normals.size() / 3) {
                    glNormal3f(model.normals[idx * 3], model.normals[idx * 3 + 1], model.normals[idx * 3 + 2]);
                }

                // Вершина
                glVertex3f(model.vertices[idx * 3], model.vertices[idx * 3 + 1], model.vertices[idx * 3 + 2]);
            }
        }
    }

    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}


void drawTexturedFloor() {
    glEnable(GL_TEXTURE_2D);

    if (floorTexture.id != 0) {
        glBindTexture(GL_TEXTURE_2D, floorTexture.id);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        if (!groundSkyElements.empty()) {
            glColor3f(groundSkyElements[0]->color.r,
                groundSkyElements[0]->color.g,
                groundSkyElements[0]->color.b);
        }
    }

    float size = 5.0f;
    float repeat = 10.0f;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -0.5f, -size);
    glTexCoord2f(repeat, 0.0f); glVertex3f(size, -0.5f, -size);
    glTexCoord2f(repeat, repeat); glVertex3f(size, -0.5f, size);
    glTexCoord2f(0.0f, repeat); glVertex3f(-size, -0.5f, size);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void drawGrid() {
    if (!currentConfig.gridEnabled) return; // Если сетка выключена, не рисуем

    glDisable(GL_LIGHTING);
    glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);

    // Устанавливаем толщину линий
    glLineWidth(currentConfig.gridLineWidth);

    glBegin(GL_LINES);
    for (int i = -2; i <= 2; i++) {
        glVertex3f(i, -0.5f, -2);
        glVertex3f(i, -0.5f, 2);
        glVertex3f(-2, -0.5f, i);
        glVertex3f(2, -0.5f, i);
    }
    glEnd();

    // Возвращаем толщину по умолчанию
    glLineWidth(1.0f);
}

void renderModelPreview(ModelData& model, const char* title, float x, float y, float w, float h,
    float& rotation, bool& autoRotate, float& lastTime, float scale = 1.0f) {
    // Сохраняем текущий viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Устанавливаем viewport для предпросмотра
    glViewport((GLint)x, windowHeight - (GLint)y - (GLint)h, (GLint)w, (GLint)h);
    glScissor((GLint)x, windowHeight - (GLint)y - (GLint)h, (GLint)w, (GLint)h);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // Настройки 3D
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    // Проекционная матрица
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    float aspect = w / h;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    // Видовая матрица
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glm::vec3 eye(3.0f, 2.0f, 5.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));

    // Рисуем пол
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-2.0f, -0.5f, -2.0f);
    glVertex3f(2.0f, -0.5f, -2.0f);
    glVertex3f(2.0f, -0.5f, 2.0f);
    glVertex3f(-2.0f, -0.5f, 2.0f);
    glEnd();

    // Рисуем сетку
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    for (int i = -4; i <= 4; i++) {
        float pos = i * 0.5f;
        glVertex3f(pos, -0.4f, -2.0f);
        glVertex3f(pos, -0.4f, 2.0f);
        glVertex3f(-2.0f, -0.4f, pos);
        glVertex3f(2.0f, -0.4f, pos);
    }
    glEnd();

    // Рисуем модель
    if (model.loaded && !model.vertices.empty()) {
        glPushMatrix();
        glRotatef(rotation, 0.0f, 1.0f, 0.0f);
        glScalef(scale, scale, scale);

        // Центрируем модель
        float minX = model.vertices[0], maxX = model.vertices[0];
        float minY = model.vertices[1], maxY = model.vertices[1];
        float minZ = model.vertices[2], maxZ = model.vertices[2];

        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            float vx = model.vertices[i * 3];
            float vy = model.vertices[i * 3 + 1];
            float vz = model.vertices[i * 3 + 2];

            minX = min(minX, vx);
            minY = min(minY, vy);
            minZ = min(minZ, vz);
            maxX = max(maxX, vx);
            maxY = max(maxY, vy);
            maxZ = max(maxZ, vz);
        }

        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        glTranslatef(-centerX, -centerY, -centerZ);

        drawModel(model);
        glPopMatrix();
    }
    else {
        // Рисуем тестовый куб
        glPushMatrix();
        glRotatef(rotation, 0.0f, 1.0f, 0.0f);
        glScalef(scale, scale, scale);

        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.0f, 0.0f);

        glBegin(GL_QUADS);
        // Передняя грань
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        // Задняя грань
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glEnd();

        glPopMatrix();
    }

    // Восстанавливаем матрицы
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Восстанавливаем viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);

    // Возвращаемся к 2D проекции для UI
    reset2DProjection();

    // Рисуем рамку и кнопки
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    renderRussianText(title, x + 10, y + 25, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    char scaleText[50];
    sprintf_s(scaleText, "Scale: %.2f", scale);
    renderRussianText(scaleText, x + w - 100, y + 25, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));

    int arrowY = (int)(y + h + 25);
    int arrowCenterX = (int)(x + w / 2);
    int arrowWidth = 60;
    int arrowHeight = 35;

    if (drawButton(arrowCenterX - arrowWidth - 10, arrowY, arrowWidth, arrowHeight, "<-")) {
        rotation -= 15.0f;
        lastTime = (float)glfwGetTime();
        autoRotate = false;
    }

    if (drawButton(arrowCenterX + 10, arrowY, arrowWidth, arrowHeight, "->")) {
        rotation += 15.0f;
        lastTime = (float)glfwGetTime();
        autoRotate = false;
    }

    float currentTime = (float)glfwGetTime();
    if (!autoRotate) {
        if (currentTime - lastTime >= autoRotateDelay) {
            autoRotate = true;
        }
    }

    if (autoRotate) {
        rotation += rotationSpeed;
        if (rotation >= 360) rotation -= 360;
    }
}
void debugTexture(GLuint textureID, const std::string& name) {
    if (textureID == 0) {
        std::cout << "Texture " << name << " is NULL" << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    GLint width, height, format;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);

    std::cout << "Texture " << name << " ID: " << textureID
        << " Size: " << width << "x" << height
        << " Format: " << format << std::endl;

    glBindTexture(GL_TEXTURE_2D, 0);
}

void validateModelData(ModelData& model) {
    std::cout << "\n=== VALIDATING MODEL DATA ===" << std::endl;

    size_t numVerts = model.vertices.size() / 3;
    std::cout << "Vertices: " << numVerts << std::endl;
    std::cout << "Normals: " << model.normals.size() / 3 << std::endl;
    std::cout << "TexCoords: " << model.texCoords.size() / 2 << std::endl;
    std::cout << "Materials: " << model.materials.size() << std::endl;
    std::cout << "Material indices: " << model.materialIndices.size() << std::endl;

    // Проверяем соответствие
    if (numVerts > 0) {
        if (model.normals.size() / 3 != numVerts) {
            std::cout << "⚠ WARNING: Normal count mismatch!" << std::endl;
        }
        if (!model.texCoords.empty() && model.texCoords.size() / 2 != numVerts) {
            std::cout << "⚠ WARNING: TexCoord count mismatch!" << std::endl;
        }
    }

    // Проверяем индексы материалов
    if (!model.materialIndices.empty()) {
        int maxIndex = 0;
        for (int idx : model.materialIndices) {
            if (idx > maxIndex) maxIndex = idx;
        }
        std::cout << "Max material index: " << maxIndex << std::endl;
        if (maxIndex >= (int)model.materials.size()) {
            std::cout << "⚠ WARNING: Material index out of range!" << std::endl;
        }
    }

    std::cout << "============================\n" << std::endl;
}
//=============================================================================
// СОХРАНЕНИЕ КОНФИГА
//=============================================================================
void saveConfig() {
    // Змея
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

    // Пол и небо
    if (groundSkyElements.size() >= 2) {
        currentConfig.floorModel = groundSkyElements[0]->modelFile;
        currentConfig.floorTexture = groundSkyElements[0]->textureFile;
        currentConfig.floorColor = groundSkyElements[0]->color;

        currentConfig.skyColor = groundSkyElements[1]->color;
    }

    // Преграды
    if (obstaclesElements.size() >= 4) {
        currentConfig.treeModel = obstaclesElements[0]->modelFile;
        currentConfig.rockModel = obstaclesElements[1]->modelFile;
        currentConfig.fenceModel = obstaclesElements[2]->modelFile;
        currentConfig.appleModel = obstaclesElements[3]->modelFile;
        currentConfig.initialFoodCount = obstaclesElements[3]->count;
    }

    // Окружение
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
// 2D ПРОЕКЦИЯ
//=============================================================================
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
// ШРИФТЫ И ТЕКСТ
//=============================================================================
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
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
    std::cout << "\n========== ИНИЦИАЛИЗАЦИЯ ШРИФТОВ ==========" << std::endl;

    if (FT_Init_FreeType(&ft)) {
        std::cerr << "❌ Не удалось инициализировать FreeType" << std::endl;
        return;
    }
    std::cout << "✓ FreeType инициализирован" << std::endl;

    std::string fontPath = "C:/Windows/Fonts/arial.ttf";
    std::cout << "Загрузка шрифта: " << fontPath << std::endl;

    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "❌ Не удалось загрузить шрифт Arial" << std::endl;
        return;
    }

    std::cout << "✓ Шрифт загружен успешно" << std::endl;

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int loadedCount = 0;
    for (unsigned char c = 32; c < 128; c++) {
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
        Characters.insert(std::pair<unsigned char, Character>(c, character));
        loadedCount++;
    }

    std::cout << "✓ Загружено ASCII символов: " << loadedCount << std::endl;

    int russianLoaded = 0;

    for (int i = 0; i < 32; i++) {
        int unicode = 0x0410 + i;
        unsigned char cp1251 = 0xC0 + i;

        if (FT_Load_Char(face, unicode, FT_LOAD_RENDER)) {
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
        Characters.insert(std::pair<unsigned char, Character>(cp1251, character));
        russianLoaded++;
    }

    if (FT_Load_Char(face, 0x0401, FT_LOAD_RENDER) == 0) {
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
        Characters.insert(std::pair<unsigned char, Character>(0xA8, character));
        russianLoaded++;
    }

    for (int i = 0; i < 32; i++) {
        int unicode = 0x0430 + i;
        unsigned char cp1251 = 0xE0 + i;

        if (FT_Load_Char(face, unicode, FT_LOAD_RENDER)) {
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
        Characters.insert(std::pair<unsigned char, Character>(cp1251, character));
        russianLoaded++;
    }

    if (FT_Load_Char(face, 0x0451, FT_LOAD_RENDER) == 0) {
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
        Characters.insert(std::pair<unsigned char, Character>(0xB8, character));
        russianLoaded++;
    }

    std::cout << "✓ Загружено русских символов: " << russianLoaded << std::endl;
    std::cout << "✓ Всего символов: " << Characters.size() << std::endl;
    std::cout << "============================================\n" << std::endl;

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

float getTextWidth(const char* text, float scale) {
    float width = 0;
    for (const unsigned char* c = (const unsigned char*)text; *c; c++) {
        auto it = Characters.find(*c);
        if (it != Characters.end()) {
            width += (it->second.Advance >> 6) * scale;
        }
        else {
            auto spaceIt = Characters.find(' ');
            if (spaceIt != Characters.end()) {
                width += (spaceIt->second.Advance >> 6) * scale;
            }
        }
    }
    return width;
}

void renderText(const char* text, float x, float y, float scale, glm::vec3 color, bool centerX, bool centerY) {
    if (!text || !*text) return;

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

    for (const unsigned char* c = (const unsigned char*)text; *c; c++) {
        auto it = Characters.find(*c);
        if (it == Characters.end()) {
            it = Characters.find(' ');
            if (it == Characters.end()) continue;
        }

        Character ch = it->second;

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

bool drawSlider(int x, int y, int w, float* value, float minVal, float maxVal, const char* label) {
    bool hover = (mouseX >= x && mouseX <= x + w && mouseY >= y - 10 && mouseY <= y + 10);
    bool changed = false;

    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glEnd();

    float thumbX = x + (*value - minVal) / (maxVal - minVal) * w;

    if (hover && mousePressed) {
        *value = minVal + (mouseX - x) / w * (maxVal - minVal);
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
    glVertex2f(thumbX - 5, y - 10);
    glVertex2f(thumbX + 5, y - 10);
    glVertex2f(thumbX + 5, y + 10);
    glVertex2f(thumbX - 5, y + 10);
    glEnd();

    char valueText[100];
    sprintf_s(valueText, "%s: %.2f", label, *value);
    renderText(valueText, x + w + 10, y - 5, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));

    return changed;
}

bool drawIntSlider(int x, int y, int w, int* value, int minVal, int maxVal, const char* label) {
    float fval = (float)*value;
    bool changed = drawSlider(x, y, w, &fval, (float)minVal, (float)maxVal, label);
    if (changed) {
        *value = (int)(fval + 0.5f);
    }
    return changed;
}

void drawInfoBox(int x, int y, int w, int h, const char* label, const char* value) {
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

void drawColorPicker(int x, int y, const char* label, glm::vec3& color) {
    renderRussianText(label, x, y - 20, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 60, y);
    glVertex2f(x + 60, y + 35);
    glVertex2f(x, y + 35);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + 60, y);
    glVertex2f(x + 60, y + 35);
    glVertex2f(x, y + 35);
    glEnd();

    drawSlider(x + 70, y + 5, 120, &color.r, 0.0f, 1.0f, "R");
    drawSlider(x + 70, y + 40, 120, &color.g, 0.0f, 1.0f, "G");
    drawSlider(x + 70, y + 75, 120, &color.b, 0.0f, 1.0f, "B");
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
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            if (currentMode != MODE_MAIN) {
                currentMode = MODE_MAIN;
            }
            else {
                glfwSetWindowShouldClose(window, true);
            }
            break;
        }
    }
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

//=============================================================================
// ГЛАВНОЕ МЕНЮ
//=============================================================================
void renderMainMenu() {
    reset2DProjection();

    renderRussianText("РЕДАКТОР КОНФИГУРАЦИИ ИГРЫ ЗМЕЙКА",
        windowWidth / 2, windowHeight * 0.08f, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int buttonWidth = windowWidth * 0.28f;
    int buttonHeight = windowHeight * 0.07f;
    int startY = windowHeight * 0.2f;
    int centerX = windowWidth / 2 - buttonWidth / 2;
    int spacing = windowHeight * 0.08f;

    if (drawRussianButton(centerX, startY, buttonWidth, buttonHeight, "1. ЗМЕЯ")) {
        currentMode = MODE_SNAKE_EDITOR;
        selectedPart = 0;
        updatePreviewForCurrentMode();
    }

    if (drawRussianButton(centerX, startY + spacing, buttonWidth, buttonHeight, "2. ПОЛ И НЕБО")) {
        currentMode = MODE_GROUND_SKY_EDITOR;
        selectedGroundSky = 0;
        updatePreviewForCurrentMode();
    }

    if (drawRussianButton(centerX, startY + spacing * 2, buttonWidth, buttonHeight, "3. ПРЕГРАДЫ")) {
        currentMode = MODE_OBSTACLES_EDITOR;
        selectedObstacle = 0;
        updatePreviewForCurrentMode();
    }

    if (drawRussianButton(centerX, startY + spacing * 3, buttonWidth, buttonHeight, "4. ОКРУЖЕНИЕ")) {
        currentMode = MODE_ENVIRONMENT_EDITOR;
        selectedEnv = 0;
        updatePreviewForCurrentMode();
    }

    if (drawRussianButton(centerX, startY + spacing * 4, buttonWidth, buttonHeight, "5. ИГРОВОЕ ПОЛЕ")) {
        currentMode = MODE_GRID_EDITOR;
        updatePreviewForCurrentMode();
    }

    if (drawRussianButton(centerX, startY + spacing * 5 + 20, buttonWidth, buttonHeight, "СОХРАНИТЬ И ВЫЙТИ")) {
        saveConfig();
        glfwSetWindowShouldClose(window, true);
    }

    char gridInfo[100];
    sprintf_s(gridInfo, "Сетка: %dx%d  Размер ячейки: %.2f",
        currentConfig.gridWidth, currentConfig.gridDepth, currentConfig.cellSize);
    renderRussianText(gridInfo, windowWidth / 2, windowHeight * 0.9f, 0.3f, glm::vec3(0.8f, 0.8f, 1.0f), true, false);
}

//=============================================================================
// РЕДАКТОР ЗМЕЙКИ
//=============================================================================
void renderSnakeEditor() {
    reset2DProjection();

    renderRussianText("РЕДАКТОР ЗМЕЙКИ", windowWidth / 2, windowHeight * 0.05f, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int listX = windowWidth * 0.03f;
    int listY = windowHeight * 0.12f;
    int listWidth = windowWidth * 0.1f;
    int listHeight = windowHeight * 0.05f;

    // Список частей змеи
    for (size_t i = 0; i < snakeElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", snakeElements[i]->name.c_str());
        if (drawRussianButton(listX, listY + i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedPart = i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedPart >= 0 && selectedPart < snakeElements.size()) {
        VisualElement* el = snakeElements[selectedPart];

        int editX = windowWidth * 0.18f;
        int editY = windowHeight * 0.12f;
        int buttonWidth = 90;
        int buttonSpacing = 100;

        renderRussianText(el->name.c_str(), editX, editY - 20, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));

        // ===== МОДЕЛЬ =====
        int modelY = editY;
        renderRussianText("Модель:", editX, modelY + 20, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderRussianText(el->modelFile.c_str(), editX + 80, modelY + 20, 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (drawRussianButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
            openFileDialog(el->modelFile, folder);
        }

        if (drawRussianButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedPart == 0) el->modelFile = "snake_head.fbx";
            else if (selectedPart == 1) el->modelFile = "snake_body.fbx";
            else el->modelFile = "snake_tail.fbx";
            updatePreviewForCurrentMode();
        }

        // ===== ЦВЕТ =====
        int colorY = modelY + 90;
        renderRussianText("Цвет:", editX, colorY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        glColor3f(el->color.r, el->color.g, el->color.b);
        glBegin(GL_QUADS);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &el->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &el->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &el->color.b, 0.0f, 1.0f, "B");

        if (drawRussianButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            if (selectedPart == 0) el->color = glm::vec3(0.0f, 1.0f, 0.0f);
            else if (selectedPart == 1) el->color = glm::vec3(0.0f, 0.7f, 0.0f);
            else el->color = glm::vec3(0.0f, 0.5f, 0.0f);
        }

        // ===== МАСШТАБ =====
        int scaleY = colorY + 80;
        renderRussianText("Масштаб:", editX, scaleY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        renderRussianText(scaleText, editX + 100, scaleY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawRussianButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            el->scale = 0.8f;
        }

        // ===== КНОПКА ОЧИСТИТЬ ВСЕ =====
        if (drawRussianButton(editX, scaleY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            if (selectedPart == 0) {
                el->modelFile = "snake_head.fbx";
                el->color = glm::vec3(0.0f, 1.0f, 0.0f);
                el->scale = 0.8f;
            }
            else if (selectedPart == 1) {
                el->modelFile = "snake_body.fbx";
                el->color = glm::vec3(0.0f, 0.7f, 0.0f);
                el->scale = 0.8f;
            }
            else {
                el->modelFile = "snake_tail.fbx";
                el->color = glm::vec3(0.0f, 0.5f, 0.0f);
                el->scale = 0.8f;
            }
            updatePreviewForCurrentMode();
        }
    }

    if (selectedPart >= 0 && selectedPart < snakeElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            windowWidth * 0.46f, windowHeight * 0.12f,
            windowWidth * 0.36f, windowHeight * 0.5f,
            previewRotation, autoRotate, lastRotationTime,
            snakeElements[selectedPart]->scale);  // ← передаем масштаб
    }

    if (drawRussianButton(windowWidth * 0.03f, windowHeight * 0.9f, 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ПОЛА И НЕБА
//=============================================================================
void renderGroundSkyEditor() {
    reset2DProjection();

    renderRussianText("ПОЛ И НЕБО", windowWidth / 2, windowHeight * 0.05f, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int listX = windowWidth * 0.03f;
    int listY = windowHeight * 0.12f;
    int listWidth = windowWidth * 0.1f;
    int listHeight = windowHeight * 0.05f;

    for (size_t i = 0; i < groundSkyElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", groundSkyElements[i]->name.c_str());
        if (drawRussianButton(listX, listY + i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedGroundSky = i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedGroundSky == 0) {
        // ПОЛ
        VisualElement* ground = groundSkyElements[0];

        int editX = windowWidth * 0.18f;
        int editY = windowHeight * 0.12f;
        int buttonWidth = 90;
        int buttonSpacing = 100;

        renderRussianText(ground->name.c_str(), editX, editY - 20, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));

        // ===== МОДЕЛЬ ПОЛА =====
        int modelY = editY;
        renderRussianText("Модель:", editX, modelY + 20, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderRussianText(ground->modelFile.c_str(), editX + 80, modelY + 20, 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (drawRussianButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(ground->modelFile, "floor");
        }

        if (drawRussianButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            ground->modelFile = "";
            updatePreviewForCurrentMode();
        }

        // ===== ТЕКСТУРА ПОЛА =====
        int textureY = modelY + 90;
        renderRussianText("Текстура:", editX, textureY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderRussianText(ground->textureFile.c_str(), editX + 100, textureY, 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (drawRussianButton(editX, textureY + 20, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openTextureFileDialog(ground->textureFile, floorTexture);
        }

        if (drawRussianButton(editX + buttonSpacing, textureY + 20, buttonWidth, 30, "СБРОСИТЬ")) {
            ground->textureFile = "";
            if (floorTexture.id != 0) {
                glDeleteTextures(1, &floorTexture.id);
                floorTexture.id = 0;
            }
        }

        // ===== ЦВЕТ ПОЛА =====
        int colorY = textureY + 70;
        renderRussianText("Цвет:", editX, colorY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        glColor3f(ground->color.r, ground->color.g, ground->color.b);
        glBegin(GL_QUADS);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &ground->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &ground->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &ground->color.b, 0.0f, 1.0f, "B");

        if (drawRussianButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            ground->color = glm::vec3(0.3f, 0.6f, 0.2f);
        }

        // ===== КНОПКА ОЧИСТИТЬ ВСЕ =====
        if (drawRussianButton(editX, colorY + 80, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
            ground->modelFile = "";
            ground->textureFile = "";
            ground->color = glm::vec3(0.3f, 0.6f, 0.2f);
            if (floorTexture.id != 0) {
                glDeleteTextures(1, &floorTexture.id);
                floorTexture.id = 0;
            }
            updatePreviewForCurrentMode();
        }

        if (selectedGroundSky == 0) {
            renderModelPreview(previewModel, "ПРЕДПРОСМОТР ПОЛА",
                windowWidth * 0.46f, windowHeight * 0.12f,
                windowWidth * 0.36f, windowHeight * 0.5f,
                previewRotation, autoRotate, lastRotationTime,
                groundSkyElements[0]->scale);  // ← передаем масштаб пола
        }
    }
    else {
        // НЕБО
        VisualElement* sky = groundSkyElements[1];

        int editX = windowWidth * 0.18f;
        int editY = windowHeight * 0.12f;
        int buttonWidth = 90;
        int buttonSpacing = 100;

        renderRussianText(sky->name.c_str(), editX, editY - 20, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));

        // ===== ТЕКСТУРА НЕБА =====
        int textureY = editY;
        renderRussianText("Текстура:", editX, textureY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderRussianText(sky->textureFile.c_str(), editX + 100, textureY, 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (drawRussianButton(editX, textureY + 20, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openTextureFileDialog(sky->textureFile, skyTexture);
        }

        if (drawRussianButton(editX + buttonSpacing, textureY + 20, buttonWidth, 30, "СБРОСИТЬ")) {
            sky->textureFile = "";
            if (skyTexture.id != 0) {
                glDeleteTextures(1, &skyTexture.id);
                skyTexture.id = 0;
            }
        }

        // ===== ЦВЕТ НЕБА =====
        int colorY = textureY + 70;
        renderRussianText("Цвет:", editX, colorY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        glColor3f(sky->color.r, sky->color.g, sky->color.b);
        glBegin(GL_QUADS);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &sky->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &sky->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &sky->color.b, 0.0f, 1.0f, "B");

        if (drawRussianButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            sky->color = glm::vec3(0.53f, 0.81f, 0.92f);
        }

        // ===== КНОПКА ОЧИСТИТЬ ВСЕ =====
        if (drawRussianButton(editX, colorY + 80, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
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
        glVertex2f(windowWidth * 0.46f, windowHeight * 0.12f);
        glVertex2f(windowWidth * 0.46f + windowWidth * 0.36f, windowHeight * 0.12f);
        glVertex2f(windowWidth * 0.46f + windowWidth * 0.36f, windowHeight * 0.12f + windowHeight * 0.5f);
        glVertex2f(windowWidth * 0.46f, windowHeight * 0.12f + windowHeight * 0.5f);
        glEnd();

        renderRussianText("НЕТ ПРЕДПРОСМОТРА ДЛЯ НЕБА",
            windowWidth * 0.46f + windowWidth * 0.18f,
            windowHeight * 0.12f + windowHeight * 0.25f,
            0.3f, glm::vec3(1.0f, 1.0f, 0.0f), true, true);
    }

    if (drawRussianButton(windowWidth * 0.03f, windowHeight * 0.9f, 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ПРЕГРАД
//=============================================================================
void renderObstaclesEditor() {
    reset2DProjection();

    renderRussianText("ПРЕГРАДЫ", windowWidth / 2, windowHeight * 0.05f, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int listX = windowWidth * 0.03f;
    int listY = windowHeight * 0.12f;
    int listWidth = windowWidth * 0.1f;
    int listHeight = windowHeight * 0.05f;

    for (size_t i = 0; i < obstaclesElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", obstaclesElements[i]->name.c_str());
        if (drawRussianButton(listX, listY + i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedObstacle = i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedObstacle >= 0 && selectedObstacle < obstaclesElements.size()) {
        VisualElement* el = obstaclesElements[selectedObstacle];

        int editX = windowWidth * 0.18f;
        int editY = windowHeight * 0.12f;
        int buttonWidth = 90;
        int buttonSpacing = 100;

        renderRussianText(el->name.c_str(), editX, editY - 20, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));

        // ===== МОДЕЛЬ =====
        int modelY = editY;
        renderRussianText("Модель:", editX, modelY + 20, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderRussianText(el->modelFile.c_str(), editX + 80, modelY + 20, 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (drawRussianButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(el->modelFile, "obstacles");
        }

        if (drawRussianButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->modelFile = "tree.fbx";
            else if (selectedObstacle == 1) el->modelFile = "rock.fbx";
            else if (selectedObstacle == 2) el->modelFile = "fence.fbx";
            else el->modelFile = "apple.fbx";
            updatePreviewForCurrentMode();
        }

        // ===== ЦВЕТ =====
        int colorY = modelY + 90;
        renderRussianText("Цвет:", editX, colorY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        glColor3f(el->color.r, el->color.g, el->color.b);
        glBegin(GL_QUADS);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &el->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &el->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &el->color.b, 0.0f, 1.0f, "B");

        if (drawRussianButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->color = glm::vec3(0.1f, 0.4f, 0.1f);
            else if (selectedObstacle == 1) el->color = glm::vec3(0.5f, 0.5f, 0.5f);
            else if (selectedObstacle == 2) el->color = glm::vec3(0.6f, 0.4f, 0.2f);
            else el->color = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        // ===== МАСШТАБ =====
        int scaleY = colorY + 80;
        renderRussianText("Масштаб:", editX, scaleY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        renderRussianText(scaleText, editX + 100, scaleY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawRussianButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->scale = 1.5f;
            else if (selectedObstacle == 1) el->scale = 1.2f;
            else if (selectedObstacle == 2) el->scale = 1.0f;
            else el->scale = 0.8f;
        }

        // ===== КОЛИЧЕСТВО =====
        int countY = scaleY + 70;
        renderRussianText("Количество:", editX, countY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        char countText[20];
        sprintf_s(countText, "%d", el->count);
        renderRussianText(countText, editX + 120, countY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

        drawIntSlider(editX, countY + 20, 250, &el->count, 0, 50, "");

        if (drawRussianButton(editX + 260, countY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedObstacle == 0) el->count = 10;
            else if (selectedObstacle == 1) el->count = 5;
            else if (selectedObstacle == 2) el->count = 8;
            else el->count = 10;
        }

        // ===== КНОПКА ОЧИСТИТЬ ВСЕ =====
        if (drawRussianButton(editX, countY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
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

    if (selectedObstacle >= 0 && selectedObstacle < obstaclesElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            windowWidth * 0.46f, windowHeight * 0.12f,
            windowWidth * 0.36f, windowHeight * 0.5f,
            previewRotation, autoRotate, lastRotationTime,
            obstaclesElements[selectedObstacle]->scale);  // ← передаем масштаб
    }

    if (drawRussianButton(windowWidth * 0.03f, windowHeight * 0.9f, 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ОКРУЖЕНИЯ
//=============================================================================
void renderEnvironmentEditor() {
    reset2DProjection();

    renderRussianText("ОКРУЖЕНИЕ", windowWidth / 2, windowHeight * 0.05f, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int listX = windowWidth * 0.03f;
    int listY = windowHeight * 0.12f;
    int listWidth = windowWidth * 0.1f;
    int listHeight = windowHeight * 0.05f;

    for (size_t i = 0; i < environmentElements.size(); i++) {
        char btnText[100];
        sprintf_s(btnText, "%s", environmentElements[i]->name.c_str());
        if (drawRussianButton(listX, listY + i * (listHeight + 10), listWidth, listHeight, btnText)) {
            selectedEnv = i;
            updatePreviewForCurrentMode();
        }
    }

    if (selectedEnv >= 0 && selectedEnv < environmentElements.size()) {
        VisualElement* el = environmentElements[selectedEnv];

        int editX = windowWidth * 0.18f;
        int editY = windowHeight * 0.12f;
        int buttonWidth = 90;
        int buttonSpacing = 100;

        renderRussianText(el->name.c_str(), editX, editY - 20, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));

        std::string folder = (selectedEnv == 0) ? "flowers" : (selectedEnv == 1) ? "birds" : "clouds";

        // ===== МОДЕЛЬ =====
        int modelY = editY;
        renderRussianText("Модель:", editX, modelY + 20, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderRussianText(el->modelFile.c_str(), editX + 80, modelY + 20, 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (drawRussianButton(editX, modelY + 40, buttonWidth, 30, "ЗАГРУЗИТЬ")) {
            openFileDialog(el->modelFile, folder);
        }

        if (drawRussianButton(editX + buttonSpacing, modelY + 40, buttonWidth, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->modelFile = "flower.fbx";
            else if (selectedEnv == 1) el->modelFile = "bird.fbx";
            else el->modelFile = "cloud.fbx";
            updatePreviewForCurrentMode();
        }

        // ===== ЦВЕТ =====
        int colorY = modelY + 90;
        renderRussianText("Цвет:", editX, colorY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        glColor3f(el->color.r, el->color.g, el->color.b);
        glBegin(GL_QUADS);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(editX + 60, colorY - 15);
        glVertex2f(editX + 110, colorY - 15);
        glVertex2f(editX + 110, colorY + 15);
        glVertex2f(editX + 60, colorY + 15);
        glEnd();

        drawSlider(editX + 120, colorY - 10, 150, &el->color.r, 0.0f, 1.0f, "R");
        drawSlider(editX + 120, colorY + 15, 150, &el->color.g, 0.0f, 1.0f, "G");
        drawSlider(editX + 120, colorY + 40, 150, &el->color.b, 0.0f, 1.0f, "B");

        if (drawRussianButton(editX + 280, colorY + 15, 80, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->color = glm::vec3(1.0f, 0.0f, 1.0f);
            else if (selectedEnv == 1) el->color = glm::vec3(0.5f, 0.5f, 0.5f);
            else el->color = glm::vec3(1.0f, 1.0f, 1.0f);
        }

        // ===== МАСШТАБ =====
        int scaleY = colorY + 80;
        renderRussianText("Масштаб:", editX, scaleY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        char scaleText[20];
        sprintf_s(scaleText, "%.2f", el->scale);
        renderRussianText(scaleText, editX + 100, scaleY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

        drawSlider(editX, scaleY + 20, 250, &el->scale, 0.1f, 3.0f, "");

        if (drawRussianButton(editX + 260, scaleY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->scale = 0.7f;
            else if (selectedEnv == 1) el->scale = 0.6f;
            else el->scale = 1.5f;
        }

        // ===== КОЛИЧЕСТВО =====
        int countY = scaleY + 70;
        renderRussianText("Количество:", editX, countY, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));

        char countText[20];
        sprintf_s(countText, "%d", el->count);
        renderRussianText(countText, editX + 120, countY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

        drawIntSlider(editX, countY + 20, 250, &el->count, 0, 50, "");

        if (drawRussianButton(editX + 260, countY + 10, 80, 30, "СБРОСИТЬ")) {
            if (selectedEnv == 0) el->count = 25;
            else if (selectedEnv == 1) el->count = 15;
            else el->count = 20;
        }

        // ===== КНОПКА ОЧИСТИТЬ ВСЕ =====
        if (drawRussianButton(editX, countY + 70, 150, 35, "ОЧИСТИТЬ ВСЕ")) {
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

    if (selectedEnv >= 0 && selectedEnv < environmentElements.size()) {
        renderModelPreview(previewModel, "ПРЕДПРОСМОТР",
            windowWidth * 0.46f, windowHeight * 0.12f,
            windowWidth * 0.36f, windowHeight * 0.5f,
            previewRotation, autoRotate, lastRotationTime,
            environmentElements[selectedEnv]->scale);  // ← передаем масштаб
    }

    if (drawRussianButton(windowWidth * 0.03f, windowHeight * 0.9f, 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
}

//=============================================================================
// РЕДАКТОР ИГРОВОГО ПОЛЯ
//=============================================================================
void renderGridEditor() {
    reset2DProjection();

    renderRussianText("ИГРОВОЕ ПОЛЕ", windowWidth / 2, windowHeight * 0.05f, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int startX = windowWidth * 0.14f;
    int startY = windowHeight * 0.12f;
    int spacing = windowHeight * 0.08f;
    int sliderWidth = windowWidth * 0.25f;

    // Включение/выключение сетки
    renderRussianText("СЕТКА", startX, startY - 30, 0.35f, glm::vec3(1.0f, 1.0f, 0.0f));

    char btnText[50];
    sprintf_s(btnText, "Сетка: %s", currentConfig.gridEnabled ? "ВКЛ" : "ВЫКЛ");
    if (drawRussianButton(startX, startY, 150, 40, btnText)) {
        currentConfig.gridEnabled = !currentConfig.gridEnabled;
    }

    // Кнопка сброса настроек сетки
    if (drawRussianButton(startX + 160, startY, 120, 40, "СБРОСИТЬ")) {
        currentConfig.gridEnabled = true;
        currentConfig.gridLineWidth = 1.0f;
        currentConfig.gridWidth = 120;
        currentConfig.gridDepth = 120;
        currentConfig.cellSize = 0.1f;
        currentConfig.gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    }

    // Толщина линий (только если сетка включена)
    if (currentConfig.gridEnabled) {
        renderRussianText("Толщина линий:", startX, startY + 60, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        drawSlider(startX, startY + 90, sliderWidth, &currentConfig.gridLineWidth, 0.5f, 5.0f, "Толщина");

        // Кнопка сброса толщины
        if (drawRussianButton(startX + sliderWidth + 20, startY + 80, 100, 30, "СБРОСИТЬ")) {
            currentConfig.gridLineWidth = 1.0f;
        }
        startY += 60;
    }

    startY += spacing;

    // Размер сетки
    renderRussianText("РАЗМЕР СЕТКИ", startX, startY - 30, 0.35f, glm::vec3(1.0f, 1.0f, 0.0f));

    drawIntSlider(startX, startY, sliderWidth, &currentConfig.gridWidth, 5, 200, "Ширина");
    drawIntSlider(startX, startY + 50, sliderWidth, &currentConfig.gridDepth, 5, 200, "Глубина");

    // Кнопка сброса размера
    if (drawRussianButton(startX + sliderWidth + 20, startY + 10, 100, 30, "СБРОСИТЬ")) {
        currentConfig.gridWidth = 120;
        currentConfig.gridDepth = 120;
    }

    // Размер ячейки
    startY += spacing + 50;
    renderRussianText("РАЗМЕР ЯЧЕЙКИ", startX, startY - 30, 0.35f, glm::vec3(1.0f, 1.0f, 0.0f));
    drawSlider(startX, startY, sliderWidth, &currentConfig.cellSize, 0.05f, 1.0f, "Размер");

    // Кнопка сброса размера ячейки
    if (drawRussianButton(startX + sliderWidth + 20, startY - 10, 100, 30, "СБРОСИТЬ")) {
        currentConfig.cellSize = 0.1f;
    }

    // Цвет сетки
    startY += spacing;
    drawColorPicker(startX, startY, "Цвет сетки:", currentConfig.gridColor);

    // Кнопка сброса цвета
    if (drawRussianButton(startX + sliderWidth + 20, startY + 50, 100, 30, "СБРОСИТЬ")) {
        currentConfig.gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    }

    // Кнопка "ОЧИСТИТЬ ВСЕ" внизу
    if (drawRussianButton(startX, windowHeight * 0.8f, 150, 40, "ОЧИСТИТЬ ВСЕ")) {
        currentConfig.gridEnabled = true;
        currentConfig.gridLineWidth = 1.0f;
        currentConfig.gridWidth = 120;
        currentConfig.gridDepth = 120;
        currentConfig.cellSize = 0.1f;
        currentConfig.gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    }

    // Предпросмотр сетки
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    int previewX = windowWidth * 0.46f;
    int previewY = windowHeight * 0.12f;
    int previewW = windowWidth * 0.36f;
    int previewH = windowHeight * 0.6f;

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
    glm::vec3 center(0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));

    // Рисуем пол
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    float size = currentConfig.gridWidth * currentConfig.cellSize / 2;
    float depth = currentConfig.gridDepth * currentConfig.cellSize / 2;
    glVertex3f(-size, -0.5f, -depth);
    glVertex3f(size, -0.5f, -depth);
    glVertex3f(size, -0.5f, depth);
    glVertex3f(-size, -0.5f, depth);
    glEnd();

    // Рисуем сетку если включена
    if (currentConfig.gridEnabled) {
        glColor3f(currentConfig.gridColor.r, currentConfig.gridColor.g, currentConfig.gridColor.b);

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glLineWidth(currentConfig.gridLineWidth);

        glBegin(GL_LINES);
        for (int i = -currentConfig.gridWidth / 2; i <= currentConfig.gridWidth / 2; i++) {
            float x = i * currentConfig.cellSize;
            glVertex3f(x, -0.4f, -depth);
            glVertex3f(x, -0.4f, depth);
        }
        for (int i = -currentConfig.gridDepth / 2; i <= currentConfig.gridDepth / 2; i++) {
            float z = i * currentConfig.cellSize;
            glVertex3f(-size, -0.4f, z);
            glVertex3f(size, -0.4f, z);
        }
        glEnd();

        glLineWidth(1.0f);
        glDisable(GL_LINE_SMOOTH);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();

    reset2DProjection();

    // Рамка предпросмотра
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(previewX, previewY);
    glVertex2f(previewX + previewW, previewY);
    glVertex2f(previewX + previewW, previewY + previewH);
    glVertex2f(previewX, previewY + previewH);
    glEnd();

    renderRussianText("ПРЕДПРОСМОТР СЕТКИ", previewX + 10, previewY + 25, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    if (drawRussianButton(windowWidth * 0.03f, windowHeight * 0.9f, 150, 50, "НАЗАД")) {
        currentMode = MODE_MAIN;
        saveConfig();
    }
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

    if (!initOpenGL()) {
        return -1;
    }

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
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "=         РАБОТА ЗАВЕРШЕНА             =" << std::endl;
    std::cout << "=========================================\n" << std::endl;

    return 0;
}