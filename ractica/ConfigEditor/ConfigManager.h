#pragma once

#include <string>
#include <vector>
#include "../Shared/ConfigTypes.h"

class ConfigManager {
public:
    // Основные методы для конфигурации игры
    static bool loadGameConfig(const std::string& filename, GameConfig& config);
    static bool saveGameConfig(const std::string& filename, const GameConfig& config);

    // Методы для работы с препятствиями
    static bool loadObstacles(const std::string& filename, std::vector<ObstacleData>& obstacles);
    static bool saveObstacles(const std::string& filename, const std::vector<ObstacleData>& obstacles);


};