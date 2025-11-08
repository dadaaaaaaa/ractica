#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Core/GameConfig.h"
#include "Obstacle.h"
#include "Sprite.h"
#include "Bird.h"
#include "../UI/NetworkManager.h"

class GameObjects {
private:
    // Игровые объекты
    std::vector<Point> snake;
    std::vector<Point> food;
    std::vector<Obstacle> obstacles;
    std::vector<Point> fenceBlocks;

    // Декорации
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

    // Система рекордов и времени
    std::vector<HighScore> highScores;
    int gameDuration;
    float gameTimer;

    // Настройки игрока
    std::string playerName;
    std::vector<float> speedMultipliers;
    int currentSpeedIndex;
    NetworkManager networkManager;

    // Имена файлов
    const std::string settingsFileName = "settings.dat";
    const std::string saveFileName = "savegame.dat";

    // Внутренние методы
    void handleGameOver();
    void updateGameSpeedFromMultiplier();

public:
    GameObjects();
    ~GameObjects();

    // Основные методы игры
    void initGame();
    void update();
    void saveOnExit();
    void pauseGame();
    void resumeGame();
    void returnToMainMenu();
    void reset();

    // Управление скоростью
    float getSpeedMultiplier() const;
    std::string getSpeedDisplayText() const;
    void increaseSpeed();
    void decreaseSpeed();

    // Система сохранения/загрузки
    void saveSettings();
    void loadSettings();
    bool saveGame();
    bool loadGame();
    bool hasSaveGame() const;
    void deleteSaveGame();

    // Система рекордов
    void loadHighScores();
    void refreshHighScores();
    void updateHighScores();
    bool isNewHighScore(int score) const;
    bool isNetworkAvailable() const;

    // Генерация игрового мира
    void generateFence();
    void generateClouds();
    void generateBirds();
    void generateGroundSprites();
    void generateObstacles();
    void generateSingleFood();
    void generateInitialFood();

    // Обновление декораций
    void updateClouds();
    void updateBirds();

    // Обработка ввода
    void handleGameKeyPress(int key);
    void handleSettingsKeyPress(int key);
    void handleMenuKeyPress(int key);

    // Отладочные методы
    void debugSnakeInfo();
    void printDebugInfo() const;
    std::string gameStateToString(GameState state) const;

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
    bool isPaused() const { return gameState == PAUSED; }
    GameState getGameState() const { return gameState; }
    float getGameSpeed() const { return gameSpeed; }
    GameState getPreviousState() const { return previousState; }
    const std::vector<HighScore>& getHighScores() const { return highScores; }
    const std::string& getPlayerName() const { return playerName; }
    int getGameDuration() const { return gameDuration; }

    // Сеттеры
    void setCurrentDirection(Direction direction) { currentDirection = direction; }
    void setVerticalDirection(int direction) { verticalDirection = direction; }
    void setScore(int newScore) { score = newScore; }
    void setGameOver(bool over) { gameOver = over; }
    void setGameState(GameState state) {
        previousState = gameState;
        gameState = state;
    }
    void setGameSpeed(float speed) { gameSpeed = speed; }
    void setPlayerName(const std::string& name);
};