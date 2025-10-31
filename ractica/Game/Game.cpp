#include "../pch.h"
#include "Game.h"

// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern Camera g_camera;
extern GLuint uiVAO, uiVBO;

Game::Game() {
    // Инициализация делегируется компонентам
}

void Game::initialize() {
    std::cout << "=== GAME INITIALIZATION START ===" << std::endl;

    // ПРАВИЛЬНЫЙ ПОРЯДОК (без двойной инициализации):

    // 1. Сначала рендерер (шейдеры)
    std::cout << "Initializing renderer..." << std::endl;
    renderer.initialize();

    // 2. Потом UI (ОДИН РАЗ!)
    std::cout << "Initializing UI..." << std::endl;
        ui.initUI();
    

    // 3. Игровые объекты
    std::cout << "Loading high scores..." << std::endl;
    objects.loadHighScores();

    std::cout << "Initializing game..." << std::endl;
    objects.initGame();

    std::cout << "=== GAME INITIALIZATION COMPLETE ===" << std::endl;
}

void Game::update() {
    if (objects.isGameOver() || objects.getGameState() != PLAYING) return;

    objects.update();
}

void Game::render() {
    switch (objects.getGameState()) {
    case MAIN_MENU:

        objects.initGame();
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
            objects.getSpeedDisplayText());
        break;
    case HIGH_SCORES:
        ui.drawHighScoresMenu(objects.getHighScores());
        break;
    case CONTROLS:
        ui.drawControlsMenu();
        break;
    }
}

void Game::handleKeyPress(int key) {
    // Делегируем обработку ввода соответствующим компонентам
    switch (objects.getGameState()) {
    case PLAYING:
        objects.handleGameKeyPress(key);
        break;
    case SETTINGS:
        objects.handleSettingsKeyPress(key);
        break;
    default:
        objects.handleMenuKeyPress(key);
        break;
    }
}

void Game::handleMouseClick() {
    if (objects.getGameState() == SETTINGS) {
        // Проверяем клики на кнопки скорости
        if (ui.isSpeedIncreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.increaseSpeed();
        }
        else if (ui.isSpeedDecreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.decreaseSpeed();
        }
    }
    ui.handleMouseClick(objects);
}

void Game::handleMouseScroll(double yoffset) {
    g_camera.zoom(yoffset);
}