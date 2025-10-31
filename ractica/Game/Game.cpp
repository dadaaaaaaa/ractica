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

    // Правильный порядок инициализации
    ui.initUI();
    renderer.initialize();
    objects.loadHighScores();
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
        ui.drawSettingsMenu(objects.getGameSpeed(), objects.getPlayerName());
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
    ui.handleMouseClick(objects);
}

void Game::handleMouseScroll(double yoffset) {
    g_camera.zoom(yoffset);
}