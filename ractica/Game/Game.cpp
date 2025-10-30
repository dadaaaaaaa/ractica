#include "../pch.h"
#include "../Game/Game.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"

// Глобальные экземпляры (пока оставим как есть, но в будущем можно инкапсулировать)
extern ShaderManager g_shaderManager;
extern Camera g_camera;
extern GLuint uiVAO, uiVBO;

Game::Game()
    : currentDirection(FORWARD),
    verticalDirection(0),
    score(0),
    gameOver(false),
    gameState(MAIN_MENU),
    gameSpeed(0.12f),
    windowWidth(1200),
    windowHeight(800),
    mouseX(0),
    mouseY(0),
    mousePressed(false),
    playerName("Player") {
}

void Game::initialize() {
    loadHighScores();
    loadAllModels();
    initUI();
    initGame();
}

void Game::update() {
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

    if (newHead.x < 0 || newHead.x >= GRID_WIDTH ||
        newHead.z < 0 || newHead.z >= GRID_DEPTH) {
        gameOver = true;
        gameState = GAME_OVER;
        saveHighScore();
        std::cout << "Game Over: Wall collision!" << std::endl;
        return;
    }

    for (const auto& segment : snake) {
        if (segment == newHead) {
            gameOver = true;
            gameState = GAME_OVER;
            saveHighScore();
            std::cout << "Game Over: Self collision!" << std::endl;
            return;
        }
    }

    for (const auto& obstacle : obstacles) {
        if (obstacle.contains(newHead)) {
            gameOver = true;
            gameState = GAME_OVER;
            saveHighScore();
            std::cout << "Game Over: Obstacle collision at (" << newHead.x << ", " << newHead.z << ")!" << std::endl;
            return;
        }
    }

    snake.insert(snake.begin(), newHead);

    auto foodIt = std::find(food.begin(), food.end(), newHead);
    if (foodIt != food.end()) {
        score++;
        food.erase(foodIt);
        generateSingleFood();
        std::cout << "Food eaten! Score: " << score << std::endl;
    }
    else {
        snake.pop_back();
    }

}

void Game::render() {
    switch (gameState) {
    case MAIN_MENU:
        drawMainMenu();
        break;
    case PLAYING:
        renderGame();
        break;
    case PAUSED:
        renderGame();
        drawPauseMenu();
        break;
    case SETTINGS:
        drawSettingsMenu();
        break;
    case HIGH_SCORES:
        drawHighScoresMenu();
        break;
    case CONTROLS:
        drawControlsMenu();
        break;
    case GAME_OVER:
        renderGame();
        drawGameOver();
        break;
    }
}



void Game::handleMouseClick() {
    mousePressed = true;

    switch (gameState) {
    case MAIN_MENU:
        for (size_t i = 0; i < mainMenuButtons.size(); i++) {
            if (mainMenuButtons[i].contains(mouseX, mouseY)) {
                switch (i) {
                case 0: // PLAY
                    gameState = PLAYING;
                    initGame();
                    break;
                case 1: // SETTINGS
                    gameState = SETTINGS;
                    break;
                case 2: // HIGH SCORES
                    gameState = HIGH_SCORES;
                    break;
                case 3: // CONTROLS
                    gameState = CONTROLS;
                    break;
                case 4: // EXIT
                    // Обработка выхода будет в main
                    break;
                }
            }
        }
        break;
        // Аналогично для других состояний...
    }
}

void Game::handleMouseScroll(double yoffset) {
    g_camera.zoom(yoffset);
}

void Game::initGame() {
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

void Game::debugSnakeInfo() {
    if (snake.empty()) return;
    std::cout << "Snake head: (" << snake[0].x << ", " << snake[0].y << ", " << snake[0].z << ")" << std::endl;
}

void Game::saveHighScore() {
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

void Game::loadHighScores() {
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
// Генерация объектов
void Game::generateFence() {
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

void Game::generateClouds() {
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

void Game::generateBirds() {
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

void Game::generateGroundSprites() {
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

void Game::generateObstacles() {
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

void Game::generateSingleFood() {
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

void Game::generateInitialFood() {
    food.clear();
    for (int i = 0; i < INITIAL_FOOD_COUNT; i++) {
        generateSingleFood();
    }
}

void Game::updateClouds() {
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

void Game::updateBirds() {
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


// Методы отрисовки
void Game::drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();
}

void Game::drawFloor() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
    model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(glm::vec3(0.3f, 0.6f, 0.2f));
    g_shaderManager.setUseTexture(floorModel.hasTexture);

    floorModel.draw();
}

void Game::drawSnake() {
    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];
        float x = (segment.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = segment.y * CELL_SIZE + 0.05f;
        float z = (segment.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 color = (i == 0) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.7f, 0.0f);
        drawModel(snakeModel, x, y, z, CELL_SIZE * 0.8f, color);
    }
}

void Game::drawFood() {
    for (const auto& apple : food) {
        float x = (apple.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = apple.y * CELL_SIZE + 0.05f;
        float z = (apple.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        drawModel(appleModel, x, y, z, CELL_SIZE * 0.8f, glm::vec3(1.0f, 0.8f, 0.2f));
    }
}

void Game::drawObstaclesAsTrees() {
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = (block.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
            float y = block.y * CELL_SIZE;
            float z = (block.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

            drawModel(treeModel, x, y, z, CELL_SIZE * 1.5f, glm::vec3(0.1f, 0.4f, 0.1f));
        }
    }
}

void Game::drawFence() {
    for (const auto& fenceBlock : fenceBlocks) {
        float x = (fenceBlock.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = fenceBlock.y * CELL_SIZE;
        float z = (fenceBlock.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 fenceColor(0.55f, 0.27f, 0.07f);
        drawModel(fenceModel, x, y, z, CELL_SIZE * 1.2f, fenceColor);
    }
}

void Game::drawClouds() {
    for (const auto& cloud : cloudSprites) {
        float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceToCenter < 8.0f) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cloud.position);
        model = glm::scale(model, glm::vec3(cloud.size));

        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - cloud.position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(cloud.color);
        g_shaderManager.setUseTexture(cloudModel.hasTexture);

        cloudModel.draw();
    }
}

void Game::drawBird(const Bird& bird) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, bird.position);
    model = glm::scale(model, glm::vec3(bird.size));

    if (glm::length(bird.direction) > 0.1f) {
        float angle = atan2f(bird.direction.x, bird.direction.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        float pitch = atan2f(bird.direction.y, glm::length(glm::vec2(bird.direction.x, bird.direction.z)));
        model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    model = glm::rotate(model, bird.wingAngle * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(bird.color);
    g_shaderManager.setUseTexture(birdModel.hasTexture);

    birdModel.draw();
}

void Game::drawBirds() {
    for (const auto& bird : birds) {
        drawBird(bird);
    }
}

void Game::drawGroundSprites() {
    for (const auto& flower : flowerSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, flower.position);
        model = glm::scale(model, glm::vec3(flower.size));

        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - flower.position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(flower.color);
        g_shaderManager.setUseTexture(flowerModel.hasTexture);

        flowerModel.draw();
    }
}

void Game::renderGame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    g_shaderManager.use3DShader();

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(g_camera.getPosition(), g_camera.getPosition() + g_camera.getFront(), g_camera.getUp());

    g_shaderManager.setViewMatrix(view);
    g_shaderManager.setProjectionMatrix(projection);

    drawFloor();
    drawGroundSprites();
    drawFence();
    drawObstaclesAsTrees();
    drawSnake();
    drawFood();
    drawClouds();
    drawBirds();
}
// UI методы
void Game::drawQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha) {
    std::cout << "=== DRAW_QUAD CALLED ===" << std::endl;
    std::cout << "Position: (" << x << ", " << y << "), Size: (" << width << "x" << height << ")" << std::endl;
    std::cout << "Color: (" << color.r << ", " << color.g << ", " << color.b << "), Alpha: " << alpha << std::endl;

    // Проверяем текущий шейдер ДО переключения
    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    std::cout << "Previous shader program: " << prevProgram << std::endl;

    g_shaderManager.useUIShader();

    // Проверяем текущий шейдер ПОСЛЕ переключения
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    std::cout << "Current UI shader program: " << currentProgram << std::endl;

    if (currentProgram == 0) {
        std::cout << "ERROR: UI shader program is 0!" << std::endl;
        return;
    }

    // Проверяем все uniform locations
    GLuint projLoc = glGetUniformLocation(currentProgram, "projection");
    GLuint modelLoc = glGetUniformLocation(currentProgram, "model");
    GLuint colorLoc = glGetUniformLocation(currentProgram, "color");
    GLuint alphaLoc = glGetUniformLocation(currentProgram, "alpha");

    std::cout << "Uniform locations - projection: " << projLoc
        << ", model: " << modelLoc
        << ", color: " << colorLoc
        << ", alpha: " << alphaLoc << std::endl;

    if (projLoc == -1 || modelLoc == -1 || colorLoc == -1 || alphaLoc == -1) {
        std::cout << "ERROR: Some uniform locations are invalid!" << std::endl;
    }

    // Устанавливаем матрицы и uniform
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    g_shaderManager.setUIProjection(projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));
    g_shaderManager.setUIModel(model);

    g_shaderManager.setUIColor(color);
    g_shaderManager.setUIAlpha(alpha);

    // Проверяем VAO
    std::cout << "UI VAO: " << uiVAO << std::endl;

    if (uiVAO == 0) {
        std::cout << "ERROR: UI VAO is 0!" << std::endl;
        return;
    }

    // Проверяем OpenGL ошибки перед рисованием
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error before draw: " << error << std::endl;
    }

    // Рисуем
    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Проверяем OpenGL ошибки после рисования
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error after draw: " << error << std::endl;
    }

    std::cout << "=== DRAW_QUAD FINISHED ===" << std::endl;
}

void Game::drawText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    float startX = x;
    float charWidth = 12 * scale;
    float charHeight = 18 * scale;

    for (char c : text) {
        if (c != ' ') {
            drawQuad(startX, y, charWidth * 0.6f, charHeight, color);
        }
        startX += charWidth;
    }
}

void Game::drawButton(const MenuButton& button) {
    std::cout << "DRAWING BUTTON: " << button.text
        << " at (" << button.x << ", " << button.y << ")"
        << " size (" << button.width << "x" << button.height << ")" << std::endl;

    // ВРЕМЕННО: используем яркие контрастные цвета
    glm::vec3 bgColor;
    if (button.hovered) {
        bgColor = glm::vec3(1.0f, 0.0f, 0.0f); // Красный при наведении
    }
    else {
        bgColor = glm::vec3(0.0f, 0.0f, 1.0f); // Синий обычный
    }

    // Рисуем основную кнопку БОЛЬШОГО размера
    drawQuad(button.x, button.y, button.width, button.height, bgColor, 1.0f);

    // ВРЕМЕННО: рисуем белую рамку для видимости
    drawQuad(button.x - 2, button.y - 2, button.width + 4, 2, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // верх
    drawQuad(button.x - 2, button.y + button.height, button.width + 4, 2, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // низ
    drawQuad(button.x - 2, button.y, 2, button.height, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // лево
    drawQuad(button.x + button.width, button.y, 2, button.height, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f); // право

    std::cout << "BUTTON " << button.text << " DRAWN" << std::endl;
}
void Game::drawMainMenu() {
    std::cout << "=== DRAWING MAIN MENU (BUTTONS ONLY) ===" << std::endl;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    // ТЕСТ: только кнопки, без текста
    for (auto& button : mainMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButton(button);
    }

    std::cout << "=== MAIN MENU BUTTONS DRAWN ===" << std::endl;
}

void Game::drawTestQuad(float x, float y, float width, float height, const glm::vec3& color) {
    std::cout << "TEST QUAD: (" << x << "," << y << ") size (" << width << "x" << height << ")" << std::endl;

    g_shaderManager.useUIShader();

    // Проекция с Y вниз (как в UI)
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f);
    g_shaderManager.setUIProjection(projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));
    g_shaderManager.setUIModel(model);

    g_shaderManager.setUIColor(color);

    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
void Game::drawPauseMenu() {
    std::cout << "=== DRAWING PAUSE MENU (BUTTONS ONLY) ===" << std::endl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Полупрозрачный черный фон
    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);

    // Только кнопки, без текста
    for (auto& button : pauseMenuButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButton(button);
    }

    glDisable(GL_BLEND);
    std::cout << "=== PAUSE MENU BUTTONS DRAWN ===" << std::endl;
}

void Game::drawGameOver() {
    std::cout << "=== DRAWING GAME OVER (BUTTONS ONLY) ===" << std::endl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Полупрозрачный черный фон
    drawQuad(0, 0, windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.7f);

    // Только кнопки, без текста
    for (auto& button : gameOverButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButton(button);
    }

    glDisable(GL_BLEND);
    std::cout << "=== GAME OVER BUTTONS DRAWN ===" << std::endl;
}



void Game::drawSettingsMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    drawText("SETTINGS", 520, 550, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    std::string speedText = "Game Speed: " + std::to_string(gameSpeed).substr(0, 4);
    drawText(speedText, 450, 400, 1.5f, glm::vec3(1.0f, 1.0f, 0.0f));
    drawText("Press + to increase, - to decrease", 400, 350, 1.2f, glm::vec3(0.8f, 0.8f, 0.8f));

    std::string nameText = "Player Name: " + playerName;
    drawText(nameText, 450, 250, 1.5f, glm::vec3(0.0f, 1.0f, 1.0f));

    for (auto& button : settingsButtons) {
        button.hovered = button.contains(mouseX, mouseY);
        drawButton(button);
    }
}

void Game::drawHighScoresMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.1f, 0.1f, 1.0f);

    drawText("HIGH SCORES", 480, 550, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    int yPos = 450;
    for (size_t i = 0; i < highScores.size() && i < 10; i++) {
        std::string scoreText = std::to_string(i + 1) + ". " + highScores[i].playerName +
            " - " + std::to_string(highScores[i].score) +
            " (" + highScores[i].date + ")";
        drawText(scoreText, 400, yPos, 1.2f, glm::vec3(1.0f, 1.0f, 0.0f));
        yPos -= 40;
    }

    drawText("Press B to go back", 500, 100, 1.2f, glm::vec3(1.0f, 1.0f, 1.0f));
}

void Game::drawControlsMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

    drawText("CONTROLS", 520, 550, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    int yPos = 450;
    drawText("LEFT/RIGHT - Turn snake", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;
    drawText("Q/E - Rotate camera", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;
    drawText("Mouse Wheel - Zoom", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;
    drawText("ESC - Pause/Menu", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;
    drawText("P - Toggle pause", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;
    drawText("R - Restart game", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;
    drawText("D - Debug info", 400, yPos, 1.2f, glm::vec3(0.0f, 1.0f, 0.0f)); yPos -= 40;

    drawText("Press B to go back", 500, 100, 1.2f, glm::vec3(1.0f, 1.0f, 1.0f));
}


// Инициализация UI
void Game::initUI() {
    // Создаем VAO и VBO для 2D прямоугольников
    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
    };

    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    int centerX = windowWidth / 2 - 100; // Центрируем по горизонтали
    // Инициализируем кнопки с правильными координатами
    mainMenuButtons.clear();
    mainMenuButtons.push_back(MenuButton("PLAY", centerX, 450, 200, 50));
    mainMenuButtons.push_back(MenuButton("SETTINGS", centerX, 370, 200, 50));
    mainMenuButtons.push_back(MenuButton("HIGH SCORES", centerX, 290, 200, 50));
    mainMenuButtons.push_back(MenuButton("CONTROLS", centerX, 210, 200, 50));
    mainMenuButtons.push_back(MenuButton("EXIT", centerX, 130, 200, 50));


    pauseMenuButtons.clear();
    pauseMenuButtons.push_back(MenuButton("RESUME", centerX, 450, 200, 50));        // ВЫШЕ
    pauseMenuButtons.push_back(MenuButton("SETTINGS", centerX, 370, 200, 50));      // ВЫШЕ
    pauseMenuButtons.push_back(MenuButton("MAIN MENU", centerX, 290, 200, 50));     // ВЫШЕ

    settingsButtons.clear();
    settingsButtons.push_back(MenuButton("BACK", centerX, 150, 200, 50));           // ВЫШЕ

    gameOverButtons.clear();
    gameOverButtons.push_back(MenuButton("RESTART", centerX, 450, 150, 50));        // ВЫШЕ
    gameOverButtons.push_back(MenuButton("MAIN MENU", centerX, 370, 150, 50));      // ВЫШЕ

    std::cout << "UI initialized successfully!" << std::endl;
}

// Методы создания моделей
void Game::loadAllModels() {
    std::cout << "Loading models..." << std::endl;

    createTexturedCubeModel(snakeModel);
    createTexturedSphereModel(foodModel);
    createTexturedCubeModel(obstacleModel);
    createTexturedFloorModel(floorModel);
    createFenceModel(fenceModel);
    createCloudModel(cloudModel);
    createAnimatedBirdModel(birdModel);
    createFlowerModel(flowerModel);
    createTreeModel(treeModel);
    createDetailedAppleModel(appleModel);

    std::cout << "All models loaded successfully!" << std::endl;
}

void Game::createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal) {
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        Vertex v1, v2, v3;
        v1.position = glm::vec3(cx, cy, 0.0f);
        v2.position = glm::vec3(cx + cos(angle1) * radius, cy + sin(angle1) * radius, 0.0f);
        v3.position = glm::vec3(cx + cos(angle2) * radius, cy + sin(angle2) * radius, 0.0f);

        v1.normal = v2.normal = v3.normal = normal;

        v1.texCoords = glm::vec2(0.5f, 0.5f);
        v2.texCoords = glm::vec2(0.5f + cos(angle1) * 0.5f, 0.5f + sin(angle1) * 0.5f);
        v3.texCoords = glm::vec2(0.5f + cos(angle2) * 0.5f, 0.5f + sin(angle2) * 0.5f);

        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
    }
}

void Game::createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color) {
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(x + cos(angle1) * radius, y, z + sin(angle1) * radius);
        glm::vec3 p2(x + cos(angle2) * radius, y, z + sin(angle2) * radius);
        glm::vec3 p3(x + cos(angle1) * radius, y + height, z + sin(angle1) * radius);
        glm::vec3 p4(x + cos(angle2) * radius, y + height, z + sin(angle2) * radius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }
}

void Game::createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color) {
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                cx + radius * sin(theta1) * cos(phi1),
                cy + radius * cos(theta1),
                cz + radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                cx + radius * sin(theta1) * cos(phi2),
                cy + radius * cos(theta1),
                cz + radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                cx + radius * sin(theta2) * cos(phi2),
                cy + radius * cos(theta2),
                cz + radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                cx + radius * sin(theta2) * cos(phi1),
                cy + radius * cos(theta2),
                cz + radius * sin(theta2) * sin(phi1)
            );

            glm::vec3 normal1 = glm::normalize(v1 - glm::vec3(cx, cy, cz));
            glm::vec3 normal2 = glm::normalize(v2 - glm::vec3(cx, cy, cz));
            glm::vec3 normal3 = glm::normalize(v3 - glm::vec3(cx, cy, cz));
            glm::vec3 normal4 = glm::normalize(v4 - glm::vec3(cx, cy, cz));

            glm::vec2 t1 = glm::vec2((float)j / segments, (float)i / rings);
            glm::vec2 t2 = glm::vec2((float)(j + 1) / segments, (float)i / rings);
            glm::vec2 t3 = glm::vec2((float)(j + 1) / segments, (float)(i + 1) / rings);
            glm::vec2 t4 = glm::vec2((float)j / segments, (float)(i + 1) / rings);

            vertices.push_back({ v1, normal1, t1 });
            vertices.push_back({ v2, normal2, t2 });
            vertices.push_back({ v3, normal3, t3 });

            vertices.push_back({ v3, normal3, t3 });
            vertices.push_back({ v4, normal4, t4 });
            vertices.push_back({ v1, normal1, t1 });
        }
    }
}

void Game::createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius) {
    createSpherePart(vertices, x, y, z, radius, 8, 4, glm::vec3(1.0f));
}

void Game::createCloudModel(Model& model) {
    createCloudPart(model.vertices, 0.0f, 0.0f, 0.0f, 0.4f);
    createCloudPart(model.vertices, 0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, -0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, 0.0f, 0.3f, 0.0f, 0.25f);
    createCloudPart(model.vertices, 0.2f, -0.1f, 0.0f, 0.25f);
    createCloudPart(model.vertices, -0.2f, -0.1f, 0.0f, 0.25f);

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createAnimatedBirdModel(Model& model) {
    createSpherePart(model.vertices, 0.0f, 0.0f, 0.0f, 0.3f, 12, 8, glm::vec3(1.0f));
    createSpherePart(model.vertices, 0.4f, 0.1f, 0.0f, 0.15f, 8, 6, glm::vec3(1.0f));

    Vertex beak[] = {
        {{0.55f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.7f, 0.1f, -0.05f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.7f, 0.1f, 0.05f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.55f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.7f, 0.15f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        {{0.7f, 0.1f, -0.05f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : beak) {
        model.vertices.push_back(v);
    }

    Vertex tail[] = {
        {{-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.6f, 0.0f, -0.15f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.6f, 0.0f, 0.15f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.6f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        {{-0.6f, 0.0f, -0.15f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : tail) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createFlowerModel(Model& model) {
    createCircle(model.vertices, 0.0f, 0.0f, 0.1f, 8);

    for (int i = 0; i < 6; i++) {
        float angle = i * 3.14159f / 3.0f;
        float x = cos(angle) * 0.25f;
        float y = sin(angle) * 0.25f;
        createCircle(model.vertices, x, y, 0.08f, 6);
    }

    Vertex stem[] = {
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : stem) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createTreeModel(Model& model) {
    createCylinder(model.vertices, 0.0f, 0.0f, 0.0f, 0.08f, 0.6f, 8, glm::vec3(0.4f, 0.2f, 0.1f));
    createSpherePart(model.vertices, 0.0f, 0.8f, 0.0f, 0.3f, 12, 8, glm::vec3(0.1f, 0.4f, 0.1f));

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createDetailedAppleModel(Model& model) {
    createSpherePart(model.vertices, 0.0f, 0.0f, 0.0f, 0.5f, 16, 12, glm::vec3(1.0f, 0.0f, 0.0f));
    createSpherePart(model.vertices, 0.0f, 0.3f, 0.0f, 0.1f, 8, 4, glm::vec3(0.3f, 0.2f, 0.1f));

    Vertex stem[] = {
        {{-0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : stem) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createTexturedCubeModel(Model& model) {
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (int i = 0; i < 288; i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex.normal = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        vertex.texCoords = glm::vec2(vertices[i + 6], vertices[i + 7]);
        model.vertices.push_back(vertex);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createTexturedSphereModel(Model& model) {
    const int segments = 16;
    const int rings = 16;
    const float radius = 0.5f;

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                radius * sin(theta1) * cos(phi1),
                radius * cos(theta1),
                radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                radius * sin(theta1) * cos(phi2),
                radius * cos(theta1),
                radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                radius * sin(theta2) * cos(phi2),
                radius * cos(theta2),
                radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                radius * sin(theta2) * cos(phi1),
                radius * cos(theta2),
                radius * sin(theta2) * sin(phi1)
            );

            glm::vec2 t1 = glm::vec2((float)j / segments, (float)i / rings);
            glm::vec2 t2 = glm::vec2((float)(j + 1) / segments, (float)i / rings);
            glm::vec2 t3 = glm::vec2((float)(j + 1) / segments, (float)(i + 1) / rings);
            glm::vec2 t4 = glm::vec2((float)j / segments, (float)(i + 1) / rings);

            model.vertices.push_back({ v1, glm::normalize(v1), t1 });
            model.vertices.push_back({ v2, glm::normalize(v2), t2 });
            model.vertices.push_back({ v3, glm::normalize(v3), t3 });

            model.vertices.push_back({ v3, glm::normalize(v3), t3 });
            model.vertices.push_back({ v4, glm::normalize(v4), t4 });
            model.vertices.push_back({ v1, glm::normalize(v1), t1 });
        }
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createTexturedFloorModel(Model& model) {
    float floorSize = 8.0f;

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(-floorSize, -0.1f, -floorSize);
    v2.position = glm::vec3(-floorSize, -0.1f, floorSize);
    v3.position = glm::vec3(floorSize, -0.1f, -floorSize);
    v4.position = glm::vec3(floorSize, -0.1f, floorSize);

    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    v1.normal = v2.normal = v3.normal = v4.normal = normal;

    v1.texCoords = glm::vec2(0.0f, 0.0f);
    v2.texCoords = glm::vec2(0.0f, 4.0f);
    v3.texCoords = glm::vec2(4.0f, 0.0f);
    v4.texCoords = glm::vec2(4.0f, 4.0f);

    model.vertices.push_back(v1);
    model.vertices.push_back(v3);
    model.vertices.push_back(v4);

    model.vertices.push_back(v4);
    model.vertices.push_back(v2);
    model.vertices.push_back(v1);

    model.hasTexture = true;
    model.setupBuffers();
}

void Game::createFenceModel(Model& model) {
    float vertices[] = {
        -0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 0.0f,
         0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 3.0f,
         0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 3.0f,
        -0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 3.0f,
        -0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 0.0f,
         0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 3.0f,
         0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 3.0f,
        -0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 3.0f,
        -0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.3f,  1.5f,  0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.3f,  1.5f, -0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.3f, -0.5f, -0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.3f, -0.5f, -0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.3f, -0.5f,  0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.3f,  1.5f,  0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.3f,  1.5f,  0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.3f,  1.5f, -0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.3f, -0.5f, -0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.3f, -0.5f, -0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.3f, -0.5f,  0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.3f,  1.5f,  0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         -0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (int i = 0; i < 240; i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex.normal = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        vertex.texCoords = glm::vec2(vertices[i + 6], vertices[i + 7]);
        model.vertices.push_back(vertex);
    }

    model.hasTexture = true;
    model.setupBuffers();
}
// В Game.cpp добавьте метод handleKeyPress:
void Game::handleKeyPress(int key) {
    switch (gameState) {
    case MAIN_MENU:
        switch (key) {
        case GLFW_KEY_1:
            gameState = PLAYING;
            initGame();
            break;
        case GLFW_KEY_2:
            gameState = SETTINGS;
            break;
        case GLFW_KEY_3:
            gameState = HIGH_SCORES;
            break;
        case GLFW_KEY_4:
            gameState = CONTROLS;
            break;
        case GLFW_KEY_5:
            // Выход обрабатывается в main.cpp
            break;
        }
        break;

    case PLAYING:
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
        case GLFW_KEY_Q:
            // Вращение камеры обрабатывается в main.cpp
            break;
        case GLFW_KEY_E:
            // Вращение камеры обрабатывается в main.cpp
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
        break;

    case PAUSED:
        switch (key) {
        case GLFW_KEY_Q:
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
        switch (key) {
        case GLFW_KEY_B:
            gameState = MAIN_MENU;
            break;
        case GLFW_KEY_EQUAL: // +
            gameSpeed = std::max(0.05f, gameSpeed - 0.02f);
            break;
        case GLFW_KEY_MINUS: // -
            gameSpeed = std::min(0.3f, gameSpeed + 0.02f);
            break;
        }
        break;

    case HIGH_SCORES:
    case CONTROLS:
        if (key == GLFW_KEY_B) {
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