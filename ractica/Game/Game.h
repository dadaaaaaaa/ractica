#pragma once

#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Objects/GameObjects.h"
#include "../Graphics/GameRenderer.h"
#include "../UI/GameUI.h"

// Âíåøíèå ïåğåìåííûå äëÿ ïóòåé (áóäóò îïğåäåëåíû â Game.cpp)
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

    // ÌÅÒÎÄÛ ÄËß ÏÓÒÅÉ È ÊÎÍÔÈÃÀ
    void initPaths();
    void loadConfig();

    // === ÃÅÒÒÅĞÛ ÄËß ÇÌÅÉÊÈ ===
    const std::string& getSnakeHeadModel() const { return objects.getSnakeHeadModel(); }
    const std::string& getSnakeBodyModel() const { return objects.getSnakeBodyModel(); }
    const std::string& getSnakeTailModel() const { return objects.getSnakeTailModel(); }

    const glm::vec3& getSnakeHeadColor() const { return objects.getSnakeHeadColor(); }
    const glm::vec3& getSnakeBodyColor() const { return objects.getSnakeBodyColor(); }
    const glm::vec3& getSnakeTailColor() const { return objects.getSnakeTailColor(); }

    // ÂÀÆÍÎ: İòè ìåòîäû íóæíû äëÿ ìàñøòàáèğîâàíèÿ â GameRenderer
    float getSnakeHeadScale() const { return objects.getSnakeHeadScale(); }
    float getSnakeBodyScale() const { return objects.getSnakeBodyScale(); }
    float getSnakeTailScale() const { return objects.getSnakeTailScale(); }
    float getSnakeScale() const { return objects.getSnakeScale(); } // Äëÿ îáğàòíîé ñîâìåñòèìîñòè

    // === ÃÅÒÒÅĞÛ ÑÎÑÒÎßÍÈß ===
    GameState getGameState() const { return objects.getGameState(); }
    int getScore() const { return objects.getScore(); }
    float getGameSpeed() const { return objects.getGameSpeed(); }
    const std::vector<Point>& getSnake() const { return objects.getSnake(); }

    // === ÃÅÒÒÅĞÛ UI ===
    int getWindowWidth() const { return ui.getWindowWidth(); }
    int getWindowHeight() const { return ui.getWindowHeight(); }
    double getMouseX() const { return ui.getMouseX(); }
    double getMouseY() const { return ui.getMouseY(); }

    // === ÃÅÒÒÅĞÛ ÏÓÒÅÉ ===
    const std::string& getAssetsPath() const { return g_assetsPath; }
    const std::string& getModelsPath() const { return g_modelsPath; }
    const std::string& getTexturesPath() const { return g_texturesPath; }

    // === ÑÅÒÒÅĞÛ ===
    void setGameState(GameState state) { objects.setGameState(state); }
    void setMousePosition(double x, double y) { ui.setMousePosition(x, y); }
    void setMousePressed(bool pressed) { ui.setMousePressed(pressed); }
    void setCurrentDirection(Direction direction) { objects.setCurrentDirection(direction); }
    void setGameSpeed(float speed) { objects.setGameSpeed(speed); }
    void setPlayerName(const std::string& name) { objects.setPlayerName(name); }
    void setWindowSize(int width, int height) { ui.setWindowSize(width, height); }
    void toggleRayTracing() { renderer.toggleRayTracing(); forceRedraw(); }
    // === ÌÅÒÎÄÛ ÄËß ÈÌÅÍÈ ===
    bool isNameInputActive() const { return objects.isNameInputActive(); }
    void handleNameInput(int key) { objects.handleNameInput(key); }
    void setNameInputActive(bool active) { objects.setNameInputActive(active); }
    void addCharacterToName(char c) { objects.addCharacterToName(c); }

    // === ÎÁĞÀÁÎÒÊÀ ÂÂÎÄÀ ===
    void handleKeyPress(int key);
    void handleMouseClick();
    void handleMouseScroll(double yoffset);

    // === ÈÃĞÎÂÛÅ ÌÅÒÎÄÛ ===
    void initGame() { objects.initGame(); }
    void debugSnakeInfo() { objects.debugSnakeInfo(); }
    void saveHighScore() { objects.saveHighScore(); }
};