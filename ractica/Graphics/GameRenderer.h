#pragma once
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "Model.h"
#include "ShaderManager.h"
#include "Camera.h"
#include "../Objects/GameObjects.h"
#include "../UI/GameUI.h"

class GameRenderer {
private:
    // === МОДЕЛИ ИГРОВЫХ ОБЪЕКТОВ ===
    Model snakeHeadModel;
    Model snakeBodyModel;
    Model snakeTailModel;
    Model foodModel;
    Model obstacleModel;
    Model floorModel;
    Model fenceModel;
    Model cloudModel;
    Model birdModel;
    Model flowerModel;
    Model treeModel;
    Model appleModel;

    // === СИСТЕМЫ РЕНДЕРИНГА ===
    ShaderManager shaderManager;
    Camera camera;

public:
    GameRenderer();
    void createSnakeHeadModel(Model& model);
    void createSnakeBodyModel(Model& model);
    void createSnakeTailModel(Model& model);
    // === ГЕТТЕРЫ МОДЕЛЕЙ И СИСТЕМ ===
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

    const Camera& getCamera() const { return camera; }
    Camera& getCamera() { return camera; }
    const ShaderManager& getShaderManager() const { return shaderManager; }

    // === ОСНОВНЫЕ МЕТОДЫ РЕНДЕРИНГА ===
    void initialize();                           // Инициализация рендерера
    void renderGame(const GameObjects& objects); // Рендеринг игрового мира

    // === МЕТОДЫ ОТРИСОВКИ ОБЪЕКТОВ ===
    void drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color);
    void drawFloor();                            // Отрисовка пола
    void drawSnake(const std::vector<Point>& snake); // Отрисовка змейки
    void drawFood(const std::vector<Point>& food);   // Отрисовка еды
    void drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles); // Препятствия как деревья
    void drawFence(const std::vector<Point>& fenceBlocks); // Отрисовка забора
    void drawClouds(const std::vector<Sprite>& cloudSprites); // Отрисовка облаков
    void drawBird(const Bird& bird);             // Отрисовка одной птицы
    void drawBirds(const std::vector<Bird>& birds); // Отрисовка всех птиц
    void drawGroundSprites(const std::vector<Sprite>& flowerSprites); // Отрисовка цветов на земле
    void drawModelWithRotation(const Model& model, float x, float y, float z, float scale,
        const glm::vec3& color, float rotationAngle);
    float calculateSegmentRotation(const std::vector<Point>& snake, size_t index);
    // === СИСТЕМА ЗАГРУЗКИ МОДЕЛЕЙ И ТЕКСТУР ===
    void loadAllModels();                        // Загрузка всех моделей
    bool loadModelFromFile(Model& model, const std::string& modelPath, const std::string& texturePath); // Загрузка модели из файла
    bool loadOBJModel(Model& model, const std::string& path); // Парсинг OBJ файлов
    bool loadTexture(Model& model, const std::string& path); // Загрузка текстуры
    bool loadTextureFromFile(Model& model, const std::string& path); // Загрузка текстуры из файла
    bool createProceduralTexture(Model& model, const std::string& name); // Создание procedural текстуры

    // === GENERATIVE ТЕКСТУРЫ ===
    void createSnakeTexture(std::vector<unsigned char>& data, int size); // Текстура для змейки
    void createCheckerboardTexture(std::vector<unsigned char>& data, int size); // Шахматная текстура

    // === МЕТОДЫ СОЗДАНИЯ ГЕОМЕТРИИ (ПРИМИТИВЫ) ===
    void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal = glm::vec3(0.0f, 0.0f, 1.0f));
    void createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color);
    void createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color);
    void createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius);

    // === МЕТОДЫ СОЗДАНИЯ КОНКРЕТНЫХ МОДЕЛЕЙ ===
    void createCloudModel(Model& model);         // Модель облака
    void createAnimatedBirdModel(Model& model);  // Модель птицы с анимацией
    void createFlowerModel(Model& model);        // Модель цветка
    void createTreeModel(Model& model);          // Модель дерева
    void createDetailedAppleModel(Model& model); // Детальная модель яблока
    void createTexturedCubeModel(Model& model);  // Текстурированный куб
    void createTexturedSphereModel(Model& model); // Текстурированная сфера
    void createTexturedFloorModel(Model& model); // Текстурированный пол
    void createFenceModel(Model& model);         // Модель забора
};