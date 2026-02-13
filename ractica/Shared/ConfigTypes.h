#pragma once
#include <glm/glm.hpp>
#include <string>


// Структура для основных настроек игры
struct GameConfig {
    // Game Settings
    int gridWidth = 120;
    int gridDepth = 120;
    float cellSize = 0.1f;
    int initialFoodCount = 10;
    int obstacleCount = 10;
    int cloudCount = 20;
    int birdCount = 15;
    int flowerCount = 25;

    // Colors
    glm::vec3 skyColor = glm::vec3(0.53f, 0.81f, 0.92f);
    glm::vec3 floorColor = glm::vec3(0.3f, 0.6f, 0.2f);
    glm::vec3 gridColor = glm::vec3(0.2f, 0.5f, 0.15f);

    // Snake
    std::string snakeHeadModel = "models/snake_head.obj";
    glm::vec3 snakeHeadColor = glm::vec3(0.0f, 1.0f, 0.0f);
    float snakeHeadScale = 0.8f;

    std::string snakeBodyModel = "models/snake_body.obj";
    glm::vec3 snakeBodyColor = glm::vec3(0.0f, 0.7f, 0.0f);
    float snakeBodyScale = 0.8f;

    std::string snakeTailModel = "models/snake_tail.obj";
    glm::vec3 snakeTailColor = glm::vec3(0.0f, 0.5f, 0.0f);
    float snakeTailScale = 0.8f;
};

// Структура для препятствий
struct ObstacleData {
    std::string name;
    std::string modelFile;
    int sizeX = 3;
    int sizeY = 1;
    int sizeZ = 3;
    glm::vec3 color = glm::vec3(0.1f, 0.4f, 0.1f);
    float spawnChance = 1.0f;
    int minDistanceFromStart = 5;
};