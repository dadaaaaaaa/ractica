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
#include <map>
#include "../Primitives/SnakeTailPrimitive.h"
#include "../Primitives/TreePrimitive.h"
#include "../Primitives/ApplePrimitive.h"
#include "../Primitives/BirdPrimitive.h"
#include "../Primitives/CloudPrimitive.h"
#include "../Primitives/FencePrimitive.h"
#include "../Primitives/FlowerPrimitive.h"
#include "../Primitives/SnakeHeadPrimitive.h"
#include "../Primitives/SnakeBodyPrimitive.h"
#include "../Primitives/PrimitiveBase.h"
#include "../Core/Types.h"
#include "../Objects/GameObjects.h"
#include "../Graphics/ShadowMapper.h"
#include "../Graphics/RayTracer.h"
#include "Model.h"
#include "ModelLoader.h"

class GameRenderer {
public:
    GameRenderer();
    ~GameRenderer();
    float getFloorHeight() const { return m_floorHeight; }
    void initialize();
    void renderGame(const GameObjects& objects);
    void loadModelsFromConfig(const GameObjects& objects);
    void setFloorTexture(const std::string& texturePath);
    void markDynamicShadowsDirty();
    void resetShadows();

    // Настройки
    void setRayTracingEnabled(bool enabled) { m_rayTracingEnabled = enabled; }
    void setShadowMapEnabled(bool enabled) { shadow_map = enabled; }
    void setGridEnabled(bool enabled) { gridEnabled = enabled; }
    void setRenderWireframe(bool enabled) { m_renderWireframe = enabled; }

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
    void createPrimitives();
    void drawSnakeEyes();

    // НОВЫЕ МЕТОДЫ ДЛЯ НОРМАЛЕЙ
    void toggleDebugNormals();
    bool isDebugNormalsEnabled() const { return m_debugNormalsEnabled; }

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
    static void initOpenGLSettings();
    static void checkGLError(const char* functionName);

    void drawSphereImmediate(const glm::vec3& center, float radius);

    void renderShadowMap();
    void toggleShadowMap();
    void toggleDebugRays();
    void setShowGroundRays(bool show) { m_showGroundRays = show; }
    void setMaterial(const glm::vec3& color, float shininess = 32.0f, float specularStrength = 0.3f);
    void drawModelWithShadow(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color, bool inShadow);
    bool loadFBXModelToModel(const std::string& filename, Model& outModel, const std::string& subFolder = "");
    void drawModelWithMaterial(const Model& model, float x, float y, float z, float scale, const glm::vec3& color = glm::vec3(1.0f));
    void setLightType(LightType type);
    void setLightPosition(const glm::vec3& pos);
    void setLightDirection(const glm::vec3& dir);
    void setLightColor(const glm::vec3& color);
    void updateLighting();
    LightType getLightType() const { return m_lightType; }
    glm::vec3 getLightColor() const { return m_lightColor; }
    glm::vec3 getLightPosition() const { return m_lightPos; }
    glm::vec3 getLightDirection() const { return m_lightDir; }
private:
    float m_floorHeight = 0.0f;
    void setupPointLight();
    void setupSpotLight();
    void setupDirectionalLight();
    void setupFixedPipelineLighting();
    void updateLightPosition();
    void setupTexture(GLuint textureID);
    void resetDepthState();
    void drawCube();
    void computeShadowsIfNeeded(const GameObjects& objects);
    HitInfo intersectScene(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ);
    ShadowMapper::ShadowTraceMode m_currentShadowMode;
public:
    void toggleAmbient();
    void toggleSpecular();
    void toggleShowAllRays();
    void toggleShadowTraceMode();  // Переключение между режимами (F8)
    void setShadowTraceMode(bool useCorners);  // true = углы, false = центр
    bool isUsingCornerTrace() const { return m_useCornerTrace; }
public:
    ShadowMapper::ShadowTraceMode getCurrentShadowMode() const { return m_currentShadowMode; }
    const char* getCurrentShadowModeName() const { return ShadowMapper::getModeName(m_currentShadowMode); }
public:
    void setShadowTraceModeByIndex(int modeIndex);
    void markFoodShadowsDirty();
    void forceFoodShadowsUpdate(const GameObjects& objects) {
        m_foodShadowsDirty = true;
        m_foodShadow.clearObjectBounds();  // ОЧИЩАЕМ СФЕРЫ!

        m_foodShadow.setIntersectCallback(
            [this, &objects](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
                float offsetX = m_gridWidth * m_cellSize / 2.0f;
                float offsetZ = m_gridDepth * m_cellSize / 2.0f;
                HitInfo hit = intersectScene(ray, objects, offsetX, offsetZ);
                if (hit.hit && hit.distance > 0.01f) {
                    hitDist = hit.distance;
                    hitPoint = hit.point;
                    return true;
                }
                return false;
            }
        );
        m_foodShadow.computeShadows();
        m_foodShadowsDirty = false;
    }

    void toggleUseExactModels();  // НОВЫЙ МЕТОД
    bool isUsingExactModels() const { return m_useExactModels; }
    // Добавить в секцию public:

    void setFloorTiles(int tilesX, int tilesZ) {
        m_floorTilesX = tilesX;
        m_floorTilesZ = tilesZ;
        m_floorTileSizeX = (m_gridWidth * m_cellSize) / tilesX;
        m_floorTileSizeZ = (m_gridDepth * m_cellSize) / tilesZ;
        m_staticShadowsDirty = true;
    }

    int getFloorTilesX() const { return m_floorTilesX; }
    int getFloorTilesZ() const { return m_floorTilesZ; }
    float getFloorTileSizeX() const { return m_floorTileSizeX; }
    float getFloorTileSizeZ() const { return m_floorTileSizeZ; }
private:
    bool m_ambientEnabled;
    bool m_specularEnabled;
    bool m_showAllRays;
    ShadowMapper m_foodShadow;     // Для еды (яблоки) - НОВОЕ
    bool m_useCornerTrace = false;
    bool m_foodShadowsDirty;
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

    bool convertModelDataToModel(const ModelData& modelData, Model& outModel);
    void setupModelTexture(Model& model, GLuint textureID);


    void initRayTracingResources();
    void cleanupRayTracingResources();
    void renderWithRayTracing(const GameObjects& objects);
    glm::vec3 traceRay(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ, int depth = 0);
    bool rayIntersectsAABB(const Ray& ray, const glm::vec3& min, const glm::vec3& max, float& tMin, float& tMax);
    bool rayIntersectsSphere(const Ray& ray, const glm::vec3& center, float radius, float& tHit);
    glm::vec3 computeNormal(const glm::vec3& point, const glm::vec3& min, const glm::vec3& max);

    void drawDebugRaysIfEnabled();
    void drawDebugRays(const std::vector<DebugRay>& rays, float lineWidth = 1.0f);
    void drawRay(const DebugRay& ray, const glm::vec3& color);

    // НОВЫЕ МЕТОДЫ ДЛЯ ОТЛАДКИ НОРМАЛЕЙ
    void drawDebugNormals(const GameObjects& objects);
    void drawModelNormals(const Model& model, const glm::mat4& transform, float normalLength = 0.15f);
    void drawModelNormalsWithTransform(const Model& model, float x, float y, float z,
        float scale, float rotationAngle, float normalLength = 0.15f);
    glm::vec3 getSnakeSegmentPosition(const Point& segment, size_t index);
    glm::vec3 getSnakeSegmentScale(const Point& segment, size_t index);
    glm::vec3 getFoodPosition(const Point& food);
    glm::vec3 getObstaclePosition(const Point& block);
    glm::vec3 getBirdPosition(const Bird& bird);
    glm::vec3 getCloudPosition(const Sprite& cloud);
    glm::vec3 getFlowerPosition(const Sprite& flower);
    void drawTiledFloor(const GameObjects& objects);
    void drawFallbackFloor();
    void drawFloorGrid();
private:
    int m_floorTilesX;           // Количество полигонов по X
    int m_floorTilesZ;           // Количество полигонов по Z
    float m_floorTileSizeX;      // Размер одного полигона по X
    float m_floorTileSizeZ;      // Размер одного полигона по Z
    bool m_floorUseSubdivision;  // Использовать ли подразбиение для пола
    int m_floorSubdivisionLevel; // Уровень подразбиения (для subdivided режимов)
    bool m_useExactModels = false;
    bool rayIntersectsModel(const Ray& ray, const Model& model, const glm::mat4& transform, float& hitDistance, glm::vec3& hitPoint);
    // Основные модели примитивов
    Model m_snakeHeadModel;
    Model m_snakeBodyModel;
    Model m_snakeTailModel;
    Model m_appleModel;
    Model m_treeModel;
    Model m_cloudModel;
    Model m_birdModel;
    Model m_flowerModel;
    Model m_cubeModel;
    Model m_sphereModel;
    Model m_cylinderModel;
    Model m_fenceModel;
    Model m_floorModel;
    LightType m_lightType;
    glm::vec3 m_lightDir;
    glm::vec3 m_lightPos;
    glm::vec3 m_lightColor;
    std::map<std::string, Model> m_loadedFBXModels;

    void createTexturedFloorModel(Model& model);

    glm::vec3 skyColor;
    glm::vec3 floorColor;
    glm::vec3 gridColor;
    bool useFloorTexture;
    bool gridEnabled;
    float gridLineWidth;
    int m_gridWidth;
    int m_gridDepth;
    float m_cellSize;

    bool shadow_map;
    bool m_shadowMapEnabled;
    int m_shadowStrideX;
    int m_shadowStrideZ;

    ShadowMapper m_staticShadow;
    ShadowMapper m_dynamicShadow;
    bool m_staticShadowsDirty;
    bool m_dynamicShadowsDirty;

    bool m_rayTracingEnabled;
    bool m_renderWireframe;
    int m_rayTracingStepSize;
    bool m_rayTracingUseAdaptive;
    RayTracer m_rayTracer;

    GLuint m_rayTracingTexture;
    GLuint m_rayTracingVAO;
    GLuint m_rayTracingVBO;
    GLuint m_rayTracingShader;

    bool m_debugRaysEnabled;
    bool m_showGroundRays;

    // НОВЫЙ ФЛАГ ДЛЯ НОРМАЛЕЙ
    bool m_debugNormalsEnabled;

    struct Texture {
        GLuint id = 0;
        int width = 0;
        int height = 0;
        std::string path;
    } floorTexture;
};