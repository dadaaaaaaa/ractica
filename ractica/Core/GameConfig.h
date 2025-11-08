#pragma once
#include "Constants.h"
#include "Types.h"

// Конфигурация игры, которая может меняться во время выполнения и настраивается пользователем
struct GameConfig {
    // === НАСТРОЙКИ ГРАФИКИ ===
    int windowWidth = DEFAULT_WINDOW_WIDTH;
    int windowHeight = DEFAULT_WINDOW_HEIGHT;
    bool vsync = true;
    bool fullscreen = false;
    int fpsLimit = 60;
    float uiScale = 1.0f;
    bool enableShadows = true;
    bool enableBloom = false;
    int textureQuality = 2; // 0-низкая, 1-средняя, 2-высокая

    // === НАСТРОЙКИ ГЕЙМПЛЕЯ ===
    float gameSpeed = BASE_GAME_SPEED;
    float speedMultiplier = 1.0f;
    bool enableObstacles = true;
    bool enableDecorations = true;
    bool enableCollisions = true;
    int difficultyLevel = 1; // 1-легко, 2-нормально, 3-сложно
    bool autoSaveEnabled = true;
    int autoSaveInterval = 30; // секунды

    // === НАСТРОЙКИ УПРАВЛЕНИЯ ===
    float cameraSensitivity = 1.0f;
    float cameraSmoothness = CAMERA_SMOOTHNESS;
    bool invertMouseY = false;
    bool invertMouseX = false;
    float mouseSensitivity = 1.0f;
    float scrollSensitivity = 1.0f;

    // === НАСТРОЙКИ ЗВУКА ===
    bool enableSound = true;
    bool enableMusic = true;
    float soundVolume = 1.0f;
    float musicVolume = 0.7f;
    float ambientVolume = 0.5f;

    // === НАСТРОЙКИ СЕТИ ===
    std::string playerName = DEFAULT_PLAYER_NAME;
    bool enableOnlineFeatures = true;
    bool autoSubmitScores = true;
    std::string customServerUrl = DEFAULT_SERVER_URL;
    bool showNetworkStats = false;

    // === НАСТРОЙКИ ИНТЕРФЕЙСА ===
    bool showFPS = false;
    bool showDebugInfo = ENABLE_DEBUG_INFO;
    std::string language = "en";
    bool showTutorialHints = true;
    bool showScorePopup = true;
    bool showSpeedIndicator = true;
    int uiTheme = 0; // 0-темная, 1-светлая, 2-цветная

    // === СПЕЦИФИЧНЫЕ НАСТРОЙКИ ИГРЫ ===
    bool enableClouds = true;
    bool enableBirds = true;
    bool enableFlowers = true;
    int cloudDensity = CLOUD_COUNT;
    int birdDensity = BIRD_COUNT;
    int flowerDensity = FLOWER_COUNT;
    float fieldOfView = CAMERA_FOV;

    // === ИГРОВЫЕ ЛИМИТЫ (для валидации) ===
    static constexpr int MAX_SNAKE_LENGTH = 1000;
    static constexpr int MAX_HIGH_SCORES = 10;
    static constexpr float MIN_GAME_SPEED = 0.05f;
    static constexpr float MAX_GAME_SPEED = 0.3f;
    static constexpr float MIN_SPEED_MULTIPLIER = 0.1f;
    static constexpr float MAX_SPEED_MULTIPLIER = 5.0f;
    static constexpr int MIN_WINDOW_WIDTH = 800;
    static constexpr int MIN_WINDOW_HEIGHT = 600;
    static constexpr int MAX_WINDOW_WIDTH = 3840;
    static constexpr int MAX_WINDOW_HEIGHT = 2160;

    // === МЕТОДЫ ДЛЯ РАБОТЫ С КОНФИГОМ ===

    // Получить текущую скорость игры с учетом множителя
    float getCurrentGameSpeed() const {
        return gameSpeed * speedMultiplier;
    }

    // Получить множитель сложности
    float getDifficultyMultiplier() const {
        switch (difficultyLevel) {
        case 1: return 0.7f; // Легко
        case 2: return 1.0f; // Нормально
        case 3: return 1.5f; // Сложно
        default: return 1.0f;
        }
    }

    // Получить итоговую скорость с учетом сложности
    float getFinalGameSpeed() const {
        return getCurrentGameSpeed() * getDifficultyMultiplier();
    }

    // Получить количество облаков с учетом настроек плотности
    int getCloudCount() const {
        return enableClouds ? cloudDensity : 0;
    }

    // Получить количество птиц с учетом настроек плотности
    int getBirdCount() const {
        return enableBirds ? birdDensity : 0;
    }

    // Получить количество цветов с учетом настроек плотности
    int getFlowerCount() const {
        return enableFlowers ? flowerDensity : 0;
    }

    // Проверить валидность настроек
    bool isValid() const {
        return windowWidth >= MIN_WINDOW_WIDTH &&
            windowWidth <= MAX_WINDOW_WIDTH &&
            windowHeight >= MIN_WINDOW_HEIGHT &&
            windowHeight <= MAX_WINDOW_HEIGHT &&
            gameSpeed >= MIN_GAME_SPEED &&
            gameSpeed <= MAX_GAME_SPEED &&
            speedMultiplier >= MIN_SPEED_MULTIPLIER &&
            speedMultiplier <= MAX_SPEED_MULTIPLIER &&
            playerName.length() <= MAX_PLAYER_NAME_LENGTH &&
            playerName.length() >= 1 &&
            cloudDensity >= 0 && cloudDensity <= 100 &&
            birdDensity >= 0 && birdDensity <= 50 &&
            flowerDensity >= 0 && flowerDensity <= 200;
    }

    // Сброс к настройкам по умолчанию
    void resetToDefaults() {
        windowWidth = DEFAULT_WINDOW_WIDTH;
        windowHeight = DEFAULT_WINDOW_HEIGHT;
        vsync = true;
        fullscreen = false;
        fpsLimit = 60;
        uiScale = 1.0f;

        gameSpeed = BASE_GAME_SPEED;
        speedMultiplier = 1.0f;
        enableObstacles = true;
        enableDecorations = true;
        enableCollisions = true;
        difficultyLevel = 1;
        autoSaveEnabled = true;
        autoSaveInterval = 30;

        cameraSensitivity = 1.0f;
        cameraSmoothness = CAMERA_SMOOTHNESS;
        invertMouseY = false;
        invertMouseX = false;
        mouseSensitivity = 1.0f;
        scrollSensitivity = 1.0f;

        enableSound = true;
        enableMusic = true;
        soundVolume = 1.0f;
        musicVolume = 0.7f;
        ambientVolume = 0.5f;

        playerName = DEFAULT_PLAYER_NAME;
        enableOnlineFeatures = true;
        autoSubmitScores = true;
        customServerUrl = DEFAULT_SERVER_URL;
        showNetworkStats = false;

        showFPS = false;
        showDebugInfo = ENABLE_DEBUG_INFO;
        language = "en";
        showTutorialHints = true;
        showScorePopup = true;
        showSpeedIndicator = true;
        uiTheme = 0;

        enableClouds = true;
        enableBirds = true;
        enableFlowers = true;
        cloudDensity = CLOUD_COUNT;
        birdDensity = BIRD_COUNT;
        flowerDensity = FLOWER_COUNT;
        fieldOfView = CAMERA_FOV;
    }

    // Применить ограничения к значениям
    void clampValues() {
        windowWidth = std::clamp(windowWidth, MIN_WINDOW_WIDTH, MAX_WINDOW_WIDTH);
        windowHeight = std::clamp(windowHeight, MIN_WINDOW_HEIGHT, MAX_WINDOW_HEIGHT);
        gameSpeed = std::clamp(gameSpeed, MIN_GAME_SPEED, MAX_GAME_SPEED);
        speedMultiplier = std::clamp(speedMultiplier, MIN_SPEED_MULTIPLIER, MAX_SPEED_MULTIPLIER);
        difficultyLevel = std::clamp(difficultyLevel, 1, 3);
        cloudDensity = std::clamp(cloudDensity, 0, 100);
        birdDensity = std::clamp(birdDensity, 0, 50);
        flowerDensity = std::clamp(flowerDensity, 0, 200);

        // Обрезать имя игрока если слишком длинное
        if (playerName.length() > MAX_PLAYER_NAME_LENGTH) {
            playerName = playerName.substr(0, MAX_PLAYER_NAME_LENGTH);
        }
    }
};