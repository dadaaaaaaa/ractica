#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <iomanip>

#include "../Core/Types.h"
#include "../Objects/GameObjects.h"
#include "../Graphics/ShadowMapper.h"
#include "../Graphics/RayTracer.h"
#include "Model.h"

class GameRenderer {
public:
    GameRenderer();
    ~GameRenderer() = default;

    void initialize();
    void renderGame(const GameObjects& objects);
    void loadModelsFromConfig(const GameObjects& objects);
    void setFloorTexture(const std::string& texturePath);
    void markDynamicShadowsDirty();
    void resetShadows();
    void toggleDebugRays();

    // Настройки
    void setRayTracingEnabled(bool enabled) { m_rayTracingEnabled = enabled; }
    void setShadowMapEnabled(bool enabled) { shadow_map = enabled; }
    void setGridEnabled(bool enabled) { gridEnabled = enabled; }
    void setRenderWireframe(bool enabled) { m_renderWireframe = enabled; }
    void setShowGroundRays(bool show) { m_showGroundRays = show; }

    // ДОБАВЛЕННЫЕ МЕТОДЫ
    void setSkyColor(const glm::vec3& color) { skyColor = color; }
    void setFloorColor(const glm::vec3& color) { floorColor = color; }
    void setGridColor(const glm::vec3& color) { gridColor = color; }
    void setGridSettings(bool enabled, float lineWidth) { gridEnabled = enabled; gridLineWidth = lineWidth; }
    void setGridDimensions(int width, int depth, float cellSize) {
        m_gridWidth = width;
        m_gridDepth = depth;
        m_cellSize = cellSize;
    }
    void toggleRayTracing() { m_rayTracingEnabled = !m_rayTracingEnabled; }
    void toggleshadow_map() { shadow_map = !shadow_map; }
    void markStaticShadowsDirty() { m_staticShadowsDirty = true; }
    void createPrimitives();  // Теперь public

    // Геттеры
    bool isRayTracingEnabled() const { return m_rayTracingEnabled; }
    bool isShadowMapEnabled() const { return shadow_map; }
    bool isGridEnabled() const { return gridEnabled; }

    // Статические методы для GLFW
    static void setupGLFWHints();
    static void setupCallbacks(GLFWwindow* window);
    static bool initGLEW();
    static void printGraphicsInfo();
    static void checkDoubleBufferSupport(GLFWwindow* window);
    static void setupVSync(GLFWwindow* window, bool enabled);
    static void initOpenGLSettings();  // Сделали static
    static void checkGLError(const char* functionName);  // Сделали static

    // Вспомогательные методы
    void drawSphereImmediate(const glm::vec3& center, float radius);

private:
    // ========== ОСНОВНЫЕ МЕТОДЫ РЕНДЕРИНГА ==========
    void setupFixedPipelineLighting();
    void updateLightPosition();
    void setupMaterial(const glm::vec3& color, float shininess = 32.0f);
    void setupTexture(GLuint textureID);
    void resetDepthState();
    void drawCube();  // Добавлен метод для рисования куба

    // ========== МЕТОДЫ ОТРИСОВКИ ОБЪЕКТОВ ==========
    void drawFloor();
    void drawSnake(const std::vector<Point>& snake);
    void drawFood(const std::vector<Point>& food);
    void drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles);
    void drawFence(const std::vector<Point>& fenceBlocks);
    void drawClouds(const std::vector<Sprite>& cloudSprites);
    void drawBirds(const std::vector<Bird>& birds);
    void drawGroundSprites(const std::vector<Sprite>& flowerSprites);
    void drawLightSource();
    void drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color);
    void drawModelWithRotation(const Model& model, float x, float y, float z, float scale,
        const glm::vec3& color, float rotationAngle);
    float calculateSegmentRotation(const std::vector<Point>& snake, size_t index);

    // ========== ПРИМИТИВЫ ДЛЯ ЗАБОРА ==========
    void createFencePost(std::vector<Vertex>& vertices, float x, float y, float z,
        float width, float height, const glm::vec3& color);
    void createFenceRailHorizontal(std::vector<Vertex>& vertices, float x, float y, float z,
        float length, float thickness, const glm::vec3& color);
    void createFenceRailVertical(std::vector<Vertex>& vertices, float x, float y, float z,
        float length, float thickness, const glm::vec3& color);
    void createFenceCorner(std::vector<Vertex>& vertices, float x, float y, float z,
        const glm::vec3& color);

    // ========== МЕТОДЫ ТЕНЕЙ ==========
    void renderShadowMap();

    // ========== МЕТОДЫ ТРАССИРОВКИ ЛУЧЕЙ ==========
    void initRayTracingResources();
    void cleanupRayTracingResources();
    void renderWithRayTracing(const GameObjects& objects);
    glm::vec3 traceRay(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ, int depth = 0);
    bool rayIntersectsAABB(const Ray& ray, const glm::vec3& min, const glm::vec3& max, float& tMin, float& tMax);
    bool rayIntersectsSphere(const Ray& ray, const glm::vec3& center, float radius, float& tHit);
    glm::vec3 computeNormal(const glm::vec3& point, const glm::vec3& min, const glm::vec3& max);

    // ========== МЕТОДЫ ОТЛАДКИ ==========
    void drawDebugRaysIfEnabled();
    void drawDebugRays(const std::vector<DebugRay>& rays, float lineWidth = 1.0f);
    void drawRay(const DebugRay& ray, const glm::vec3& color);

    // ========== ПОЗИЦИОНИРОВАНИЕ ОБЪЕКТОВ ==========
    static glm::vec3 getSnakeSegmentPosition(const Point& segment, size_t index);
    static glm::vec3 getSnakeSegmentScale(const Point& segment, size_t index);
    static glm::vec3 getFoodPosition(const Point& food);
    static glm::vec3 getObstaclePosition(const Point& block);
    static glm::vec3 getBirdPosition(const Bird& bird);
    static glm::vec3 getCloudPosition(const Sprite& cloud);
    static glm::vec3 getFlowerPosition(const Sprite& flower);

    // ========== МОДЕЛИ ==========
    Model snakeHeadModel;
    Model snakeBodyModel;
    Model snakeTailModel;
    Model appleModel;
    Model treeModel;
    Model cloudModel;
    Model birdModel;
    Model flowerModel;
    Model floorModel;
    Model spherePrimitive;
    Model cubePrimitive;

    void createTexturedFloorModel(Model& model);

    // ========== НАСТРОЙКИ ОКРУЖЕНИЯ ==========
    glm::vec3 skyColor;
    glm::vec3 floorColor;
    glm::vec3 gridColor;
    bool useFloorTexture;
    bool gridEnabled;
    float gridLineWidth;
    int m_gridWidth;
    int m_gridDepth;
    float m_cellSize;

    // ========== НАСТРОЙКИ ОСВЕЩЕНИЯ ==========
    glm::vec3 m_lightDir;
    glm::vec3 m_lightColor;
    LightType m_lightType;
    glm::vec3 m_lightPos;
    bool shadow_map;
    int m_shadowStrideX;
    int m_shadowStrideZ;

    ShadowMapper m_staticShadow;
    ShadowMapper m_dynamicShadow;
    bool m_staticShadowsDirty;
    bool m_dynamicShadowsDirty;

    // ========== НАСТРОЙКИ ТРАССИРОВКИ ==========
    bool m_rayTracingEnabled;
    bool m_renderWireframe;
    int m_rayTracingStepSize;
    bool m_rayTracingUseAdaptive;
    RayTracer m_rayTracer;

    GLuint m_rayTracingTexture;
    GLuint m_rayTracingVAO;
    GLuint m_rayTracingVBO;
    GLuint m_rayTracingShader;

    // ========== НАСТРОЙКИ ОТЛАДКИ ==========
    bool m_debugRaysEnabled;
    bool m_showGroundRays;

    struct Texture {
        GLuint id = 0;
        int width = 0;
        int height = 0;
        std::string path;
    } floorTexture;
};