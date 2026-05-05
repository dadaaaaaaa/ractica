#include "ConfigManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Вспомогательные функции
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static glm::vec3 parseVec3(const std::string& str) {
    std::istringstream iss(str);
    float x, y, z;
    iss >> x >> y >> z;
    return glm::vec3(x, y, z);
}

static bool parseBool(const std::string& str) {
    std::string lower = str;
    for (auto& c : lower) c = tolower(c);
    return (lower == "true" || lower == "1" || lower == "yes");
}

static int parseInt(const std::string& str) {
    try {
        return std::stoi(str);
    }
    catch (...) {
        return 0;
    }
}

static float parseFloat(const std::string& str) {
    try {
        return std::stof(str);
    }
    catch (...) {
        return 0.0f;
    }
}

bool ConfigManager::loadGameConfig(const std::string& filename, GameConfig& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << filename << std::endl;
        // Возвращаем true с дефолтными значениями
        return true;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) continue;

        std::string key = trim(line.substr(0, equalsPos));
        std::string value = trim(line.substr(equalsPos + 1));

        // Game Settings
        if (key == "GRID_WIDTH") config.gridWidth = parseInt(value);
        else if (key == "GRID_DEPTH") config.gridDepth = parseInt(value);
        else if (key == "CELL_SIZE") config.cellSize = parseFloat(value);
        else if (key == "INITIAL_FOOD_COUNT") config.initialFoodCount = parseInt(value);
        else if (key == "OBSTACLE_COUNT") config.obstacleCount = parseInt(value);

        // Grid settings
        else if (key == "GRID_ENABLED") config.gridEnabled = parseBool(value);
        else if (key == "GRID_LINE_WIDTH") config.gridLineWidth = parseFloat(value);

        // Floor polygon settings
        else if (key == "FLOOR_POLYGONS_X") config.floorPolygonsX = parseInt(value);
        else if (key == "FLOOR_POLYGONS_Z") config.floorPolygonsZ = parseInt(value);
        else if (key == "FLOOR_USE_SUBDIVISION") config.floorUseSubdivision = parseBool(value);
        else if (key == "FLOOR_SUBDIVISION_LEVEL") config.floorSubdivisionLevel = parseInt(value);

        // Environment counts
        else if (key == "CLOUD_COUNT") config.cloudCount = parseInt(value);
        else if (key == "BIRD_COUNT") config.birdCount = parseInt(value);
        else if (key == "FLOWER_COUNT") config.flowerCount = parseInt(value);

        // Colors
        else if (key == "SKY_COLOR") config.skyColor = parseVec3(value);
        else if (key == "FLOOR_COLOR") config.floorColor = parseVec3(value);
        else if (key == "GRID_COLOR") config.gridColor = parseVec3(value);

        // Light settings
        else if (key == "LIGHT_TYPE") config.lightType = parseInt(value);
        else if (key == "LIGHT_COLOR") config.lightColor = parseVec3(value);
        else if (key == "LIGHT_DIR") config.lightDir = parseVec3(value);
        else if (key == "LIGHT_POS") config.lightPos = parseVec3(value);

        // Shadow settings
        else if (key == "SHADOW_TRACE_MODE") config.shadowTraceMode = parseInt(value);
        else if (key == "SHADOW_SUBDIVISION_SIZE") config.shadowSubdivisionSize = parseInt(value);
        else if (key == "SHADOW_STRIDE_X") config.shadowStrideX = parseInt(value);
        else if (key == "SHADOW_STRIDE_Z") config.shadowStrideZ = parseInt(value);
        else if (key == "SHADOW_MAP_ENABLED") config.shadowMapEnabled = parseBool(value);
        else if (key == "AMBIENT_ENABLED") config.ambientEnabled = parseBool(value);
        else if (key == "SPECULAR_ENABLED") config.specularEnabled = parseBool(value);

        // Snake Models and Colors
        else if (key == "SNAKE_HEAD_MODEL") config.snakeHeadModel = value;
        else if (key == "SNAKE_HEAD_COLOR") config.snakeHeadColor = parseVec3(value);
        else if (key == "SNAKE_HEAD_SCALE") config.snakeHeadScale = parseFloat(value);

        else if (key == "SNAKE_BODY_MODEL") config.snakeBodyModel = value;
        else if (key == "SNAKE_BODY_COLOR") config.snakeBodyColor = parseVec3(value);
        else if (key == "SNAKE_BODY_SCALE") config.snakeBodyScale = parseFloat(value);

        else if (key == "SNAKE_TAIL_MODEL") config.snakeTailModel = value;
        else if (key == "SNAKE_TAIL_COLOR") config.snakeTailColor = parseVec3(value);
        else if (key == "SNAKE_TAIL_SCALE") config.snakeTailScale = parseFloat(value);

        // Environment Models
        else if (key == "APPLE_MODEL") config.appleModel = value;
        else if (key == "TREE_MODEL") config.treeModel = value;
        else if (key == "CLOUD_MODEL") config.cloudModel = value;
        else if (key == "BIRD_MODEL") config.birdModel = value;
        else if (key == "FLOWER_MODEL") config.flowerModel = value;
        else if (key == "FLOOR_MODEL") config.floorModel = value;
        else if (key == "FLOOR_TEXTURE") config.floorTexture = value;

        // Additional Models
        else if (key == "FENCE_MODEL") config.fenceModel = value;
        else if (key == "ROCK_MODEL") config.rockModel = value;
        else if (key == "GRASS_MODEL") config.grassModel = value;
    }

    file.close();

    // Валидация значений
    if (config.shadowSubdivisionSize < 2) config.shadowSubdivisionSize = 2;
    if (config.shadowSubdivisionSize > 20) config.shadowSubdivisionSize = 20;
    if (config.floorPolygonsX < 1) config.floorPolygonsX = 1;
    if (config.floorPolygonsZ < 1) config.floorPolygonsZ = 1;

    return true;
}

bool ConfigManager::saveGameConfig(const std::string& filename, const GameConfig& config) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot save config file: " << filename << std::endl;
        return false;
    }

    file << "# Game Configuration File\n";
    file << "# Auto-generated by ConfigEditor\n\n";

    file << "# Game Settings\n";
    file << "GRID_WIDTH = " << config.gridWidth << "\n";
    file << "GRID_DEPTH = " << config.gridDepth << "\n";
    file << "CELL_SIZE = " << config.cellSize << "\n";
    file << "INITIAL_FOOD_COUNT = " << config.initialFoodCount << "\n";
    file << "OBSTACLE_COUNT = " << config.obstacleCount << "\n\n";

    file << "# Grid Settings\n";
    file << "GRID_ENABLED = " << (config.gridEnabled ? "true" : "false") << "\n";
    file << "GRID_LINE_WIDTH = " << config.gridLineWidth << "\n\n";

    file << "# Floor Polygon Settings (for accurate shadows)\n";
    file << "FLOOR_POLYGONS_X = " << config.floorPolygonsX << "\n";
    file << "FLOOR_POLYGONS_Z = " << config.floorPolygonsZ << "\n";
    file << "FLOOR_USE_SUBDIVISION = " << (config.floorUseSubdivision ? "true" : "false") << "\n";
    file << "FLOOR_SUBDIVISION_LEVEL = " << config.floorSubdivisionLevel << "\n\n";

    file << "# Environment Counts\n";
    file << "CLOUD_COUNT = " << config.cloudCount << "\n";
    file << "BIRD_COUNT = " << config.birdCount << "\n";
    file << "FLOWER_COUNT = " << config.flowerCount << "\n\n";

    file << "# Colors\n";
    file << "SKY_COLOR = " << config.skyColor.r << " " << config.skyColor.g << " " << config.skyColor.b << "\n";
    file << "FLOOR_COLOR = " << config.floorColor.r << " " << config.floorColor.g << " " << config.floorColor.b << "\n";
    file << "GRID_COLOR = " << config.gridColor.r << " " << config.gridColor.g << " " << config.gridColor.b << "\n\n";

    file << "# Light Settings\n";
    file << "LIGHT_TYPE = " << config.lightType << "  # 0=Directional, 1=Point, 2=Spot\n";
    file << "LIGHT_COLOR = " << config.lightColor.r << " " << config.lightColor.g << " " << config.lightColor.b << "\n";
    file << "LIGHT_DIR = " << config.lightDir.x << " " << config.lightDir.y << " " << config.lightDir.z << "\n";
    file << "LIGHT_POS = " << config.lightPos.x << " " << config.lightPos.y << " " << config.lightPos.z << "\n\n";

    file << "# Shadow Settings\n";
    file << "SHADOW_TRACE_MODE = " << config.shadowTraceMode << "  # 0=CENTER, 1=CORNERS, 2=CENTER_SUBDIVIDED, 3=CORNERS_SUBDIVIDED\n";
    file << "SHADOW_SUBDIVISION_SIZE = " << config.shadowSubdivisionSize << "  # Size of subdivision grid (NxN)\n";
    file << "SHADOW_STRIDE_X = " << config.shadowStrideX << "\n";
    file << "SHADOW_STRIDE_Z = " << config.shadowStrideZ << "\n";
    file << "SHADOW_MAP_ENABLED = " << (config.shadowMapEnabled ? "true" : "false") << "\n";
    file << "AMBIENT_ENABLED = " << (config.ambientEnabled ? "true" : "false") << "\n";
    file << "SPECULAR_ENABLED = " << (config.specularEnabled ? "true" : "false") << "\n\n";

    file << "# Snake Appearance\n";
    file << "SNAKE_HEAD_MODEL = " << config.snakeHeadModel << "\n";
    file << "SNAKE_HEAD_COLOR = " << config.snakeHeadColor.r << " " << config.snakeHeadColor.g << " " << config.snakeHeadColor.b << "\n";
    file << "SNAKE_HEAD_SCALE = " << config.snakeHeadScale << "\n\n";

    file << "SNAKE_BODY_MODEL = " << config.snakeBodyModel << "\n";
    file << "SNAKE_BODY_COLOR = " << config.snakeBodyColor.r << " " << config.snakeBodyColor.g << " " << config.snakeBodyColor.b << "\n";
    file << "SNAKE_BODY_SCALE = " << config.snakeBodyScale << "\n\n";

    file << "SNAKE_TAIL_MODEL = " << config.snakeTailModel << "\n";
    file << "SNAKE_TAIL_COLOR = " << config.snakeTailColor.r << " " << config.snakeTailColor.g << " " << config.snakeTailColor.b << "\n";
    file << "SNAKE_TAIL_SCALE = " << config.snakeTailScale << "\n\n";

    file << "# Food and Decorations\n";
    file << "APPLE_MODEL = " << config.appleModel << "\n";
    file << "TREE_MODEL = " << config.treeModel << "\n";
    file << "CLOUD_MODEL = " << config.cloudModel << "\n";
    file << "BIRD_MODEL = " << config.birdModel << "\n";
    file << "FLOWER_MODEL = " << config.flowerModel << "\n";
    file << "FLOOR_MODEL = " << config.floorModel << "\n";
    file << "FLOOR_TEXTURE = " << config.floorTexture << "\n\n";

    file << "# Additional Models (optional)\n";
    file << "FENCE_MODEL = " << config.fenceModel << "\n";
    file << "ROCK_MODEL = " << config.rockModel << "\n";
    file << "GRASS_MODEL = " << config.grassModel << "\n";

    file.close();
    return true;
}

bool ConfigManager::loadObstacles(const std::string& filename, std::vector<ObstacleData>& obstacles) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open obstacles file: " << filename << std::endl;
        return false;
    }

    size_t count;
    file.read((char*)&count, sizeof(count));

    obstacles.clear();
    obstacles.reserve(count);

    for (size_t i = 0; i < count; i++) {
        ObstacleData obs;

        size_t len;
        file.read((char*)&len, sizeof(len));
        obs.name.resize(len);
        file.read(&obs.name[0], len);

        file.read((char*)&len, sizeof(len));
        obs.modelFile.resize(len);
        file.read(&obs.modelFile[0], len);

        file.read((char*)&obs.sizeX, sizeof(obs.sizeX));
        file.read((char*)&obs.sizeY, sizeof(obs.sizeY));
        file.read((char*)&obs.sizeZ, sizeof(obs.sizeZ));
        file.read((char*)&obs.color, sizeof(obs.color));
        file.read((char*)&obs.spawnChance, sizeof(obs.spawnChance));
        file.read((char*)&obs.minDistanceFromStart, sizeof(obs.minDistanceFromStart));

        obstacles.push_back(obs);
    }

    file.close();
    std::cout << "Loaded " << obstacles.size() << " obstacles from " << filename << std::endl;
    return true;
}

bool ConfigManager::saveObstacles(const std::string& filename, const std::vector<ObstacleData>& obstacles) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot save obstacles file: " << filename << std::endl;
        return false;
    }

    size_t count = obstacles.size();
    file.write((char*)&count, sizeof(count));

    for (const auto& obs : obstacles) {
        size_t len = obs.name.length();
        file.write((char*)&len, sizeof(len));
        file.write(obs.name.c_str(), len);

        len = obs.modelFile.length();
        file.write((char*)&len, sizeof(len));
        file.write(obs.modelFile.c_str(), len);

        file.write((char*)&obs.sizeX, sizeof(obs.sizeX));
        file.write((char*)&obs.sizeY, sizeof(obs.sizeY));
        file.write((char*)&obs.sizeZ, sizeof(obs.sizeZ));
        file.write((char*)&obs.color, sizeof(obs.color));
        file.write((char*)&obs.spawnChance, sizeof(obs.spawnChance));
        file.write((char*)&obs.minDistanceFromStart, sizeof(obs.minDistanceFromStart));
    }

    file.close();
    std::cout << "Saved " << obstacles.size() << " obstacles to " << filename << std::endl;
    return true;
}