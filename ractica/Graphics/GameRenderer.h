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
    // Модели игровых объектов
    Model snakeModel;
    Model foodModel;
    Model obstacleModel;
    Model floorModel;
    Model fenceModel;
    Model cloudModel;
    Model birdModel;
    Model flowerModel;
    Model treeModel;
    Model appleModel;

    // Системы рендеринга
    ShaderManager shaderManager;
    Camera camera;

public:
    GameRenderer();

    // Геттеры моделей
    const Model& getSnakeModel() const { return snakeModel; }
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

    // Основные методы рендеринга
    void initialize();
    void render(const GameObjects& objects, const GameUI& ui);
    void renderGame(const GameObjects& objects);

    // Методы отрисовки объектов
    void drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color);
    void drawFloor();
    void drawSnake(const std::vector<Point>& snake);
    void drawFood(const std::vector<Point>& food);
    void drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles);
    void drawFence(const std::vector<Point>& fenceBlocks);
    void drawClouds(const std::vector<Sprite>& cloudSprites);
    void drawBird(const Bird& bird);
    void drawBirds(const std::vector<Bird>& birds);
    void drawGroundSprites(const std::vector<Sprite>& flowerSprites);

    // Создание моделей
    void loadAllModels();
    void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal = glm::vec3(0.0f, 0.0f, 1.0f));
    void createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color);
    void createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color);
    void createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius);
    void createCloudModel(Model& model);
    void createAnimatedBirdModel(Model& model);
    void createFlowerModel(Model& model);
    void createTreeModel(Model& model);
    void createDetailedAppleModel(Model& model);
    void createTexturedCubeModel(Model& model);
    void createTexturedSphereModel(Model& model);
    void createTexturedFloorModel(Model& model);
    void createFenceModel(Model& model);
};