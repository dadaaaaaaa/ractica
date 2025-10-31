#include "../pch.h"
#include "GameObjects.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h" 
// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern Camera g_camera;

GameObjects::GameObjects()
    : currentDirection(FORWARD),
    verticalDirection(0),
    score(0),
    gameOver(false),
    gameState(MAIN_MENU),
    previousState(MAIN_MENU),
    gameSpeed(0.12f),
    playerName("Player") {

    // Правильный порядок множителей от медленного к быстрому
    speedMultipliers = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f };
    currentSpeedIndex = 3; // Начинаем с 1.0x (нормальная скорость)
    updateGameSpeedFromMultiplier();
}

void GameObjects::updateGameSpeedFromMultiplier() {
    float baseSpeed = 0.12f; // Базовая скорость (для 1.0x)
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
        std::cout << "Speed increased to: " << getSpeedDisplayText() << std::endl;
    }
}

void GameObjects::decreaseSpeed() {
    if (currentSpeedIndex > 0) {
        currentSpeedIndex--;
        updateGameSpeedFromMultiplier();
        std::cout << "Speed decreased to: " << getSpeedDisplayText() << std::endl;
    }
}

void GameObjects::handleSettingsKeyPress(int key) {
    switch (key) {
    case GLFW_KEY_EQUAL: // + 
    case GLFW_KEY_RIGHT: // Стрелка вправо
        increaseSpeed();
        break;
    case GLFW_KEY_MINUS: // -
    case GLFW_KEY_LEFT:  // Стрелка влево
        decreaseSpeed();
        break;
    }
}
void GameObjects::update() {
    if (gameOver || gameState != PLAYING) return;

    Point newHead = snake[0];

    switch (currentDirection) {
    case FORWARD: newHead.z++; break;
    case BACKWARD: newHead.z--; break;
    case RIGHT: newHead.x--; break;
    case LEFT: newHead.x++; break;
    }

    newHead.y = 0;
    debugSnakeInfo();

    // Проверка столкновений
    if (newHead.x < 0 || newHead.x >= GRID_WIDTH ||
        newHead.z < 0 || newHead.z >= GRID_DEPTH) {
        gameOver = true;
        gameState = GAME_OVER;
        saveHighScore();
        return;
    }

    for (const auto& segment : snake) {
        if (segment == newHead) {
            gameOver = true;
            gameState = GAME_OVER;
            saveHighScore();
            return;
        }
    }

    for (const auto& obstacle : obstacles) {
        if (obstacle.contains(newHead)) {
            gameOver = true;
            gameState = GAME_OVER;
            saveHighScore();
            return;
        }
    }

    // Добавление новой головы
    snake.insert(snake.begin(), newHead);

    // Проверка поедания пищи
    auto foodIt = std::find(food.begin(), food.end(), newHead);
    if (foodIt != food.end()) {
        score++;
        food.erase(foodIt);
        generateSingleFood();
    }
    else {
        snake.pop_back();
    }
}

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

    g_camera.setTargetDistance(5.0f);
}

void GameObjects::debugSnakeInfo() {
    if (snake.empty()) return;
    std::cout << "Snake head: (" << snake[0].x << ", " << snake[0].y << ", " << snake[0].z << ")" << std::endl;
}

void GameObjects::saveHighScore() {
    if (score > 0) {
        time_t now = time(0);
        tm* localTime = localtime(&now);
        char dateStr[11];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", localTime);

        highScores.push_back(HighScore(playerName, score, dateStr));

        std::sort(highScores.begin(), highScores.end(),
            [](const HighScore& a, const HighScore& b) {
                return a.score > b.score;
            });

        if (highScores.size() > 10) {
            highScores.resize(10);
        }
    }
}

void GameObjects::loadHighScores() {
    highScores.clear();
    highScores.push_back(HighScore("Champion", 150, "2024-01-15"));
    highScores.push_back(HighScore("ProPlayer", 120, "2024-01-14"));
    highScores.push_back(HighScore("SnakeMaster", 100, "2024-01-13"));
    highScores.push_back(HighScore("Beginner", 80, "2024-01-12"));
    highScores.push_back(HighScore("Newbie", 50, "2024-01-11"));

    std::sort(highScores.begin(), highScores.end(),
        [](const HighScore& a, const HighScore& b) {
            return a.score > b.score;
        });
}

void GameObjects::generateFence() {
    fenceBlocks.clear();
    for (int x = 0; x < GRID_WIDTH; x += 2) {
        fenceBlocks.push_back(Point(x, 0, 0));
    }
    for (int x = 0; x < GRID_WIDTH; x += 2) {
        fenceBlocks.push_back(Point(x, 0, GRID_DEPTH - 1));
    }
    for (int z = 2; z < GRID_DEPTH - 2; z += 2) {
        fenceBlocks.push_back(Point(0, 0, z));
    }
    for (int z = 2; z < GRID_DEPTH - 2; z += 2) {
        fenceBlocks.push_back(Point(GRID_WIDTH - 1, 0, z));
    }
}

void GameObjects::generateClouds() {
    cloudSprites.clear();
    float gameFieldMinX = -GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMaxX = GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMinZ = -GRID_DEPTH * CELL_SIZE * 0.5f;
    float gameFieldMaxZ = GRID_DEPTH * CELL_SIZE * 0.5f;
    float safeDistance = 2.0f;

    for (int i = 0; i < 20; i++) {
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
    float gameFieldMinX = -GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMaxX = GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMinZ = -GRID_DEPTH * CELL_SIZE * 0.5f;
    float gameFieldMaxZ = GRID_DEPTH * CELL_SIZE * 0.5f;
    float safeDistance = 1.5f;

    for (int i = 0; i < 15; i++) {
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
    for (int i = 0; i < 25; i++) {
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
            if (attempts > 100) {
                std::cout << "Warning: Could not find valid obstacle position after 100 attempts" << std::endl;
                break;
            }

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
        if (attempts > 50) {
            std::cout << "Warning: Could not find valid food position after 50 attempts" << std::endl;
            break;
        }

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

void GameObjects::handleGameKeyPress(int key) {
    switch (key) {
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
    case GLFW_KEY_D:
        debugSnakeInfo();
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
        case GLFW_KEY_Q: // Настройки из паузы
            previousState = PAUSED;
            gameState = SETTINGS;
            break;
        case GLFW_KEY_M: // В главное меню
            gameState = MAIN_MENU;
            break;
        case GLFW_KEY_P: // Продолжить
            gameState = PLAYING;
            break;
        case GLFW_KEY_B: // Назад (альтернативная кнопка)
            gameState = PLAYING;
            break;
        }
        break;

    case SETTINGS:
        switch (key) {
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_B:
        case GLFW_KEY_BACKSPACE: // Добавляем Backspace как альтернативу
            gameState = previousState;
            std::cout << "Returning to previous state from settings" << std::endl;
            break;
        }
        break;

    case HIGH_SCORES:
        if (key == GLFW_KEY_B || key == GLFW_KEY_ESCAPE || key == GLFW_KEY_BACKSPACE) {
            gameState = previousState;
            std::cout << "Returning to previous state from high scores" << std::endl;
        }
        break;

    case CONTROLS:
        if (key == GLFW_KEY_B || key == GLFW_KEY_ESCAPE || key == GLFW_KEY_BACKSPACE) {
            gameState = MAIN_MENU;
            std::cout << "Returning to main menu from controls" << std::endl;
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
        case GLFW_KEY_B: // Назад в главное меню
            gameState = MAIN_MENU;
            break;
        }
        break;
    }
}