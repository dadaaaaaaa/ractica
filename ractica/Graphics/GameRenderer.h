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

// Предварительное объявление enum LightType, если его нет в Types.h


struct ModelBounds {
    glm::vec3 center;
    float radius;
    glm::mat4 transform;
    const Model* model;
};

//=============================================================================
// ГЛАВНЫЙ КЛАСС РЕНДЕРЕРА
//=============================================================================
class GameRenderer {
public:
    //=========================================================================
    // КОНСТРУКТОР / ДЕСТРУКТОР
    //=========================================================================
    GameRenderer();
    ~GameRenderer();

    //=========================================================================
    // ИНИЦИАЛИЗАЦИЯ И ОСНОВНЫЕ МЕТОДЫ
    //=========================================================================
    void initialize();                          // Инициализация рендерера
    void renderGame(const GameObjects& objects); // Отрисовка всей игры
    void loadModelsFromConfig(const GameObjects& objects); // Загрузка моделей из конфига
    void createPrimitives();                    // Создание примитивов по умолчанию
    void resetShadows();                        // Полный сброс всех теней
    void toggleAmbient();                       // Переключение Ambient освещения
    void toggleSpecular();                      // Переключение Specular отражений

    //=========================================================================
    // ГЕТТЕРЫ / СЕТТЕРЫ: НАСТРОЙКИ СЕТКИ (Grid)
    //=========================================================================
    void setGridEnabled(bool enabled);          // Вкл/Выкл отрисовку сетки
    void setGridSettings(bool enabled, float lineWidth); // Настройки сетки
    void setGridDimensions(int width, int depth, float cellSize); // Размеры сетки
    void setGridColor(const glm::vec3& color);  // Цвет линий сетки

    bool isGridEnabled() const;                 // Включена ли сетка
    float getGridLineWidth() const;             // Толщина линий сетки

    //=========================================================================
    // ГЕТТЕРЫ / СЕТТЕРЫ: ЦВЕТА (Colors)
    //=========================================================================
    void setSkyColor(const glm::vec3& color);   // Цвет неба
    void setFloorColor(const glm::vec3& color); // Цвет пола

    const glm::vec3& getSkyColor() const;       // Получить цвет неба
    const glm::vec3& getFloorColor() const;     // Получить цвет пола
    const glm::vec3& getGridColor() const;      // Получить цвет сетки

    //=========================================================================
    // ГЕТТЕРЫ / СЕТТЕРЫ: НАСТРОЙКИ ПОЛА (Floor)
    //=========================================================================
    void setFloorTexture(const std::string& texturePath); // Установить текстуру пола
    void setFloorTiles(int tilesX, int tilesZ);  // Количество тайлов пола для теней
    float getFloorHeight() const;               // Высота пола (Y-координата)
    int getFloorTilesX() const;                 // Количество тайлов по X
    int getFloorTilesZ() const;                 // Количество тайлов по Z
    float getFloorTileSizeX() const;            // Размер тайла по X
    float getFloorTileSizeZ() const;            // Размер тайла по Z
    void updateSpheresRadii();
    //=========================================================================
    // ГЕТТЕРЫ / СЕТТЕРЫ: ОСВЕЩЕНИЕ (Lighting)
    //=========================================================================
    void setLightType(LightType type);          // Тип источника (Directional/Point/Spot)
    void setLightColor(const glm::vec3& color); // Цвет света
    void setLightPosition(const glm::vec3& pos); // Позиция (для Point и Spot)
    void setLightDirection(const glm::vec3& dir); // Направление (для Directional)
    void setLightDir(const glm::vec3& dir);     // Алиас для setLightDirection
    void setLightPos(const glm::vec3& pos);     // Алиас для setLightPosition
    void setAmbientEnabled(bool enabled);       // Вкл/Выкл Ambient освещение
    void setSpecularEnabled(bool enabled);      // Вкл/Выкл Specular отражения
    bool isAmbientEnabled() const { return m_ambientEnabled; }
    bool isSpecularEnabled() const { return m_specularEnabled; }

    void updateLighting();                      // Обновить параметры освещения

    LightType getLightType() const;             // Получить тип источника
    glm::vec3 getLightColor() const;            // Получить цвет света
    glm::vec3 getLightPosition() const;         // Получить позицию света
    glm::vec3 getLightDirection() const;        // Получить направление света

    //=========================================================================
    // ГЕТТЕРЫ / СЕТТЕРЫ: ТЕНИ (Shadows)
    //=========================================================================
    void setShadowMapEnabled(bool enabled);     // Вкл/Выкл отрисовку теней
    void setShadowTraceMode(bool useCorners);   // Режим трассировки (центр/углы)
    void setShadowTraceModeByIndex(int modeIndex); // Установка режима по индексу (0-3)
    void setShadowSubdivisionSize(int size);    // Размер подразбиения ячейки (для режимов 2 и 3)
    void setShadowStride(int strideX, int strideZ); // Шаг теневой сетки

    void toggleShadowMap();                     // Переключить тени (вкл/выкл)
    void toggleShadowTraceMode();               // Переключить режим трассировки (F8)

    bool isShadowMapEnabled() const;            // Включены ли тени
    bool isUsingCornerTrace() const;            // Используется ли режим углов (Corners)
    ShadowMapper::ShadowTraceMode getCurrentShadowMode() const; // Текущий режим трассировки
    const char* getCurrentShadowModeName() const; // Имя текущего режима для отладки

    //=========================================================================
    // УПРАВЛЕНИЕ "ГРЯЗНЫМИ" ТЕНЯМИ (Dirty Shadow Management)
    //=========================================================================
    void markStaticShadowsDirty();              // Пометить статические тени как устаревшие
    void markDynamicShadowsDirty();             // Пометить динамические тени как устаревшие
    void markFoodShadowsDirty();                // Пометить тени еды как устаревшие
    void forceFoodShadowsUpdate(const GameObjects& objects); // Принудительное обновление теней еды

    //=========================================================================
    // ДЕБАГ И ОТЛАДКА (Debug)
    //=========================================================================
    void toggleDebugNormals();                  // Вкл/Выкл отрисовку нормалей моделей
    void toggleDebugRays();                     // Вкл/Выкл отрисовку лучей для теней
    void toggleShowAllRays();                   // Показать все лучи (не только попавшие)
    void toggleRenderWireframe();               // Вкл/Выкл wireframe режим

    void setShowGroundRays(bool show);          // Показывать лучи, попавшие в пол

    bool isDebugNormalsEnabled() const;         // Включена ли отрисовка нормалей
    bool isRayTracingEnabled() const;           // Включена ли трассировка лучей

    //=========================================================================
    // УПРАВЛЕНИЕ РЕНДЕРИНГОМ (Rendering Controls)
    //=========================================================================
    void setRayTracingEnabled(bool enabled);    // Вкл/Выкл трассировку лучей
    void setRenderWireframe(bool enabled);      // Вкл/Выкл wireframe режим

    void toggleRayTracing();                    // Переключить трассировку лучей
    void toggleshadow_map();                    // Переключить карту теней

    void drawSphereImmediate(const glm::vec3& center, float radius); // Отрисовка сферы
    void drawSnakeEyes();                        // Отрисовка глаз змейки

    //=========================================================================
    // ФУНКЦИИ ПЕРЕСЕЧЕНИЯ ДЛЯ ТЕНЕЙ
    //=========================================================================
    HitInfo intersectTreesOnly(const Ray& ray, const GameObjects& objects,
        float offsetX, float offsetZ);
    HitInfo intersectSnakeOnly(const Ray& ray, const GameObjects& objects,
        float offsetX, float offsetZ);
    HitInfo intersectFoodOnly(const Ray& ray, const GameObjects& objects,
        float offsetX, float offsetZ);

    //=========================================================================
    // ФУНКЦИИ ДЛЯ РАБОТЫ С МОДЕЛЯМИ
    //=========================================================================
    bool loadFBXModelToModel(const std::string& filename, Model& outModel,
        const std::string& subFolder = "");
    void toggleUseExactModels();
    bool isUsingExactModels() const;

    void setMaterial(const glm::vec3& color, float shininess = 32.0f,
        float specularStrength = 0.3f);

    void drawModelWithShadow(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color, bool inShadow);

    void drawModelWithMaterial(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color = glm::vec3(1.0f));

    //=========================================================================
    // СТАТИЧЕСКИЕ МЕТОДЫ ДЛЯ НАСТРОЙКИ OPENGL
    //=========================================================================
    static void setupGLFWHints();
    static void setupCallbacks(GLFWwindow* window);
    static bool initGLEW();
    static void printGraphicsInfo();
    static void checkDoubleBufferSupport(GLFWwindow* window);
    static void setupVSync(GLFWwindow* window, bool enabled);
    static void initOpenGLSettings();
    static void checkGLError(const char* functionName);

private:
    //=========================================================================
    // ВНУТРЕННИЕ МЕТОДЫ ОСВЕЩЕНИЯ
    //=========================================================================
    void setupFixedPipelineLighting();           // Настройка фиксированного освещения
    void setupDirectionalLight();                // Настройка направленного света
    void setupPointLight();                      // Настройка точечного света
    void setupSpotLight();                       // Настройка прожектора
    void updateLightPosition();                  // Обновление позиции света
    void setupTexture(GLuint textureID);         // Настройка текстуры
    bool intersectTreeById(const Ray& ray, int treeId, float offsetX, float offsetZ,
        float& hitDist, glm::vec3& hitPoint);
    bool intersectSnakeSegmentById(const Ray& ray, int segmentId, float offsetX, float offsetZ,
        float& hitDist, glm::vec3& hitPoint);
    bool intersectAppleById(const Ray& ray, int appleId, float offsetX, float offsetZ,
        float& hitDist, glm::vec3& hitPoint);
    //=========================================================================
    // ВНУТРЕННИЕ МЕТОДЫ ОТРИСОВКИ
    //=========================================================================
    void drawFloor();
    void drawSnake(const std::vector<Point>& snake);
    void drawFood(const std::vector<Point>& food);
    void drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles);
    void drawFence(const std::vector<Point>& fenceBlocks);
    void drawClouds(const std::vector<Sprite>& cloudSprites);
    void drawBirds(const std::vector<Bird>& birds);
    void drawGroundSprites(const std::vector<Sprite>& flowerSprites);
    void drawLightSource();
    void drawTiledFloor(const GameObjects& objects);
    void drawFallbackFloor();
    void drawFloorGrid();

    void drawModel(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color);
    void drawModelWithRotation(const Model& model, float x, float y, float z,
        float scale, const glm::vec3& color, float rotationAngle);

    void drawDebugSpheres();
    void drawDebugRaysIfEnabled();
    void drawDebugRays(const std::vector<DebugRay>& rays, float lineWidth = 1.0f);
    void drawRay(const DebugRay& ray, const glm::vec3& color);
    void drawDebugNormals(const GameObjects& objects);
    void drawModelNormals(const Model& model, const glm::mat4& transform,
        float normalLength = 0.15f);
    void drawModelNormalsWithTransform(const Model& model, float x, float y, float z,
        float scale, float rotationAngle, float normalLength = 0.15f);
    bool m_useSpheres = true;
    //=========================================================================
    // ВНУТРЕННИЕ МЕТОДЫ ТЕНЕЙ
    //=========================================================================
    void computeShadowsIfNeeded(const GameObjects& objects);
    HitInfo intersectScene(const Ray& ray, const GameObjects& objects,
        float offsetX, float offsetZ, bool treesOnly = false);
    bool rayIntersectsModel(const Ray& ray, const Model& model,
        const glm::mat4& transform, float& hitDistance, glm::vec3& hitPoint);
    void renderShadowMap();

    //=========================================================================
    // ВНУТРЕННИЕ МЕТОДЫ ТРАССИРОВКИ ЛУЧЕЙ
    //=========================================================================
    void initRayTracingResources();
    void cleanupRayTracingResources();
    void renderWithRayTracing(const GameObjects& objects);
    glm::vec3 traceRay(const Ray& ray, const GameObjects& objects,
        float offsetX, float offsetZ, int depth = 0);
    bool rayIntersectsAABB(const Ray& ray, const glm::vec3& min,
        const glm::vec3& max, float& tMin, float& tMax);
    bool rayIntersectsSphere(const Ray& ray, const glm::vec3& center,
        float radius, float& tHit);
    glm::vec3 computeNormal(const glm::vec3& point, const glm::vec3& min,
        const glm::vec3& max);

    //=========================================================================
    // ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    //=========================================================================
    float calculateSegmentRotation(const std::vector<Point>& snake, size_t index);
    void resetDepthState();
    void drawCube();
    void createTexturedFloorModel(Model& model);
    bool convertModelDataToModel(const ModelData& modelData, Model& outModel);
    void setupModelTexture(Model& model, GLuint textureID);

    glm::vec3 getSnakeSegmentPosition(const Point& segment, size_t index);
    glm::vec3 getSnakeSegmentScale(const Point& segment, size_t index);
    glm::vec3 getFoodPosition(const Point& food);
    glm::vec3 getObstaclePosition(const Point& block);
    glm::vec3 getBirdPosition(const Bird& bird);
    glm::vec3 getCloudPosition(const Sprite& cloud);
    glm::vec3 getFlowerPosition(const Sprite& flower);

    //=========================================================================
    // ПЕРЕМЕННЫЕ ПОЛА
    //=========================================================================
    float m_floorHeight = 0.0f;
    int m_floorTilesX = 8;
    int m_floorTilesZ = 8;
    float m_floorTileSizeX = 1.0f;
    float m_floorTileSizeZ = 1.0f;
    bool m_floorUseSubdivision = false;
    int m_floorSubdivisionLevel = 10;

    //=========================================================================
    // ПЕРЕМЕННЫЕ ЦВЕТОВ
    //=========================================================================
    glm::vec3 skyColor = glm::vec3(0.53f, 0.81f, 0.92f);
    glm::vec3 floorColor = glm::vec3(0.3f, 0.6f, 0.2f);
    glm::vec3 gridColor = glm::vec3(0.2f, 0.5f, 0.15f);
    bool useFloorTexture = false;

    //=========================================================================
    // ПЕРЕМЕННЫЕ СЕТКИ
    //=========================================================================
    bool gridEnabled = true;
    float gridLineWidth = 1.0f;
    int m_gridWidth = 120;
    int m_gridDepth = 120;
    float m_cellSize = 0.1f;

    //=========================================================================
    // ПЕРЕМЕННЫЕ ОСВЕЩЕНИЯ
    //=========================================================================
    LightType m_lightType = LightType::Points;  // Используем POINT вместо LightType::POINT
    glm::vec3 m_lightDir = glm::vec3(-1.0f, -1.0f, 0.5f);
    glm::vec3 m_lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 m_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    bool m_ambientEnabled = true;
    bool m_specularEnabled = true;
    bool isShadowDirty() const { return m_shadowsDirty; }
    void markShadowDirty() { m_shadowsDirty = true; }
    void clearShadowDirty() { m_shadowsDirty = false; }
    bool m_shadowsDirty = false;
    //=========================================================================
    // ПЕРЕМЕННЫЕ ТЕНЕЙ
    //=========================================================================
    bool shadow_map = true;
    bool m_shadowMapEnabled = true;
    int m_shadowStrideX = 2;
    int m_shadowStrideZ = 2;
    bool m_useCornerTrace = true;
    ShadowMapper::ShadowTraceMode m_currentShadowMode = ShadowMapper::TRACE_CORNERS_SUBDIVIDED;

    ShadowMapper m_staticShadow;
    ShadowMapper m_dynamicShadow;
    ShadowMapper m_foodShadow;
    bool m_staticShadowsDirty = true;
    bool m_dynamicShadowsDirty = true;
    bool m_foodShadowsDirty = true;

    //=========================================================================
    // ПЕРЕМЕННЫЕ МОДЕЛЕЙ
    //=========================================================================
    Model m_snakeHeadModel;
    Model m_snakeBodyModel;
    Model m_snakeTailModel;
    Model m_appleModel;
    Model m_treeModel;
    Model m_cloudModel;
    Model m_birdModel;
    Model m_flowerModel;
    Model m_fenceModel;
    Model m_floorModel;
    Model m_cubeModel;
    Model m_sphereModel;
    Model m_cylinderModel;

    std::map<std::string, Model> m_loadedFBXModels;
    bool m_useExactModels = false;

    //=========================================================================
    // ПЕРЕМЕННЫЕ ТРАССИРОВКИ ЛУЧЕЙ
    //=========================================================================
    bool m_rayTracingEnabled = false;
    bool m_renderWireframe = false;
    int m_rayTracingStepSize = 4;
    bool m_rayTracingUseAdaptive = true;
    RayTracer m_rayTracer;
    GLuint m_rayTracingTexture = 0;
    GLuint m_rayTracingVAO = 0;
    GLuint m_rayTracingVBO = 0;
    GLuint m_rayTracingShader = 0;

    //=========================================================================
    // ДЕБАГ ПЕРЕМЕННЫЕ
    //=========================================================================
    bool m_debugRaysEnabled = false;
    bool m_showGroundRays = false;
    bool m_showAllRays = false;
    bool m_debugNormalsEnabled = false;

    //=========================================================================
    // ТЕКСТУРА ПОЛА
    //=========================================================================
    struct Texture {
        GLuint id = 0;
        int width = 0;
        int height = 0;
        std::string path;
    } floorTexture;
};