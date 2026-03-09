#pragma once

#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../Shared/ConfigTypes.h"
#include "Model.h"
#include "ShaderManager.h"
#include "Camera.h"
#include "../Objects/GameObjects.h"
#include "../Objects/Sprite.h"
#include "../Objects/Bird.h"
#include "../Objects/Obstacle.h"
#include "../UI/GameUI.h"
#include <GLFW/glfw3.h>
#include <string>
#include <functional>
#include <filesystem>
#include "ModelLoader.h"

// Структура для текстуры
struct Texture {
    unsigned int id;
    int width;
    int height;
    std::string path;

    Texture() : id(0), width(0), height(0), path("") {}
};

class GameRenderer {
private:
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

    // Примитивы (фолбэки)
    Model cubePrimitive;
    Model spherePrimitive;
    Model applePrimitive;
    Model treePrimitive;
    Model cloudPrimitive;
    Model birdPrimitive;
    Model flowerPrimitive;
    Model fencePrimitive;

    ShaderManager shaderManager;
    Camera camera;

    static bool doubleBufferingEnabled;

    // Цвета из конфига
    glm::vec3 skyColor;
    glm::vec3 floorColor;
    glm::vec3 gridColor;

    // Текстура пола
    Texture floorTexture;
    bool useFloorTexture;

    // Настройки сетки
    bool gridEnabled;
    float gridLineWidth;

public:
    GameRenderer();

    void createSnakeHeadModel(Model& model);
    void createSnakeBodyModel(Model& model);
    void createSnakeTailModel(Model& model);

    // Методы для установки цветов
    void setSkyColor(const glm::vec3& color) { skyColor = color; }
    void setFloorColor(const glm::vec3& color) { floorColor = color; }
    void setGridColor(const glm::vec3& color) { gridColor = color; }

    // Методы для настроек сетки
    void setGridSettings(bool enabled, float lineWidth) {
        gridEnabled = enabled;
        gridLineWidth = lineWidth;
    }
    bool isGridEnabled() const { return gridEnabled; }
    float getGridLineWidth() const { return gridLineWidth; }

    // Метод для установки текстуры пола
    void setFloorTexture(const std::string& texturePath);

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

    void initialize();
    void renderGame(const GameObjects& objects);

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
    void drawModelWithRotation(const Model& model, float x, float y, float z, float scale,
        const glm::vec3& color, float rotationAngle);
    float calculateSegmentRotation(const std::vector<Point>& snake, size_t index);

    // НОВЫЕ МЕТОДЫ
    void createPrimitives();
    void loadModelsFromConfig(const GameObjects& objects);
    bool loadModelWithFallback(Model& model, const std::string& modelPath,
        const std::string& subFolder,
        std::function<void(Model&)> fallbackCreator);
    bool loadTextureForModel(Model& model, const std::string& texturePath, const std::string& modelFolder);

    void loadAllModels();
    bool loadModelFromFile(Model& model, const std::string& modelPath, const std::string& texturePath);
    bool loadOBJModel(Model& model, const std::string& path);
    bool loadTexture(Model& model, const std::string& path);
    bool loadTextureFromFile(Model& model, const std::string& path);
    bool createProceduralTexture(Model& model, const std::string& name);

    void createSnakeTexture(std::vector<unsigned char>& data, int size);
    void createCheckerboardTexture(std::vector<unsigned char>& data, int size);

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

    static void setupGLFWHints();
    static bool initGLEW();
    static void initOpenGLSettings();
    static void checkGLError(const char* functionName);
    static void checkDoubleBufferSupport(GLFWwindow* window);
    static void setupVSync(GLFWwindow* window, bool enabled = true);
    static void printGraphicsInfo();
    static void setupCallbacks(GLFWwindow* window);

    static void resetDepthState();
    void createFencePost(std::vector<Vertex>& vertices, float x, float y, float z,
        float width, float height, const glm::vec3& color);
    void createFenceRailHorizontal(std::vector<Vertex>& vertices, float x, float y, float z,
        float length, float thickness, const glm::vec3& color);
    void createFenceRailVertical(std::vector<Vertex>& vertices, float x, float y, float z,
        float length, float thickness, const glm::vec3& color);
    void createFenceCorner(std::vector<Vertex>& vertices, float x, float y, float z,
        const glm::vec3& color);
};