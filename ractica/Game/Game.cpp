#include "../pch.h"
#include "Game.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../ConfigEditor/ConfigManager.h"  // ПРАВИЛЬНЫЙ ПУТЬ!
#include "../Shared/ConfigTypes.h"
#include <windows.h>
#include <iostream>
// Внешние глобальные переменные
extern ShaderManager g_shaderManager;
extern Camera g_camera;
extern GLuint uiVAO, uiVBO;
extern bool g_shouldExitGame;
extern GLFWwindow* g_mainWindow; // ДОБАВЛЕНО
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

    // Убираем лишний "\ractica" если он есть
    if (rootPath.find("\\ractica") != std::string::npos) {
        // Убираем последний сегмент пути
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
    std::cout << "\n===== LOADING GAME CONFIG =====" << std::endl;
    std::cout << "Attempting to load from: " << g_configPath << std::endl;

    GameConfig config;

    if (ConfigManager::loadGameConfig(g_configPath, config)) {
        std::cout << "✓ CONFIG LOADED SUCCESSFULLY!" << std::endl;

        // Показываем что загрузили
        std::cout << "\nLoaded values from file:" << std::endl;
        std::cout << "  Sky color: (" << config.skyColor.r << ", " << config.skyColor.g << ", " << config.skyColor.b << ")" << std::endl;
        std::cout << "  Floor color: (" << config.floorColor.r << ", " << config.floorColor.g << ", " << config.floorColor.b << ")" << std::endl;
        std::cout << "  Grid color: (" << config.gridColor.r << ", " << config.gridColor.g << ", " << config.gridColor.b << ")" << std::endl;
        std::cout << "  Cloud count: " << config.cloudCount << std::endl;
        std::cout << "  Bird count: " << config.birdCount << std::endl;
        std::cout << "  Flower count: " << config.flowerCount << std::endl;

        // Модели змейки
        std::cout << "  Snake Head: " << config.snakeHeadModel << std::endl;
        std::cout << "  Snake Body: " << config.snakeBodyModel << std::endl;
        std::cout << "  Snake Tail: " << config.snakeTailModel << std::endl;

        // Модели окружения
        std::cout << "  Apple Model: " << config.appleModel << std::endl;
        std::cout << "  Tree Model: " << config.treeModel << std::endl;
        std::cout << "  Cloud Model: " << config.cloudModel << std::endl;
        std::cout << "  Bird Model: " << config.birdModel << std::endl;
        std::cout << "  Flower Model: " << config.flowerModel << std::endl;
        std::cout << "  Floor Model: " << config.floorModel << std::endl;

        // Применяем к GameObjects
        objects.setSnakeModels(
            config.snakeHeadModel,
            config.snakeBodyModel,
            config.snakeTailModel
        );

        objects.setAppleModel(config.appleModel);
        objects.setTreeModel(config.treeModel);
        objects.setCloudModel(config.cloudModel);
        objects.setBirdModel(config.birdModel);
        objects.setFlowerModel(config.flowerModel);
        objects.setFloorModel(config.floorModel);

        objects.setSnakeColors(
            config.snakeHeadColor,
            config.snakeBodyColor,
            config.snakeTailColor
        );

        objects.setSnakeScale(config.snakeHeadScale);
        objects.setCloudCount(config.cloudCount);
        objects.setBirdCount(config.birdCount);
        objects.setFlowerCount(config.flowerCount);

        // Применяем к Renderer
        renderer.setSkyColor(config.skyColor);
        renderer.setFloorColor(config.floorColor);
        renderer.setGridColor(config.gridColor);

        // Загружаем модели
        renderer.loadModelsFromConfig(objects);

        if (!config.floorTexture.empty()) {
            renderer.setFloorTexture(config.floorTexture);
        }
    }
    else {
        std::cout << "✗ FAILED TO LOAD CONFIG!" << std::endl;
        std::cout << "Creating default models..." << std::endl;
        // Создаем примитивы по умолчанию
        renderer.createPrimitives();
    }
    std::cout << "===============================\n" << std::endl;
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
            true); // Всегда true, так как теперь только двойная буферизация
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
        objects.handleGameKeyPress(key);

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