#include "../pch.h"
#include "GameObjects.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

extern ShaderManager g_shaderManager;
extern Camera g_camera;

//=============================================================================
// КОНСТРУКТОР - здесь только минимальные значения по умолчанию
//=============================================================================
GameObjects::GameObjects()
    : gameFrozen(false),
    currentDirection(FORWARD),
    verticalDirection(0),
    score(0),
    gameOver(false),
    gameState(MAIN_MENU),
    previousState(MAIN_MENU),
    gameSpeed(0.12f),
    playerName("Player"),
    gameDuration(0),
    gameTimer(0.0f),
    nameInputActive(false),

    // Эти значения будут перезаписаны из конфига в Game::loadConfig()
    snakeHeadModel("snake_head.obj"),
    snakeBodyModel("snake_body.obj"),
    snakeTailModel("snake_tail.obj"),
    snakeHeadColor(0.0f, 1.0f, 0.0f),
    snakeBodyColor(0.0f, 0.7f, 0.0f),
    snakeTailColor(0.0f, 0.5f, 0.0f),
    snakeScale(0.8f),

    // Модели окружения
    appleModel("apple.fbx"),
    treeModel("tree.fbx"),
    cloudModel("cloud.fbx"),
    birdModel("bird.fbx"),
    flowerModel("flower.fbx"),

    cloudCount(20),
    birdCount(15),
    flowerCount(25),
    floorModel("floor.obj"),
    floorColor(0.3f, 0.6f, 0.2f),
    floorScale(1.0f),
    useFloorTexture(false),
    floorTexture(""),
    floorPosition(0.0f, -0.5f, 0.0f) {

    speedMultipliers = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f };
    currentSpeedIndex = 3;

    loadSettings(); // загружает настройки игрока (имя, скорость)
    updateGameSpeedFromMultiplier();
    loadHighScores(); // загружает рекорды с сервера
}

//=============================================================================
// НАСТРОЙКИ СКОРОСТИ
//=============================================================================
void GameObjects::updateGameSpeedFromMultiplier() {
    float baseSpeed = 0.12f;
    gameSpeed = baseSpeed / speedMultipliers[currentSpeedIndex];
}

float GameObjects::getSpeedMultiplier() const {
    return speedMultipliers[currentSpeedIndex];
}

std::string GameObjects::getSpeedDisplayText() const {
    float multiplier = speedMultipliers[currentSpeedIndex];
    if (multiplier >= 1.0f) {
        return "x" + std::to_string((int)multiplier);
    }
    else {
        return "x" + std::to_string(multiplier).substr(0, 4);
    }
}

void GameObjects::increaseSpeed() {
    if (currentSpeedIndex < speedMultipliers.size() - 1) {
        currentSpeedIndex++;
        updateGameSpeedFromMultiplier();
        saveSettings();
    }
}

void GameObjects::decreaseSpeed() {
    if (currentSpeedIndex > 0) {
        currentSpeedIndex--;
        updateGameSpeedFromMultiplier();
        saveSettings();
    }
}

//=============================================================================
// СОХРАНЕНИЕ/ЗАГРУЗКА НАСТРОЕК ИГРОКА
//=============================================================================
struct SettingsData {
    int version = 1;
    float gameSpeed;
    int currentSpeedIndex;
    char playerName[32];
};

void GameObjects::saveSettings() {
    std::ofstream file(settingsFileName, std::ios::binary);
    if (!file.is_open()) return;

    SettingsData settings;
    settings.version = 1;
    settings.gameSpeed = gameSpeed;
    settings.currentSpeedIndex = currentSpeedIndex;
    strncpy_s(settings.playerName, playerName.c_str(), 31);
    settings.playerName[31] = '\0';

    file.write(reinterpret_cast<char*>(&settings), sizeof(SettingsData));
    file.close();

    std::cout << "Settings saved" << std::endl;
}

void GameObjects::loadSettings() {
    std::ifstream file(settingsFileName, std::ios::binary);
    if (!file.is_open()) return;

    SettingsData settings;
    file.read(reinterpret_cast<char*>(&settings), sizeof(SettingsData));
    file.close();

    if (settings.version == 1) {
        gameSpeed = settings.gameSpeed;
        currentSpeedIndex = settings.currentSpeedIndex;
        playerName = std::string(settings.playerName);
        std::cout << " Settings loaded" << std::endl;
    }
}

void GameObjects::setPlayerName(const std::string& name) {
    playerName = name;
    saveSettings();
}

//=============================================================================
// УПРАВЛЕНИЕ СОСТОЯНИЕМ
//=============================================================================
void GameObjects::pauseGame() {
    if (gameState == PLAYING) {
        gameState = PAUSED;
    }
}

void GameObjects::returnToMainMenu() {
    if (gameState == PLAYING && !gameOver) {
        saveGame();
        std::cout << " Game saved when returning to main menu" << std::endl;
    }
    gameState = MAIN_MENU;
}

void GameObjects::saveOnExit() {
    if (gameState == PLAYING && !gameOver) {
        saveGame();
        std::cout << " Game saved on exit" << std::endl;
    }
    saveSettings();
    std::cout << " Settings saved on exit" << std::endl;
}

void GameObjects::handleSettingsKeyPress(int key) {
    switch (key) {
    case GLFW_KEY_EQUAL:
    case GLFW_KEY_RIGHT:
        increaseSpeed();
        break;
    case GLFW_KEY_MINUS:
    case GLFW_KEY_LEFT:
        decreaseSpeed();
        break;
    }
}

//=============================================================================
// ИНИЦИАЛИЗАЦИЯ ИГРЫ
//=============================================================================
void GameObjects::initGame() {
    snake.clear();
    snake.push_back(Point(GRID_WIDTH / 2, 0, GRID_DEPTH / 2));
    snake.push_back(Point(GRID_WIDTH / 2 - 1, 0, GRID_DEPTH / 2));
    snake.push_back(Point(GRID_WIDTH / 2 - 2, 0, GRID_DEPTH / 2));

    generateFence();
    generateInitialFood();
    generateObstacles();
    generateClouds();
    generateBirds();
    generateGroundSprites();

    currentDirection = FORWARD;
    verticalDirection = 0;
    score = 0;
    gameOver = false;
    gameDuration = 0;
    gameTimer = 0.0f;

    g_camera.setTargetDistance(5.0f);

    std::cout << "Game initialized with config values:" << std::endl;
    std::cout << "  Cloud count: " << cloudCount << std::endl;
    std::cout << "  Bird count: " << birdCount << std::endl;
    std::cout << "  Flower count: " << flowerCount << std::endl;
    std::cout << "  Floor model: " << floorModel << std::endl;
}

//=============================================================================
// ОБНОВЛЕНИЕ ИГРЫ
//=============================================================================
void GameObjects::update() {
    if (gameOver || gameState != PLAYING) return;

    static float lastMoveTime = 0;
    float currentTime = glfwGetTime();

    if (currentTime - lastMoveTime < gameSpeed) {
        updateClouds();
        updateBirds();
        return;
    }
    lastMoveTime = currentTime;

    gameTimer += gameSpeed;
    if (gameTimer >= 1.0f) {
        gameDuration++;
        gameTimer = 0.0f;
    }

    if (gameFrozen) {
        updateClouds();
        updateBirds();
        return;
    }

    Point newHead = snake[0];
    switch (currentDirection) {
    case FORWARD: newHead.z++; break;
    case BACKWARD: newHead.z--; break;
    case RIGHT: newHead.x--; break;
    case LEFT: newHead.x++; break;
    }
    newHead.y = 0;

    // Проверка столкновения со стенками
    if (newHead.x <= 0 || newHead.x >= GRID_WIDTH - 1 ||
        newHead.z <= 0 || newHead.z >= GRID_DEPTH - 1) {
        gameOver = true;
        gameState = GAME_OVER;
        updateHighScores();
        return;
    }

    // Проверка столкновения с собой
    for (const auto& segment : snake) {
        if (segment == newHead) {
            gameOver = true;
            gameState = GAME_OVER;
            updateHighScores();
            return;
        }
    }

    // Проверка столкновения с препятствиями
    for (const auto& obstacle : obstacles) {
        if (obstacle.contains(newHead)) {
            gameOver = true;
            gameState = GAME_OVER;
            updateHighScores();
            return;
        }
    }

    snake.insert(snake.begin(), newHead);

    auto foodIt = std::find(food.begin(), food.end(), newHead);
    if (foodIt != food.end()) {
        score++;
        food.erase(foodIt);
        generateSingleFood();
    }
    else {
        snake.pop_back();
    }

    updateClouds();
    updateBirds();
}

//=============================================================================
// ГЕНЕРАЦИЯ ОБЪЕКТОВ (ИСПОЛЬЗУЮТ ЗНАЧЕНИЯ ИЗ КОНФИГА)
//=============================================================================
void GameObjects::generateClouds() {
    cloudSprites.clear();
    std::cout << "Generating " << cloudCount << " clouds (from config)" << std::endl;

    float gameFieldMinX = -GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMaxX = GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMinZ = -GRID_DEPTH * CELL_SIZE * 0.5f;
    float gameFieldMaxZ = GRID_DEPTH * CELL_SIZE * 0.5f;
    float safeDistance = 2.0f;

    for (int i = 0; i < cloudCount; i++) {
        glm::vec3 position;
        bool validPosition = false;
        int attempts = 0;

        while (!validPosition && attempts < 30) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float minDistance = 10.0f;
            float maxDistance = 16.0f;
            float distance = minDistance + (rand() % (int)(maxDistance - minDistance));

            position.x = cos(angle) * distance;
            position.z = sin(angle) * distance;
            position.y = 2.5f + (rand() % 10) * 0.1f;

            bool outsideX = position.x < gameFieldMinX - safeDistance || position.x > gameFieldMaxX + safeDistance;
            bool outsideZ = position.z < gameFieldMinZ - safeDistance || position.z > gameFieldMaxZ + safeDistance;

            if (outsideX || outsideZ) {
                validPosition = true;
            }

            attempts++;
        }

        float size = 0.4f + (rand() % 8) * 0.1f;
        float speed = 0.08f + (rand() % 6) * 0.01f;

        int cloudType = rand() % 3;
        glm::vec3 color;
        switch (cloudType) {
        case 0: color = glm::vec3(0.95f, 0.95f, 0.95f); break;
        case 1: color = glm::vec3(0.85f, 0.85f, 0.85f); break;
        case 2: color = glm::vec3(0.90f, 0.90f, 0.92f); break;
        }

        cloudSprites.push_back(Sprite(position, color, size, speed));
    }
}

void GameObjects::generateBirds() {
    birds.clear();
    std::cout << "Generating " << birdCount << " birds (from config)" << std::endl;

    float gameFieldMinX = -GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMaxX = GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMinZ = -GRID_DEPTH * CELL_SIZE * 0.5f;
    float gameFieldMaxZ = GRID_DEPTH * CELL_SIZE * 0.5f;
    float safeDistance = 1.5f;

    for (int i = 0; i < birdCount; i++) {
        glm::vec3 position;
        bool validPosition = false;
        int attempts = 0;

        while (!validPosition && attempts < 30) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float minDistance = 8.0f;
            float maxDistance = 12.0f;
            float distance = minDistance + (rand() % (int)(maxDistance - minDistance));

            position.x = cos(angle) * distance;
            position.z = sin(angle) * distance;
            position.y = 3.0f + (rand() % 8) * 0.1f;

            bool outsideX = position.x < gameFieldMinX - safeDistance || position.x > gameFieldMaxX + safeDistance;
            bool outsideZ = position.z < gameFieldMinZ - safeDistance || position.z > gameFieldMaxZ + safeDistance;

            if (outsideX || outsideZ) {
                validPosition = true;
            }
            attempts++;
        }

        float size = 0.06f + (rand() % 6) * 0.02f;
        float speed = 0.12f + (rand() % 8) * 0.02f;

        float currentAngle = atan2f(position.z, position.x);
        float movementAngle = currentAngle + 3.14159f / 2.0f;
        if (rand() % 2 == 0) movementAngle += 3.14159f;

        glm::vec3 direction = glm::vec3(cos(movementAngle), 0.0f, sin(movementAngle));

        int birdType = rand() % 4;
        glm::vec3 color;
        switch (birdType) {
        case 0: color = glm::vec3(0.1f, 0.1f, 0.3f); break;
        case 1: color = glm::vec3(0.3f, 0.2f, 0.1f); break;
        case 2: color = glm::vec3(0.8f, 0.8f, 0.9f); break;
        case 3: color = glm::vec3(0.2f, 0.2f, 0.2f); break;
        }

        Bird bird(position, color, size, speed);
        bird.direction = direction;
        birds.push_back(bird);
    }
}

void GameObjects::generateGroundSprites() {
    flowerSprites.clear();
    std::cout << "Generating " << flowerCount << " flowers (from config)" << std::endl;

    for (int i = 0; i < flowerCount; i++) {
        float x = (rand() % 200 - 100) * 0.1f;
        float y = 0.01f;
        float z = (rand() % 200 - 100) * 0.1f;
        float size = 0.1f + (rand() % 5) * 0.02f;

        int colorType = rand() % 4;
        glm::vec3 color;
        switch (colorType) {
        case 0: color = glm::vec3(1.0f, 0.2f, 0.2f); break;
        case 1: color = glm::vec3(0.2f, 0.2f, 1.0f); break;
        case 2: color = glm::vec3(1.0f, 0.8f, 0.2f); break;
        case 3: color = glm::vec3(0.8f, 0.2f, 0.8f); break;
        }

        flowerSprites.push_back(Sprite(glm::vec3(x, y, z), color, size, 0.0f));
    }
}

void GameObjects::generateFence() {
    fenceBlocks.clear();

    fenceBlocks.push_back(Point(0, 0, 0));
    fenceBlocks.push_back(Point(GRID_WIDTH - 1, 0, 0));
    fenceBlocks.push_back(Point(0, 0, GRID_DEPTH - 1));
    fenceBlocks.push_back(Point(GRID_WIDTH - 1, 0, GRID_DEPTH - 1));

    for (int x = 1; x < GRID_WIDTH - 1; x++) {
        fenceBlocks.push_back(Point(x, 0, 0));
    }
    for (int x = 1; x < GRID_WIDTH - 1; x++) {
        fenceBlocks.push_back(Point(x, 0, GRID_DEPTH - 1));
    }
    for (int z = 1; z < GRID_DEPTH - 1; z++) {
        fenceBlocks.push_back(Point(0, 0, z));
    }
    for (int z = 1; z < GRID_DEPTH - 1; z++) {
        fenceBlocks.push_back(Point(GRID_WIDTH - 1, 0, z));
    }
}

void GameObjects::generateObstacles() {
    obstacles.clear();
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        Point center;
        bool validPosition = false;
        int attempts = 0;

        do {
            center.x = 10 + rand() % (GRID_WIDTH - 20);
            center.y = 0;
            center.z = 10 + rand() % (GRID_DEPTH - 20);

            validPosition = true;
            Obstacle tempObstacle(center);

            for (const auto& block : tempObstacle.blocks) {
                if (block.x < 0 || block.x >= GRID_WIDTH || block.z < 0 || block.z >= GRID_DEPTH) {
                    validPosition = false;
                    break;
                }

                for (const auto& segment : snake) {
                    if (segment == block) {
                        validPosition = false;
                        break;
                    }
                }

                for (const auto& apple : food) {
                    if (apple == block) {
                        validPosition = false;
                        break;
                    }
                }

                for (const auto& fenceBlock : fenceBlocks) {
                    if (fenceBlock == block) {
                        validPosition = false;
                        break;
                    }
                }

                for (const auto& existingObstacle : obstacles) {
                    if (existingObstacle.contains(block)) {
                        validPosition = false;
                        break;
                    }
                }

                if (!validPosition) break;
            }

            attempts++;
            if (attempts > 100) break;

        } while (!validPosition);

        if (validPosition) {
            obstacles.push_back(Obstacle(center));
        }
    }
}

void GameObjects::generateSingleFood() {
    Point newFood;
    bool validPosition = false;
    int attempts = 0;

    do {
        newFood.x = 5 + rand() % (GRID_WIDTH - 10);
        newFood.y = 0;
        newFood.z = 5 + rand() % (GRID_DEPTH - 10);

        validPosition = true;

        for (const auto& segment : snake) {
            if (segment == newFood) {
                validPosition = false;
                break;
            }
        }

        for (const auto& apple : food) {
            if (apple == newFood) {
                validPosition = false;
                break;
            }
        }

        for (const auto& obstacle : obstacles) {
            if (obstacle.contains(newFood)) {
                validPosition = false;
                break;
            }
        }

        for (const auto& fenceBlock : fenceBlocks) {
            if (fenceBlock == newFood) {
                validPosition = false;
                break;
            }
        }

        attempts++;
        if (attempts > 50) break;

    } while (!validPosition);

    if (validPosition) {
        food.push_back(newFood);
    }
}

void GameObjects::generateInitialFood() {
    food.clear();
    for (int i = 0; i < INITIAL_FOOD_COUNT; i++) {
        generateSingleFood();
    }
}

//=============================================================================
// ОБНОВЛЕНИЕ ДЕКОРАЦИЙ
//=============================================================================
void GameObjects::updateClouds() {
    for (auto& cloud : cloudSprites) {
        float currentAngle = atan2f(cloud.position.z, cloud.position.x);
        float movementAngle = currentAngle + 3.14159f / 2.0f;

        cloud.position.x += cos(movementAngle) * cloud.speed * 0.01f;
        cloud.position.z += sin(movementAngle) * cloud.speed * 0.01f;
        cloud.position.y += (rand() % 100 - 50) * 0.0001f;

        if (cloud.position.y < 2.3f) cloud.position.y = 2.3f;
        if (cloud.position.y > 3.7f) cloud.position.y = 3.7f;

        float currentDistance = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        float targetDistance = 13.0f;

        if (currentDistance < 9.0f || currentDistance > 17.0f) {
            glm::vec3 normalized = glm::normalize(cloud.position);
            cloud.position = normalized * targetDistance;
            cloud.position.y = 2.5f + (rand() % 10) * 0.1f;
        }
    }
}

void GameObjects::updateBirds() {
    for (auto& bird : birds) {
        bird.wingAngle = sin(glfwGetTime() * bird.wingSpeed) * 0.5f;
        bird.position += bird.direction * bird.speed * 0.02f;
        bird.position.y += sin(glfwGetTime() * 2.0f + bird.position.x) * 0.005f;

        if (bird.position.y < 2.8f) bird.position.y = 2.8f;
        if (bird.position.y > 4.0f) bird.position.y = 4.0f;

        float currentDistance = glm::length(glm::vec2(bird.position.x, bird.position.z));
        float targetDistance = 10.0f;

        if (currentDistance < 6.0f || currentDistance > 14.0f) {
            glm::vec3 normalized = glm::normalize(bird.position);
            bird.position = normalized * targetDistance;
            bird.position.y = 3.0f + (rand() % 8) * 0.1f;

            float currentAngle = atan2f(bird.position.z, bird.position.x);
            float movementAngle = currentAngle + 3.14159f / 2.0f;
            if (rand() % 2 == 0) movementAngle += 3.14159f;

            bird.direction = glm::vec3(cos(movementAngle), 0.0f, sin(movementAngle));
        }

        if (rand() % 300 < 1) {
            float currentAngle = atan2f(bird.direction.z, bird.direction.x);
            float randomChange = (rand() % 20 - 10) * 3.14159f / 180.0f;
            bird.direction = glm::vec3(cos(currentAngle + randomChange), 0.0f, sin(currentAngle + randomChange));
        }
    }
}

//=============================================================================
// ОБРАБОТКА ВВОДА
//=============================================================================
void GameObjects::handleGameKeyPress(int key) {
    switch (key) {
    case GLFW_KEY_S:
        gameFrozen = !gameFrozen;
        std::cout << "Game " << (gameFrozen ? "FROZEN" : "UNFROZEN") << std::endl;
        break;
    case GLFW_KEY_LEFT:
        if (currentDirection == FORWARD) currentDirection = LEFT;
        else if (currentDirection == LEFT) currentDirection = BACKWARD;
        else if (currentDirection == BACKWARD) currentDirection = RIGHT;
        else if (currentDirection == RIGHT) currentDirection = FORWARD;
        break;
    case GLFW_KEY_RIGHT:
        if (currentDirection == FORWARD) currentDirection = RIGHT;
        else if (currentDirection == RIGHT) currentDirection = BACKWARD;
        else if (currentDirection == BACKWARD) currentDirection = LEFT;
        else if (currentDirection == LEFT) currentDirection = FORWARD;
        break;
    case GLFW_KEY_P:
        gameState = PAUSED;
        break;
    case GLFW_KEY_R:
        initGame();
        gameState = PLAYING;
        break;
    case GLFW_KEY_Q:
        g_camera.rotate(-10.0f);
        break;
    case GLFW_KEY_E:
        g_camera.rotate(10.0f);
        break;
    }
}

void GameObjects::handleMenuKeyPress(int key) {
    switch (gameState) {
    case MAIN_MENU:
        switch (key) {
        case GLFW_KEY_1:
            gameState = PLAYING;
            initGame();
            break;
        case GLFW_KEY_2:
            previousState = MAIN_MENU;
            gameState = SETTINGS;
            break;
        case GLFW_KEY_3:
            previousState = MAIN_MENU;
            gameState = HIGH_SCORES;
            break;
        case GLFW_KEY_4:
            gameState = CONTROLS;
            break;
        }
        break;

    case PAUSED:
        switch (key) {
        case GLFW_KEY_ESCAPE:
            gameState = PLAYING;
            break;
        case GLFW_KEY_Q:
            previousState = PAUSED;
            gameState = SETTINGS;
            break;
        case GLFW_KEY_M:
            gameState = MAIN_MENU;
            break;
        case GLFW_KEY_P:
            gameState = PLAYING;
            break;
        }
        break;

    case SETTINGS:
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_B || key == GLFW_KEY_BACKSPACE) {
            gameState = previousState;
        }
        break;

    case HIGH_SCORES:
        if (key == GLFW_KEY_B || key == GLFW_KEY_ESCAPE || key == GLFW_KEY_BACKSPACE) {
            gameState = previousState;
        }
        break;

    case CONTROLS:
        if (key == GLFW_KEY_B || key == GLFW_KEY_ESCAPE || key == GLFW_KEY_BACKSPACE) {
            gameState = MAIN_MENU;
        }
        break;

    case GAME_OVER:
        switch (key) {
        case GLFW_KEY_R:
            initGame();
            gameState = PLAYING;
            break;
        case GLFW_KEY_M:
            gameState = MAIN_MENU;
            break;
        }
        break;
    }
}

void GameObjects::handleNameInput(int key) {
    if (!nameInputActive) return;

    if (key == GLFW_KEY_BACKSPACE) {
        removeLastCharacterFromName();
    }
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_ESCAPE) {
        setNameInputActive(false);
        saveSettings();
    }
}

void GameObjects::addCharacterToName(char c) {
    if (playerName.length() < 15) {
        if (isalnum(c) || c == ' ' || c == '-' || c == '_') {
            playerName += c;
        }
    }
}

void GameObjects::removeLastCharacterFromName() {
    if (!playerName.empty()) {
        playerName.pop_back();
    }
}

//=============================================================================
// РЕКОРДЫ
//=============================================================================
void GameObjects::saveHighScoreToServer() {
    networkManager.submitHighScore(playerName, score, gameSpeed, gameDuration, snake.size());
}

void GameObjects::loadHighScores() {
    std::cout << "Loading high scores from server..." << std::endl;
    highScores = networkManager.getTopScores();

    if (highScores.empty()) {
        std::cout << " No high scores available (server unavailable)" << std::endl;
    }
    else {
        std::cout << " Loaded " << highScores.size() << " high scores from server" << std::endl;
    }
}

bool GameObjects::isNewHighScore(int score) const {
    if (highScores.size() < MAX_HIGH_SCORES) return true;
    return score > highScores.back().score;
}

void GameObjects::addHighScore(const std::string& playerName, int score) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    // Используем localtime_s вместо localtime
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d");

    HighScore newScore(
        playerName,
        score,
        ss.str(),
        gameSpeed,
        static_cast<int>(snake.size()),
        gameDuration
    );

    highScores.push_back(newScore);
    std::sort(highScores.begin(), highScores.end());

    if (highScores.size() > MAX_HIGH_SCORES) {
        highScores.resize(MAX_HIGH_SCORES);
    }
}

void GameObjects::updateHighScores() {
    if (score <= 0) {
        std::cout << "Score is 0, not submitting to server" << std::endl;
        return;
    }

    if (isNewHighScore(score)) {
        std::cout << "NEW HIGH SCORE! " << playerName << ": " << score << " points!" << std::endl;

        if (networkManager.submitHighScore(playerName, score, gameSpeed, gameDuration, snake.size())) {
            std::cout << " High score sent to server successfully!" << std::endl;
            loadHighScores();
        }
        else {
            std::cout << "Failed to send high score to server" << std::endl;
        }
    }
}

//=============================================================================
// МЕТОДЫ ДЛЯ КОНФИГА
//=============================================================================
void GameObjects::setSnakeModels(const std::string& head, const std::string& body, const std::string& tail) {
    std::cout << "GameObjects::setSnakeModels: было ("
        << snakeHeadModel << ", " << snakeBodyModel << ", " << snakeTailModel << ") -> "
        << "стало (" << head << ", " << body << ", " << tail << ")" << std::endl;
    snakeHeadModel = head;
    snakeBodyModel = body;
    snakeTailModel = tail;
}

void GameObjects::setSnakeColors(const glm::vec3& head, const glm::vec3& body, const glm::vec3& tail) {
    std::cout << "GameObjects::setSnakeColors: Head ("
        << head.r << "," << head.g << "," << head.b << ") "
        << "Body (" << body.r << "," << body.g << "," << body.b << ") "
        << "Tail (" << tail.r << "," << tail.g << "," << tail.b << ")" << std::endl;
    snakeHeadColor = head;
    snakeBodyColor = body;
    snakeTailColor = tail;
}

void GameObjects::setSnakeScale(float scale) {
    std::cout << "GameObjects::setSnakeScale: " << scale << std::endl;
    snakeScale = scale;
}

void GameObjects::setCloudCount(int count) {
    std::cout << "GameObjects::setCloudCount: " << count << std::endl;
    cloudCount = count;
}

void GameObjects::setBirdCount(int count) {
    std::cout << "GameObjects::setBirdCount: " << count << std::endl;
    birdCount = count;
}


void GameObjects::setFlowerCount(int count) {
    flowerCount = count;
}

//=============================================================================
// МЕТОДЫ ДЛЯ ПОЛА
//=============================================================================
void GameObjects::setFloorModel(const std::string& model) {
    floorModel = model;
    std::cout << "Floor model set to: " << model << std::endl;
}

void GameObjects::setFloorColor(const glm::vec3& color) {
    floorColor = color;
}

void GameObjects::setFloorScale(float scale) {
    floorScale = scale;
}

void GameObjects::setFloorTexture(const std::string& texture) {
    floorTexture = texture;
    useFloorTexture = !texture.empty();
    std::cout << "Floor texture set to: " << (texture.empty() ? "none" : texture) << std::endl;
}

void GameObjects::setFloorPosition(const glm::vec3& pos) {
    floorPosition = pos;
}

//=============================================================================
// СОХРАНЕНИЕ/ЗАГРУЗКА ИГРЫ
//=============================================================================
#pragma pack(push, 1)
struct SaveData {
    int version = 1;
    int score;
    int gameDuration;
    float gameSpeed;
    int currentDirection;

    int snakeLength;
    Point snake[1000];

    int foodCount;
    Point food[50];

    int obstaclesCount;
    Point obstacles[100];

    int cloudSpritesCount;
    struct CloudSave {
        glm::vec3 position;
        glm::vec3 color;
        float size;
        float speed;
    } cloudSprites[100];

    int birdsCount;
    struct BirdSave {
        glm::vec3 position;
        glm::vec3 color;
        float size;
        float speed;
        glm::vec3 direction;
    } birds[50];

    int flowerSpritesCount;
    struct FlowerSave {
        glm::vec3 position;
        glm::vec3 color;
        float size;
    } flowerSprites[100];
};
#pragma pack(pop)

bool GameObjects::saveGame() {
    std::ofstream file(saveFileName, std::ios::binary);
    if (!file.is_open()) {
        std::cout << " Failed to save game" << std::endl;
        return false;
    }

    SaveData save;
    save.version = 1;
    save.score = score;
    save.gameDuration = gameDuration;
    save.gameSpeed = gameSpeed;
    save.currentDirection = currentDirection;

    save.snakeLength = static_cast<int>(snake.size());
    for (int i = 0; i < save.snakeLength && i < 1000; i++) {
        save.snake[i] = snake[i];
    }

    save.foodCount = static_cast<int>(food.size());
    for (int i = 0; i < save.foodCount && i < 50; i++) {
        save.food[i] = food[i];
    }

    save.obstaclesCount = static_cast<int>(obstacles.size());
    for (int i = 0; i < save.obstaclesCount && i < 100; i++) {
        save.obstacles[i] = obstacles[i].center;
    }

    save.cloudSpritesCount = static_cast<int>(cloudSprites.size());
    for (int i = 0; i < save.cloudSpritesCount && i < 100; i++) {
        save.cloudSprites[i].position = cloudSprites[i].position;
        save.cloudSprites[i].color = cloudSprites[i].color;
        save.cloudSprites[i].size = cloudSprites[i].size;
        save.cloudSprites[i].speed = cloudSprites[i].speed;
    }

    save.birdsCount = static_cast<int>(birds.size());
    for (int i = 0; i < save.birdsCount && i < 50; i++) {
        save.birds[i].position = birds[i].position;
        save.birds[i].color = birds[i].color;
        save.birds[i].size = birds[i].size;
        save.birds[i].speed = birds[i].speed;
        save.birds[i].direction = birds[i].direction;
    }

    save.flowerSpritesCount = static_cast<int>(flowerSprites.size());
    for (int i = 0; i < save.flowerSpritesCount && i < 100; i++) {
        save.flowerSprites[i].position = flowerSprites[i].position;
        save.flowerSprites[i].color = flowerSprites[i].color;
        save.flowerSprites[i].size = flowerSprites[i].size;
    }

    file.write(reinterpret_cast<char*>(&save), sizeof(SaveData));
    file.close();

    std::cout << " Game saved successfully! (" << std::endl;
    return true;
}

bool GameObjects::loadGame() {
    std::ifstream file(saveFileName, std::ios::binary);
    if (!file.is_open()) {
        std::cout << " No save game found" << std::endl;
        return false;
    }

    SaveData save;
    file.read(reinterpret_cast<char*>(&save), sizeof(SaveData));
    file.close();

    if (save.version != 1) {
        std::cout << " Invalid save game version" << std::endl;
        return false;
    }

    score = save.score;
    gameDuration = save.gameDuration;
    gameSpeed = save.gameSpeed;
    currentDirection = static_cast<Direction>(save.currentDirection);
    gameOver = false;

    generateFence();

    snake.clear();
    for (int i = 0; i < save.snakeLength && i < 1000; i++) {
        snake.push_back(save.snake[i]);
    }

    food.clear();
    for (int i = 0; i < save.foodCount && i < 50; i++) {
        food.push_back(save.food[i]);
    }

    obstacles.clear();
    for (int i = 0; i < save.obstaclesCount && i < 100; i++) {
        obstacles.push_back(Obstacle(save.obstacles[i]));
    }

    cloudSprites.clear();
    for (int i = 0; i < save.cloudSpritesCount && i < 100; i++) {
        Sprite cloud(save.cloudSprites[i].position,
            save.cloudSprites[i].color,
            save.cloudSprites[i].size,
            save.cloudSprites[i].speed);
        cloudSprites.push_back(cloud);
    }

    birds.clear();
    for (int i = 0; i < save.birdsCount && i < 50; i++) {
        Bird bird(save.birds[i].position,
            save.birds[i].color,
            save.birds[i].size,
            save.birds[i].speed);
        bird.direction = save.birds[i].direction;
        birds.push_back(bird);
    }

    flowerSprites.clear();
    for (int i = 0; i < save.flowerSpritesCount && i < 100; i++) {
        Sprite flower(save.flowerSprites[i].position,
            save.flowerSprites[i].color,
            save.flowerSprites[i].size,
            0.0f);
        flowerSprites.push_back(flower);
    }

    std::cout << " Game loaded successfully!" << std::endl;
    return true;
}

bool GameObjects::hasSaveGame() const {
    std::ifstream file(saveFileName);
    return file.good();
}

void GameObjects::deleteSaveGame() {
    std::remove(saveFileName.c_str());
    std::cout << " Save game deleted" << std::endl;
}