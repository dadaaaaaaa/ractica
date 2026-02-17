#pragma once

#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Objects/GameObjects.h"
#include "../Graphics/GameRenderer.h"
#include "../UI/GameUI.h"

// Внешние переменные для путей (будут определены в Game.cpp)
extern std::string g_assetsPath;
extern std::string g_modelsPath;
extern std::string g_texturesPath;
extern std::string g_configPath;

class Game {
private:
    GameObjects objects;
    GameRenderer renderer;
    GameUI ui;

public:
    Game();

    void initialize();
    void update();
    void render();
    void initUI();
    void continueGame();
    void startNewGame();
    void forceRedraw();

    // НОВЫЕ МЕТОДЫ
    void initPaths();
    void loadConfig();

    // Геттеры для конфига
    const std::string& getSnakeHeadModel() const { return objects.getSnakeHeadModel(); }
    const std::string& getSnakeBodyModel() const { return objects.getSnakeBodyModel(); }
    const std::string& getSnakeTailModel() const { return objects.getSnakeTailModel(); }
    const glm::vec3& getSnakeHeadColor() const { return objects.getSnakeHeadColor(); }
    const glm::vec3& getSnakeBodyColor() const { return objects.getSnakeBodyColor(); }
    const glm::vec3& getSnakeTailColor() const { return objects.getSnakeTailColor(); }
    float getSnakeScale() const { return objects.getSnakeScale(); }

    GameState getGameState() const { return objects.getGameState(); }
    int getScore() const { return objects.getScore(); }
    int getWindowWidth() const { return ui.getWindowWidth(); }
    int getWindowHeight() const { return ui.getWindowHeight(); }
    double getMouseX() const { return ui.getMouseX(); }
    double getMouseY() const { return ui.getMouseY(); }
    const std::vector<Point>& getSnake() const { return objects.getSnake(); }
    float getGameSpeed() const { return objects.getGameSpeed(); }

    const std::string& getAssetsPath() const { return g_assetsPath; }
    const std::string& getModelsPath() const { return g_modelsPath; }
    const std::string& getTexturesPath() const { return g_texturesPath; }

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

    void handleKeyPress(int key);
    void handleMouseClick();
    void handleMouseScroll(double yoffset);

    void initGame() { objects.initGame(); }
    void debugSnakeInfo() { objects.debugSnakeInfo(); }
    void saveHighScore() { objects.saveHighScore(); }
};