#include "../pch.h"
#include "GameRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h"
#include "../Utils/MathUtils.h"

// Глобальные экземпляры
extern ShaderManager g_shaderManager;
extern Camera g_camera;

// Конструктор рендерера - инициализирует флаги
GameRenderer::GameRenderer() : modelsLoaded(false) {
}

// Инициализирует рендерер и загружает все модели
void GameRenderer::initialize() {
    if (modelsLoaded) return;

    std::cout << "🎮 Initializing game renderer..." << std::endl;
    loadAllModels();
    modelsLoaded = true;
    std::cout << "✅ Game renderer initialized successfully" << std::endl;
}

// Очищает ресурсы рендерера
void GameRenderer::cleanup() {
    std::cout << "🧹 Cleaning up renderer resources..." << std::endl;

    // Очищаем модели (у каждой модели есть свой деструктор)
    snakeHeadModel.cleanup();
    snakeBodyModel.cleanup();
    snakeTailModel.cleanup();
    appleModel.cleanup();
    treeModel.cleanup();
    floorModel.cleanup();
    fenceModel.cleanup();
    cloudModel.cleanup();
    birdModel.cleanup();
    flowerModel.cleanup();

    modelsLoaded = false;
    std::cout << "✅ Renderer cleanup complete" << std::endl;
}

// Основной метод рендеринга игровой сцены
void GameRenderer::renderGame(const GameObjects& objects) {
    // Очищаем буферы и устанавливаем цвет неба
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Голубой цвет неба

    // Активируем 3D шейдер для рендеринга игровых объектов
    g_shaderManager.use3DShader();

    // Получаем матрицы проекции и вида от камеры
    float aspectRatio = static_cast<float>(DEFAULT_WINDOW_WIDTH) / DEFAULT_WINDOW_HEIGHT;
    glm::mat4 projection = g_camera.getProjectionMatrix(aspectRatio);
    glm::mat4 view = g_camera.getViewMatrix();

    // Устанавливаем матрицы в шейдер
    g_shaderManager.setViewMatrix(view);
    g_shaderManager.setProjectionMatrix(projection);

    // Рендерим все элементы игровой сцены в правильном порядке
    drawFloor();                                    // Пол/земля
    drawGroundSprites(objects.getFlowerSprites());  // Цветы на земле
    drawFence(objects.getFenceBlocks());            // Забор по границам
    drawObstaclesAsTrees(objects.getObstacles());   // Деревья-препятствия
    drawSnake(objects.getSnake());                  // Змейка
    drawFood(objects.getFood());                    // Еда (яблоки)
    drawClouds(objects.getCloudSprites());          // Облака на небе
    drawBirds(objects.getBirds());                  // Птицы в небе
}

// Рендерит HUD (интерфейс поверх игровой сцены)
void GameRenderer::renderHUD(const GameObjects& objects, const GameUI& ui) {
    // Включаем смешивание для полупрозрачного HUD
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Отключаем тест глубины чтобы HUD был поверх всего
    glDisable(GL_DEPTH_TEST);

    // Здесь можно добавить отрисовку игрового HUD:
    // - Счет
    // - Таймер
    // - Индикатор скорости
    // - Индикатор длины змейки

    // Восстанавливаем настройки
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Рендерит модель в указанной позиции с заданным цветом и масштабом
void GameRenderer::drawModel(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();
}

// Рендерит модель с вращением вокруг оси Y
void GameRenderer::drawModelWithRotation(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color, float rotationAngle) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();
}

// ============================================================================
// МЕТОДЫ РЕНДЕРИНГА КОНКРЕТНЫХ ОБЪЕКТОВ
// ============================================================================

// Рендерит пол игрового поля
void GameRenderer::drawFloor() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
    model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(glm::vec3(0.3f, 0.6f, 0.2f)); // Зеленый цвет травы
    g_shaderManager.setUseTexture(floorModel.hasTexture);

    floorModel.draw();
}

// Рендерит змейку с разными моделями для головы, тела и хвоста
void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    if (snake.empty()) return;

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];

        // Конвертируем координаты сетки в мировые координаты
        float x = (segment.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = segment.y * CELL_SIZE + 0.05f; // Небольшой подъем над землей
        float z = (segment.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        // Выбираем модель и цвет в зависимости от позиции в змейке
        const Model* modelToDraw = &snakeBodyModel;
        glm::vec3 color = glm::vec3(0.0f, 0.7f, 0.0f); // Зеленый для тела

        if (i == 0) {
            // Голова змейки
            modelToDraw = &snakeHeadModel;
            color = glm::vec3(0.0f, 1.0f, 0.0f); // Ярко-зеленый
        }
        else if (i == snake.size() - 1) {
            // Хвост змейки
            modelToDraw = &snakeTailModel;
            color = glm::vec3(0.0f, 0.5f, 0.0f); // Темно-зеленый
        }

        // Вычисляем угол поворота для сегмента и рисуем
        float rotationAngle = calculateSegmentRotation(snake, i);
        drawModelWithRotation(*modelToDraw, x, y, z, CELL_SIZE * 0.8f, color, rotationAngle);
    }
}

// Вычисляет угол поворота для сегмента змейки на основе направления движения
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

    // Определяем направление и возвращаем соответствующий угол
    if (current.x > next.x) return 90.0f;    // Движение вправо
    if (current.x < next.x) return -90.0f;   // Движение влево
    if (current.z > next.z) return 0.0f;     // Движение вперед
    if (current.z < next.z) return 180.0f;   // Движение назад

    return 0.0f; // Направление по умолчанию
}

// Рендерит еду (яблоки) на игровом поле
void GameRenderer::drawFood(const std::vector<Point>& food) {
    for (const auto& apple : food) {
        float x = (apple.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = apple.y * CELL_SIZE + 0.05f; // Небольшой подъем над землей
        float z = (apple.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        drawModel(appleModel, x, y, z, CELL_SIZE * 0.8f, glm::vec3(1.0f, 0.8f, 0.2f));
    }
}

// Рендерит препятствия в виде деревьев
void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = (block.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
            float y = block.y * CELL_SIZE;
            float z = (block.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

            drawModel(treeModel, x, y, z, CELL_SIZE * 1.5f, glm::vec3(0.1f, 0.4f, 0.1f));
        }
    }
}

// Рендерит забор по границам игрового поля
void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    for (const auto& fenceBlock : fenceBlocks) {
        float x = (fenceBlock.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = fenceBlock.y * CELL_SIZE;
        float z = (fenceBlock.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 fenceColor(0.55f, 0.27f, 0.07f); // Коричневый цвет забора
        drawModel(fenceModel, x, y, z, CELL_SIZE * 1.2f, fenceColor);
    }
}

// Рендерит облака на небе (только те, что далеко от центра)
void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
    for (const auto& cloud : cloudSprites) {
        // Пропускаем облака слишком близко к центру
        float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceToCenter < 8.0f) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cloud.position);
        model = glm::scale(model, glm::vec3(cloud.size));

        // Поворачиваем облако чтобы оно всегда было ориентировано на камеру (billboard)
        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - cloud.position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(cloud.color);
        g_shaderManager.setUseTexture(cloudModel.hasTexture);

        cloudModel.draw();
    }
}

// Рендерит одну птицу с анимацией крыльев
void GameRenderer::drawBird(const Bird& bird) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, bird.position);
    model = glm::scale(model, glm::vec3(bird.size));

    // Ориентируем птицу в направлении движения
    if (glm::length(bird.direction) > 0.1f) {
        float yaw = atan2f(bird.direction.x, bird.direction.z);
        model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));

        float pitch = atan2f(bird.direction.y, glm::length(glm::vec2(bird.direction.x, bird.direction.z)));
        model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    // Анимация крыльев
    model = glm::rotate(model, bird.wingAngle * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(bird.color);
    g_shaderManager.setUseTexture(birdModel.hasTexture);

    birdModel.draw();
}

// Рендерит всех птиц
void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
    for (const auto& bird : birds) {
        drawBird(bird);
    }
}

// Рендерит цветы на земле (billboard спрайты)
void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    for (const auto& flower : flowerSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, flower.position);
        model = glm::scale(model, glm::vec3(flower.size));

        // Поворачиваем цветок чтобы он всегда был ориентирован на камеру
        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - flower.position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(flower.color);
        g_shaderManager.setUseTexture(flowerModel.hasTexture);

        flowerModel.draw();
    }
}

// ============================================================================
// МЕТОДЫ СОЗДАНИЯ МОДЕЛЕЙ ЗМЕЙКИ
// ============================================================================

// Создает простую треугольную модель головы змейки
void GameRenderer::createSnakeHeadModel(Model& model) {
    // Простая треугольная голова
    createTexturedCubeModel(model);
    model.hasTexture = false;
}

// Создает простую кубическую модель тела змейки
void GameRenderer::createSnakeBodyModel(Model& model) {
    // Простой куб для тела
    createTexturedCubeModel(model);
    model.hasTexture = false;
}

// Создает простую кубическую модель хвоста змейки
void GameRenderer::createSnakeTailModel(Model& model) {
    // Простой куб для хвоста (можно сделать меньше)
    createTexturedCubeModel(model);
    model.hasTexture = false;
}

// ============================================================================
// ЗАГРУЗКА ВСЕХ МОДЕЛЕЙ
// ============================================================================

// Загружает или создает все модели, используемые в игре
void GameRenderer::loadAllModels() {
    std::cout << "📦 Loading models..." << std::endl;

    // Для змейки пытаемся загрузить из файла
    // Загружаем отдельные модели для змейки
    if (!loadModelFromFile(snakeHeadModel, "models/snake_head.obj", "textures/snake.png")) {
        std::cout << "⚠️ Using fallback for snake head..." << std::endl;
        createSnakeHeadModel(snakeHeadModel); // Создаем простую голову
    }

    if (!loadModelFromFile(snakeBodyModel, "models/snake_body.obj", "textures/snake.png")) {
        std::cout << "⚠️ Using fallback for snake body..." << std::endl;
        createSnakeBodyModel(snakeBodyModel); // Создаем простое тело
    }

    if (!loadModelFromFile(snakeTailModel, "models/snake_tail.obj", "textures/snake.png")) {
        std::cout << "⚠️ Using fallback for snake tail..." << std::endl;
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

    std::cout << "✅ All models loaded successfully!" << std::endl;
}

// ============================================================================
// ГЕОМЕТРИЧЕСКИЕ ПРИМИТИВЫ
// ============================================================================

// Создает круг из треугольников
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

// Создает цилиндр с заданными параметрами
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

// Создает часть сферы (сетка треугольников)
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

// ============================================================================
// МЕТОДЫ СОЗДАНИЯ КОНКРЕТНЫХ МОДЕЛЕЙ
// ============================================================================

// Создает часть облака (сферический элемент)
void GameRenderer::createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius) {
    createSpherePart(vertices, x, y, z, radius, 8, 4, glm::vec3(1.0f));
}

// Создает модель облака из нескольких сферических частей
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

// Создает анимированную модель птицы
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

// Создает модель цветка с лепестками и стеблем
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

// Создает модель дерева (ствол + крона)
void GameRenderer::createTreeModel(Model& model) {
    createCylinder(model.vertices, 0.0f, 0.0f, 0.0f, 0.08f, 0.6f, 8, glm::vec3(0.4f, 0.2f, 0.1f));
    createSpherePart(model.vertices, 0.0f, 0.8f, 0.0f, 0.3f, 12, 8, glm::vec3(0.1f, 0.4f, 0.1f));

    model.hasTexture = true;
    model.setupBuffers();
}

// Создает детализированную модель яблока
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

// ============================================================================
// БАЗОВЫЕ ГЕОМЕТРИЧЕСКИЕ МОДЕЛИ
// ============================================================================

// Создает текстурированную кубическую модель
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

// Создает текстурированную сферическую модель
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

// Создает текстурированную модель пола
void GameRenderer::createTexturedFloorModel(Model& model) {
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

// Создает текстурированную модель забора
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

// ============================================================================
// ЗАГРУЗКА МОДЕЛЕЙ ИЗ ФАЙЛОВ
// ============================================================================

// Загружает модель из файла OBJ и текстуру
bool GameRenderer::loadModelFromFile(Model& model, const std::string& modelPath, const std::string& texturePath) {
    // Очищаем предыдущую модель
    model.vertices.clear();
    model.hasTexture = false;
    model.textureID = 0;

    // Пытаемся загрузить модель из .obj файла
    if (!loadOBJModel(model, modelPath)) {
        std::cout << "❌ Failed to load model: " << modelPath << std::endl;
        return false;
    }

    // Пытаемся загрузить текстуру
    if (!loadTexture(model, texturePath)) {
        std::cout << "⚠️ Failed to load texture: " << texturePath << " - using color only" << std::endl;
        model.hasTexture = false;
    }
    else {
        model.hasTexture = true;
        std::cout << "✅ Loaded texture: " << texturePath << std::endl;
    }

    model.setupBuffers();
    std::cout << "✅ Successfully loaded model: " << modelPath << " (" << model.vertices.size() << " vertices)" << std::endl;
    return true;
}

// Загружает модель из файла формата OBJ
bool GameRenderer::loadOBJModel(Model& model, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "❌ Model file not found: " << path << std::endl;
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
                    std::cout << "❌ Invalid position index in face" << std::endl;
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
        std::cout << "❌ No vertices loaded from: " << path << std::endl;
        return false;
    }

    model.vertices = vertices;
    return true;
}

// Загружает текстуру для модели
bool GameRenderer::loadTexture(Model& model, const std::string& path) {
    // Сначала пробуем загрузить из файла
    if (!loadTextureFromFile(model, path)) {
        // Если не получилось - создаем procedural текстуру
        std::cout << "⚠️ Creating procedural texture for: " << path << std::endl;
        return createProceduralTexture(model, path);
    }
    return true;
}

// Загружает текстуру из файла изображения
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
        std::cout << "❌ Not a PNG file: " << path << std::endl;
        return false;
    }

    std::cout << "✅ PNG file detected: " << path << std::endl;

    // Создаем procedural текстуру на основе типа
    return createProceduralTexture(model, path);
}

// Создает procedural текстуру на основе имени файла
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
    std::cout << "✅ Created procedural texture: " << name << std::endl;
    return true;
}

// Создает текстуру для змейки (зеленая с полосками)
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

// Создает шахматную текстуру
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