#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "Obstacle.h"
#include "Sprite.h"
#include "Bird.h"
#include "../UI/NetworkManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

class GameObjects {
private:
    // === »√–Œ¬€≈ Œ¡⁄≈ “€ ===
    std::vector<Point> snake;
    std::vector<Point> food;
    std::vector<Obstacle> obstacles;
    std::vector<Point> fenceBlocks;

    // === ƒ≈ Œ–¿÷»» ===
    std::vector<Sprite> cloudSprites;
    std::vector<Bird> birds;
    std::vector<Sprite> flowerSprites;

    // === —Œ—“ŒﬂÕ»ﬂ »√–€ ===
    bool nameInputActive = false;
    Direction currentDirection;
    int verticalDirection;
    int score;
    bool gameOver;
    GameState gameState;
    float gameSpeed;
    GameState previousState;
    bool gameFrozen;

    // === –≈ Œ–ƒ€ » ¬–≈Ãﬂ ===
    std::vector<HighScore> highScores;
    static const int MAX_HIGH_SCORES = 10;
    int gameDuration;
    float gameTimer;

    // === Õ¿—“–Œ… » —≈“ » (»«  ŒÕ‘»√¿) ===
    int gridWidth;
    int gridDepth;
    float cellSize;
    int initialFoodCount;
    int obstacleCount;
    bool gridEnabled;
    float gridLineWidth;
    int floorTileSize;

    // === Õ¿—“–Œ… » «Ã≈… » (»«  ŒÕ‘»√¿) ===
    std::string snakeHeadModel;
    std::string snakeBodyModel;
    std::string snakeTailModel;
    glm::vec3 snakeHeadColor;
    glm::vec3 snakeBodyColor;
    glm::vec3 snakeTailColor;
    float snakeHeadScale;
    float snakeBodyScale;
    float snakeTailScale;

    // === Õ¿—“–Œ… » Œ –”∆≈Õ»ﬂ (»«  ŒÕ‘»√¿) ===
    int cloudCount;
    int birdCount;
    int flowerCount;

    // === ÃŒƒ≈À» Œ –”∆≈Õ»ﬂ (»«  ŒÕ‘»√¿) ===
    std::string appleModel;
    std::string treeModel;
    std::string cloudModel;
    std::string birdModel;
    std::string flowerModel;
    std::string fenceModel;
    std::string rockModel;
    std::string grassModel;

    // === Õ¿—“–Œ… » œŒÀ¿ (»«  ŒÕ‘»√¿) ===
    std::string floorModel;
    glm::vec3 floorColor;
    float floorScale;
    bool useFloorTexture;
    std::string floorTexture;
    glm::vec3 floorPosition;

    // === Õ¿—“–Œ… » Õ≈¡¿ » —≈“ » (»«  ŒÕ‘»√¿) ===
    glm::vec3 skyColor;
    glm::vec3 gridColor;

    // === ÕŒ¬€≈ œ¿–¿Ã≈“–€: ÷¬≈“¿ » Ã¿—ÿ“¿¡€ œ–≈œﬂ“—“¬»… (»«  ŒÕ‘»√¿) ===
    glm::vec3 treeColor;
    glm::vec3 rockColor;
    glm::vec3 fenceColor;
    glm::vec3 appleColor;

    float treeScale;
    float rockScale;
    float fenceScale;
    float appleScale;

    // === ÕŒ¬€≈ œ¿–¿Ã≈“–€: ÷¬≈“¿ » Ã¿—ÿ“¿¡€ Œ –”∆≈Õ»ﬂ (»«  ŒÕ‘»√¿) ===
    glm::vec3 cloudColor;
    glm::vec3 birdColor;
    glm::vec3 flowerColor;

    float cloudScale;
    float birdScale;
    float flowerScale;

    // === Õ¿—“–Œ… » »√–Œ ¿ ===
    std::string playerName;
    std::vector<float> speedMultipliers;
    int currentSpeedIndex;
    NetworkManager networkManager;
    std::string settingsFileName = "settings.dat";
    std::string saveFileName = "savegame.dat";

public:
    GameObjects();

    // === ”œ–¿¬À≈Õ»≈ —Œ—“ŒﬂÕ»≈Ã ===
    void toggleFreeze() { gameFrozen = !gameFrozen; }
    bool isFrozen() const { return gameFrozen; }
    void pauseGame();
    void returnToMainMenu();

    // === »Õ»÷»¿À»«¿÷»ﬂ ===
    void initGame();
    void update();
    int getFloorTileSize() const { return floorTileSize; }
    void setFloorTileSize(int size) { floorTileSize = size; }

    // === √≈““≈–€ »√–Œ¬€’ Œ¡⁄≈ “Œ¬ ===
    const std::vector<Point>& getSnake() const { return snake; }
    const std::vector<Point>& getFood() const { return food; }
    const std::vector<Obstacle>& getObstacles() const { return obstacles; }
    const std::vector<Point>& getFenceBlocks() const { return fenceBlocks; }
    const std::vector<Sprite>& getCloudSprites() const { return cloudSprites; }
    const std::vector<Bird>& getBirds() const { return birds; }
    const std::vector<Sprite>& getFlowerSprites() const { return flowerSprites; }

    // === √≈““≈–€ —Œ—“ŒﬂÕ»ﬂ ===
    int getScore() const { return score; }
    bool isGameOver() const { return gameOver; }
    GameState getGameState() const { return gameState; }
    GameState getPreviousState() const { return previousState; }
    float getGameSpeed() const { return gameSpeed; }
    int getGameDuration() const { return gameDuration; }
    Direction getCurrentDirection() const { return currentDirection; }
    int getVerticalDirection() const { return verticalDirection; }

    // === √≈““≈–€ Õ¿—“–Œ≈  —≈“ » ===
    int getGridWidth() const { return gridWidth; }
    int getGridDepth() const { return gridDepth; }
    float getCellSize() const { return cellSize; }
    int getInitialFoodCount() const { return initialFoodCount; }
    int getObstacleCount() const { return obstacleCount; }
    bool isGridEnabled() const { return gridEnabled; }
    float getGridLineWidth() const { return gridLineWidth; }

    // === √≈““≈–€ «Ã≈… » ===
    const std::string& getSnakeHeadModel() const { return snakeHeadModel; }
    const std::string& getSnakeBodyModel() const { return snakeBodyModel; }
    const std::string& getSnakeTailModel() const { return snakeTailModel; }
    const glm::vec3& getSnakeHeadColor() const { return snakeHeadColor; }
    const glm::vec3& getSnakeBodyColor() const { return snakeBodyColor; }
    const glm::vec3& getSnakeTailColor() const { return snakeTailColor; }
    float getSnakeHeadScale() const { return snakeHeadScale; }
    float getSnakeBodyScale() const { return snakeBodyScale; }
    float getSnakeTailScale() const { return snakeTailScale; }
    float getSnakeScale() const { return snakeHeadScale; }

    // === √≈““≈–€ Œ –”∆≈Õ»ﬂ (ÍÓÎË˜ÂÒÚ‚Ó) ===
    int getCloudCount() const { return cloudCount; }
    int getBirdCount() const { return birdCount; }
    int getFlowerCount() const { return flowerCount; }

    // === √≈““≈–€ ÃŒƒ≈À≈… Œ –”∆≈Õ»ﬂ ===
    const std::string& getAppleModel() const { return appleModel; }
    const std::string& getTreeModel() const { return treeModel; }
    const std::string& getCloudModel() const { return cloudModel; }
    const std::string& getBirdModel() const { return birdModel; }
    const std::string& getFlowerModel() const { return flowerModel; }
    const std::string& getFenceModel() const { return fenceModel; }
    const std::string& getRockModel() const { return rockModel; }
    const std::string& getGrassModel() const { return grassModel; }

    // === √≈““≈–€ œŒÀ¿ ===
    const std::string& getFloorModel() const { return floorModel; }
    const glm::vec3& getFloorColor() const { return floorColor; }
    float getFloorScale() const { return floorScale; }
    bool isUsingFloorTexture() const { return useFloorTexture; }
    const std::string& getFloorTexture() const { return floorTexture; }
    const glm::vec3& getFloorPosition() const { return floorPosition; }

    // === √≈““≈–€ ÷¬≈“Œ¬ ===
    const glm::vec3& getSkyColor() const { return skyColor; }
    const glm::vec3& getGridColor() const { return gridColor; }

    // === ÕŒ¬€≈ √≈““≈–€: œ–≈œﬂ“—“¬»ﬂ ===
    const glm::vec3& getTreeColor() const { return treeColor; }
    float getTreeScale() const { return treeScale; }
    const glm::vec3& getRockColor() const { return rockColor; }
    float getRockScale() const { return rockScale; }
    const glm::vec3& getFenceColor() const { return fenceColor; }
    float getFenceScale() const { return fenceScale; }
    const glm::vec3& getAppleColor() const { return appleColor; }
    float getAppleScale() const { return appleScale; }

    // === ÕŒ¬€≈ √≈““≈–€: Œ –”∆≈Õ»≈ ===
    const glm::vec3& getCloudColor() const { return cloudColor; }
    float getCloudScale() const { return cloudScale; }
    const glm::vec3& getBirdColor() const { return birdColor; }
    float getBirdScale() const { return birdScale; }
    const glm::vec3& getFlowerColor() const { return flowerColor; }
    float getFlowerScale() const { return flowerScale; }

    // === —≈““≈–€ —Œ—“ŒﬂÕ»ﬂ ===
    void setGameState(GameState state) { gameState = state; }
    void setPreviousState(GameState state) { previousState = state; }
    void setGameSpeed(float speed) { gameSpeed = speed; }
    void setCurrentDirection(Direction direction) { currentDirection = direction; }
    void setVerticalDirection(int direction) { verticalDirection = direction; }
    void setScore(int newScore) { score = newScore; }
    void setGameOver(bool over) { gameOver = over; }

    // === —≈““≈–€ Õ¿—“–Œ≈  —≈“ » ===
    void setGridWidth(int width) { gridWidth = width; }
    void setGridDepth(int depth) { gridDepth = depth; }
    void setCellSize(float size) { cellSize = size; }
    void setInitialFoodCount(int count) { initialFoodCount = count; }
    void setObstacleCount(int count) { obstacleCount = count; }
    void setGridEnabled(bool enabled) { gridEnabled = enabled; }
    void setGridLineWidth(float width) { gridLineWidth = width; }

    // === —≈““≈–€ «Ã≈… » ===
    void setSnakeModels(const std::string& head, const std::string& body, const std::string& tail);
    void setSnakeColors(const glm::vec3& head, const glm::vec3& body, const glm::vec3& tail);
    void setSnakeScales(float headScale, float bodyScale, float tailScale);
    void setSnakeScale(float scale) { snakeHeadScale = snakeBodyScale = snakeTailScale = scale; }

    // === —≈““≈–€ Œ –”∆≈Õ»ﬂ (ÍÓÎË˜ÂÒÚ‚Ó) ===
    void setCloudCount(int count);
    void setBirdCount(int count);
    void setFlowerCount(int count);

    // === —≈““≈–€ ÃŒƒ≈À≈… Œ –”∆≈Õ»ﬂ ===
    void setAppleModel(const std::string& model) { appleModel = model; }
    void setTreeModel(const std::string& model) { treeModel = model; }
    void setCloudModel(const std::string& model) { cloudModel = model; }
    void setBirdModel(const std::string& model) { birdModel = model; }
    void setFlowerModel(const std::string& model) { flowerModel = model; }
    void setFenceModel(const std::string& model) { fenceModel = model; }
    void setRockModel(const std::string& model) { rockModel = model; }
    void setGrassModel(const std::string& model) { grassModel = model; }

    // === ÕŒ¬€≈ —≈““≈–€: œ–≈œﬂ“—“¬»ﬂ ===
    void setTreeColor(const glm::vec3& color);
    void setTreeScale(float scale);
    void setRockColor(const glm::vec3& color);
    void setRockScale(float scale);
    void setFenceColor(const glm::vec3& color);
    void setFenceScale(float scale);
    void setAppleColor(const glm::vec3& color);
    void setAppleScale(float scale);

    // === ÕŒ¬€≈ —≈““≈–€: Œ –”∆≈Õ»≈ ===
    void setCloudColor(const glm::vec3& color);
    void setCloudScale(float scale);
    void setBirdColor(const glm::vec3& color);
    void setBirdScale(float scale);
    void setFlowerColor(const glm::vec3& color);
    void setFlowerScale(float scale);

    // === —≈““≈–€ œŒÀ¿ ===
    void setFloorModel(const std::string& model);
    void setFloorColor(const glm::vec3& color);
    void setFloorScale(float scale);
    void setFloorTexture(const std::string& texture);
    void setFloorPosition(const glm::vec3& pos);

    // === —≈““≈–€ ÷¬≈“Œ¬ ===
    void setSkyColor(const glm::vec3& color) { skyColor = color; }
    void setGridColor(const glm::vec3& color) { gridColor = color; }

    // === Õ¿—“–Œ… » — Œ–Œ—“» ===
    void increaseSpeed();
    void decreaseSpeed();
    void updateGameSpeedFromMultiplier();
    float getSpeedMultiplier() const;
    std::string getSpeedDisplayText() const;

    // === Õ¿—“–Œ… » »√–Œ ¿ ===
    const std::string& getPlayerName() const { return playerName; }
    void setPlayerName(const std::string& name);
    bool isNameInputActive() const { return nameInputActive; }
    void setNameInputActive(bool active) { nameInputActive = active; }
    void handleNameInput(int key);
    void addCharacterToName(char c);
    void removeLastCharacterFromName();

    // === √≈Õ≈–¿÷»ﬂ Œ¡⁄≈ “Œ¬ ===
    void generateFence();
    void generateClouds();
    void generateBirds();
    void generateGroundSprites();
    void generateObstacles();
    void generateSingleFood();
    void generateInitialFood();

    // === Œ¡ÕŒ¬À≈Õ»≈ Œ¡⁄≈ “Œ¬ ===
    void updateClouds();
    void updateBirds();

    // === Œ¡–¿¡Œ“ ¿ ¬¬Œƒ¿ ===
    void handleGameKeyPress(int key);
    void handleSettingsKeyPress(int key);
    void handleMenuKeyPress(int key);

    // === –≈ Œ–ƒ€ ===
    void loadHighScores();
    void saveHighScore();
    void saveHighScoreToServer();
    void updateHighScores();
    bool isNewHighScore(int score) const;
    void addHighScore(const std::string& playerName, int score);
    const std::vector<HighScore>& getHighScores() const { return highScores; }

    // === —Œ’–¿Õ≈Õ»≈/«¿√–”« ¿ ===
    bool saveGame();
    bool loadGame();
    bool hasSaveGame() const;
    void deleteSaveGame();
    void saveSettings();
    void loadSettings();
    void saveOnExit();

    // === Œ“À¿ƒ ¿ ===
    void debugSnakeInfo() {}
};