
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
#include "../Shared/ConfigTypes.h"  // ИСПРАВЛЕНО: правильный путь
std::string g_assetsPath;
std::string g_modelsPath;
std::string g_texturesPath;
std::string g_configPath;
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

// Структура для препятствия
struct ObstacleItem {
    std::string name;
    std::string modelFile;
    glm::vec3 color;
    float scale;
    int posX, posZ;
    bool enabled;
};

// Глобальные переменные
GLFWwindow* window;
int windowWidth = 1200;
int windowHeight = 800;
GameConfig currentConfig;

// Пути
std::string basePath;
std::string modelsPath;
std::string texturesPath;
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
    MODE_SNAKE_EDITOR,
    MODE_OBSTACLE_EDITOR,
    MODE_ENVIRONMENT_EDITOR
};
EditorMode currentMode = MODE_MAIN;

// Для редактора змейки
int selectedPart = 0; // 0-голова, 1-тело, 2-хвост
float previewRotation = 0.0f;
bool autoRotate = true;
float lastRotationTime = 0.0f;
float autoRotateDelay = 5.0f;
float rotationSpeed = 0.5f;
ModelData currentModelData;

// Для редактора препятствий
std::vector<ObstacleItem> obstacles;
int selectedObstacle = -1;
float obstaclePreviewRotation = 0.0f;
bool obstacleAutoRotate = true;
float obstacleLastRotationTime = 0.0f;

// Для редактора окружения
float skyColor[3] = { 0.53f, 0.81f, 0.92f };
float floorColor[3] = { 0.3f, 0.6f, 0.2f };
float gridColor[3] = { 0.2f, 0.5f, 0.15f };
int cloudCount = 20;
int birdCount = 15;
int flowerCount = 25;

// Текстуры
Texture floorTexture;
std::vector<std::string> availableTextures;

// Для UI
bool mousePressed = false;
bool mousePressedLast = false;
double mouseX, mouseY;

// Прототипы
void renderMainMenu();
void renderSnakeEditor();
void renderObstacleEditor();
void renderEnvironmentEditor();
void render3DPreview();
void renderObstaclePreview();
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void openFileDialog();
void openObstacleFileDialog();
void initFreeType();
float getTextWidth(const std::string& text, float scale);
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, bool centerX = false, bool centerY = false);
bool drawButton(int x, int y, int w, int h, const std::string& text, bool enabled = true);
bool drawSlider(int x, int y, int w, float* value, float minVal, float maxVal, const std::string& label);
void drawInfoBox(int x, int y, int w, int h, const std::string& label, const std::string& value);
void drawGrid();
void reset2DProjection();
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder = "");
GLuint loadTextureFromFile(const std::string& path);
bool loadTexture(const std::string& filename, Texture& texture);
void drawModel(ModelData& model);
void drawTexturedFloor();
void saveConfig();
void initPaths();
void initObstacles();
void loadAvailableTextures();
std::string openTextureFileDialog();
void openModelFileDialog(const std::string& subFolder, std::string& destVar);
// Инициализация препятствий
void initObstacles() {
    obstacles.clear();

    ObstacleItem tree1;
    tree1.name = "Tree 1";
    tree1.modelFile = "tree.fbx";
    tree1.color = glm::vec3(0.1f, 0.4f, 0.1f);
    tree1.scale = 1.5f;
    tree1.posX = 20;
    tree1.posZ = 20;
    tree1.enabled = true;
    obstacles.push_back(tree1);

    ObstacleItem tree2;
    tree2.name = "Tree 2";
    tree2.modelFile = "tree.fbx";
    tree2.color = glm::vec3(0.1f, 0.4f, 0.1f);
    tree2.scale = 1.5f;
    tree2.posX = 40;
    tree2.posZ = 40;
    tree2.enabled = true;
    obstacles.push_back(tree2);

    ObstacleItem rock;
    rock.name = "Rock";
    rock.modelFile = "rock.fbx";
    rock.color = glm::vec3(0.5f, 0.5f, 0.5f);
    rock.scale = 1.2f;
    rock.posX = 30;
    rock.posZ = 30;
    rock.enabled = true;
    obstacles.push_back(rock);
}

// Инициализация путей
void initPaths() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::string exePath = std::string(currentDir);

    std::cout << "\n=========================================" << std::endl;
    std::cout << "===== CONFIG EDITOR PATH DEBUG =====" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "1. Current directory (exe): " << exePath << std::endl;

    // Находим корень проекта
    std::string rootPath = exePath;

    size_t pos = rootPath.find("\\Project2");
    if (pos != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
        std::cout << "2. Found Project2 folder, root: " << rootPath << std::endl;
    }
    else if ((pos = rootPath.find("\\x64\\Debug")) != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
        std::cout << "2. Found x64/Debug, root: " << rootPath << std::endl;
    }
    else if ((pos = rootPath.find("\\Debug")) != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
        std::cout << "2. Found Debug, root: " << rootPath << std::endl;
    }

    // Убеждаемся что мы в папке ractica
    if (rootPath.find("ractica") == std::string::npos) {
        rootPath += "\\ractica";
        std::cout << "3. Added '\\ractica', root: " << rootPath << std::endl;
    }

    g_assetsPath = rootPath + "\\assets\\";
    g_modelsPath = g_assetsPath + "models\\";
    g_texturesPath = g_assetsPath + "textures\\";
    g_configPath = g_assetsPath + "config\\game.cfg";

    std::cout << "\n--- FINAL PATHS ---" << std::endl;
    std::cout << "Assets path:  " << g_assetsPath << std::endl;
    std::cout << "Models path:  " << g_modelsPath << std::endl;
    std::cout << "Textures path: " << g_texturesPath << std::endl;
    std::cout << "Config path:  " << g_configPath << std::endl;

    // Проверяем существование папок
    std::cout << "\n--- FOLDER VALIDATION ---" << std::endl;

    DWORD attrib = GetFileAttributesA(g_assetsPath.c_str());
    std::cout << "Assets folder: " << ((attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) ? "✓ EXISTS" : "✗ NOT FOUND") << std::endl;

    std::string modelsFolder = g_modelsPath;
    attrib = GetFileAttributesA(modelsFolder.c_str());
    std::cout << "Models folder: " << ((attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) ? "✓ EXISTS" : "✗ NOT FOUND") << std::endl;

    std::string configFolder = g_assetsPath + "config\\";
    attrib = GetFileAttributesA(configFolder.c_str());
    std::cout << "Config folder: " << ((attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) ? "✓ EXISTS" : "✗ NOT FOUND") << std::endl;

    // Создаем папки если их нет
    std::cout << "\n--- CREATING FOLDERS ---" << std::endl;

    if (CreateDirectoryA(g_assetsPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "✓ Assets folder ready" << std::endl;
    }
    if (CreateDirectoryA((g_assetsPath + "config").c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "✓ Config folder ready" << std::endl;
    }
    if (CreateDirectoryA(g_modelsPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "✓ Models folder ready" << std::endl;
    }

    // Создаем подпапки для моделей
    std::vector<std::string> modelSubfolders = { "snake_head", "snake_body", "snake_tail", "obstacles" };
    for (const auto& subfolder : modelSubfolders) {
        std::string path = g_modelsPath + subfolder + "\\";
        if (CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
            std::cout << "✓ " << subfolder << " folder ready" << std::endl;
        }
    }

    std::cout << "=========================================\n" << std::endl;
}

// Загрузка текстуры из файла
GLuint loadTextureFromFile(const std::string& path) {
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
        std::cout << "Texture loaded: " << width << "x" << height << std::endl;
        return textureID;
    }
    else {
        std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
}

// Загрузка текстуры в структуру Texture
bool loadTexture(const std::string& filename, Texture& texture) {
    std::string fullPath = texturesPath + filename;

    std::cout << "Loading texture: " << fullPath << std::endl;

    if (!std::filesystem::exists(fullPath)) {
        std::cout << "Texture file does not exist!" << std::endl;
        return false;
    }

    if (texture.id != 0) {
        glDeleteTextures(1, &texture.id);
    }

    texture.id = loadTextureFromFile(fullPath);
    texture.path = filename;

    return texture.id != 0;
}

// Загрузка FBX модели с текстурами (улучшенная версия)
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder) {
    std::cout << "\n========== FBX LOADER DEBUG ==========" << std::endl;
    std::cout << "Loading model for folder: " << subFolder << std::endl;
    std::cout << "Filename: " << filename << std::endl;

    // Формируем полный путь
    std::string fullPath = g_modelsPath + subFolder + "\\" + filename;
    std::cout << "Full path: " << fullPath << std::endl;

    // Проверяем существование файла
    if (!std::filesystem::exists(fullPath)) {
        std::cout << "❌ ERROR: File does not exist!" << std::endl;

        // Показываем содержимое папки
        std::string folderPath = g_modelsPath + subFolder + "\\";
        std::cout << "Contents of " << folderPath << ":" << std::endl;
        try {
            int fileCount = 0;
            for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                std::cout << "  - " << entry.path().filename().string()
                    << " (" << std::filesystem::file_size(entry.path()) << " bytes)" << std::endl;
                fileCount++;
            }
            if (fileCount == 0) {
                std::cout << "  📁 Folder is empty!" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cout << "  ❌ Cannot read folder: " << e.what() << std::endl;
        }
        return false;
    }

    // Проверяем размер файла
    size_t fileSize = std::filesystem::file_size(fullPath);
    std::cout << "File size: " << fileSize << " bytes" << std::endl;

    if (fileSize == 0) {
        std::cout << "❌ ERROR: File is empty!" << std::endl;
        return false;
    }

    std::cout << "Initializing Assimp importer..." << std::endl;
    Assimp::Importer importer;

    // Пробуем загрузить с разными флагами
    std::cout << "Reading file with Assimp..." << std::endl;
    const aiScene* scene = importer.ReadFile(fullPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "❌ Assimp error: " << importer.GetErrorString() << std::endl;

        // Пробуем с минимальными флагами
        std::cout << "Retrying with minimal flags..." << std::endl;
        scene = importer.ReadFile(fullPath, aiProcess_Triangulate);

        if (!scene) {
            std::cout << "❌ Still failed: " << importer.GetErrorString() << std::endl;
            return false;
        }
    }

    std::cout << "✓ Scene loaded successfully!" << std::endl;
    std::cout << "Scene info:" << std::endl;
    std::cout << "  - Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  - Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "  - Animations: " << scene->mNumAnimations << std::endl;
    std::cout << "  - Textures: " << scene->mNumTextures << std::endl;

    if (!scene->mRootNode) {
        std::cout << "❌ ERROR: Scene has no root node!" << std::endl;
        return false;
    }

    // Очищаем старые данные
    model.vertices.clear();
    model.normals.clear();
    model.texCoords.clear();
    model.materials.clear();
    model.materialIndices.clear();

    std::cout << "\n--- Loading Materials ---" << std::endl;
    // Загружаем материалы
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material material;

        aiString matName;
        mat->Get(AI_MATKEY_NAME, matName);
        std::cout << "Material " << i << ": " << matName.C_Str() << std::endl;

        aiColor3D color(1.0f, 1.0f, 1.0f);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse = glm::vec3(color.r, color.g, color.b);
        std::cout << "  Diffuse color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;

        // Пытаемся загрузить текстуру
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            std::cout << "  Texture: " << path.C_Str() << std::endl;

            // Здесь можно добавить загрузку текстуры
            material.texturePath = path.C_Str();
        }

        model.materials.push_back(material);
    }

    // Если нет материалов, создаем дефолтный
    if (model.materials.empty()) {
        std::cout << "No materials found, creating default material" << std::endl;
        Material defaultMat;
        defaultMat.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        model.materials.push_back(defaultMat);
    }

    std::cout << "\n--- Loading Meshes ---" << std::endl;
    int totalVertices = 0;
    int totalFaces = 0;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "Mesh " << i << ":" << std::endl;
        std::cout << "  - Vertices: " << mesh->mNumVertices << std::endl;
        std::cout << "  - Faces: " << mesh->mNumFaces << std::endl;
        std::cout << "  - Has normals: " << (mesh->HasNormals() ? "YES" : "NO") << std::endl;
        std::cout << "  - Has texture coords: " << (mesh->HasTextureCoords(0) ? "YES" : "NO") << std::endl;

        int materialIndex = mesh->mMaterialIndex;
        if (materialIndex >= (int)model.materials.size()) {
            std::cout << "  ⚠ Material index out of range, using 0" << std::endl;
            materialIndex = 0;
        }

        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            // Вершины
            model.vertices.push_back(mesh->mVertices[j].x);
            model.vertices.push_back(mesh->mVertices[j].y);
            model.vertices.push_back(mesh->mVertices[j].z);

            // Нормали
            if (mesh->HasNormals()) {
                model.normals.push_back(mesh->mNormals[j].x);
                model.normals.push_back(mesh->mNormals[j].y);
                model.normals.push_back(mesh->mNormals[j].z);
            }
            else {
                model.normals.push_back(0.0f);
                model.normals.push_back(1.0f);
                model.normals.push_back(0.0f);
            }

            // Текстурные координаты
            if (mesh->HasTextureCoords(0)) {
                model.texCoords.push_back(mesh->mTextureCoords[0][j].x);
                model.texCoords.push_back(mesh->mTextureCoords[0][j].y);
            }
            else {
                model.texCoords.push_back(0.0f);
                model.texCoords.push_back(0.0f);
            }
        }

        // Индексы материалов для каждого треугольника
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            if (face.mNumIndices == 3) {
                model.materialIndices.push_back(materialIndex);
                totalFaces++;
            }
        }

        totalVertices += mesh->mNumVertices;
    }

    std::cout << "\n--- Loading Summary ---" << std::endl;
    std::cout << "Total vertices loaded: " << totalVertices << std::endl;
    std::cout << "Total triangles: " << totalFaces << std::endl;
    std::cout << "Total materials: " << model.materials.size() << std::endl;

    model.loaded = (model.vertices.size() > 0);

    if (model.loaded) {
        std::cout << "✅ MODEL LOADED SUCCESSFULLY!" << std::endl;
        std::cout << "   Vertex count: " << model.vertices.size() / 3 << std::endl;
        std::cout << "   Normal count: " << model.normals.size() / 3 << std::endl;
        std::cout << "   TexCoord count: " << model.texCoords.size() / 2 << std::endl;
    }
    else {
        std::cout << "❌ MODEL LOADING FAILED!" << std::endl;
    }

    std::cout << "====================================\n" << std::endl;
    return model.loaded;
}
// Функция рисования загруженной модели с текстурами
void drawModel(ModelData& model) {
    if (!model.loaded || model.vertices.empty()) {
        return; // Ничего не рисуем если модель не загружена
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, model.vertices.data());
    glNormalPointer(GL_FLOAT, 0, model.normals.data());
    glTexCoordPointer(2, GL_FLOAT, 0, model.texCoords.data());

    // Рисуем с материалами
    int vertexIndex = 0;
    for (size_t i = 0; i < model.materialIndices.size(); i++) {
        int materialIdx = model.materialIndices[i];
        if (materialIdx >= 0 && materialIdx < model.materials.size()) {
            Material& mat = model.materials[materialIdx];

            // Устанавливаем материал
            if (mat.textureID != 0) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mat.textureID);
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            else {
                glDisable(GL_TEXTURE_2D);
                glColor3f(mat.diffuse.r, mat.diffuse.g, mat.diffuse.b);
            }
        }

        glDrawArrays(GL_TRIANGLES, vertexIndex, 3);
        vertexIndex += 3;
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
}

// Рисование текстурированного пола
void drawTexturedFloor() {
    glEnable(GL_TEXTURE_2D);

    if (floorTexture.id != 0) {
        glBindTexture(GL_TEXTURE_2D, floorTexture.id);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        // Если текстуры нет, просто цвет
        glDisable(GL_TEXTURE_2D);
        glColor3f(floorColor[0], floorColor[1], floorColor[2]);
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

// Загрузка доступных текстур
void loadAvailableTextures() {
    availableTextures.clear();

    if (!std::filesystem::exists(texturesPath)) {
        return;
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(texturesPath)) {
            std::string ext = entry.path().extension().string();
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                availableTextures.push_back(entry.path().filename().string());
            }
        }
    }
    catch (...) {
        std::cout << "Could not read textures folder" << std::endl;
    }
}

// Диалог выбора текстуры
std::string openTextureFileDialog() {
    if (!std::filesystem::exists(texturesPath)) {
        std::filesystem::create_directories(texturesPath);
    }

    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = texturesPath.c_str();
    ofn.lpstrTitle = "Choose Texture";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        return fullPath.substr(fullPath.find_last_of("\\") + 1);
    }

    return "";
}

// Сохранение конфига
void saveConfig() {
    std::cout << "\n========== SAVING CONFIG ==========" << std::endl;
    std::cout << "Saving to: " << g_configPath << std::endl;

    // Проверяем существует ли папка config
    std::string configFolder = g_assetsPath + "config\\";
    DWORD attrib = GetFileAttributesA(configFolder.c_str());
    if (attrib == INVALID_FILE_ATTRIBUTES) {
        std::cout << "Config folder does not exist, creating..." << std::endl;
        CreateDirectoryA(configFolder.c_str(), NULL);
    }

    // Сохраняем
    if (ConfigManager::saveGameConfig(g_configPath, currentConfig)) {
        std::cout << "✓ Config saved successfully!" << std::endl;

        // Проверяем что файл создался
        std::ifstream checkFile(g_configPath);
        if (checkFile.is_open()) {
            std::cout << "✓ File exists at: " << g_configPath << std::endl;
            checkFile.close();
        }
    }
    else {
        std::cout << "✗ Failed to save config!" << std::endl;
    }
    std::cout << "==================================\n" << std::endl;
}

// Сброс 2D проекции
void reset2DProjection() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Функция открытия диалога для FBX файлов
void openFileDialog() {
    std::string subFolder;
    switch (selectedPart) {
    case 0: subFolder = "snake_head"; break;
    case 1: subFolder = "snake_body"; break;
    case 2: subFolder = "snake_tail"; break;
    }

    std::string folderPath = g_modelsPath + subFolder + "\\";

    std::cout << "\n========== FILE DIALOG DEBUG ==========" << std::endl;
    std::cout << "Selected part: " << subFolder << std::endl;
    std::cout << "Opening folder: " << folderPath << std::endl;

    // Создаем папку если нет
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
    ofn.lpstrTitle = "Choose Model";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1);

        std::cout << "Selected file: " << filename << std::endl;
        std::cout << "Full path: " << fullPath << std::endl;

        // Сохраняем в конфиг
        if (selectedPart == 0) currentConfig.snakeHeadModel = filename;
        else if (selectedPart == 1) currentConfig.snakeBodyModel = filename;
        else currentConfig.snakeTailModel = filename;

        // Копируем файл в папку assets если нужно
        std::string destPath = folderPath + filename;
        if (fullPath != destPath) {
            std::cout << "Copying to assets: " << destPath << std::endl;
            if (CopyFileA(fullPath.c_str(), destPath.c_str(), FALSE)) {
                std::cout << "✅ File copied successfully" << std::endl;
            }
            else {
                std::cout << "❌ Failed to copy file. Error: " << GetLastError() << std::endl;
            }
        }

        // Загружаем модель
        std::cout << "Loading model..." << std::endl;
        bool loaded = loadFBXModel(filename, currentModelData, subFolder);

        if (loaded) {
            std::cout << "✅ Model loaded and ready for preview!" << std::endl;
        }
        else {
            std::cout << "❌ Model loading failed!" << std::endl;
        }

        // Сохраняем конфиг
        saveConfig();
    }
    else {
        std::cout << "Dialog cancelled or failed" << std::endl;
    }
    std::cout << "======================================\n" << std::endl;
}
// Функция открытия диалога для препятствий
void openObstacleFileDialog() {
    std::string folderPath = modelsPath + "obstacles\\";

    if (!std::filesystem::exists(folderPath)) {
        std::filesystem::create_directories(folderPath);
    }

    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "FBX Files\0*.fbx\0OBJ Files\0*.obj\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = folderPath.c_str();
    ofn.lpstrTitle = "Choose Obstacle Model";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1);

        if (!filename.empty() && selectedObstacle >= 0) {
            obstacles[selectedObstacle].modelFile = filename;
            std::cout << "Selected obstacle model: " << filename << std::endl;
        }
    }
}

// Функция рисования сетки
void drawGrid() {
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

// Рисование кнопки
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

// Рисование слайдера
bool drawSlider(int x, int y, int w, float* value, float minVal, float maxVal, const std::string& label) {
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

    char valueText[50];
    sprintf_s(valueText, "%s: %.2f", label.c_str(), *value);
    renderText(valueText, x + w + 10, y - 5, 0.2f, glm::vec3(1.0f, 1.0f, 1.0f));

    return changed;
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

// 3D предпросмотр для змейки
// 3D предпросмотр для змейки
void render3DPreview() {
    int previewX = 650;
    int previewY = 120;
    int previewW = 500;
    int previewH = 400;

    std::cout << "\n========== 3D PREVIEW DEBUG ==========" << std::endl;
    std::cout << "Preview area: (" << previewX << ", " << previewY << ") size: " << previewW << "x" << previewH << std::endl;
    std::cout << "Current rotation: " << previewRotation << "°" << std::endl;
    std::cout << "Selected part: " << selectedPart << " (0=Head, 1=Body, 2=Tail)" << std::endl;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Устанавливаем viewport для области предпросмотра
    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    std::cout << "Viewport set: " << previewX << ", " << (windowHeight - previewY - previewH) << ", " << previewW << ", " << previewH << std::endl;

    // Очищаем буфер глубины
    glClear(GL_DEPTH_BUFFER_BIT);
    std::cout << "Depth buffer cleared" << std::endl;

    // Настройки глубины
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    std::cout << "Depth test enabled" << std::endl;

    // Настройка 3D проекции
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    float aspect = (float)previewW / previewH;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glLoadMatrixf(glm::value_ptr(projection));
    std::cout << "Projection matrix set (fov=45°, aspect=" << aspect << ")" << std::endl;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Позиция камеры
    glm::vec3 eye(3.0f, 2.0f, 5.0f);
    glm::vec3 center(0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));
    std::cout << "View matrix set: camera at (3,2,5) looking at (0,0,0)" << std::endl;

    // Вращение модели
    glRotatef(previewRotation, 0.0f, 1.0f, 0.0f);
    std::cout << "Rotation applied: " << previewRotation << "° around Y axis" << std::endl;

    // Временно отключаем освещение для отладки
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    std::cout << "Lighting disabled for debug" << std::endl;

    // Рисуем сетку
    std::cout << "Drawing grid..." << std::endl;
    drawGrid();

    // Рисуем пол
    std::cout << "Drawing floor..." << std::endl;
    drawTexturedFloor();

    // Проверяем загружена ли модель
    if (!currentModelData.loaded) {
        std::cout << "⚠ WARNING: No model loaded for preview!" << std::endl;
        std::cout << "Loading status: " << (currentModelData.loaded ? "LOADED" : "NOT LOADED") << std::endl;
        std::cout << "Vertices count: " << currentModelData.vertices.size() / 3 << std::endl;

        // Рисуем красный куб как заглушку
        std::cout << "Drawing fallback red cube..." << std::endl;
        glColor3f(1.0f, 0.0f, 0.0f);

        // Простой куб
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
        // Верхняя грань
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        // Нижняя грань
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        // Левая грань
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        // Правая грань
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glEnd();
        std::cout << "Fallback cube drawn" << std::endl;
    }
    else {
        std::cout << "✓ Model loaded successfully!" << std::endl;
        std::cout << "Model stats:" << std::endl;
        std::cout << "  - Vertices: " << currentModelData.vertices.size() / 3 << std::endl;
        std::cout << "  - Normals: " << currentModelData.normals.size() / 3 << std::endl;
        std::cout << "  - TexCoords: " << currentModelData.texCoords.size() / 2 << std::endl;
        std::cout << "  - Materials: " << currentModelData.materials.size() << std::endl;
        std::cout << "  - Triangles: " << currentModelData.materialIndices.size() << std::endl;

        // Устанавливаем цвет в зависимости от части
        switch (selectedPart) {
        case 0: glColor3f(0.0f, 1.0f, 0.0f); break; // Голова - зеленый
        case 1: glColor3f(0.0f, 0.7f, 0.0f); break; // Тело - темно-зеленый
        case 2: glColor3f(0.0f, 0.5f, 0.0f); break; // Хвост - еще темнее
        default: glColor3f(1.0f, 1.0f, 1.0f); break;
        }
        std::cout << "Drawing color set based on selected part" << std::endl;

        // Рисуем модель
        std::cout << "Calling drawModel()..." << std::endl;
        drawModel(currentModelData);
        std::cout << "✓ Model drawn successfully" << std::endl;
    }

    // Восстанавливаем матрицы
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    std::cout << "Matrices restored" << std::endl;

    glPopAttrib();
    std::cout << "Attributes restored" << std::endl;

    // Возвращаемся к 2D проекции для UI
    reset2DProjection();
    std::cout << "2D projection restored" << std::endl;

    // Рисуем рамку вокруг области предпросмотра
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(previewX, previewY);
    glVertex2f(previewX + previewW, previewY);
    glVertex2f(previewX + previewW, previewY + previewH);
    glVertex2f(previewX, previewY + previewH);
    glEnd();
    std::cout << "Preview frame drawn" << std::endl;

    // Текст "3D Preview"
    renderText("3D Preview", previewX + 10, previewY + 25, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));

    std::cout << "========== PREVIEW COMPLETE ==========\n" << std::endl;
}

// 3D предпросмотр для препятствий
void renderObstaclePreview() {
    int previewX = 650;
    int previewY = 120;
    int previewW = 500;
    int previewH = 400;

    if (selectedObstacle < 0 || selectedObstacle >= obstacles.size()) return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glViewport(previewX, windowHeight - previewY - previewH, previewW, previewH);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
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

    glm::vec3 eye(3.0f, 2.0f, 5.0f);
    glm::vec3 center(0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    glLoadMatrixf(glm::value_ptr(view));

    glRotatef(obstaclePreviewRotation, 0.0f, 1.0f, 0.0f);
    glScalef(obstacles[selectedObstacle].scale, obstacles[selectedObstacle].scale, obstacles[selectedObstacle].scale);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 2.0f, 3.0f, 2.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);

    GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    GLfloat matDiffuse[4] = {
        obstacles[selectedObstacle].color.r,
        obstacles[selectedObstacle].color.g,
        obstacles[selectedObstacle].color.b,
        1.0f
    };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);

    drawGrid();
    drawTexturedFloor();

    ModelData tempModel;
    loadFBXModel(obstacles[selectedObstacle].modelFile, tempModel, "obstacles");
    drawModel(tempModel);

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

    renderText("Obstacle Preview", previewX + 10, previewY + 25, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));
}

// Редактор змейки
void renderSnakeEditor() {
    reset2DProjection();

    renderText("SNAKE EDITOR", windowWidth / 2, 50, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int buttonWidth = 120;
    int startX = 100;
    int buttonY = 120;

    if (drawButton(startX, buttonY, buttonWidth, 40, "Head")) {
        selectedPart = 0;
        std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
        loadFBXModel(currentConfig.snakeHeadModel, currentModelData, folder);
    }
    if (drawButton(startX + 130, buttonY, buttonWidth, 40, "Body")) {
        selectedPart = 1;
        std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
        loadFBXModel(currentConfig.snakeBodyModel, currentModelData, folder);
    }
    if (drawButton(startX + 260, buttonY, buttonWidth, 40, "Tail")) {
        selectedPart = 2;
        std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
        loadFBXModel(currentConfig.snakeTailModel, currentModelData, folder);
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
        lastRotationTime = glfwGetTime();
        autoRotate = false;
        openFileDialog();
    }

    render3DPreview();

    int previewX = 650;
    int previewY = 120;
    int previewW = 500;
    int previewH = 400;

    int arrowY = previewY + previewH + 20;
    int arrowCenterX = previewX + previewW / 2;
    int arrowWidth = 60;
    int arrowHeight = 40;

    if (drawButton(arrowCenterX - arrowWidth - 10, arrowY, arrowWidth, arrowHeight, "←")) {
        previewRotation -= 15.0f;
        if (previewRotation < 0) previewRotation += 360;
        lastRotationTime = glfwGetTime();
        autoRotate = false;
    }

    if (drawButton(arrowCenterX + 10, arrowY, arrowWidth, arrowHeight, "→")) {
        previewRotation += 15.0f;
        if (previewRotation >= 360) previewRotation -= 360;
        lastRotationTime = glfwGetTime();
        autoRotate = false;
    }

    char angleText[50];
    sprintf_s(angleText, "Angle: %.0f°", previewRotation);
    renderText(angleText, arrowCenterX, arrowY + arrowHeight + 15, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    float currentTime = glfwGetTime();
    float timeSinceLastInput = currentTime - lastRotationTime;

    if (autoRotate) {
        renderText("Auto-rotation active", arrowCenterX, arrowY + arrowHeight + 40, 0.25f, glm::vec3(0.0f, 1.0f, 0.0f), true, false);
    }
    else {
        if (timeSinceLastInput < autoRotateDelay) {
            float remainingTime = autoRotateDelay - timeSinceLastInput;
            char timeText[50];
            sprintf_s(timeText, "Auto in: %.0f sec", remainingTime);
            renderText(timeText, arrowCenterX, arrowY + arrowHeight + 40, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);
        }
    }

    if (drawButton(50, 550, 100, 40, "Back")) {
        currentMode = MODE_MAIN;
    }

    if (drawButton(170, 550, 100, 40, "Save")) {
        lastRotationTime = glfwGetTime();
        autoRotate = false;
        saveConfig();
    }

    if (!autoRotate) {
        if (timeSinceLastInput >= autoRotateDelay) {
            autoRotate = true;
        }
    }

    if (autoRotate) {
        previewRotation += rotationSpeed;
        if (previewRotation >= 360) previewRotation -= 360;
    }
}

// Редактор препятствий
void renderObstacleEditor() {
    reset2DProjection();

    renderText("OBSTACLE EDITOR", windowWidth / 2, 50, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int listX = 50;
    int listY = 120;
    int listWidth = 200;
    int itemHeight = 30;

    renderText("Obstacles:", listX, listY - 20, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));

    for (int i = 0; i < obstacles.size(); i++) {
        std::string buttonText = obstacles[i].name;
        if (obstacles[i].enabled) {
            buttonText = "✓ " + buttonText;
        }

        if (drawButton(listX, listY + i * (itemHeight + 5), listWidth, itemHeight, buttonText)) {
            selectedObstacle = i;
            obstacleLastRotationTime = glfwGetTime();
            obstacleAutoRotate = false;
        }
    }

    if (drawButton(listX, listY + obstacles.size() * (itemHeight + 5) + 10, listWidth, itemHeight, "Add Obstacle")) {
        ObstacleItem newObstacle;
        newObstacle.name = "New Obstacle " + std::to_string(obstacles.size() + 1);
        newObstacle.modelFile = "tree.fbx";
        newObstacle.color = glm::vec3(0.1f, 0.4f, 0.1f);
        newObstacle.scale = 1.0f;
        newObstacle.posX = 30;
        newObstacle.posZ = 30;
        newObstacle.enabled = true;
        obstacles.push_back(newObstacle);
        selectedObstacle = obstacles.size() - 1;
    }

    if (selectedObstacle >= 0 && selectedObstacle < obstacles.size()) {
        int editX = 300;
        int editY = 120;
        int editWidth = 300;

        renderText("Properties:", editX, editY - 20, 0.25f, glm::vec3(1.0f, 1.0f, 0.0f));

        drawInfoBox(editX, editY, editWidth, 60, "Name", obstacles[selectedObstacle].name);
        drawInfoBox(editX, editY + 70, editWidth, 60, "Model", obstacles[selectedObstacle].modelFile);

        if (drawButton(editX + editWidth + 10, editY + 80, 80, 40, "Browse")) {
            openObstacleFileDialog();
        }

        renderText("Color:", editX, editY + 150, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
        drawSlider(editX, editY + 170, 150, &obstacles[selectedObstacle].color.r, 0.0f, 1.0f, "R");
        drawSlider(editX, editY + 200, 150, &obstacles[selectedObstacle].color.g, 0.0f, 1.0f, "G");
        drawSlider(editX, editY + 230, 150, &obstacles[selectedObstacle].color.b, 0.0f, 1.0f, "B");
        drawSlider(editX, editY + 270, 200, &obstacles[selectedObstacle].scale, 0.5f, 3.0f, "Scale");

        char posText[50];
        sprintf_s(posText, "Pos X: %d, Z: %d", obstacles[selectedObstacle].posX, obstacles[selectedObstacle].posZ);
        renderText(posText, editX, editY + 310, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));

        if (drawButton(editX, editY + 340, 100, 30, obstacles[selectedObstacle].enabled ? "Disable" : "Enable")) {
            obstacles[selectedObstacle].enabled = !obstacles[selectedObstacle].enabled;
        }

        if (drawButton(editX + 150, editY + 340, 100, 30, "Delete")) {
            obstacles.erase(obstacles.begin() + selectedObstacle);
            selectedObstacle = -1;
        }

        renderObstaclePreview();

        int previewX = 650;
        int previewY = 120;
        int previewW = 500;
        int previewH = 400;

        int arrowY = previewY + previewH + 20;
        int arrowCenterX = previewX + previewW / 2;
        int arrowWidth = 60;
        int arrowHeight = 40;

        if (drawButton(arrowCenterX - arrowWidth - 10, arrowY, arrowWidth, arrowHeight, "←")) {
            obstaclePreviewRotation -= 15.0f;
            if (obstaclePreviewRotation < 0) obstaclePreviewRotation += 360;
            obstacleLastRotationTime = glfwGetTime();
            obstacleAutoRotate = false;
        }

        if (drawButton(arrowCenterX + 10, arrowY, arrowWidth, arrowHeight, "→")) {
            obstaclePreviewRotation += 15.0f;
            if (obstaclePreviewRotation >= 360) obstaclePreviewRotation -= 360;
            obstacleLastRotationTime = glfwGetTime();
            obstacleAutoRotate = false;
        }

        float currentTime = glfwGetTime();
        float timeSinceLastInput = currentTime - obstacleLastRotationTime;

        if (!obstacleAutoRotate) {
            if (timeSinceLastInput >= autoRotateDelay) {
                obstacleAutoRotate = true;
            }
        }

        if (obstacleAutoRotate) {
            obstaclePreviewRotation += rotationSpeed;
            if (obstaclePreviewRotation >= 360) obstaclePreviewRotation -= 360;
        }
    }

    if (drawButton(50, 550, 100, 40, "Back")) {
        currentMode = MODE_MAIN;
    }

    if (drawButton(170, 550, 100, 40, "Save")) {
        saveConfig();
    }
}

// Редактор окружения
void renderEnvironmentEditor() {
    reset2DProjection();

    renderText("ENVIRONMENT EDITOR", windowWidth / 2, 50, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int startX = 50;
    int startY = 120;

    // === ЦВЕТА ===
    renderText("COLORS", startX, startY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    renderText("Sky Color:", startX, startY + 40, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    drawSlider(startX, startY + 60, 200, &skyColor[0], 0.0f, 1.0f, "R");
    drawSlider(startX, startY + 90, 200, &skyColor[1], 0.0f, 1.0f, "G");
    drawSlider(startX, startY + 120, 200, &skyColor[2], 0.0f, 1.0f, "B");

    renderText("Floor Color:", startX, startY + 160, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    drawSlider(startX, startY + 180, 200, &floorColor[0], 0.0f, 1.0f, "R");
    drawSlider(startX, startY + 210, 200, &floorColor[1], 0.0f, 1.0f, "G");
    drawSlider(startX, startY + 240, 200, &floorColor[2], 0.0f, 1.0f, "B");

    renderText("Grid Color:", startX, startY + 280, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    drawSlider(startX, startY + 300, 200, &gridColor[0], 0.0f, 1.0f, "R");
    drawSlider(startX, startY + 330, 200, &gridColor[1], 0.0f, 1.0f, "G");
    drawSlider(startX, startY + 360, 200, &gridColor[2], 0.0f, 1.0f, "B");

    // === МОДЕЛИ ОКРУЖЕНИЯ ===
    int modelX = 350;
    renderText("ENVIRONMENT MODELS", modelX, startY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    int modelY = startY + 40;

    // Яблоко
    renderText("Apple:", modelX, modelY, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    renderText(currentConfig.appleModel, modelX + 100, modelY, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    if (drawButton(modelX + 250, modelY - 10, 80, 25, "Browse")) {
        openModelFileDialog("food", currentConfig.appleModel);
    }
    modelY += 35;

    // Дерево
    renderText("Tree:", modelX, modelY, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    renderText(currentConfig.treeModel, modelX + 100, modelY, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    if (drawButton(modelX + 250, modelY - 10, 80, 25, "Browse")) {
        openModelFileDialog("obstacles", currentConfig.treeModel);
    }
    modelY += 35;

    // Облако
    renderText("Cloud:", modelX, modelY, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    renderText(currentConfig.cloudModel, modelX + 100, modelY, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    if (drawButton(modelX + 250, modelY - 10, 80, 25, "Browse")) {
        openModelFileDialog("clouds", currentConfig.cloudModel);
    }
    modelY += 35;

    // Птица
    renderText("Bird:", modelX, modelY, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    renderText(currentConfig.birdModel, modelX + 100, modelY, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    if (drawButton(modelX + 250, modelY - 10, 80, 25, "Browse")) {
        openModelFileDialog("birds", currentConfig.birdModel);
    }
    modelY += 35;

    // Цветок
    renderText("Flower:", modelX, modelY, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    renderText(currentConfig.flowerModel, modelX + 100, modelY, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    if (drawButton(modelX + 250, modelY - 10, 80, 25, "Browse")) {
        openModelFileDialog("flowers", currentConfig.flowerModel);
    }
    modelY += 35;

    // Пол
    renderText("Floor Model:", modelX, modelY, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    renderText(currentConfig.floorModel, modelX + 150, modelY, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    if (drawButton(modelX + 250, modelY - 10, 80, 25, "Browse")) {
        openModelFileDialog("floor", currentConfig.floorModel);
    }
    modelY += 35;

    // === ТЕКСТУРЫ ===
    int texX = 650;
    renderText("TEXTURES", texX, startY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    renderText("Floor Texture:", texX, startY + 40, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));

    std::string texName = floorTexture.path.empty() ? "None" : floorTexture.path;
    renderText(texName, texX + 150, startY + 40, 0.2f, glm::vec3(1.0f, 1.0f, 0.0f));

    if (drawButton(texX, startY + 60, 100, 30, "Load")) {
        std::string filename = openTextureFileDialog();
        if (!filename.empty()) {
            loadTexture(filename, floorTexture);
        }
    }

    if (drawButton(texX + 110, startY + 60, 100, 30, "Clear")) {
        if (floorTexture.id != 0) {
            glDeleteTextures(1, &floorTexture.id);
            floorTexture.id = 0;
            floorTexture.path = "";
        }
    }

    // === ПРЕДПРОСМОТР ===
    int previewX = 850;
    int previewY = 200;
    int previewSize = 150;

    glColor3f(skyColor[0], skyColor[1], skyColor[2]);
    glBegin(GL_QUADS);
    glVertex2f(previewX, previewY);
    glVertex2f(previewX + previewSize, previewY);
    glVertex2f(previewX + previewSize, previewY + previewSize / 2);
    glVertex2f(previewX, previewY + previewSize / 2);
    glEnd();
    renderText("Sky", previewX + 5, previewY + 5, 0.2f, glm::vec3(1.0f, 1.0f, 1.0f));

    glColor3f(floorColor[0], floorColor[1], floorColor[2]);
    glBegin(GL_QUADS);
    glVertex2f(previewX, previewY + previewSize / 2);
    glVertex2f(previewX + previewSize, previewY + previewSize / 2);
    glVertex2f(previewX + previewSize, previewY + previewSize);
    glVertex2f(previewX, previewY + previewSize);
    glEnd();
    renderText("Floor", previewX + 5, previewY + previewSize / 2 + 5, 0.2f, glm::vec3(1.0f, 1.0f, 1.0f));

    glColor3f(gridColor[0], gridColor[1], gridColor[2]);
    glBegin(GL_LINES);
    glVertex2f(previewX, previewY + previewSize / 2);
    glVertex2f(previewX + previewSize, previewY + previewSize / 2);
    glEnd();

    // === ОБЪЕКТЫ ===
    int objX = 850;
    int objY = 400;
    renderText("OBJECTS", objX, objY, 0.3f, glm::vec3(1.0f, 1.0f, 0.0f));

    char countText[50];
    sprintf_s(countText, "Clouds: %d", cloudCount);
    renderText(countText, objX, objY + 40, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    if (drawButton(objX + 100, objY + 30, 40, 30, "+")) cloudCount++;
    if (drawButton(objX + 150, objY + 30, 40, 30, "-") && cloudCount > 0) cloudCount--;

    sprintf_s(countText, "Birds: %d", birdCount);
    renderText(countText, objX, objY + 80, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    if (drawButton(objX + 100, objY + 70, 40, 30, "+")) birdCount++;
    if (drawButton(objX + 150, objY + 70, 40, 30, "-") && birdCount > 0) birdCount--;

    sprintf_s(countText, "Flowers: %d", flowerCount);
    renderText(countText, objX, objY + 120, 0.25f, glm::vec3(1.0f, 1.0f, 1.0f));
    if (drawButton(objX + 100, objY + 110, 40, 30, "+")) flowerCount++;
    if (drawButton(objX + 150, objY + 110, 40, 30, "-") && flowerCount > 0) flowerCount--;

    // Кнопки навигации
    if (drawButton(50, 550, 100, 40, "Back")) {
        currentMode = MODE_MAIN;
    }

    // Кнопка Save
    if (drawButton(170, 550, 100, 40, "Save")) {
        std::cout << "\n⚠️ SAVE BUTTON CLICKED in Environment Editor!" << std::endl;

        currentConfig.skyColor = glm::vec3(skyColor[0], skyColor[1], skyColor[2]);
        currentConfig.floorColor = glm::vec3(floorColor[0], floorColor[1], floorColor[2]);
        currentConfig.gridColor = glm::vec3(gridColor[0], gridColor[1], gridColor[2]);
        currentConfig.cloudCount = cloudCount;
        currentConfig.birdCount = birdCount;
        currentConfig.flowerCount = flowerCount;

        saveConfig();
    }
}
// Главное меню
void renderMainMenu() {
    reset2DProjection();

    renderText("SNAKE GAME CONFIG EDITOR", windowWidth / 2, 80, 0.6f, glm::vec3(1.0f, 1.0f, 0.0f), true, false);

    int buttonWidth = 250;
    int buttonHeight = 50;
    int startY = 200;
    int centerX = windowWidth / 2 - buttonWidth / 2;

    if (drawButton(centerX, startY, buttonWidth, buttonHeight, "1. Snake Editor")) {
        currentMode = MODE_SNAKE_EDITOR;
        std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
        loadFBXModel(currentConfig.snakeHeadModel, currentModelData, folder);
    }

    if (drawButton(centerX, startY + 70, buttonWidth, buttonHeight, "2. Obstacle Editor")) {
        currentMode = MODE_OBSTACLE_EDITOR;
    }

    if (drawButton(centerX, startY + 140, buttonWidth, buttonHeight, "3. Environment Editor")) {
        currentMode = MODE_ENVIRONMENT_EDITOR;
        skyColor[0] = currentConfig.skyColor.r;
        skyColor[1] = currentConfig.skyColor.g;
        skyColor[2] = currentConfig.skyColor.b;
        floorColor[0] = currentConfig.floorColor.r;
        floorColor[1] = currentConfig.floorColor.g;
        floorColor[2] = currentConfig.floorColor.b;
        gridColor[0] = currentConfig.gridColor.r;
        gridColor[1] = currentConfig.gridColor.g;
        gridColor[2] = currentConfig.gridColor.b;
        cloudCount = currentConfig.cloudCount;
        birdCount = currentConfig.birdCount;
        flowerCount = currentConfig.flowerCount;
    }

    if (drawButton(centerX, startY + 210, buttonWidth, buttonHeight, "4. Save and Exit")) {
        saveConfig();
        glfwSetWindowShouldClose(window, true);
    }

    std::string gridInfo = "Grid: " + std::to_string(currentConfig.gridWidth) + "x" +
        std::to_string(currentConfig.gridDepth);
    renderText(gridInfo, windowWidth / 2, 600, 0.3f, glm::vec3(0.8f, 0.8f, 1.0f), true, false);
}

// Колбэки
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mousePressedLast = mousePressed;
        mousePressed = (action == GLFW_PRESS);

        if (currentMode == MODE_SNAKE_EDITOR && action == GLFW_PRESS) {
            lastRotationTime = glfwGetTime();
            autoRotate = false;
        }
        if (currentMode == MODE_OBSTACLE_EDITOR && action == GLFW_PRESS) {
            obstacleLastRotationTime = glfwGetTime();
            obstacleAutoRotate = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_LEFT:
            if (currentMode == MODE_SNAKE_EDITOR) {
                previewRotation -= 15.0f;
                if (previewRotation < 0) previewRotation += 360;
                lastRotationTime = glfwGetTime();
                autoRotate = false;
            }
            else if (currentMode == MODE_OBSTACLE_EDITOR) {
                obstaclePreviewRotation -= 15.0f;
                if (obstaclePreviewRotation < 0) obstaclePreviewRotation += 360;
                obstacleLastRotationTime = glfwGetTime();
                obstacleAutoRotate = false;
            }
            break;
        case GLFW_KEY_RIGHT:
            if (currentMode == MODE_SNAKE_EDITOR) {
                previewRotation += 15.0f;
                if (previewRotation >= 360) previewRotation -= 360;
                lastRotationTime = glfwGetTime();
                autoRotate = false;
            }
            else if (currentMode == MODE_OBSTACLE_EDITOR) {
                obstaclePreviewRotation += 15.0f;
                if (obstaclePreviewRotation >= 360) obstaclePreviewRotation -= 360;
                obstacleLastRotationTime = glfwGetTime();
                obstacleAutoRotate = false;
            }
            break;
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
    glfwSetKeyCallback(window, keyCallback);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initFreeType();
    initPaths();
    initObstacles();
    loadAvailableTextures();

    return true;
}

int main() {
    if (!initOpenGL()) {
        return -1;
    }

    initPaths();  // Это уже должно быть

    // ПОДРОБНАЯ ОТЛАДКА ЗАГРУЗКИ КОНФИГА
    std::cout << "\n========== CONFIG LOAD DEBUG ==========" << std::endl;
    std::cout << "Config path from initPaths: " << g_configPath << std::endl;

    // Проверяем существование файла
    std::ifstream testFile(g_configPath);
    if (testFile.is_open()) {
        std::cout << "✓ Config file EXISTS at: " << g_configPath << std::endl;

        // Показываем первые несколько строк
        std::cout << "\nFirst 10 lines of config file:" << std::endl;
        std::string line;
        int lineCount = 0;
        while (std::getline(testFile, line) && lineCount < 10) {
            std::cout << "  " << line << std::endl;
            lineCount++;
        }
        testFile.close();
    }
    else {
        std::cout << "✗ Config file DOES NOT EXIST at: " << g_configPath << std::endl;

        // Проверяем существует ли папка
        std::string configFolder = g_assetsPath + "config\\";
        DWORD attrib = GetFileAttributesA(configFolder.c_str());
        std::cout << "Config folder exists: " << ((attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) ? "✓ YES" : "✗ NO") << std::endl;

        // Показываем содержимое папки assets
        std::cout << "\nContents of assets folder:" << std::endl;
        std::string searchPath = g_assetsPath + "*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                    std::cout << "  - " << findData.cFileName;
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        std::cout << " (folder)";
                    }
                    std::cout << std::endl;
                }
            } while (FindNextFileA(hFind, &findData) != 0);
            FindClose(hFind);
        }

        // Показываем содержимое папки config если есть
        if (attrib != INVALID_FILE_ATTRIBUTES) {
            std::cout << "\nContents of config folder:" << std::endl;
            searchPath = configFolder + "*";
            hFind = FindFirstFileA(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                        std::cout << "  - " << findData.cFileName << std::endl;
                    }
                } while (FindNextFileA(hFind, &findData) != 0);
                FindClose(hFind);
            }
        }
    }

    // Пытаемся загрузить конфиг
    std::cout << "\nAttempting to load config with ConfigManager..." << std::endl;
    if (ConfigManager::loadGameConfig(g_configPath, currentConfig)) {
        std::cout << "✓ CONFIG LOADED SUCCESSFULLY!" << std::endl;
        std::cout << "Loaded values:" << std::endl;
        std::cout << "  Snake head: " << currentConfig.snakeHeadModel << std::endl;
        std::cout << "  Snake body: " << currentConfig.snakeBodyModel << std::endl;
        std::cout << "  Snake tail: " << currentConfig.snakeTailModel << std::endl;
        std::cout << "  Sky color: (" << currentConfig.skyColor.r << ", " << currentConfig.skyColor.g << ", " << currentConfig.skyColor.b << ")" << std::endl;
        std::cout << "  Floor color: (" << currentConfig.floorColor.r << ", " << currentConfig.floorColor.g << ", " << currentConfig.floorColor.b << ")" << std::endl;
        std::cout << "  Grid color: (" << currentConfig.gridColor.r << ", " << currentConfig.gridColor.g << ", " << currentConfig.gridColor.b << ")" << std::endl;
        std::cout << "  Cloud count: " << currentConfig.cloudCount << std::endl;
        std::cout << "  Bird count: " << currentConfig.birdCount << std::endl;
        std::cout << "  Flower count: " << currentConfig.flowerCount << std::endl;
    }
    else {
        std::cout << "✗ FAILED TO LOAD CONFIG!" << std::endl;
        std::cout << "Creating new config at: " << g_configPath << std::endl;
    }
    std::cout << "=====================================\n" << std::endl;

    std::string folder = (selectedPart == 0) ? "snake_head" : (selectedPart == 1) ? "snake_body" : "snake_tail";
    loadFBXModel(currentConfig.snakeHeadModel, currentModelData, folder);


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        switch (currentMode) {
        case MODE_MAIN: renderMainMenu(); break;
        case MODE_SNAKE_EDITOR: renderSnakeEditor(); break;
        case MODE_OBSTACLE_EDITOR: renderObstacleEditor(); break;
        case MODE_ENVIRONMENT_EDITOR: renderEnvironmentEditor(); break;
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
// Вспомогательная функция для открытия диалога выбора модели
void openModelFileDialog(const std::string& subFolder, std::string& destVar) {
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
    ofn.lpstrTitle = ("Choose " + subFolder + " Model").c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        std::string fullPath = fileName;
        std::string filename = fullPath.substr(fullPath.find_last_of("\\") + 1);
        destVar = filename;

        // Копируем в папку assets если нужно
        std::string destPath = folderPath + filename;
        if (fullPath != destPath) {
            CopyFileA(fullPath.c_str(), destPath.c_str(), FALSE);
        }
    }
}