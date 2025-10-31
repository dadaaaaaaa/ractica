#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "Obstacle.h"
#include "Sprite.h"
#include "Bird.h"
#include <vector>

class GameObjects {
private:
    std::vector<Point> snake;
    std::vector<Point> food;
    std::vector<Obstacle> obstacles;
    std::vector<Point> fenceBlocks;

    std::vector<Sprite> cloudSprites;
    std::vector<Bird> birds;
    std::vector<Sprite> flowerSprites;

    Direction currentDirection;
    int verticalDirection;
    int score;
    bool gameOver;
    GameState gameState;
    float gameSpeed;
    GameState previousState;

    std::vector<HighScore> highScores;
    std::string playerName;
    std::vector<float> speedMultipliers;
    int currentSpeedIndex;

public:
    GameObjects();

    void updateGameSpeedFromMultiplier();
    float getSpeedMultiplier() const;
    std::string getSpeedDisplayText() const;
    void increaseSpeed();
    void decreaseSpeed();

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

    void setCurrentDirection(Direction direction) { currentDirection = direction; }
    void setVerticalDirection(int direction) { verticalDirection = direction; }
    void setScore(int newScore) { score = newScore; }
    void setGameOver(bool over) { gameOver = over; }
    void setGameState(GameState state) { gameState = state; }
    void setGameSpeed(float speed) { gameSpeed = speed; }
    void setPreviousState(GameState state) { previousState = state; }
    void setPlayerName(const std::string& name) { playerName = name; }

    void generateFence();
    void generateClouds();
    void generateBirds();
    void generateGroundSprites();
    void generateObstacles();
    void generateSingleFood();
    void generateInitialFood();

    void update();
    void updateClouds();
    void updateBirds();

    void initGame();
    void debugSnakeInfo();
    void loadHighScores();
    void saveHighScore();

    void handleGameKeyPress(int key);
    void handleSettingsKeyPress(int key);
    void handleMenuKeyPress(int key);
};