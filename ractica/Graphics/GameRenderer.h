#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "Model.h"
#include "ShaderManager.h"
#include "Camera.h"
#include "../Objects/GameObjects.h"
#include "../UI/GameUI.h"

// Основной класс рендеринга игрового мира
class GameRenderer {
private:
    // ============================================================================
    // МОДЕЛИ ИГРОВЫХ ОБЪЕКТОВ
    // ============================================================================

    Model snakeHeadModel;      // Модель головы змейки
    Model snakeBodyModel;      // Модель тела змейки  
    Model snakeTailModel;      // Модель хвоста змейки
    Model foodModel;           // Модель еды (яблоко)
    Model obstacleModel;       // Модель препятствия
    Model floorModel;          // Модель пола/земли
    Model fenceModel;          // Модель забора
    Model cloudModel;          // Модель облака
    Model birdModel;           // Модель птицы
    Model flowerModel;         // Модель цветка
    Model treeModel;           // Модель дерева
    Model appleModel;          // Модель яблока (альтернативная)

    bool modelsLoaded = false; // Флаг загрузки моделей

public:
    // ============================================================================
    // КОНСТРУКТОР И ДЕСТРУКТОР
    // ============================================================================

    GameRenderer();
    ~GameRenderer() = default;

    // ============================================================================
    // ОСНОВНЫЕ МЕТОДЫ ЖИЗНЕННОГО ЦИКЛА
    // ============================================================================

    // Инициализирует рендерер и загружает все модели
    void initialize();

    // Очищает ресурсы рендерера
    void cleanup();

    // Рендерит всю игровую сцену
    void renderGame(const GameObjects& objects);

    // Рендерит HUD (интерфейс поверх игровой сцены)
    void renderHUD(const GameObjects& objects, const GameUI& ui);

    // ============================================================================
    // МЕТОДЫ ОТРИСОВКИ КОНКРЕТНЫХ ОБЪЕКТОВ
    // ============================================================================

    // Основной метод отрисовки модели
    void drawModel(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color);

    // Отрисовка модели с вращением
    void drawModelWithRotation(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color, float rotationAngle);

    // Отрисовка игровых объектов
    void drawFloor();
    void drawSnake(const std::vector<Point>& snake);
    void drawFood(const std::vector<Point>& food);
    void drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles);
    void drawFence(const std::vector<Point>& fenceBlocks);
    void drawClouds(const std::vector<Sprite>& cloudSprites);
    void drawBird(const Bird& bird);
    void drawBirds(const std::vector<Bird>& birds);
    void drawGroundSprites(const std::vector<Sprite>& flowerSprites);

    // ============================================================================
    // ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ РЕНДЕРИНГА
    // ============================================================================

    // Вычисляет угол поворота для сегмента змейки
    float calculateSegmentRotation(const std::vector<Point>& snake, size_t index);

    // Проверяет, инициализирован ли рендерер
    bool isInitialized() const { return modelsLoaded; }

    // ============================================================================
    // СИСТЕМА ЗАГРУЗКИ МОДЕЛЕЙ И ТЕКСТУР
    // ============================================================================

    // Загружает все модели игры
    void loadAllModels();

    // Загружает модель из файла OBJ
    bool loadModelFromFile(Model& model, const std::string& modelPath,
        const std::string& texturePath);

    // Парсит файл формата OBJ
    bool loadOBJModel(Model& model, const std::string& path);

    // Загружает текстуру для модели
    bool loadTexture(Model& model, const std::string& path);

    // Загружает текстуру из файла изображения
    bool loadTextureFromFile(Model& model, const std::string& path);

    // ============================================================================
    // GENERATIVE ТЕКСТУРЫ (PROCEDURAL)
    // ============================================================================

    // Создает procedural текстуру
    bool createProceduralTexture(Model& model, const std::string& name);

    // Создает текстуру для змейки
    void createSnakeTexture(std::vector<unsigned char>& data, int size);

    // Создает шахматную текстуру
    void createCheckerboardTexture(std::vector<unsigned char>& data, int size);

    // ============================================================================
    // МЕТОДЫ СОЗДАНИЯ ГЕОМЕТРИЧЕСКИХ ПРИМИТИВОВ
    // ============================================================================

    // Базовые геометрические примитивы
    void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius,
        int segments, const glm::vec3& normal = glm::vec3(0.0f, 0.0f, 1.0f));

    void createCylinder(std::vector<Vertex>& vertices, float x, float y, float z,
        float radius, float height, int segments, const glm::vec3& color);

    void createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz,
        float radius, int segments, int rings, const glm::vec3& color);

    void createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius);

    // ============================================================================
    // МЕТОДЫ СОЗДАНИЯ КОНКРЕТНЫХ МОДЕЛЕЙ
    // ============================================================================

    // Модели для змейки (fallback)
    void createSnakeHeadModel(Model& model);
    void createSnakeBodyModel(Model& model);
    void createSnakeTailModel(Model& model);

    // Декоративные модели
    void createCloudModel(Model& model);
    void createAnimatedBirdModel(Model& model);
    void createFlowerModel(Model& model);
    void createTreeModel(Model& model);
    void createDetailedAppleModel(Model& model);

    // Базовые геометрические модели
    void createTexturedCubeModel(Model& model);
    void createTexturedSphereModel(Model& model);
    void createTexturedFloorModel(Model& model);
    void createFenceModel(Model& model);

    // ============================================================================
    // ГЕТТЕРЫ ДЛЯ ДОСТУПА К МОДЕЛЯМ И СИСТЕМАМ
    // ============================================================================

    // Геттеры моделей
    const Model& getSnakeHeadModel() const { return snakeHeadModel; }
    const Model& getSnakeBodyModel() const { return snakeBodyModel; }
    const Model& getSnakeTailModel() const { return snakeTailModel; }
    const Model& getFoodModel() const { return foodModel; }
    const Model& getObstacleModel() const { return obstacleModel; }
    const Model& getFloorModel() const { return floorModel; }
    const Model& getFenceModel() const { return fenceModel; }
    const Model& getCloudModel() const { return cloudModel; }
    const Model& getBirdModel() const { return birdModel; }
    const Model& getFlowerModel() const { return flowerModel; }
    const Model& getTreeModel() const { return treeModel; }
    const Model& getAppleModel() const { return appleModel; }

    // Геттеры систем
    Camera& getCamera() { return camera; }
    const Camera& getCamera() const { return camera; }
    ShaderManager& getShaderManager() { return shaderManager; }
    const ShaderManager& getShaderManager() const { return shaderManager; }

private:
    // ============================================================================
    // ПРИВАТНЫЕ СИСТЕМЫ РЕНДЕРИНГА
    // ============================================================================

    ShaderManager shaderManager; // Менеджер шейдеров
    Camera camera;              // Камера для обзора сцены

    // ============================================================================
    // ПРИВАТНЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    // ============================================================================

    // Проверяет готовность рендерера к работе
    bool checkRenderReadiness() const;

    // Настраивает общие параметры рендеринга
    void setupRenderState();

    // Восстанавливает состояние рендеринга после HUD
    void restoreRenderState();
};