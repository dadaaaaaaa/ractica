#include "../pch.h"
#include "GameObjects.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h"
#include "../Core/Types.h"

// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern Camera g_camera;

// ============================================================================
// КОНСТРУКТОР И ИНИЦИАЛИЗАЦИЯ
// ============================================================================

GameObjects::GameObjects()
    : currentDirection(FORWARD),
    verticalDirection(0),
    score(0),
    gameOver(false),
    gameState(MAIN_MENU),
    previousState(MAIN_MENU),
    gameSpeed(BASE_GAME_SPEED),
    playerName(DEFAULT_PLAYER_NAME),
    gameDuration(0),
    gameTimer(0.0f) {

    // Инициализация множителей скорости
    speedMultipliers = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f };
    currentSpeedIndex = 3; // Начинаем с множителя 1.0x

    // Загружаем настройки и данные при создании
    loadSettings();
    updateGameSpeedFromMultiplier();
    loadHighScores();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "🎮 GameObjects initialized" << std::endl;
    }
}

// ============================================================================
// УПРАВЛЕНИЕ СКОРОСТЬЮ ИГРЫ
// ============================================================================

void GameObjects::updateGameSpeedFromMultiplier() {
    gameSpeed = BASE_GAME_SPEED / speedMultipliers[currentSpeedIndex];
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
        if (ENABLE_DEBUG_INFO) {
            std::cout << "🚀 Speed increased to: " << getSpeedDisplayText() << std::endl;
        }
    }
}

void GameObjects::decreaseSpeed() {
    if (currentSpeedIndex > 0) {
        currentSpeedIndex--;
        updateGameSpeedFromMultiplier();
        saveSettings();
        if (ENABLE_DEBUG_INFO) {
            std::cout << "🐢 Speed decreased to: " << getSpeedDisplayText() << std::endl;
        }
    }
}

// ============================================================================
// СИСТЕМА СОХРАНЕНИЯ И ЗАГРУЗКИ НАСТРОЕК
// ============================================================================

void GameObjects::saveSettings() {
    std::ofstream file(settingsFileName, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "❌ Failed to open settings file for writing" << std::endl;
        return;
    }

    SettingsData settings;
    settings.version = 1;
    settings.gameSpeed = gameSpeed;
    settings.currentSpeedIndex = currentSpeedIndex;
    strncpy_s(settings.playerName, playerName.c_str(), MAX_PLAYER_NAME_LENGTH);
    settings.playerName[MAX_PLAYER_NAME_LENGTH] = '\0';

    file.write(reinterpret_cast<char*>(&settings), sizeof(SettingsData));
    file.close();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ Settings saved: " << playerName << ", speed: " << getSpeedDisplayText() << std::endl;
    }
}

void GameObjects::loadSettings() {
    std::ifstream file(settingsFileName, std::ios::binary);
    if (!file.is_open()) {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "⚠️ No settings file found, using defaults" << std::endl;
        }
        return;
    }

    SettingsData settings;
    file.read(reinterpret_cast<char*>(&settings), sizeof(SettingsData));
    file.close();

    if (settings.version == 1) {
        gameSpeed = settings.gameSpeed;
        currentSpeedIndex = settings.currentSpeedIndex;
        playerName = std::string(settings.playerName);
        if (ENABLE_DEBUG_INFO) {
            std::cout << "✅ Settings loaded: " << playerName << ", speed: " << getSpeedDisplayText() << std::endl;
        }
    }
    else {
        std::cout << "❌ Invalid settings version" << std::endl;
    }
}

// ============================================================================
// СИСТЕМА РЕКОРДОВ
// ============================================================================

void GameObjects::loadHighScores() {
    if (ENABLE_DEBUG_INFO) {
        std::cout << "🔄 Loading high scores..." << std::endl;
    }

    highScores = networkManager.getTopScores();

    if (highScores.empty()) {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "❌ No high scores available" << std::endl;
        }
    }
    else {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "✅ Loaded " << highScores.size() << " high scores" << std::endl;
        }
    }
}

void GameObjects::refreshHighScores() {
    loadHighScores();
}

bool GameObjects::isNewHighScore(int score) const {
    if (highScores.size() < HIGH_SCORES_DISPLAY_LIMIT) return true;
    return score > highScores.back().score;
}

bool GameObjects::isNetworkAvailable() const {
    return networkManager.isConnected();
}

void GameObjects::updateHighScores() {
    if (score < MIN_SCORE_FOR_HIGHSCORE) {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "⚠️ Score too low (" << score << "), not submitting" << std::endl;
        }
        return;
    }

    if (isNewHighScore(score)) {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "🎉 NEW HIGH SCORE! " << playerName << ": " << score << " points!" << std::endl;
        }

        if (networkManager.submitHighScore(playerName, score, gameSpeed, gameDuration, snake.size())) {
            if (ENABLE_DEBUG_INFO) {
                std::cout << "✅ High score sent successfully!" << std::endl;
            }
            loadHighScores();
        }
        else {
            std::cout << "❌ Failed to send high score" << std::endl;
        }
    }
}

// ============================================================================
// ОСНОВНОЙ ИГРОВОЙ ЦИКЛ
// ============================================================================

void GameObjects::update() {
    if (gameOver || gameState != PLAYING) return;

    // Обновление времени игры
    gameTimer += PHYSICS_TIMESTEP;
    if (gameTimer >= 1.0f) {
        gameDuration++;
        gameTimer = 0.0f;
    }

    // Обновление декораций
    updateClouds();
    updateBirds();

    // Вычисление новой позиции головы змейки
    Point newHead = snake[0];
    switch (currentDirection) {
    case FORWARD: newHead.z++; break;
    case BACKWARD: newHead.z--; break;
    case RIGHT: newHead.x--; break;
    case LEFT: newHead.x++; break;
    }
    newHead.y = 0;

    // Проверка столкновения со стенами
    if (newHead.x < 0 || newHead.x >= GRID_WIDTH ||
        newHead.z < 0 || newHead.z >= GRID_DEPTH) {
        handleGameOver();
        return;
    }

    // Проверка столкновения с собой
    for (const auto& segment : snake) {
        if (segment == newHead) {
            handleGameOver();
            return;
        }
    }

    // Проверка столкновения с препятствиями
    for (const auto& obstacle : obstacles) {
        if (obstacle.contains(newHead)) {
            handleGameOver();
            return;
        }
    }

    // Перемещение змейки
    snake.insert(snake.begin(), newHead);

    // Проверка съедания еды
    auto foodIt = std::find(food.begin(), food.end(), newHead);
    if (foodIt != food.end()) {
        score++;
        food.erase(foodIt);
        generateSingleFood();

        // Увеличение скорости с ростом счета
        gameSpeed += SPEED_INCREMENT_PER_FOOD;
    }
    else {
        snake.pop_back();
    }
}

void GameObjects::handleGameOver() {
    gameOver = true;
    gameState = GAME_OVER;
    updateHighScores();
    if (ENABLE_DEBUG_INFO) {
        std::cout << "💀 Game Over! Final score: " << score << std::endl;
    }
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ ИГРЫ
// ============================================================================

void GameObjects::initGame() {
    // Очистка всех игровых объектов
    snake.clear();
    food.clear();
    obstacles.clear();
    fenceBlocks.clear();
    cloudSprites.clear();
    birds.clear();
    flowerSprites.clear();

    // Создание начальной змейки
    snake.push_back(Point(GRID_WIDTH / 2, 0, GRID_DEPTH / 2));
    for (int i = 1; i < INITIAL_SNAKE_LENGTH; i++) {
        snake.push_back(Point(GRID_WIDTH / 2 - i, 0, GRID_DEPTH / 2));
    }

    // Генерация игрового мира
    generateFence();
    generateInitialFood();
    generateObstacles();
    generateClouds();
    generateBirds();
    generateGroundSprites();

    // Сброс состояния игры
    currentDirection = FORWARD;
    verticalDirection = 0;
    score = 0;
    gameOver = false;
    gameDuration = 0;
    gameTimer = 0.0f;
    gameSpeed = BASE_GAME_SPEED / speedMultipliers[currentSpeedIndex];

    // Настройка камеры
    g_camera.setTargetDistance(5.0f);

    if (ENABLE_DEBUG_INFO) {
        std::cout << "🎮 New game started!" << std::endl;
    }
}

// ============================================================================
// ГЕНЕРАЦИЯ ИГРОВОГО МИРА
// ============================================================================

void GameObjects::generateFence() {
    fenceBlocks.clear();

    for (int x = 0; x < GRID_WIDTH; x += 2) {
        fenceBlocks.push_back(Point(x, 0, 0));
        fenceBlocks.push_back(Point(x, 0, GRID_DEPTH - 1));
    }

    for (int z = 2; z < GRID_DEPTH - 2; z += 2) {
        fenceBlocks.push_back(Point(0, 0, z));
        fenceBlocks.push_back(Point(GRID_WIDTH - 1, 0, z));
    }
}

void GameObjects::generateClouds() {
    cloudSprites.clear();

    float gameFieldMinX = -GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMaxX = GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMinZ = -GRID_DEPTH * CELL_SIZE * 0.5f;
    float gameFieldMaxZ = GRID_DEPTH * CELL_SIZE * 0.5f;

    for (int i = 0; i < CLOUD_COUNT; i++) {
        glm::vec3 position;
        bool validPosition = false;
        int attempts = 0;

        while (!validPosition && attempts < MAX_GENERATION_ATTEMPTS) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float distance = CLOUD_MIN_DISTANCE + (rand() % (int)(CLOUD_MAX_DISTANCE - CLOUD_MIN_DISTANCE));

            position.x = cos(angle) * distance;
            position.z = sin(angle) * distance;
            position.y = CLOUD_MIN_HEIGHT + (rand() % 10) * 0.1f;

            bool outsideX = position.x < gameFieldMinX - CLOUD_SAFE_DISTANCE ||
                position.x > gameFieldMaxX + CLOUD_SAFE_DISTANCE;
            bool outsideZ = position.z < gameFieldMinZ - CLOUD_SAFE_DISTANCE ||
                position.z > gameFieldMaxZ + CLOUD_SAFE_DISTANCE;

            if (outsideX || outsideZ) {
                validPosition = true;
            }
            attempts++;
        }

        if (validPosition) {
            float size = 0.4f + (rand() % 8) * 0.1f;
            float speed = CLOUD_SPEED + (rand() % 6) * 0.01f;

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
}

void GameObjects::generateBirds() {
    birds.clear();

    float gameFieldMinX = -GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMaxX = GRID_WIDTH * CELL_SIZE * 0.5f;
    float gameFieldMinZ = -GRID_DEPTH * CELL_SIZE * 0.5f;
    float gameFieldMaxZ = GRID_DEPTH * CELL_SIZE * 0.5f;

    for (int i = 0; i < BIRD_COUNT; i++) {
        glm::vec3 position;
        bool validPosition = false;
        int attempts = 0;

        while (!validPosition && attempts < MAX_GENERATION_ATTEMPTS) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float distance = BIRD_MIN_DISTANCE + (rand() % (int)(BIRD_MAX_DISTANCE - BIRD_MIN_DISTANCE));

            position.x = cos(angle) * distance;
            position.z = sin(angle) * distance;
            position.y = BIRD_MIN_HEIGHT + (rand() % 8) * 0.1f;

            bool outsideX = position.x < gameFieldMinX - BIRD_SAFE_DISTANCE ||
                position.x > gameFieldMaxX + BIRD_SAFE_DISTANCE;
            bool outsideZ = position.z < gameFieldMinZ - BIRD_SAFE_DISTANCE ||
                position.z > gameFieldMaxZ + BIRD_SAFE_DISTANCE;

            if (outsideX || outsideZ) {
                validPosition = true;
            }
            attempts++;
        }

        if (validPosition) {
            float size = 0.06f + (rand() % 6) * 0.02f;
            float speed = BIRD_SPEED + (rand() % 8) * 0.02f;

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
            bird.setDirection(direction);
            birds.push_back(bird);
        }
    }
}

void GameObjects::generateGroundSprites() {
    flowerSprites.clear();

    for (int i = 0; i < FLOWER_COUNT; i++) {
        float x = (rand() % (int)(FIELD_EXTENT * 20) - FIELD_EXTENT * 10) * 0.1f;
        float z = (rand() % (int)(FIELD_EXTENT * 20) - FIELD_EXTENT * 10) * 0.1f;
        float y = FLOWER_HEIGHT;

        float size = MIN_FLOWER_SIZE + (rand() % (int)((MAX_FLOWER_SIZE - MIN_FLOWER_SIZE) * 100)) * 0.01f;

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
            center.x = OBSTACLE_GENERATION_MARGIN + rand() % (GRID_WIDTH - 2 * (int)OBSTACLE_GENERATION_MARGIN);
            center.y = 0;
            center.z = OBSTACLE_GENERATION_MARGIN + rand() % (GRID_DEPTH - 2 * (int)OBSTACLE_GENERATION_MARGIN);

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
            if (attempts > MAX_OBSTACLE_GENERATION_ATTEMPTS) break;

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
        newFood.x = FOOD_GENERATION_MARGIN + rand() % (GRID_WIDTH - 2 * (int)FOOD_GENERATION_MARGIN);
        newFood.y = 0;
        newFood.z = FOOD_GENERATION_MARGIN + rand() % (GRID_DEPTH - 2 * (int)FOOD_GENERATION_MARGIN);

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
        if (attempts > MAX_FOOD_GENERATION_ATTEMPTS) break;

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

// ============================================================================
// ОБНОВЛЕНИЕ ДЕКОРАЦИЙ
// ============================================================================

void GameObjects::updateClouds() {
    for (auto& cloud : cloudSprites) {
        float currentAngle = atan2f(cloud.position.z, cloud.position.x);
        float movementAngle = currentAngle + 3.14159f / 2.0f;

        cloud.position.x += cos(movementAngle) * cloud.speed * 0.01f;
        cloud.position.z += sin(movementAngle) * cloud.speed * 0.01f;

        cloud.position.y += (rand() % 100 - 50) * CLOUD_HEIGHT_VARIATION;

        cloud.position.y = glm::clamp(cloud.position.y, CLOUD_MIN_HEIGHT, CLOUD_MAX_HEIGHT);

        float currentDistance = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (currentDistance < CLOUD_MIN_DISTANCE_THRESHOLD || currentDistance > CLOUD_MAX_DISTANCE_THRESHOLD) {
            glm::vec3 normalized = glm::normalize(cloud.position);
            cloud.position = normalized * CLOUD_TARGET_DISTANCE;
            cloud.position.y = CLOUD_MIN_HEIGHT + (rand() % 10) * 0.1f;
        }
    }
}

void GameObjects::updateBirds() {
    for (auto& bird : birds) {
        bird.update(PHYSICS_TIMESTEP);

        float currentDistance = glm::length(glm::vec2(bird.getPosition().x, bird.getPosition().z));
        if (currentDistance < BIRD_MIN_DISTANCE_THRESHOLD || currentDistance > BIRD_MAX_DISTANCE_THRESHOLD) {
            glm::vec3 normalized = glm::normalize(bird.getPosition());
            glm::vec3 newPosition = normalized * BIRD_TARGET_DISTANCE;
            newPosition.y = BIRD_MIN_HEIGHT + (rand() % 8) * 0.1f;
            bird.setPosition(newPosition);

            float currentAngle = atan2f(newPosition.z, newPosition.x);
            float movementAngle = currentAngle + 3.14159f / 2.0f;
            if (rand() % 2 == 0) movementAngle += 3.14159f;

            bird.setDirection(glm::vec3(cos(movementAngle), 0.0f, sin(movementAngle)));
        }
    }
}

// ============================================================================
// ОБРАБОТКА ВВОДА
// ============================================================================

void GameObjects::handleGameKeyPress(int key) {
    switch (key) {
    case KEY_LEFT:
        if (currentDirection == FORWARD) currentDirection = LEFT;
        else if (currentDirection == LEFT) currentDirection = BACKWARD;
        else if (currentDirection == BACKWARD) currentDirection = RIGHT;
        else if (currentDirection == RIGHT) currentDirection = FORWARD;
        break;
    case KEY_RIGHT:
        if (currentDirection == FORWARD) currentDirection = RIGHT;
        else if (currentDirection == RIGHT) currentDirection = BACKWARD;
        else if (currentDirection == BACKWARD) currentDirection = LEFT;
        else if (currentDirection == LEFT) currentDirection = FORWARD;
        break;
    case KEY_P:
        gameState = PAUSED;
        if (ENABLE_DEBUG_INFO) {
            std::cout << "⏸️ Game paused" << std::endl;
        }
        break;
    case KEY_R:
        initGame();
        gameState = PLAYING;
        if (ENABLE_DEBUG_INFO) {
            std::cout << "🔄 Game restarted" << std::endl;
        }
        break;
    case KEY_Q:
        g_camera.rotate(-10.0f);
        if (ENABLE_DEBUG_INFO) {
            std::cout << "📷 Camera rotated left" << std::endl;
        }
        break;
    case KEY_E:
        g_camera.rotate(10.0f);
        if (ENABLE_DEBUG_INFO) {
            std::cout << "📷 Camera rotated right" << std::endl;
        }
        break;
    }
}

void GameObjects::handleSettingsKeyPress(int key) {
    switch (key) {
    case KEY_EQUAL:
    case KEY_RIGHT:
        increaseSpeed();
        break;
    case KEY_MINUS:
    case KEY_LEFT:
        decreaseSpeed();
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
        case KEY_ESC:
            gameState = PLAYING;
            break;
        case KEY_Q:
            previousState = PAUSED;
            gameState = SETTINGS;
            break;
        case KEY_M:
            gameState = MAIN_MENU;
            break;
        case KEY_P:
            gameState = PLAYING;
            break;
        }
        break;

    case SETTINGS:
        if (key == KEY_ESC || key == KEY_B || key == GLFW_KEY_BACKSPACE) {
            gameState = previousState;
        }
        break;

    case HIGH_SCORES:
        if (key == KEY_B || key == KEY_ESC || key == GLFW_KEY_BACKSPACE) {
            gameState = previousState;
        }
        break;

    case CONTROLS:
        if (key == KEY_B || key == KEY_ESC || key == GLFW_KEY_BACKSPACE) {
            gameState = MAIN_MENU;
        }
        break;

    case GAME_OVER:
        switch (key) {
        case KEY_R:
            initGame();
            gameState = PLAYING;
            break;
        case KEY_M:
            gameState = MAIN_MENU;
            break;
        }
        break;
    }
}

// ============================================================================
// СИСТЕМА СОХРАНЕНИЯ ИГРЫ
// ============================================================================

bool GameObjects::saveGame() {
    std::ofstream file(saveFileName, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "❌ Failed to save game" << std::endl;
        return false;
    }

    SaveData save;
    save.version = 1;
    save.score = score;
    save.gameDuration = gameDuration;
    save.gameSpeed = gameSpeed;
    save.currentDirection = currentDirection;

    // Сохраняем змейку
    save.snakeLength = static_cast<int>(snake.size());
    for (int i = 0; i < save.snakeLength && i < MAX_SNAKE_LENGTH; i++) {
        save.snake[i] = snake[i];
    }

    // Сохраняем еду
    save.foodCount = static_cast<int>(food.size());
    for (int i = 0; i < save.foodCount && i < 50; i++) {
        save.food[i] = food[i];
    }

    // Сохраняем препятствия
    save.obstaclesCount = static_cast<int>(obstacles.size());
    for (int i = 0; i < save.obstaclesCount && i < 100; i++) {
        save.obstacles[i] = obstacles[i].center;
    }

    file.write(reinterpret_cast<char*>(&save), sizeof(SaveData));
    file.close();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ Game saved successfully! ("
            << save.snakeLength << " snake segments, "
            << save.foodCount << " food, "
            << save.obstaclesCount << " obstacles)" << std::endl;
    }
    return true;
}

bool GameObjects::loadGame() {
    std::ifstream file(saveFileName, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "❌ No save game found" << std::endl;
        return false;
    }

    SaveData save;
    file.read(reinterpret_cast<char*>(&save), sizeof(SaveData));
    file.close();

    if (save.version != 1) {
        std::cout << "❌ Invalid save game version" << std::endl;
        return false;
    }

    // Восстанавливаем основное состояние
    score = save.score;
    gameDuration = save.gameDuration;
    gameSpeed = save.gameSpeed;
    currentDirection = static_cast<Direction>(save.currentDirection);
    gameOver = false;

    // Генерируем забор заново
    generateFence();

    // Восстанавливаем змейку
    snake.clear();
    for (int i = 0; i < save.snakeLength && i < MAX_SNAKE_LENGTH; i++) {
        snake.push_back(save.snake[i]);
    }

    // Восстанавливаем еду
    food.clear();
    for (int i = 0; i < save.foodCount && i < 50; i++) {
        food.push_back(save.food[i]);
    }

    // Восстанавливаем препятствия
    obstacles.clear();
    for (int i = 0; i < save.obstaclesCount && i < 100; i++) {
        obstacles.push_back(Obstacle(save.obstacles[i]));
    }

    // Генерируем декорации заново
    generateClouds();
    generateBirds();
    generateGroundSprites();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ Game loaded successfully! ("
            << save.snakeLength << " snake segments, "
            << save.foodCount << " food, "
            << save.obstaclesCount << " obstacles)" << std::endl;
    }
    return true;
}

// ============================================================================
// ДОПОЛНИТЕЛЬНЫЕ МЕТОДЫ
// ============================================================================

bool GameObjects::hasSaveGame() const {
    std::ifstream file(saveFileName);
    return file.good();
}

void GameObjects::deleteSaveGame() {
    if (std::remove(saveFileName.c_str()) == 0) {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "✅ Save game deleted" << std::endl;
        }
    }
    else {
        std::cout << "❌ Failed to delete save game" << std::endl;
    }
}

void GameObjects::pauseGame() {
    if (gameState == PLAYING) {
        gameState = PAUSED;
        if (ENABLE_DEBUG_INFO) {
            std::cout << "⏸️ Game paused" << std::endl;
        }
    }
}

void GameObjects::resumeGame() {
    if (gameState == PAUSED) {
        gameState = PLAYING;
        if (ENABLE_DEBUG_INFO) {
            std::cout << "▶️ Game resumed" << std::endl;
        }
    }
}

void GameObjects::returnToMainMenu() {
    if (gameState == PLAYING && !gameOver) {
        saveGame();
        if (ENABLE_DEBUG_INFO) {
            std::cout << "💾 Game saved and returning to main menu" << std::endl;
        }
    }
    gameState = MAIN_MENU;
}

void GameObjects::saveOnExit() {
    if (gameState == PLAYING && !gameOver) {
        saveGame();
    }
    saveSettings();
}

void GameObjects::setPlayerName(const std::string& name) {
    playerName = name;
    if (playerName.empty()) {
        playerName = DEFAULT_PLAYER_NAME;
    }
    saveSettings();
    if (ENABLE_DEBUG_INFO) {
        std::cout << "👤 Player name set to: " << playerName << std::endl;
    }
}

// ============================================================================
// ОТЛАДОЧНЫЕ ФУНКЦИИ
// ============================================================================

void GameObjects::debugSnakeInfo() {
    if (ENABLE_DEBUG_INFO) {
        std::cout << "🐍 Snake length: " << snake.size()
            << ", Head: (" << snake[0].x << ", " << snake[0].z << ")"
            << ", Direction: " << currentDirection << std::endl;
    }
}

void GameObjects::printDebugInfo() const {
    if (!ENABLE_DEBUG_INFO) return;

    std::cout << "=== GAME DEBUG INFO ===" << std::endl;
    std::cout << "State: " << gameStateToString(gameState) << std::endl;
    std::cout << "Score: " << score << std::endl;
    std::cout << "Duration: " << gameDuration << "s" << std::endl;
    std::cout << "Speed: " << getSpeedDisplayText() << std::endl;
    std::cout << "Snake length: " << snake.size() << std::endl;
    std::cout << "Food count: " << food.size() << std::endl;
    std::cout << "Obstacles: " << obstacles.size() << std::endl;
    std::cout << "Clouds: " << cloudSprites.size() << std::endl;
    std::cout << "Birds: " << birds.size() << std::endl;
    std::cout << "Flowers: " << flowerSprites.size() << std::endl;
    std::cout << "Player: " << playerName << std::endl;
    std::cout << "Network: " << (isNetworkAvailable() ? "Available" : "Unavailable") << std::endl;
    std::cout << "======================" << std::endl;
}

std::string GameObjects::gameStateToString(GameState state) const {
    switch (state) {
    case MAIN_MENU: return "MAIN_MENU";
    case PLAYING: return "PLAYING";
    case PAUSED: return "PAUSED";
    case GAME_OVER: return "GAME_OVER";
    case SETTINGS: return "SETTINGS";
    case HIGH_SCORES: return "HIGH_SCORES";
    case CONTROLS: return "CONTROLS";
    default: return "UNKNOWN";
    }
}

// ============================================================================
// ОЧИСТКА РЕСУРСОВ
// ============================================================================

void GameObjects::reset() {
    snake.clear();
    food.clear();
    obstacles.clear();
    fenceBlocks.clear();
    cloudSprites.clear();
    birds.clear();
    flowerSprites.clear();
    highScores.clear();

    score = 0;
    gameOver = false;
    gameDuration = 0;
    gameTimer = 0.0f;
    currentDirection = FORWARD;
    verticalDirection = 0;

    if (ENABLE_DEBUG_INFO) {
        std::cout << "🔄 All game data reset" << std::endl;
    }
}

GameObjects::~GameObjects() {
    saveOnExit();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "🧹 GameObjects cleaned up" << std::endl;
    }
}