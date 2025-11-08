#include "../pch.h"
#include "Game.h"

// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern Camera g_camera;
extern GLuint uiVAO, uiVBO;

// Конструктор - инициализация делегируется компонентам
Game::Game() : isInitialized(false) {
}

// Инициализирует все системы игры
void Game::initialize() {
    if (isInitialized) return;

    std::cout << "=== GAME INITIALIZATION START ===" << std::endl;

    // 1. Сначала рендерер (шейдеры и графика)
    std::cout << "Initializing renderer..." << std::endl;
    if (!renderer.initialize()) {
        std::cerr << "❌ Failed to initialize renderer!" << std::endl;
        return;
    }

    // 2. Потом UI (один раз!)
    std::cout << "Initializing UI..." << std::endl;
    ui.initUI();

    // 3. Игровые объекты и данные
    std::cout << "Loading game data..." << std::endl;
    objects.loadHighScores();
    objects.loadSettings();

    // 4. Сеть (если включено)
    if (objects.getGameConfig().enableOnlineFeatures) {
        std::cout << "Testing network connection..." << std::endl;
        networkManager.testConnection();
    }

    isInitialized = true;
    std::cout << "✅ === GAME INITIALIZATION COMPLETE ===" << std::endl;
}

// Начинает новую игру
void Game::startNewGame() {
    objects.initGame();
    objects.setGameState(PLAYING);
    objects.deleteSaveGame(); // Удаляем старое сохранение
    std::cout << "🎮 New game started" << std::endl;
}

// Продолжает сохраненную игру
void Game::continueGame() {
    if (objects.loadGame()) {
        objects.setGameState(PLAYING);
        std::cout << "🎮 Game loaded successfully" << std::endl;
    }
    else {
        // Если не удалось загрузить - начинаем новую
        std::cout << "⚠️  Failed to load save, starting new game" << std::endl;
        startNewGame();
    }
}

// Обновляет игровую логику
void Game::update() {
    if (!isInitialized) return;

    GameState currentState = objects.getGameState();

    // Обновляем только если игра активна
    if (currentState == PLAYING && !objects.isGameOver()) {
        objects.update();

        // Автосохранение каждые 30 секунд
        static double lastAutoSave = 0.0;
        double currentTime = glfwGetTime();
        if (currentTime - lastAutoSave > AUTO_SAVE_INTERVAL) {
            objects.saveGame();
            lastAutoSave = currentTime;
        }
    }

    // Обновляем анимации UI независимо от состояния
    ui.updateAnimations();
}

// Рендерит текущее состояние игры
void Game::render() {
    if (!isInitialized) return;

    GameState currentState = objects.getGameState();

    switch (currentState) {
    case MAIN_MENU:
        ui.updateSaveGameInfo(objects.hasSaveGame());
        ui.drawMainMenu();
        break;

    case PLAYING:
        renderer.renderGame(objects);
        renderer.renderHUD(objects, ui);
        break;

    case PAUSED:
        renderer.renderGame(objects);
        renderer.renderHUD(objects, ui);
        ui.drawPauseMenu();
        break;

    case GAME_OVER:
        renderer.renderGame(objects);
        renderer.renderHUD(objects, ui);
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

// Обрабатывает нажатия клавиш
void Game::handleKeyPress(int key) {
    if (!isInitialized) return;

    GameState currentState = objects.getGameState();

    switch (currentState) {
    case PLAYING:
        objects.handleGameKeyPress(key);

        // Быстрое сохранение по F5
        if (key == GLFW_KEY_F5) {
            objects.saveGame();
            std::cout << "💾 Quick save completed!" << std::endl;
        }
        break;

    case SETTINGS:
        objects.handleSettingsKeyPress(key);
        break;

    default:
        objects.handleMenuKeyPress(key);
        break;
    }
}

// Обрабатывает клики мыши
void Game::handleMouseClick() {
    if (!isInitialized) return;

    // Обработка специальных кнопок в настройках
    if (objects.getGameState() == SETTINGS) {
        if (ui.isSpeedIncreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.increaseSpeed();
        }
        else if (ui.isSpeedDecreaseButtonClicked(ui.getMouseX(), ui.getMouseY())) {
            objects.decreaseSpeed();
        }
    }

    // Делегируем основную обработку UI
    ui.handleMouseClick(objects);
}

// Обрабатывает прокрутку мыши
void Game::handleMouseScroll(double yoffset) {
    if (!isInitialized) return;

    // Прокрутка работает в игровых состояниях
    GameState currentState = objects.getGameState();
    if (currentState == PLAYING || currentState == PAUSED || currentState == GAME_OVER) {
        g_camera.zoom(yoffset);
    }
}

// Устанавливает размер окна
void Game::setWindowSize(int width, int height) {
    ui.setWindowSize(width, height);
    objects.getGameConfig().windowWidth = width;
    objects.getGameConfig().windowHeight = height;
}

// Устанавливает позицию мыши
void Game::setMousePosition(double x, double y) {
    ui.setMousePosition(x, y);
}

// Устанавливает состояние нажатия мыши
void Game::setMousePressed(bool pressed) {
    ui.setMousePressed(pressed);
}

// Возвращает текущее состояние игры
GameState Game::getGameState() const {
    return objects.getGameState();
}

// Возвращает текущий счет
int Game::getScore() const {
    return objects.getScore();
}

// Возвращает текущую скорость игры
float Game::getGameSpeed() const {
    return objects.getGameSpeed();
}

// Возвращает змейку (для камеры)
const std::vector<Point>& Game::getSnake() const {
    return objects.getSnake();
}

// Очищает ресурсы игры
void Game::cleanup() {
    if (!isInitialized) return;

    std::cout << "Cleaning up game resources..." << std::endl;

    // Сохраняем настройки при выходе
    objects.saveSettings();

    // Сохраняем игру если она активна
    if (objects.getGameState() == PLAYING) {
        objects.saveGame();
    }

    // Очищаем компоненты
    renderer.cleanup();
    ui.cleanup();

    isInitialized = false;
    std::cout << "✅ Game cleanup complete" << std::endl;
}