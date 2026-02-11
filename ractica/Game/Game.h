#pragma once

#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Objects/GameObjects.h"
#include "../Graphics/GameRenderer.h"
#include "../UI/GameUI.h"

class Game {
private:
    // Компоненты игры
    GameObjects objects;
    GameRenderer renderer;
    GameUI ui;

public:
    Game();

    // Основные методы
    void initialize();
    void update();
    void render();
    void initUI();
    void continueGame();
    void startNewGame();
    void forceRedraw(); // Добавляем

    // Геттеры
    GameState getGameState() const { return objects.getGameState(); }
    int getScore() const { return objects.getScore(); }
    int getWindowWidth() const { return ui.getWindowWidth(); }
    int getWindowHeight() const { return ui.getWindowHeight(); }
    double getMouseX() const { return ui.getMouseX(); }
    double getMouseY() const { return ui.getMouseY(); }
    const std::vector<Point>& getSnake() const { return objects.getSnake(); }
    float getGameSpeed() const { return objects.getGameSpeed(); }

    // Сеттеры
    void setGameState(GameState state) { objects.setGameState(state); }
    void setMousePosition(double x, double y) { ui.setMousePosition(x, y); }
    void setMousePressed(bool pressed) { ui.setMousePressed(pressed); }
    void setCurrentDirection(Direction direction) { objects.setCurrentDirection(direction); }
    void setGameSpeed(float speed) { objects.setGameSpeed(speed); }
    void setPlayerName(const std::string& name) { objects.setPlayerName(name); }
    void setWindowSize(int width, int height) { ui.setWindowSize(width, height); }
    bool isNameInputActive() const { return objects.isNameInputActive(); }
    void handleNameInput(int key) { objects.handleNameInput(key); }
    void setNameInputActive(bool active) { objects.setNameInputActive(active); }
    void addCharacterToName(char c) { objects.addCharacterToName(c); }

    // Обработка ввода
    void handleKeyPress(int key);
    void handleMouseClick();
    void handleMouseScroll(double yoffset);

    // Вспомогательные методы
    void initGame() { objects.initGame(); }
    void debugSnakeInfo() { objects.debugSnakeInfo(); }
    void saveHighScore() { objects.saveHighScore(); }
};