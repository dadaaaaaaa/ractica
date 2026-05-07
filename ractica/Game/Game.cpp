#include "../pch.h"
#include "Game.h"
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
    std::cout << "\n========== LOADING CONFIG ==========" << std::endl;
    std::cout << "Config path: " << g_configPath << std::endl;

    GameConfig config;

    // Используем правильное пространство имён ConfigManagerUtils
    if (ConfigManager::loadGameConfig(g_configPath, config)) {
        std::cout << "✓ CONFIG LOADED SUCCESSFULLY!" << std::endl;

        // ВЫВОДИМ ВСЕ ЗНАЧЕНИЯ ИЗ КОНФИГА
        std::cout << "\n--- CONFIG VALUES ---" << std::endl;

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

        // Масштабы змейки
        std::cout << "SNAKE_HEAD_SCALE = " << config.snakeHeadScale << std::endl;
        std::cout << "SNAKE_BODY_SCALE = " << config.snakeBodyScale << std::endl;
        std::cout << "SNAKE_TAIL_SCALE = " << config.snakeTailScale << std::endl;

        // ========== НОВЫЕ ПАРАМЕТРЫ ПРЕПЯТСТВИЙ ==========
        std::cout << "\n--- OBSTACLES ---" << std::endl;
        std::cout << "TREE_MODEL = " << config.treeModel << std::endl;
        std::cout << "TREE_COLOR = " << config.treeColor.r << " " << config.treeColor.g << " " << config.treeColor.b << std::endl;
        std::cout << "TREE_SCALE = " << config.treeScale << std::endl;

        std::cout << "ROCK_MODEL = " << config.rockModel << std::endl;
        std::cout << "ROCK_COLOR = " << config.rockColor.r << " " << config.rockColor.g << " " << config.rockColor.b << std::endl;
        std::cout << "ROCK_SCALE = " << config.rockScale << std::endl;

        std::cout << "FENCE_MODEL = " << config.fenceModel << std::endl;
        std::cout << "FENCE_COLOR = " << config.fenceColor.r << " " << config.fenceColor.g << " " << config.fenceColor.b << std::endl;
        std::cout << "FENCE_SCALE = " << config.fenceScale << std::endl;

        std::cout << "APPLE_MODEL = " << config.appleModel << std::endl;
        std::cout << "APPLE_COLOR = " << config.appleColor.r << " " << config.appleColor.g << " " << config.appleColor.b << std::endl;
        std::cout << "APPLE_SCALE = " << config.appleScale << std::endl;

        // ========== НОВЫЕ ПАРАМЕТРЫ ОКРУЖЕНИЯ ==========
        std::cout << "\n--- ENVIRONMENT ---" << std::endl;
        std::cout << "CLOUD_MODEL = " << config.cloudModel << std::endl;
        std::cout << "CLOUD_COLOR = " << config.cloudColor.r << " " << config.cloudColor.g << " " << config.cloudColor.b << std::endl;
        std::cout << "CLOUD_SCALE = " << config.cloudScale << std::endl;
        std::cout << "CLOUD_COUNT = " << config.cloudCount << std::endl;

        std::cout << "BIRD_MODEL = " << config.birdModel << std::endl;
        std::cout << "BIRD_COLOR = " << config.birdColor.r << " " << config.birdColor.g << " " << config.birdColor.b << std::endl;
        std::cout << "BIRD_SCALE = " << config.birdScale << std::endl;
        std::cout << "BIRD_COUNT = " << config.birdCount << std::endl;

        std::cout << "FLOWER_MODEL = " << config.flowerModel << std::endl;
        std::cout << "FLOWER_COLOR = " << config.flowerColor.r << " " << config.flowerColor.g << " " << config.flowerColor.b << std::endl;
        std::cout << "FLOWER_SCALE = " << config.flowerScale << std::endl;
        std::cout << "FLOWER_COUNT = " << config.flowerCount << std::endl;

        // Настройки пола
        std::cout << "\n--- FLOOR ---" << std::endl;
        std::cout << "FLOOR_MODEL = " << config.floorModel << std::endl;
        std::cout << "FLOOR_TEXTURE = " << (config.floorTexture.empty() ? "none" : config.floorTexture) << std::endl;

        // Настройки света и теней
        std::cout << "\n--- LIGHT & SHADOWS ---" << std::endl;
        std::cout << "LIGHT_TYPE = " << config.lightType << std::endl;
        std::cout << "LIGHT_COLOR = " << config.lightColor.r << " " << config.lightColor.g << " " << config.lightColor.b << std::endl;
        std::cout << "LIGHT_DIR = " << config.lightDir.x << " " << config.lightDir.y << " " << config.lightDir.z << std::endl;
        std::cout << "LIGHT_POS = " << config.lightPos.x << " " << config.lightPos.y << " " << config.lightPos.z << std::endl;
        std::cout << "SHADOW_TRACE_MODE = " << config.shadowTraceMode << std::endl;
        std::cout << "SHADOW_SUBDIVISION_SIZE = " << config.shadowSubdivisionSize << std::endl;
        std::cout << "SHADOW_STRIDE_X = " << config.shadowStrideX << std::endl;
        std::cout << "SHADOW_STRIDE_Z = " << config.shadowStrideZ << std::endl;
        std::cout << "SHADOW_MAP_ENABLED = " << (config.shadowMapEnabled ? "true" : "false") << std::endl;
        std::cout << "AMBIENT_ENABLED = " << (config.ambientEnabled ? "true" : "false") << std::endl;
        std::cout << "SPECULAR_ENABLED = " << (config.specularEnabled ? "true" : "false") << std::endl;

        std::cout << "----------------------------\n" << std::endl;

        // ПРИМЕНЯЕМ К GameObjects
        std::cout << "--- APPLYING TO GameObjects ---" << std::endl;

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
        std::cout << "  setSnakeColors applied" << std::endl;

        // Масштабы змейки
        objects.setSnakeScales(
            config.snakeHeadScale,
            config.snakeBodyScale,
            config.snakeTailScale
        );
        std::cout << "  setSnakeScales applied" << std::endl;

        // Модели препятствий
        objects.setTreeModel(config.treeModel);
        objects.setTreeColor(config.treeColor);
        objects.setTreeScale(config.treeScale);
        std::cout << "  setTreeModel/Color/Scale applied" << std::endl;

        objects.setRockModel(config.rockModel);
        objects.setRockColor(config.rockColor);
        objects.setRockScale(config.rockScale);
        std::cout << "  setRockModel/Color/Scale applied" << std::endl;

        objects.setFenceModel(config.fenceModel);
        objects.setFenceColor(config.fenceColor);
        objects.setFenceScale(config.fenceScale);
        std::cout << "  setFenceModel/Color/Scale applied" << std::endl;

        objects.setAppleModel(config.appleModel);
        objects.setAppleColor(config.appleColor);
        objects.setAppleScale(config.appleScale);
        std::cout << "  setAppleModel/Color/Scale applied" << std::endl;

        // Модели окружения
        objects.setCloudModel(config.cloudModel);
        objects.setCloudColor(config.cloudColor);
        objects.setCloudScale(config.cloudScale);
        objects.setCloudCount(config.cloudCount);
        std::cout << "  setCloudModel/Color/Scale/Count applied" << std::endl;

        objects.setBirdModel(config.birdModel);
        objects.setBirdColor(config.birdColor);
        objects.setBirdScale(config.birdScale);
        objects.setBirdCount(config.birdCount);
        std::cout << "  setBirdModel/Color/Scale/Count applied" << std::endl;

        objects.setFlowerModel(config.flowerModel);
        objects.setFlowerColor(config.flowerColor);
        objects.setFlowerScale(config.flowerScale);
        objects.setFlowerCount(config.flowerCount);
        std::cout << "  setFlowerModel/Color/Scale/Count applied" << std::endl;

        // Настройки пола
        objects.setFloorModel(config.floorModel);
        objects.setFloorColor(config.floorColor);
        objects.setFloorTexture(config.floorTexture);
        std::cout << "  setFloor applied" << std::endl;

        // Цвета неба и сетки
        objects.setSkyColor(config.skyColor);
        objects.setGridColor(config.gridColor);

        // ПРИМЕНЯЕМ К Renderer
        std::cout << "\n--- APPLYING TO Renderer ---" << std::endl;

        renderer.setSkyColor(config.skyColor);
        renderer.setFloorColor(config.floorColor);
        renderer.setGridColor(config.gridColor);
        renderer.setGridSettings(config.gridEnabled, config.gridLineWidth);

        // Настройки света и теней
        renderer.setLightType(static_cast<LightType>(config.lightType));
        renderer.setLightColor(config.lightColor);
        renderer.setLightDir(config.lightDir);
        renderer.setLightPos(config.lightPos);

        renderer.setShadowTraceMode(config.shadowTraceMode);
        renderer.setShadowSubdivisionSize(config.shadowSubdivisionSize);
        renderer.setShadowStride(config.shadowStrideX, config.shadowStrideZ);
        renderer.setShadowMapEnabled(config.shadowMapEnabled);
        renderer.setAmbientEnabled(config.ambientEnabled);
        renderer.setSpecularEnabled(config.specularEnabled);

        std::cout << "  Light and shadow settings applied" << std::endl;

        // Передаём настройки сетки в рендерер
        renderer.setGridDimensions(
            objects.getGridWidth(),
            objects.getGridDepth(),
            objects.getCellSize()
        );

        // ЗАГРУЖАЕМ МОДЕЛИ
        std::cout << "\n--- LOADING MODELS ---" << std::endl;
        renderer.loadModelsFromConfig(objects);

        std::cout << "===============================\n" << std::endl;
    }
    else {
        std::cout << "✗ FAILED TO LOAD CONFIG!" << std::endl;
        std::cout << "Creating default primitives..." << std::endl;
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
    objects.deleteSaveGame();

    // Сбрасываем тени при старте новой игры
    renderer.resetShadows();
    renderer.markStaticShadowsDirty();
    renderer.markDynamicShadowsDirty();
    renderer.markFoodShadowsDirty();
}

void Game::continueGame() {
    if (objects.loadGame()) {
        objects.setGameState(PLAYING);
        // Пересчитываем тени после загрузки
        renderer.markStaticShadowsDirty();
        renderer.markDynamicShadowsDirty();
        renderer.markFoodShadowsDirty();
    }
    else {
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
    if (g_mainWindow) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render();
        glfwSwapBuffers(g_mainWindow);
    }
}

void Game::handleKeyPress(int key) {
    switch (objects.getGameState()) {
    case PLAYING:
        switch (key) {
        case GLFW_KEY_W:
            if (objects.getCurrentDirection() != BACKWARD)
                objects.setCurrentDirection(FORWARD);
            renderer.markDynamicShadowsDirty();
            break;

        case GLFW_KEY_S:
            if (objects.getCurrentDirection() != FORWARD)
                objects.setCurrentDirection(BACKWARD);
            renderer.markDynamicShadowsDirty();
            break;

        case GLFW_KEY_A:
            if (objects.getCurrentDirection() != RIGHT)
                objects.setCurrentDirection(LEFT);
            renderer.markDynamicShadowsDirty();
            break;

        case GLFW_KEY_D:
            if (objects.getCurrentDirection() != LEFT)
                objects.setCurrentDirection(RIGHT);
            renderer.markDynamicShadowsDirty();
            break;

        case GLFW_KEY_R:
            objects.initGame();
            objects.setGameState(PLAYING);
            renderer.resetShadows();
            renderer.markStaticShadowsDirty();
            renderer.markDynamicShadowsDirty();
            renderer.markFoodShadowsDirty();
            break;

        case GLFW_KEY_Y:
            renderer.toggleShadowMap();
            std::cout << "Shadow map toggled" << std::endl;
            break;

        case GLFW_KEY_F7:
            renderer.toggleDebugNormals();
            break;

        case GLFW_KEY_F8:
            renderer.toggleShadowTraceMode();
            std::cout << "Shadow trace mode toggled (F8)" << std::endl;
            break;

        case GLFW_KEY_F6:
            renderer.toggleDebugRays();
            break;

        case GLFW_KEY_F5:
            objects.saveGame();
            std::cout << "Game saved!" << std::endl;
            break;

        case GLFW_KEY_M:
            renderer.toggleUseExactModels();
            break;
        }

        objects.handleGameKeyPress(key);
        break;

    case SETTINGS:
        if (objects.isNameInputActive()) {
            objects.handleNameInput(key);
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