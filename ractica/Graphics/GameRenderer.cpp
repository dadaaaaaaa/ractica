#include "../pch.h"
#include "GameRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h"
#include "../Utils/MathUtils.h"
#include "../Game/Game.h"

// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern Camera g_camera;
extern Game g_game;

GameRenderer::GameRenderer() {
   
}

void GameRenderer::initialize() {
    loadAllModels();
    initOpenGLSettings();
}

void GameRenderer::renderGame(const GameObjects& objects) {
    resetDepthState();
    // Очистка буферов
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    // Сбрасываем настройки глубины
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Отключаем смешивание для 3D объектов
    glDisable(GL_BLEND);

    g_shaderManager.use3DShader();

    // Матрицы проекции и вида
    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        1200.0f / 800.0f,
        0.2f, // Увеличиваем near plane
        100.0f);

    glm::mat4 view = glm::lookAt(g_camera.getPosition(),
        g_camera.getPosition() + g_camera.getFront(),
        g_camera.getUp());

    g_shaderManager.setViewMatrix(view);
    g_shaderManager.setProjectionMatrix(projection);

    // Отрисовка в порядке от дальних к ближним
    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
    drawObstaclesAsTrees(objects.getObstacles());
    drawFence(objects.getFenceBlocks());
    drawGroundSprites(objects.getFlowerSprites());
    drawFloor();
    drawFood(objects.getFood());
    drawSnake(objects.getSnake());

    // Включаем смешивание для UI
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GameRenderer::drawModel(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    // Смещение полигонов для предотвращения z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 2.0f);

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawFloor() {
    // МАКСИМАЛЬНЫЙ polygon offset для полного устранения мерцания
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(5.0f, 10.0f); // Очень большие значения

    // Опускаем пол НИЖЕ всех остальных объектов
    glm::mat4 model = glm::mat4(1.0f);

    // Пол должен быть достаточно большим
    float floorScale = GRID_WIDTH * CELL_SIZE * 3.0f; // В 3 раза больше игрового поля
    model = glm::scale(model, glm::vec3(floorScale, 1.0f, floorScale));

    // ОПУСКАЕМ на 0.05 единиц
    model = glm::translate(model, glm::vec3(0.0f, -0.05f, 0.0f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(glm::vec3(0.3f, 0.6f, 0.2f));
    g_shaderManager.setUseTexture(false); // Отключаем текстуру для пола

    g_shaderManager.setIsFloor(true);
    g_shaderManager.setCellSize(CELL_SIZE);

    floorModel.draw();

    g_shaderManager.setIsFloor(false);

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    if (snake.empty()) return;

    // Специальное смещение для змейки (самая ближняя)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f); // Больше смещение для змейки

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];
        float x = (segment.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = segment.y * CELL_SIZE + 0.1f; // Поднимаем выше на 0.1f
        float z = (segment.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        const Model* modelToDraw = &snakeBodyModel;
        glm::vec3 color = glm::vec3(0.0f, 0.7f, 0.0f);

        if (i == 0) {
            modelToDraw = &snakeHeadModel;
            color = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        else if (i == snake.size() - 1) {
            modelToDraw = &snakeTailModel;
            color = glm::vec3(0.0f, 0.5f, 0.0f);
        }

        float rotationAngle = calculateSegmentRotation(snake, i);
        drawModelWithRotation(*modelToDraw, x, y, z, CELL_SIZE * 0.8f, color, rotationAngle);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

float GameRenderer::calculateSegmentRotation(const std::vector<Point>& snake, size_t index) {
    if (snake.size() <= 1) return 0.0f;

    Point current = snake[index];
    Point next;

    // Для головы смотрим куда она движется
    if (index == 0) {
        next = snake[1];
    }
    // Для остальных сегментов смотрим откуда пришли
    else {
        next = snake[index - 1];
    }

    // Определяем направление
    if (current.x > next.x) return 90.0f;    // Движение вправо
    if (current.x < next.x) return -90.0f;   // Движение влево
    if (current.z > next.z) return 0.0f;     // Движение вперед
    if (current.z < next.z) return 180.0f;   // Движение назад

    return 0.0f;
}


void GameRenderer::drawModelWithRotation(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color, float rotationAngle) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 2.0f);

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();

    glDisable(GL_POLYGON_OFFSET_FILL);
}
void GameRenderer::drawFood(const std::vector<Point>& food) {
    // Смещение для еды
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 3.0f);

    for (const auto& apple : food) {
        float x = (apple.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = apple.y * CELL_SIZE + 0.1f; // Поднимаем на уровень пола
        float z = (apple.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        drawModel(appleModel, x, y, z, CELL_SIZE * 0.8f, glm::vec3(1.0f, 0.8f, 0.2f));
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    // Смещение для деревьев
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.2f, 2.5f);

    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = (block.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
            float y = block.y * CELL_SIZE; // Деревья на уровне пола
            float z = (block.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

            drawModel(treeModel, x, y, z, CELL_SIZE * 1.5f, glm::vec3(0.1f, 0.4f, 0.1f));
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    std::vector<Vertex> allFenceVertices;

    for (const auto& fenceBlock : fenceBlocks) {
        float x = (fenceBlock.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = fenceBlock.y * CELL_SIZE + 0.05f; // Поднимаем забор немного над полом
        float z = (fenceBlock.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 fenceColor(0.55f, 0.27f, 0.07f);
        glm::vec3 darkColor(0.45f, 0.17f, 0.05f);

        // Определяем углы
        bool isCorner = (fenceBlock.x == 0 && fenceBlock.z == 0) ||
            (fenceBlock.x == GRID_WIDTH - 1 && fenceBlock.z == 0) ||
            (fenceBlock.x == 0 && fenceBlock.z == GRID_DEPTH - 1) ||
            (fenceBlock.x == GRID_WIDTH - 1 && fenceBlock.z == GRID_DEPTH - 1);

        // Определяем стороны
        bool isNorth = (fenceBlock.z == 0);
        bool isSouth = (fenceBlock.z == GRID_DEPTH - 1);
        bool isWest = (fenceBlock.x == 0);
        bool isEast = (fenceBlock.x == GRID_WIDTH - 1);

        if (isCorner) {
            // Угловой столб - побольше
            createFencePost(allFenceVertices, x, y, z, 0.1f, 0.28f, fenceColor);

            // Соединительные перекладины
            if (fenceBlock.x == 0) { // Левый угол
                // Перекладина вправо
                createFenceRailHorizontal(allFenceVertices, x + CELL_SIZE / 2.0f, y + 0.2f, z,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
                createFenceRailHorizontal(allFenceVertices, x + CELL_SIZE / 2.0f, y + 0.1f, z,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
            }
            else { // Правый угол
                // Перекладина влево
                createFenceRailHorizontal(allFenceVertices, x - CELL_SIZE / 2.0f, y + 0.2f, z,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
                createFenceRailHorizontal(allFenceVertices, x - CELL_SIZE / 2.0f, y + 0.1f, z,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
            }

            if (fenceBlock.z == 0) { // Верхний угол
                // Перекладина вниз
                createFenceRailVertical(allFenceVertices, x, y + 0.2f, z + CELL_SIZE / 2.0f,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
                createFenceRailVertical(allFenceVertices, x, y + 0.1f, z + CELL_SIZE / 2.0f,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
            }
            else { // Нижний угол
                // Перекладина вверх
                createFenceRailVertical(allFenceVertices, x, y + 0.2f, z - CELL_SIZE / 2.0f,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
                createFenceRailVertical(allFenceVertices, x, y + 0.1f, z - CELL_SIZE / 2.0f,
                    CELL_SIZE / 2.0f, 0.03f, darkColor);
            }
        }
        else if (isNorth || isSouth || isWest || isEast) {
            // Обычный столбик (меньше для плотного забора)
            createFencePost(allFenceVertices, x, y, z, 0.06f, 0.25f, fenceColor);

            // Перекладины (теперь короче, так как столбики ближе)
            if (isNorth || isSouth) {
                // Горизонтальные перекладины (по X)
                createFenceRailHorizontal(allFenceVertices, x, y + 0.2f, z,
                    CELL_SIZE, 0.03f, darkColor);
                createFenceRailHorizontal(allFenceVertices, x, y + 0.1f, z,
                    CELL_SIZE, 0.03f, darkColor);
            }
            else {
                // Вертикальные перекладины (по Z)
                createFenceRailVertical(allFenceVertices, x, y + 0.2f, z,
                    CELL_SIZE, 0.03f, darkColor);
                createFenceRailVertical(allFenceVertices, x, y + 0.1f, z,
                    CELL_SIZE, 0.03f, darkColor);
            }
        }
    }

    // Рисуем весь забор как монолитную структуру
    if (!allFenceVertices.empty()) {
        Model monolithicFenceModel;
        monolithicFenceModel.vertices = allFenceVertices;
        monolithicFenceModel.hasTexture = false;
        monolithicFenceModel.setupBuffers();

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        g_shaderManager.setModelMatrix(modelMatrix);
        g_shaderManager.setColor(glm::vec3(0.55f, 0.27f, 0.07f));
        g_shaderManager.setUseTexture(false);

        monolithicFenceModel.draw();
    }
}void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
    // Смещение для облаков (самые дальние)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.1f, 0.5f); // Минимальное смещение

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

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawBird(const Bird& bird) {
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

void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
    // Смещение для птиц
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.8f, 1.5f);

    for (const auto& bird : birds) {
        drawBird(bird);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    // Смещение для цветов
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.7f, 1.2f);

    for (const auto& flower : flowerSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        // Поднимаем цветы на уровень пола
        glm::vec3 position = flower.position;
        position.y += 0.05f; // Немного поднимаем над полом
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(flower.size));

        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(flower.color);
        g_shaderManager.setUseTexture(flowerModel.hasTexture);

        flowerModel.draw();
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}
void GameRenderer::createSnakeHeadModel(Model& model) {
    // Простая треугольная голова
    createTexturedCubeModel(model);
    model.hasTexture = false;
}

void GameRenderer::createSnakeBodyModel(Model& model) {
    // Простой куб для тела
    createTexturedCubeModel(model);
    model.hasTexture = false;
}

void GameRenderer::createSnakeTailModel(Model& model) {
    // Простой куб для хвоста (можно сделать меньше)
    createTexturedCubeModel(model);
    model.hasTexture = false;
}
void GameRenderer::loadAllModels() {
    std::cout << "Loading models..." << std::endl;

    // Для змейки пытаемся загрузить из файла
    // Загружаем отдельные модели для змейки
    if (!loadModelFromFile(snakeHeadModel, "models/snake_head.obj", "textures/snake.png")) {
        std::cout << " Using fallback for snake head..." << std::endl;
        createSnakeHeadModel(snakeHeadModel); // Создаем простую голову
    }

    if (!loadModelFromFile(snakeBodyModel, "models/snake_body.obj", "textures/snake.png")) {
        std::cout << "Using fallback for snake body..." << std::endl;
        createSnakeBodyModel(snakeBodyModel); // Создаем простое тело
    }

    if (!loadModelFromFile(snakeTailModel, "models/snake_tail.obj", "textures/snake.png")) {
        std::cout << "Using fallback for snake tail..." << std::endl;
        createSnakeTailModel(snakeTailModel); // Создаем простой хвост
    }

    // Остальные модели создаем как раньше
    createTexturedSphereModel(foodModel);
    foodModel.hasTexture = false;

    createTexturedCubeModel(obstacleModel);
    obstacleModel.hasTexture = false;

    createTexturedFloorModel(floorModel);
    floorModel.hasTexture = false;

    createFenceModel(fenceModel);
    fenceModel.hasTexture = false;

    createCloudModel(cloudModel);
    cloudModel.hasTexture = false;

    createAnimatedBirdModel(birdModel);
    birdModel.hasTexture = false;

    createFlowerModel(flowerModel);
    flowerModel.hasTexture = false;

    createTreeModel(treeModel);
    treeModel.hasTexture = false;

    createDetailedAppleModel(appleModel);
    appleModel.hasTexture = false;

    std::cout << "All models loaded successfully!" << std::endl;
}
void GameRenderer::createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal) {
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

void GameRenderer::createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color) {
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

void GameRenderer::createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color) {
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

void GameRenderer::createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius) {
    createSpherePart(vertices, x, y, z, radius, 8, 4, glm::vec3(1.0f));
}

void GameRenderer::createCloudModel(Model& model) {
    createCloudPart(model.vertices, 0.0f, 0.0f, 0.0f, 0.4f);
    createCloudPart(model.vertices, 0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, -0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, 0.0f, 0.3f, 0.0f, 0.25f);
    createCloudPart(model.vertices, 0.2f, -0.1f, 0.0f, 0.25f);
    createCloudPart(model.vertices, -0.2f, -0.1f, 0.0f, 0.25f);

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createAnimatedBirdModel(Model& model) {
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

void GameRenderer::createFlowerModel(Model& model) {
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

void GameRenderer::createTreeModel(Model& model) {
    createCylinder(model.vertices, 0.0f, 0.0f, 0.0f, 0.08f, 0.6f, 8, glm::vec3(0.4f, 0.2f, 0.1f));
    createSpherePart(model.vertices, 0.0f, 0.8f, 0.0f, 0.3f, 12, 8, glm::vec3(0.1f, 0.4f, 0.1f));

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createDetailedAppleModel(Model& model) {
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

void GameRenderer::createTexturedCubeModel(Model& model) {
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

void GameRenderer::createTexturedSphereModel(Model& model) {
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

void GameRenderer::createTexturedFloorModel(Model& model) {
    float floorSize = 8.0f;

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(-floorSize, 0.0f, -floorSize); // y = 0
    v2.position = glm::vec3(-floorSize, 0.0f, floorSize);  // y = 0
    v3.position = glm::vec3(floorSize, 0.0f, -floorSize);  // y = 0
    v4.position = glm::vec3(floorSize, 0.0f, floorSize);   // y = 0

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

void GameRenderer::createFenceModel(Model& model) {
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
bool GameRenderer::loadModelFromFile(Model& model, const std::string& modelPath, const std::string& texturePath) {
    // Очищаем предыдущую модель
    model.vertices.clear();
    model.hasTexture = false;
    model.textureID = 0;

    // Пытаемся загрузить модель из .obj файла
    if (!loadOBJModel(model, modelPath)) {
        std::cout << "Failed to load model: " << modelPath << std::endl;
        return false;
    }

    // Пытаемся загрузить текстуру
    if (!loadTexture(model, texturePath)) {
        std::cout << "Failed to load texture: " << texturePath << " - using color only" << std::endl;
        model.hasTexture = false;
    }
    else {
        model.hasTexture = true;
        std::cout << "Loaded texture: " << texturePath << std::endl;
    }

    model.setupBuffers();
    std::cout << " Successfully loaded model: " << modelPath << " (" << model.vertices.size() << " vertices)" << std::endl;
    return true;
}

bool GameRenderer::loadOBJModel(Model& model, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Model file not found: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") { // Vertex position
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        }
        else if (type == "vn") { // Vertex normal
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        }
        else if (type == "vt") { // Texture coordinate
            float u, v;
            iss >> u >> v;
            texCoords.push_back(glm::vec2(u, 1.0f - v)); // Flip V coordinate for OpenGL
        }
        else if (type == "f") { // Face
            std::string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            std::vector<std::string> facePoints = { v1, v2, v3 };

            for (const auto& point : facePoints) {
                std::istringstream viss(point);
                std::string indexStr;
                std::vector<int> indices;

                // Парсим форматы: f v, f v/vt, f v/vt/vn, f v//vn
                while (std::getline(viss, indexStr, '/')) {
                    if (!indexStr.empty()) {
                        indices.push_back(std::stoi(indexStr) - 1); // OBJ uses 1-based indexing
                    }
                    else {
                        indices.push_back(-1);
                    }
                }

                Vertex vertex;

                // Position (обязательно)
                if (indices.size() > 0 && indices[0] >= 0 && indices[0] < positions.size()) {
                    vertex.position = positions[indices[0]];
                }
                else {
                    std::cout << " Invalid position index in face" << std::endl;
                    continue;
                }

                // Texture coordinates (опционально)
                if (indices.size() > 1 && indices[1] >= 0 && indices[1] < texCoords.size()) {
                    vertex.texCoords = texCoords[indices[1]];
                }
                else {
                    vertex.texCoords = glm::vec2(0.0f, 0.0f);
                }

                // Normal (опционально)
                if (indices.size() > 2 && indices[2] >= 0 && indices[2] < normals.size()) {
                    vertex.normal = normals[indices[2]];
                }
                else {
                    // Вычисляем нормаль по умолчанию
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                vertices.push_back(vertex);
            }
        }
    }

    file.close();

    if (vertices.empty()) {
        std::cout << " No vertices loaded from: " << path << std::endl;
        return false;
    }

    model.vertices = vertices;
    return true;
}

bool GameRenderer::loadTexture(Model& model, const std::string& path) {
    // Сначала пробуем загрузить из файла
    if (!loadTextureFromFile(model, path)) {
        // Если не получилось - создаем procedural текстуру
        std::cout << "Creating procedural texture for: " << path << std::endl;
        return createProceduralTexture(model, path);
    }
    return true;
}

bool GameRenderer::loadTextureFromFile(Model& model, const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Простая проверка на PNG (первые 8 байт)
    char header[8];
    file.read(header, 8);
    file.close();

    // Проверяем сигнатуру PNG
    bool isPNG = (header[0] == -119 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G');

    if (!isPNG) {
        std::cout << " Not a PNG file: " << path << std::endl;
        return false;
    }

    std::cout << " PNG file detected: " << path << std::endl;

    // Создаем procedural текстуру на основе типа
    return createProceduralTexture(model, path);
}

bool GameRenderer::createProceduralTexture(Model& model, const std::string& name) {
    const int TEXTURE_SIZE = 64;
    std::vector<unsigned char> textureData(TEXTURE_SIZE * TEXTURE_SIZE * 3); // RGB

    // Создаем procedural текстуру в зависимости от имени
    if (name.find("snake") != std::string::npos) {
        createSnakeTexture(textureData, TEXTURE_SIZE);
    }
    else {
        // Дефолтная текстура - шахматная доска
        createCheckerboardTexture(textureData, TEXTURE_SIZE);
    }

    // Загружаем в OpenGL
    glGenTextures(1, &model.textureID);
    glBindTexture(GL_TEXTURE_2D, model.textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, 0,
        GL_RGB, GL_UNSIGNED_BYTE, textureData.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    model.hasTexture = true;
    std::cout << " Created procedural texture: " << name << std::endl;
    return true;
}

void GameRenderer::createSnakeTexture(std::vector<unsigned char>& data, int size) {
    // Зеленая текстура с темными полосками для змейки
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 3;

            // Основной зеленый цвет
            unsigned char r = 0;
            unsigned char g = 200;
            unsigned char b = 0;

            // Добавляем полоски
            if ((x / 8) % 2 == 0) {
                g = 150; // Темно-зеленые полоски
            }

            // Добавляем шум для текстуры
            if (rand() % 100 < 30) {
                g += (rand() % 50) - 25;
                g = max(0, min(255, (int)g)); // Clamp
            }

            data[index] = r;
            data[index + 1] = g;
            data[index + 2] = b;
        }
    }
}

void GameRenderer::createCheckerboardTexture(std::vector<unsigned char>& data, int size) {
    // Шахматная текстура
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 3;

            bool isBlack = ((x / 8) + (y / 8)) % 2 == 0;

            if (isBlack) {
                data[index] = 50;
                data[index + 1] = 50;
                data[index + 2] = 50;
            }
            else {
                data[index] = 200;
                data[index + 1] = 200;
                data[index + 2] = 200;
            }
        }
    }
}
void GameRenderer::setupGLFWHints() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Всегда включена
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
}

bool GameRenderer::initGLEW() {
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        return false;
    }
    glGetError(); // Игнорируем первую ошибку
    return true;
}

void GameRenderer::initOpenGLSettings() {
    // Базовые настройки OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // Меняем GL_LEQUAL на GL_LESS
    glClearDepth(1.0f);
    glDepthRange(0.0, 1.0);
    glDepthMask(GL_TRUE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GLint samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (samples > 0) {
        glEnable(GL_MULTISAMPLE);
        std::cout << "MSAA enabled: " << samples << "x samples" << std::endl;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void GameRenderer::checkGLError(const char* functionName) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL error " << error << " in " << functionName << std::endl;
    }
}

void GameRenderer::checkDoubleBufferSupport(GLFWwindow* window) {
    int doubleBuffer = glfwGetWindowAttrib(window, GLFW_DOUBLEBUFFER);
    if (doubleBuffer) {
        std::cout << "Double buffering is ENABLED" << std::endl;
    }
    else {
        std::cout << "WARNING: Double buffering is DISABLED" << std::endl;
    }
}

void GameRenderer::setupVSync(GLFWwindow* window, bool enabled) {
    if (enabled) {
        glfwSwapInterval(1);
        std::cout << "VSync ENABLED" << std::endl;
    }
    else {
        glfwSwapInterval(0);
        std::cout << "VSync DISABLED" << std::endl;
    }
}

void GameRenderer::printGraphicsInfo() {
    std::cout << "=== GRAPHICS SYSTEM INFO ===" << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    bool hasDoubleBuffer = false;

    for (GLint i = 0; i < numExtensions; i++) {
        const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (strstr(extension, "double_buffer") != nullptr) {
            hasDoubleBuffer = true;
        }
    }
    std::cout << "Double buffer support: " << (hasDoubleBuffer ? "YES" : "NO") << std::endl;
    std::cout << "===========================" << std::endl;
}

// Вспомогательные функции-колбэки
// Колбэки GLFW (добавляем в начало main.cpp или в GameRenderer.cpp)
namespace {
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            g_game.handleKeyPress(key);

            // Обработка ESC отдельно
            if (key == GLFW_KEY_ESCAPE) {
                GameState currentState = g_game.getGameState();
                if (currentState == PLAYING) {
                    g_game.setGameState(PAUSED);
                }
                else if (currentState == PAUSED) {
                    g_game.setGameState(PLAYING);
                }
                else if (currentState == MAIN_MENU) {
                    glfwSetWindowShouldClose(window, GL_TRUE);
                }
                else {
                    g_game.setGameState(MAIN_MENU);
                }
            }
        }
    }

    void charCallback(GLFWwindow* window, unsigned int codepoint) {
        if (g_game.getGameState() == SETTINGS && g_game.isNameInputActive()) {
            if (codepoint < 128) {
                char c = static_cast<char>(codepoint);
                if (isalnum(c) || c == ' ' || c == '-' || c == '_') {
                    g_game.addCharacterToName(c);
                }
            }
        }
    }

    void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        g_game.handleMouseScroll(yoffset);
    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            g_game.setMousePressed(true);
            g_game.handleMouseClick();
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            g_game.setMousePressed(false);
        }
    }

    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        g_game.setMousePosition(xpos, ypos);
    }

    void windowSizeCallback(GLFWwindow* window, int width, int height) {
        g_game.setWindowSize(width, height);
        glViewport(0, 0, width, height);
    }
}
void GameRenderer::resetDepthState() {
    // Сбрасываем состояние глубины между кадрами
    glDepthMask(GL_TRUE);
    glClearDepth(1.0f);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    // Отключаем все смещения полигонов
    glDisable(GL_POLYGON_OFFSET_FILL);
}
void GameRenderer::setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}
// Создание столба забора (низкого)
void GameRenderer::createFencePost(std::vector<Vertex>& vertices, float x, float y, float z,
    float width, float height, const glm::vec3& color) {
    // Создаем низкий вертикальный столб
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    // Передняя грань
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y, z - halfWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z - halfWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z - halfWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z - halfWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });

    // Задняя грань
    vertices.push_back({ {x - halfWidth, y, z + halfWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y, z + halfWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z + halfWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z + halfWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });

    // Левая грань
    vertices.push_back({ {x - halfWidth, y + height, z + halfWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z - halfWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z + halfWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z + halfWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });

    // Правая грань
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z - halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z - halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z - halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z + halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });

    // Верхняя грань
    vertices.push_back({ {x - halfWidth, y + height, z - halfWidth}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z - halfWidth}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z + halfWidth}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z - halfWidth}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });

    // Нижняя грань
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z - halfWidth}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z + halfWidth}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y, z + halfWidth}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y, z + halfWidth}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
}

// Создание горизонтальной перекладины (вытянутой по оси X)
void GameRenderer::createFenceRailHorizontal(std::vector<Vertex>& vertices, float x, float y, float z,
    float length, float thickness, const glm::vec3& color) {
    float halfLength = length / 2.0f;
    float halfThickness = thickness / 2.0f;
    float railWidth = 0.04f; // Толщина перекладины по Z

    // Передняя грань (по Z)
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });

    // Задняя грань (по Z)
    vertices.push_back({ {x - halfLength, y - halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });

    // Левая грань (по X)
    vertices.push_back({ {x - halfLength, y + halfThickness, z + railWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z - railWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z + railWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z + railWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });

    // Правая грань (по X)
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z - railWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z - railWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z - railWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z + railWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });

    // Верхняя грань (по Y)
    vertices.push_back({ {x - halfLength, y + halfThickness, z - railWidth}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z - railWidth}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z + railWidth}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z - railWidth}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });

    // Нижняя грань (по Y)
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z - railWidth}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z + railWidth}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z + railWidth}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z + railWidth}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
}

// Создание вертикальной перекладины (вытянутой по оси Z)
void GameRenderer::createFenceRailVertical(std::vector<Vertex>& vertices, float x, float y, float z,
    float length, float thickness, const glm::vec3& color) {
    float halfLength = length / 2.0f;
    float halfThickness = thickness / 2.0f;
    float railWidth = 0.04f; // Толщина перекладины по X

    // Левая грань (по X)
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z + halfLength}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z - halfLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });

    // Правая грань (по X)
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z + halfLength}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z - halfLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });

    // Передняя грань (по Z)
    vertices.push_back({ {x - railWidth, y + halfThickness, z - halfLength}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z - halfLength}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z - halfLength}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} });

    // Задняя грань (по Z)
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z + halfLength}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z + halfLength}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z + halfLength}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} });

    // Верхняя грань (по Y)
    vertices.push_back({ {x - railWidth, y + halfThickness, z - halfLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z - halfLength}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z - halfLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });

    // Нижняя грань (по Y)
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z + halfLength}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z + halfLength}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z + halfLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
}


// Создание углового столба (просто квадратный столб)
void GameRenderer::createFenceCorner(std::vector<Vertex>& vertices, float x, float y, float z,
    const glm::vec3& color) {
    // Угловой столб - такой же как обычный, но немного шире (0.1 ширины, 0.3 высоты)
    createFencePost(vertices, x, y, z, 0.1f, 0.3f, color);

    // Добавляем соединительные перекладины без зазоров
    // Горизонтальная перекладина (влево/вправо)
    createFenceRailHorizontal(vertices, x + CELL_SIZE / 2.0f, y + 0.25f, z,
        CELL_SIZE, 0.04f, color);
    createFenceRailHorizontal(vertices, x - CELL_SIZE / 2.0f, y + 0.25f, z,
        CELL_SIZE, 0.04f, color);

    // Вертикальная перекладина (вперед/назад)
    createFenceRailVertical(vertices, x, y + 0.25f, z + CELL_SIZE / 2.0f,
        CELL_SIZE, 0.04f, color);
    createFenceRailVertical(vertices, x, y + 0.25f, z - CELL_SIZE / 2.0f,
        CELL_SIZE, 0.04f, color);
}