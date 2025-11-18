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

    // 1. Сначала рендерер (шейдеры)
    std::cout << "Initializing renderer..." << std::endl;
    renderer.initialize();

    // 2. Потом UI (ОДИН РАЗ!)
    std::cout << "Initializing UI..." << std::endl;
    ui.initUI();

    // 3. Игровые объекты
    std::cout << "Loading high scores..." << std::endl;
    objects.loadHighScores();

    // НЕ инициализируем игру сразу - ждем выбора в меню
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
            objects.isNameInputActive()); // Добавляем параметр активности
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

        // Сохранение по F5
        if (key == GLFW_KEY_F5) {
            objects.saveGame();
            std::cout << " Game saved!" << std::endl;
        }
        break;
    case SETTINGS:
        if (objects.isNameInputActive()) {
            objects.handleNameInput(key); // Обрабатываем Backspace, Enter, Escape
        } else {
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
        // Проверяем клики на кнопки скорости
        if (ui.isSpeedIncreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.increaseSpeed();
        }
        else if (ui.isSpeedDecreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.decreaseSpeed();
        }
        // Проверяем клик на поле имени
        else if (ui.isNameFieldClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.setNameInputActive(true);
        }
    }
    ui.handleMouseClick(objects);
}

void Game::handleMouseScroll(double yoffset) {
    g_camera.zoom(yoffset);
}