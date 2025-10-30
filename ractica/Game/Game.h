#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Objects/Obstacle.h"
#include "../Graphics/Model.h"
#include "../Objects/Sprite.h"
#include "../Objects/Bird.h"
#include "../UI/MenuButton.h"
#include <vector>

class Game {
private:
    // Игровые объекты
    std::vector<Point> snake;
    std::vector<Point> food;
    std::vector<Obstacle> obstacles;
    std::vector<Point> fenceBlocks;

    // Спрайты
    std::vector<Sprite> cloudSprites;
    std::vector<Bird> birds;
    std::vector<Sprite> flowerSprites;

    // Модели
    Model snakeModel;
    Model foodModel;
    Model obstacleModel;
    Model floorModel;
    Model fenceModel;
    Model cloudModel;
    Model birdModel;
    Model flowerModel;
    Model treeModel;
    Model appleModel;

    // UI
    std::vector<MenuButton> mainMenuButtons;
    std::vector<MenuButton> pauseMenuButtons;
    std::vector<MenuButton> settingsButtons;
    std::vector<MenuButton> gameOverButtons;

    // Рекорды
    std::vector<HighScore> highScores;

    // Переменные состояния
    Direction currentDirection;
    int verticalDirection;
    int score;
    bool gameOver;
    GameState gameState;
    float gameSpeed;

    // Интерфейс
    int windowWidth;
    int windowHeight;
    double mouseX, mouseY;
    bool mousePressed;
    std::string playerName;

public:
    Game();

    // Основные методы
    void initialize();
    void update();
    void render();
    void initUI();

    // Геттеры
    GameState getGameState() const { return gameState; }
    int getScore() const { return score; }
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }
    const std::vector<Point>& getSnake() const { return snake; }
    float getGameSpeed() const { return gameSpeed; }

    // Сеттеры
    void setGameState(GameState state) { gameState = state; }
    void setMousePosition(double x, double y) { mouseX = x; mouseY = windowHeight - y; }
    void setMousePressed(bool pressed) { mousePressed = pressed; }
    void setCurrentDirection(Direction direction) { currentDirection = direction; }
    void setGameSpeed(float speed) { gameSpeed = speed; }
    void setPlayerName(const std::string& name) { playerName = name; }
    void setWindowSize(int width, int height) { windowWidth = width; windowHeight = height; }

    // Обработка ввода
    void handleKeyPress(int key);
    void handleMouseClick();
    void handleMouseScroll(double yoffset);

    // Вспомогательные методы
    void initGame();
    void debugSnakeInfo();
    void saveHighScore();

    // Методы для обновления спрайтов
    void updateClouds();
    void updateBirds();

private:
    // Внутренние методы
    void generateFence();
    void generateClouds();
    void generateBirds();
    void generateGroundSprites();
    void generateObstacles();
    void generateSingleFood();
    void generateInitialFood();
    void loadHighScores();

    // Методы отрисовки
    void drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color);
    void drawFloor();
    void drawSnake();
    void drawFood();
    void drawObstaclesAsTrees();
    void drawFence();
    void drawClouds();
    void drawBird(const Bird& bird);
    void drawBirds();
    void drawGroundSprites();
    void drawButton(const MenuButton& button);
    void drawTestQuad(float x, float y, float width, float height, const glm::vec3& color);
    void drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha = 1.0f);
    void drawText(const std::string& text, float x, float y, float scale, const glm::vec3& color);

    // Методы меню
    void drawMainMenu();
    void drawPauseMenu();
    void drawSettingsMenu();
    void drawHighScoresMenu();
    void drawControlsMenu();
    void drawGameOver();
    void renderGame();

    // Создание моделей
    void loadAllModels();
    void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal = glm::vec3(0.0f, 0.0f, 1.0f));
    void createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color);
    void createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color);
    void createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius);
    void createCloudModel(Model& model);
    void createAnimatedBirdModel(Model& model);
    void createFlowerModel(Model& model);
    void createTreeModel(Model& model);
    void createDetailedAppleModel(Model& model);
    void createTexturedCubeModel(Model& model);
    void createTexturedSphereModel(Model& model);
    void createTexturedFloorModel(Model& model);
    void createFenceModel(Model& model);
};