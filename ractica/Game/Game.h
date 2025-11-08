#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Core/GameConfig.h"
#include "../Objects/GameObjects.h"
#include "../Graphics/GameRenderer.h"
#include "../UI/GameUI.h"
#include "../UI/NetworkManager.h"

class Game {
private:
    // Компоненты игры
    GameObjects objects;
    GameRenderer renderer;
    GameUI ui;
    NetworkManager networkManager;

    bool isInitialized = false;

public:
    Game();

    // ============================================================================
    // ОСНОВНЫЕ МЕТОДЫ ЖИЗНЕННОГО ЦИКЛА
    // ============================================================================

    // Инициализирует все системы игры
    void initialize();

    // Обновляет игровую логику
    void update();

    // Рендерит текущее состояние игры
    void render();

    // Очищает ресурсы игры
    void cleanup();

    // ============================================================================
    // УПРАВЛЕНИЕ ИГРОВЫМИ СЕССИЯМИ
    // ============================================================================

    // Начинает новую игру
    void startNewGame();

    // Продолжает сохраненную игру
    void continueGame();

    // Перезапускает текущую игру
    void restartGame();

    // ============================================================================
    // ОБРАБОТКА ВВОДА
    // ============================================================================

    // Обрабатывает нажатия клавиш
    void handleKeyPress(int key);

    // Обрабатывает клики мыши
    void handleMouseClick();

    // Обрабатывает прокрутку мыши
    void handleMouseScroll(double yoffset);

    // ============================================================================
    // ГЕТТЕРЫ (только необходимые для внешнего использования)
    // ============================================================================

    // Состояние игры
    GameState getGameState() const { return objects.getGameState(); }
    bool isRunning() const { return isInitialized; }

    // Игровая статистика
    int getScore() const { return objects.getScore(); }
    float getGameSpeed() const { return objects.getGameSpeed(); }
    const std::vector<Point>& getSnake() const { return objects.getSnake(); }

    // UI состояние
    int getWindowWidth() const { return ui.getWindowWidth(); }
    int getWindowHeight() const { return ui.getWindowHeight(); }
    double getMouseX() const { return ui.getMouseX(); }
    double getMouseY() const { return ui.getMouseY(); }

    // Конфигурация
    const GameConfig& getConfig() const { return objects.getGameConfig(); }

    // ============================================================================
    // СЕТТЕРЫ (только необходимые для внешнего использования)
    // ============================================================================

    // Управление состоянием
    void setGameState(GameState state) { objects.setGameState(state); }
    void setWindowSize(int width, int height);
    void setMousePosition(double x, double y) { ui.setMousePosition(x, y); }
    void setMousePressed(bool pressed) { ui.setMousePressed(pressed); }

    // ============================================================================
    // СЕТЕВЫЕ ФУНКЦИИ
    // ============================================================================

    // Получает топ рекордов с сервера
    std::vector<HighScore> getTopScores() { return networkManager.getTopScores(); }

    // Отправляет рекорд на сервер
    bool submitHighScore() {
        return networkManager.submitHighScore(
            objects.getPlayerName(),
            objects.getScore(),
            objects.getGameSpeed(),
            objects.getGameDuration(),
            objects.getSnakeLength()
        );
    }

    // Проверяет соединение с сервером
    bool testConnection() { return networkManager.testConnection(); }

private:
    // ============================================================================
    // ПРИВАТНЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    // ============================================================================

    // Сохраняет игру при необходимости
    void autoSaveIfNeeded();

    // Обрабатывает переход в состояние игры
    void handleGameStateTransition(GameState newState);

    // Обновляет HUD и интерфейс
    void updateHUD();
};