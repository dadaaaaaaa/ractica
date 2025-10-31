#pragma once
#include "Constants.h"
#include "Types.h"

// Конфигурация игры, которая может меняться во время выполнения
struct GameConfig {
    // Настройки графики
    int windowWidth = 1200;
    int windowHeight = 800;
    bool vsync = true;

    // Настройки геймплея
    float gameSpeed = 0.12f;
    bool enableObstacles = true;
    bool enableDecorations = true;

    // Настройки управления
    float cameraSensitivity = 1.0f;
    float cameraSmoothness = 0.1f;

    // Настройки звука
    bool enableSound = true;
    float soundVolume = 1.0f;

    // Игровые ограничения
    static constexpr int MAX_SNAKE_LENGTH = 1000;
    static constexpr int MAX_HIGH_SCORES = 10;
    static constexpr float MIN_GAME_SPEED = 0.05f;
    static constexpr float MAX_GAME_SPEED = 0.3f;
};