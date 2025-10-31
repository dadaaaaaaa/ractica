#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "Obstacle.h"
#include "Sprite.h"
#include "Bird.h"
#include <vector>

class GameObjects {
private:
    // Основные игровые объекты
    std::vector<Point> snake;
    std::vector<Point> food;
    std::vector<Obstacle> obstacles;
    std::vector<Point> fenceBlocks;

    // Спрайты окружения
    std::vector<Sprite> cloudSprites;
    std::vector<Bird> birds;
    std::vector<Sprite> flowerSprites;

    // Состояние игры
    Direction currentDirection;
    int verticalDirection;
    int score;
    bool gameOver;
    GameState gameState;
    float gameSpeed;
    GameState previousState;

    // Рекорды
    std::vector<HighScore> highScores;
    std::string playerName;

public:
    GameObjects();

    // Геттеры
    const std::vector<Point>& getSnake() const { return snake; }
    const std::vector<Point>& getFood() const { return food; }
    const std::vector<Obstacle>& getObstacles() const { return obstacles; }
    const std::vector<Point>& getFenceBlocks() const { return fenceBlocks; }
    const std::vector<Sprite>& getCloudSprites() const { return cloudSprites; }
    const std::vector<Bird>& getBirds() const { return birds; }
    const std::vector<Sprite>& getFlowerSprites() const { return flowerSprites; }

    Direction getCurrentDirection() const { return currentDirection; }
    int getVerticalDirection() const { return verticalDirection; }
    int getScore() const { return score; }
    bool isGameOver() const { return gameOver; }
    GameState getGameState() const { return gameState; }
    float getGameSpeed() const { return gameSpeed; }
    GameState getPreviousState() const { return previousState; }
    const std::vector<HighScore>& getHighScores() const { return highScores; }
    const std::string& getPlayerName() const { return playerName; }

    // Сеттеры
    void setSnake(const std::vector<Point>& newSnake) { snake = newSnake; }
    void setCurrentDirection(Direction direction) { currentDirection = direction; }
    void setVerticalDirection(int direction) { verticalDirection = direction; }
    void setScore(int newScore) { score = newScore; }
    void setGameOver(bool over) { gameOver = over; }
    void setGameState(GameState state) { gameState = state; }
    void setGameSpeed(float speed) { gameSpeed = speed; }
    void setPreviousState(GameState state) { previousState = state; }
    void setPlayerName(const std::string& name) { playerName = name; }

    // Генерация объектов
    void generateFence();
    void generateClouds();
    void generateBirds();
    void generateGroundSprites();
    void generateObstacles();
    void generateSingleFood();
    void generateInitialFood();

    // Обновление объектов
    void update();
    void updateClouds();
    void updateBirds();

    // Управление состоянием
    void initGame();
    void debugSnakeInfo();
    void loadHighScores();
    void saveHighScore();

    // Обработка ввода
    void handleGameKeyPress(int key);
    void handleSettingsKeyPress(int key);
    void handleMenuKeyPress(int key);

    // Модификация объектов
    void addSnakeSegment(const Point& segment);
    void removeSnakeSegment();
    void addFood(const Point& foodPoint);
    void clearFood();
    void addObstacle(const Obstacle& obstacle);
    void clearObstacles();
};