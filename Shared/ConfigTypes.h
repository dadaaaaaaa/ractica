#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Структура для материала
struct Material {
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string texturePath;
    unsigned int textureID;

    Material() : diffuse(1.0f), specular(1.0f), shininess(32.0f), textureID(0) {}
};

// Структура для данных модели (имя ModelData)
struct ModelData {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<Material> materials;
    std::vector<int> materialIndices;
    bool loaded;

    ModelData() : loaded(false) {}
};

// Структура для препятствий
struct ObstacleData {
    std::string name;
    std::string modelFile;
    float sizeX, sizeY, sizeZ;
    glm::vec3 color;
    float spawnChance;
    float minDistanceFromStart;

    ObstacleData() : sizeX(1.0f), sizeY(1.0f), sizeZ(1.0f),
        color(1.0f, 1.0f, 1.0f), spawnChance(1.0f), minDistanceFromStart(0.0f) {
    }
};

// Конфигурация игры
struct GameConfig {
    // Настройки сетки
    int gridWidth = 120;
    int gridDepth = 120;
    float cellSize = 0.1f;
    int initialFoodCount = 10;
    int obstacleCount = 10;
    bool gridEnabled = true;
    float gridLineWidth = 1.0f;

    // Параметры пола
    int floorPolygonsX = 8;
    int floorPolygonsZ = 8;
    bool floorUseSubdivision = false;
    int floorSubdivisionLevel = 10;

    // Настройки окружения
    int cloudCount = 20;
    int birdCount = 15;
    int flowerCount = 25;
    int floorTileSize = 2;

    // Цвета
    glm::vec3 skyColor = glm::vec3(0.53f, 0.81f, 0.92f);
    glm::vec3 floorColor = glm::vec3(0.3f, 0.6f, 0.2f);
    glm::vec3 gridColor = glm::vec3(0.2f, 0.5f, 0.15f);

    // Настройки света
    int lightType = 1;
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 lightDir = glm::vec3(-1.0f, -1.0f, 0.5f);
    glm::vec3 lightPos = glm::vec3(0.0f, 5.0f, 0.0f);

    // Настройки теней
    int shadowTraceMode = 3;
    int shadowSubdivisionSize = 10;
    int shadowStrideX = 2;
    int shadowStrideZ = 2;
    bool shadowMapEnabled = true;
    bool ambientEnabled = true;
    bool specularEnabled = true;

    // Модели змейки
    std::string snakeHeadModel = "snake_head.fbx";
    std::string snakeBodyModel = "snake_body.fbx";
    std::string snakeTailModel = "snake_tail.fbx";

    glm::vec3 snakeHeadColor = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 snakeBodyColor = glm::vec3(0.0f, 0.7f, 0.0f);
    glm::vec3 snakeTailColor = glm::vec3(0.0f, 0.5f, 0.0f);

    float snakeHeadScale = 0.8f;
    float snakeBodyScale = 0.8f;
    float snakeTailScale = 0.8f;

    // Модели окружения
    std::string appleModel = "apple.fbx";
    std::string treeModel = "tree.fbx";
    std::string cloudModel = "cloud.fbx";
    std::string birdModel = "bird.fbx";
    std::string flowerModel = "flower.fbx";
    std::string floorModel = "floor.fbx";
    std::string floorTexture = "";

    // Дополнительные модели
    std::string fenceModel = "fence.fbx";
    std::string rockModel = "rock.fbx";
    std::string grassModel = "grass.fbx";

    int getShadowSubdivisionSize() const {
        return (shadowTraceMode == 2 || shadowTraceMode == 3) ? shadowSubdivisionSize : 1;
    }

    bool useCornerTrace() const {
        return (shadowTraceMode == 1 || shadowTraceMode == 3);
    }

    const char* getShadowModeName() const {
        switch (shadowTraceMode) {
        case 0: return "CENTER (1 ray per cell, binary)";
        case 1: return "CORNERS (4 rays per cell + gradient)";
        case 2: return "CENTER_SUBDIVIDED (adaptive, binary per subcell)";
        case 3: return "CORNERS_SUBDIVIDED (adaptive, gradient per subcell)";
        default: return "UNKNOWN";
        }
    }
};