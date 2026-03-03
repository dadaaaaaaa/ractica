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

// Структура для данных модели
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

    // Настройки окружения
    int cloudCount = 20;
    int birdCount = 15;
    int flowerCount = 25;

    // Цвета
    glm::vec3 skyColor = glm::vec3(0.53f, 0.81f, 0.92f);
    glm::vec3 floorColor = glm::vec3(0.3f, 0.6f, 0.2f);
    glm::vec3 gridColor = glm::vec3(0.2f, 0.5f, 0.15f);

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

    // Дополнительные модели (опционально)
    std::string fenceModel = "fence.fbx";
    std::string rockModel = "rock.fbx";
    std::string grassModel = "grass.fbx";
};