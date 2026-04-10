#include "../pch.h"
#include "Game.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../ConfigEditor/ConfigManager.h"
#include "../Shared/ConfigTypes.h"
#include <windows.h>
#include <iostream>

// Внешние глобальные переменные

extern Camera g_camera;
extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;
extern GLFWwindow* g_mainWindow;

std::string g_assetsPath;
std::string g_modelsPath;
std::string g_texturesPath;
std::string g_configPath;

Game::Game() {
    // Инициализация делегируется компонентам
}

void Game::initPaths() {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::string exePath = std::string(currentDir);

    std::cout << "\n===== GAME PATH DEBUG =====" << std::endl;
    std::cout << "Current directory (exe): " << exePath << std::endl;

    // Находим корень проекта (поднимаемся до папки, где лежит assets)
    std::string rootPath = exePath;

    // Проверяем где находится exe
    size_t pos = rootPath.find("\\x64\\Debug");
    if (pos != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
        std::cout << "Found x64/Debug, root: " << rootPath << std::endl;
    }
    else if ((pos = rootPath.find("\\Debug")) != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
        std::cout << "Found Debug, root: " << rootPath << std::endl;
    }
    else if ((pos = rootPath.find("\\Release")) != std::string::npos) {
        rootPath = rootPath.substr(0, pos);
        std::cout << "Found Release, root: " << rootPath << std::endl;
    }

    // Убираем лишний сегмент если есть
    if (rootPath.find("\\ractica") != std::string::npos) {
        size_t lastSlash = rootPath.find_last_of("\\");
        if (lastSlash != std::string::npos) {
            rootPath = rootPath.substr(0, lastSlash);
            std::cout << "Removed extra \\ractica, new root: " << rootPath << std::endl;
        }
    }

    g_assetsPath = rootPath + "\\assets\\";
    g_modelsPath = g_assetsPath + "models\\";
    g_texturesPath = g_assetsPath + "textures\\";
    g_configPath = g_assetsPath + "config\\game.cfg";

    std::cout << "\n--- FINAL PATHS ---" << std::endl;
    std::cout << "Assets path: " << g_assetsPath << std::endl;
    std::cout << "Models path: " << g_modelsPath << std::endl;
    std::cout << "Textures path: " << g_texturesPath << std::endl;
    std::cout << "Config path: " << g_configPath << std::endl;

    // Проверяем существование
    DWORD attrib = GetFileAttributesA(g_assetsPath.c_str());
    std::cout << "\nAssets folder exists: " << ((attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) ? "✓ YES" : "✗ NO") << std::endl;

    attrib = GetFileAttributesA(g_configPath.c_str());
    std::cout << "Config file exists: " << ((attrib != INVALID_FILE_ATTRIBUTES) ? "✓ YES" : "✗ NO") << std::endl;

    std::cout << "===========================\n" << std::endl;
}

void Game::loadConfig() {
    std::cout << "\n========== ГРУЗИМ КОНФИГ ==========" << std::endl;
    std::cout << "Путь к конфигу: " << g_configPath << std::endl;

    GameConfig config;

    if (ConfigManager::loadGameConfig(g_configPath, config)) {
        std::cout << "✓ КОНФИГ ЗАГРУЖЕН УСПЕШНО!" << std::endl;

        // ВЫВОДИМ ВСЕ ЗНАЧЕНИЯ ИЗ КОНФИГА
        std::cout << "\n--- ЗНАЧЕНИЯ ИЗ КОНФИГА ---" << std::endl;

        // Цвета
        std::cout << "SKY_COLOR = " << config.skyColor.r << " " << config.skyColor.g << " " << config.skyColor.b << std::endl;
        std::cout << "FLOOR_COLOR = " << config.floorColor.r << " " << config.floorColor.g << " " << config.floorColor.b << std::endl;
        std::cout << "GRID_COLOR = " << config.gridColor.r << " " << config.gridColor.g << " " << config.gridColor.b << std::endl;

        // Настройки сетки
        std::cout << "GRID_WIDTH = " << config.gridWidth << std::endl;
        std::cout << "GRID_DEPTH = " << config.gridDepth << std::endl;
        std::cout << "CELL_SIZE = " << config.cellSize << std::endl;
        std::cout << "INITIAL_FOOD_COUNT = " << config.initialFoodCount << std::endl;
        std::cout << "OBSTACLE_COUNT = " << config.obstacleCount << std::endl;
        std::cout << "GRID_ENABLED = " << (config.gridEnabled ? "true" : "false") << std::endl;
        std::cout << "GRID_LINE_WIDTH = " << config.gridLineWidth << std::endl;

        // Модели змейки
        std::cout << "SNAKE_HEAD_MODEL = " << config.snakeHeadModel << std::endl;
        std::cout << "SNAKE_BODY_MODEL = " << config.snakeBodyModel << std::endl;
        std::cout << "SNAKE_TAIL_MODEL = " << config.snakeTailModel << std::endl;

        // Цвета змейки
        std::cout << "SNAKE_HEAD_COLOR = " << config.snakeHeadColor.r << " " << config.snakeHeadColor.g << " " << config.snakeHeadColor.b << std::endl;
        std::cout << "SNAKE_BODY_COLOR = " << config.snakeBodyColor.r << " " << config.snakeBodyColor.g << " " << config.snakeBodyColor.b << std::endl;
        std::cout << "SNAKE_TAIL_COLOR = " << config.snakeTailColor.r << " " << config.snakeTailColor.g << " " << config.snakeTailColor.b << std::endl;

        // Масштабы
        std::cout << "SNAKE_HEAD_SCALE = " << config.snakeHeadScale << std::endl;
        std::cout << "SNAKE_BODY_SCALE = " << config.snakeBodyScale << std::endl;
        std::cout << "SNAKE_TAIL_SCALE = " << config.snakeTailScale << std::endl;

        // Настройки окружения
        std::cout << "CLOUD_COUNT = " << config.cloudCount << std::endl;
        std::cout << "BIRD_COUNT = " << config.birdCount << std::endl;
        std::cout << "FLOWER_COUNT = " << config.flowerCount << std::endl;

        // Модели окружения
        std::cout << "APPLE_MODEL = " << config.appleModel << std::endl;
        std::cout << "TREE_MODEL = " << config.treeModel << std::endl;
        std::cout << "CLOUD_MODEL = " << config.cloudModel << std::endl;
        std::cout << "BIRD_MODEL = " << config.birdModel << std::endl;
        std::cout << "FLOWER_MODEL = " << config.flowerModel << std::endl;
        std::cout << "FENCE_MODEL = " << config.fenceModel << std::endl;
        std::cout << "ROCK_MODEL = " << config.rockModel << std::endl;
        std::cout << "GRASS_MODEL = " << config.grassModel << std::endl;
        std::cout << "FLOOR_MODEL = " << config.floorModel << std::endl;
        std::cout << "FLOOR_TEXTURE = " << config.floorTexture << std::endl;

        std::cout << "----------------------------\n" << std::endl;

        // ПРИМЕНЯЕМ К GameObjects
        std::cout << "--- ПРИМЕНЯЕМ К GameObjects ---" << std::endl;

        // Настройки сетки
        objects.setGridWidth(config.gridWidth);
        objects.setGridDepth(config.gridDepth);
        objects.setCellSize(config.cellSize);
        objects.setInitialFoodCount(config.initialFoodCount);
        objects.setObstacleCount(config.obstacleCount);
        objects.setGridEnabled(config.gridEnabled);
        objects.setGridLineWidth(config.gridLineWidth);
        objects.setFloorTileSize(config.floorTileSize);
        // Модели змейки
        objects.setSnakeModels(
            config.snakeHeadModel,
            config.snakeBodyModel,
            config.snakeTailModel
        );
        std::cout << "  setSnakeModels: Head=" << config.snakeHeadModel
            << " Body=" << config.snakeBodyModel
            << " Tail=" << config.snakeTailModel << std::endl;

        // Цвета змейки
        objects.setSnakeColors(
            config.snakeHeadColor,
            config.snakeBodyColor,
            config.snakeTailColor
        );
        std::cout << "  setSnakeColors: Head=("
            << config.snakeHeadColor.r << "," << config.snakeHeadColor.g << "," << config.snakeHeadColor.b << ") "
            << "Body=(" << config.snakeBodyColor.r << "," << config.snakeBodyColor.g << "," << config.snakeBodyColor.b << ") "
            << "Tail=(" << config.snakeTailColor.r << "," << config.snakeTailColor.g << "," << config.snakeTailColor.b << ")" << std::endl;

        // Масштабы змейки
        objects.setSnakeScales(
            config.snakeHeadScale,
            config.snakeBodyScale,
            config.snakeTailScale
        );
        std::cout << "  setSnakeScales: Head=" << config.snakeHeadScale
            << " Body=" << config.snakeBodyScale
            << " Tail=" << config.snakeTailScale << std::endl;

        // Модели окружения
        objects.setAppleModel(config.appleModel);
        std::cout << "  setAppleModel: " << config.appleModel << std::endl;

        objects.setTreeModel(config.treeModel);
        std::cout << "  setTreeModel: " << config.treeModel << std::endl;

        objects.setCloudModel(config.cloudModel);
        std::cout << "  setCloudModel: " << config.cloudModel << std::endl;

        objects.setBirdModel(config.birdModel);
        std::cout << "  setBirdModel: " << config.birdModel << std::endl;

        objects.setFlowerModel(config.flowerModel);
        std::cout << "  setFlowerModel: " << config.flowerModel << std::endl;

        objects.setFenceModel(config.fenceModel);
        std::cout << "  setFenceModel: " << config.fenceModel << std::endl;

        objects.setRockModel(config.rockModel);
        std::cout << "  setRockModel: " << config.rockModel << std::endl;

        objects.setGrassModel(config.grassModel);
        std::cout << "  setGrassModel: " << config.grassModel << std::endl;

        // Настройки окружения (количество)
        objects.setCloudCount(config.cloudCount);
        std::cout << "  setCloudCount: " << config.cloudCount << std::endl;

        objects.setBirdCount(config.birdCount);
        std::cout << "  setBirdCount: " << config.birdCount << std::endl;

        objects.setFlowerCount(config.flowerCount);
        std::cout << "  setFlowerCount: " << config.flowerCount << std::endl;

        // Настройки пола
        objects.setFloorModel(config.floorModel);
        std::cout << "  setFloorModel: " << config.floorModel << std::endl;

        objects.setFloorColor(config.floorColor);
        std::cout << "  setFloorColor: (" << config.floorColor.r << "," << config.floorColor.g << "," << config.floorColor.b << ")" << std::endl;

        objects.setFloorTexture(config.floorTexture);
        std::cout << "  setFloorTexture: " << (config.floorTexture.empty() ? "none" : config.floorTexture) << std::endl;

        // Цвета неба и сетки
        objects.setSkyColor(config.skyColor);
        objects.setGridColor(config.gridColor);

        // ПРИМЕНЯЕМ К Renderer
        std::cout << "\n--- ПРИМЕНЯЕМ К Renderer ---" << std::endl;

        renderer.setSkyColor(config.skyColor);
        std::cout << "  setSkyColor: (" << config.skyColor.r << "," << config.skyColor.g << "," << config.skyColor.b << ")" << std::endl;

        renderer.setFloorColor(config.floorColor);
        std::cout << "  setFloorColor: (" << config.floorColor.r << "," << config.floorColor.g << "," << config.floorColor.b << ")" << std::endl;

        renderer.setGridColor(config.gridColor);
        std::cout << "  setGridColor: (" << config.gridColor.r << "," << config.gridColor.g << "," << config.gridColor.b << ")" << std::endl;

        // Передаем настройки сетки в рендерер
        renderer.setGridSettings(config.gridEnabled, config.gridLineWidth);

        // ЗАГРУЖАЕМ МОДЕЛИ
        std::cout << "\n--- ЗАГРУЖАЕМ МОДЕЛИ ---" << std::endl;
        renderer.loadModelsFromConfig(objects);
        renderer.setGridDimensions(
            objects.getGridWidth(),
            objects.getGridDepth(),
            objects.getCellSize()
        );
        std::cout << "===============================\n" << std::endl;
    }
    else {
        std::cout << "✗ НЕ УДАЛОСЬ ЗАГРУЗИТЬ КОНФИГ!" << std::endl;
        std::cout << "Создаем примитивы по умолчанию..." << std::endl;
        renderer.createPrimitives();
    }
}

void Game::initialize() {
    std::cout << "=== GAME INITIALIZATION START ===" << std::endl;

    initPaths();
    loadConfig();

    std::cout << "Initializing renderer..." << std::endl;
    renderer.initialize();

    std::cout << "Initializing UI..." << std::endl;
    ui.initUI();

    std::cout << "Loading high scores..." << std::endl;
    objects.loadHighScores();

    std::cout << "=== GAME INITIALIZATION COMPLETE ===" << std::endl;
}

void Game::startNewGame() {
    objects.initGame();
    objects.setGameState(PLAYING);
    objects.deleteSaveGame(); // Удаляем старое сохранение
}

void Game::continueGame() {
    if (objects.loadGame()) {
        objects.setGameState(PLAYING);
    }
    else {
        // Если не удалось загрузить - начинаем новую
        startNewGame();
    }
}

void Game::update() {
    if (objects.isGameOver() || objects.getGameState() != PLAYING) return;

    objects.update();
}

void Game::render() {
    switch (objects.getGameState()) {
    case MAIN_MENU:
        ui.updateSaveGameInfo(objects.hasSaveGame());
        ui.drawMainMenu();
        break;
    case PLAYING:
        renderer.renderGame(objects);
        break;
    case PAUSED:
        renderer.renderGame(objects);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        ui.drawPauseMenu();
        break;
    case GAME_OVER:
        renderer.renderGame(objects);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        ui.drawGameOver(objects.getScore());
        break;
    case SETTINGS:
        ui.drawSettingsMenu(objects.getGameSpeed(),
            objects.getPlayerName(),
            objects.getSpeedMultiplier(),
            objects.getSpeedDisplayText(),
            objects.isNameInputActive(),
            true);
        break;
    case HIGH_SCORES:
        ui.drawHighScoresMenu(objects.getHighScores());
        break;
    case CONTROLS:
        ui.drawControlsMenu();
        break;
    }
}

void Game::forceRedraw() {
    // Принудительно перерисовываем текущее состояние
    if (g_mainWindow) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render();
        glfwSwapBuffers(g_mainWindow);
    }
}

void Game::handleKeyPress(int key) {
    // Делегируем обработку ввода соответствующим компонентам
    switch (objects.getGameState()) {
    case PLAYING:

        switch (key) {
        case GLFW_KEY_W:
            if (objects.getCurrentDirection() != BACKWARD)
                objects.setCurrentDirection (FORWARD);
            break;

        case GLFW_KEY_S:
            if (objects.getCurrentDirection() != FORWARD)
                objects.setCurrentDirection(BACKWARD);
            break;

        case GLFW_KEY_A:
            if (objects.getCurrentDirection() != RIGHT)
                objects.setCurrentDirection(LEFT);
            break;

        case GLFW_KEY_D:
            if (objects.getCurrentDirection() != LEFT)
                objects.setCurrentDirection(RIGHT);
            break;
        case GLFW_KEY_R:
            objects.initGame();
            objects.setGameState(PLAYING);
            // Добавьте здесь принудительный сброс теней
            renderer.markStaticShadowsDirty();
            renderer.markDynamicShadowsDirty();
            break;
        case GLFW_KEY_F6:
            renderer.toggleDebugRays();
            break;
        }

        objects.handleGameKeyPress(key);

        if (key == GLFW_KEY_Y) {
            renderer.toggleshadow_map();
            renderer.renderGame(objects);
            std::cout << " Game saved!" << std::endl;
        }
        // Сохранение по F5
        if (key == GLFW_KEY_F5) {
            objects.saveGame();
            std::cout << " Game saved!" << std::endl;
        }
        break;
    case SETTINGS:
        if (objects.isNameInputActive()) {
            objects.handleNameInput(key); // Обрабатываем Backspace, Enter, Escape
        }
        else {
            objects.handleSettingsKeyPress(key);
        }
        break;
    default:
        objects.handleMenuKeyPress(key);
        break;
    }
}

void Game::handleMouseClick() {
    if (objects.getGameState() == SETTINGS) {
        if (ui.isSpeedIncreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.increaseSpeed();
        }
        else if (ui.isSpeedDecreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.decreaseSpeed();
        }
        else if (ui.isNameFieldClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.setNameInputActive(true);
        }
    }
    ui.handleMouseClick(objects);
}

void Game::handleMouseScroll(double yoffset) {
    g_camera.zoom(yoffset);
}