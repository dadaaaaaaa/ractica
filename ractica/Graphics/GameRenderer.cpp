#include "../pch.h"
#include "GameRenderer.h"
#include "Model.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h"
#include "../Utils/MathUtils.h"
#include "../Game/Game.h"
#include "../Objects/Sprite.h"
#include "../Objects/Bird.h"
#include "../Objects/Obstacle.h"
#include "../Primitives/PrimitiveBase.h"
#include "../Primitives/ApplePrimitive.h"
#include "../Primitives/TreePrimitive.h"
#include "../Primitives/CloudPrimitive.h"
#include "../Primitives/BirdPrimitive.h"
#include "../Primitives/FlowerPrimitive.h"
#include "../Primitives/FencePrimitive.h"
#include "../Primitives/SnakeHeadPrimitive.h"
#include "../Primitives/SnakeBodyPrimitive.h"
#include "../Primitives/SnakeTailPrimitive.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <filesystem>

extern Camera g_camera;
extern Game g_game;
extern std::string g_modelsPath;
extern std::string g_texturesPath;
static const int DEFAULT_SHADOW_MODE = 3;
//=============================================================================
// КОНСТРУКТОР И ИНИЦИАЛИЗАЦИЯ
//=============================================================================

GameRenderer::GameRenderer()
    : skyColor(0.53f, 0.81f, 0.92f)
    , floorColor(0.3f, 0.6f, 0.2f)
    , gridColor(0.2f, 0.5f, 0.15f)
    , useFloorTexture(false)
    , gridEnabled(true)
    , gridLineWidth(1.0f)
    , m_gridWidth(120)
    , m_gridDepth(120)
    , m_cellSize(0.1f)
    , m_rayTracingEnabled(false)
    , m_renderWireframe(false)
    , m_rayTracingStepSize(1)
    , m_rayTracingUseAdaptive(true)
    , m_shadowMapEnabled(false)
    , m_debugRaysEnabled(false)
    , m_showGroundRays(false)
    , m_shadowStrideX(1)
    , m_shadowStrideZ(1)
    , m_staticShadowsDirty(true)
    , m_dynamicShadowsDirty(true)
    , m_foodShadowsDirty(true)
    , shadow_map(false)
    , m_debugNormalsEnabled(false)
    , m_ambientEnabled(false)
    , m_specularEnabled(false)
    , m_showAllRays(false)
    , m_floorTilesX(120)
    , m_floorTilesZ(120)
    , m_floorTileSizeX(0.1f)
    , m_floorTileSizeZ(0.1f)
    , m_floorUseSubdivision(false)
    , m_floorSubdivisionLevel(10)
    , m_useCornerTrace(DEFAULT_SHADOW_MODE == 1)  // CORNERS или CORNERS_SUBDIVIDED
    , m_currentShadowMode(static_cast<ShadowMapper::ShadowTraceMode>(DEFAULT_SHADOW_MODE))
{
    m_lightType = LightType::Points;
    m_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_lightDir = glm::normalize(glm::vec3(-1, -1, 0.5f));
    m_lightPos = glm::vec3(0.0f, 2.0f, 0.0f);

    // Устанавливаем режим из константы
    setShadowTraceModeByIndex(DEFAULT_SHADOW_MODE);
}
void GameRenderer::drawDebugSpheres() {
    if (!m_debugNormalsEnabled) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    // Включаем прозрачность
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const GameObjects& objects = g_game.getGameObjects();
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    // ✅ Получаем масштабы из конфига
    float treeScaleFromConfig = objects.getTreeScale();
    float appleScaleFromConfig = objects.getAppleScale();
    float snakeHeadScaleFromConfig = objects.getSnakeHeadScale();
    float snakeBodyScaleFromConfig = objects.getSnakeBodyScale();
    float snakeTailScaleFromConfig = objects.getSnakeTailScale();

    // Предварительно вычисляем радиусы моделей
    static float treeModelRadius = 0.0f;
    static float headRadius = 0.0f, bodyRadius = 0.0f, tailRadius = 0.0f;
    static float appleModelRadius = 0.0f;
    static bool radiiComputed = false;

    if (!radiiComputed) {
        // Радиус дерева
        if (!m_treeModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& vert : m_treeModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            treeModelRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
        }

        // Радиусы змейки
        if (!m_snakeHeadModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& vert : m_snakeHeadModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            headRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;

            if (!m_snakeBodyModel.vertices.empty()) {
                minX = FLT_MAX, maxX = -FLT_MAX;
                minY = FLT_MAX, maxY = -FLT_MAX;
                minZ = FLT_MAX, maxZ = -FLT_MAX;
                for (const auto& vert : m_snakeBodyModel.vertices) {
                    minX = std::min(minX, vert.position.x);
                    maxX = std::max(maxX, vert.position.x);
                    minY = std::min(minY, vert.position.y);
                    maxY = std::max(maxY, vert.position.y);
                    minZ = std::min(minZ, vert.position.z);
                    maxZ = std::max(maxZ, vert.position.z);
                }
                bodyRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            }

            if (!m_snakeTailModel.vertices.empty()) {
                minX = FLT_MAX, maxX = -FLT_MAX;
                minY = FLT_MAX, maxY = -FLT_MAX;
                minZ = FLT_MAX, maxZ = -FLT_MAX;
                for (const auto& vert : m_snakeTailModel.vertices) {
                    minX = std::min(minX, vert.position.x);
                    maxX = std::max(maxX, vert.position.x);
                    minY = std::min(minY, vert.position.y);
                    maxY = std::max(maxY, vert.position.y);
                    minZ = std::min(minZ, vert.position.z);
                    maxZ = std::max(maxZ, vert.position.z);
                }
                tailRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            }
        }

        // Радиус яблока
        if (!m_appleModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& vert : m_appleModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            appleModelRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
        }

        radiiComputed = true;
        std::cout << "Debug spheres radii: tree=" << treeModelRadius
            << ", head=" << headRadius << ", body=" << bodyRadius
            << ", tail=" << tailRadius << ", apple=" << appleModelRadius << std::endl;
    }

    // ===== РИСУЕМ СФЕРЫ ДЛЯ ДЕРЕВЬЕВ (красные, полупрозрачные) =====
    glColor4f(1.0f, 0.0f, 0.0f, 0.3f);

    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float z = block.z * m_cellSize - offsetZ;
            float y = block.y * m_cellSize;

            // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ДЕРЕВА ИЗ КОНФИГА
            float scale = m_cellSize * treeScaleFromConfig;
            float worldRadius = treeModelRadius * scale;

            glm::vec3 center(
                x + m_cellSize * 0.5f,
                y + scale * 1.0f,
                z + m_cellSize * 0.5f
            );

            float currentRadius = worldRadius;
            if (m_lightType != LightType::Directional) {
                currentRadius = worldRadius * 1.2f;
            }

            GLUquadric* quad = gluNewQuadric();
            glPushMatrix();
            glTranslatef(center.x, center.y, center.z);
            gluSphere(quad, currentRadius, 24, 24);
            glPopMatrix();
            gluDeleteQuadric(quad);
        }
    }

    // ===== РИСУЕМ СФЕРЫ ДЛЯ ЗМЕЙКИ (зелёные, полупрозрачные) =====
    glColor4f(0.0f, 1.0f, 0.0f, 0.3f);

    for (size_t i = 0; i < objects.getSnake().size(); i++) {
        const Point& segment = objects.getSnake()[i];
        float x = segment.x * m_cellSize - offsetX;
        float z = segment.z * m_cellSize - offsetZ;
        float y = segment.y * m_cellSize + 0.1f;

        float scale;
        float modelRadius;

        if (i == 0) {
            scale = m_cellSize * snakeHeadScaleFromConfig;
            modelRadius = headRadius;
        }
        else if (i == objects.getSnake().size() - 1) {
            scale = m_cellSize * snakeTailScaleFromConfig;
            modelRadius = tailRadius;
        }
        else {
            scale = m_cellSize * snakeBodyScaleFromConfig;
            modelRadius = bodyRadius;
        }

        float worldRadius = modelRadius * scale;

        float currentRadius = worldRadius;
        if (m_lightType != LightType::Directional) {
            currentRadius = worldRadius * 1.2f;
        }

        GLUquadric* quad = gluNewQuadric();
        glPushMatrix();
        glTranslatef(x, y, z);
        gluSphere(quad, currentRadius, 24, 24);
        glPopMatrix();
        gluDeleteQuadric(quad);
    }

    // ===== РИСУЕМ СФЕРЫ ДЛЯ ЕДЫ (жёлтые, полупрозрачные) =====
    glColor4f(1.0f, 1.0f, 0.0f, 0.3f);

    for (const auto& apple : objects.getFood()) {
        float x = apple.x * m_cellSize - offsetX;
        float z = apple.z * m_cellSize - offsetZ;

        // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ЯБЛОКА ИЗ КОНФИГА
        float scale = m_cellSize * appleScaleFromConfig;
        float worldRadius = appleModelRadius * scale;

        float centerX = x + m_cellSize * 0.5f;
        float centerY = m_floorHeight + worldRadius;
        float centerZ = z + m_cellSize * 0.5f;

        float currentRadius = worldRadius;
        if (m_lightType != LightType::Directional) {
            currentRadius = worldRadius * 1.2f;
        }

        GLUquadric* quad = gluNewQuadric();
        glPushMatrix();
        glTranslatef(centerX, centerY, centerZ);
        gluSphere(quad, currentRadius, 24, 24);
        glPopMatrix();
        gluDeleteQuadric(quad);
    }

    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
}
void GameRenderer::toggleUseExactModels() {
    m_useExactModels = !m_useExactModels;
    std::cout << "Exact models collision: " << (m_useExactModels ? "ENABLED (using mesh geometry)" : "DISABLED (using AABB/spheres)") << std::endl;

    // Помечаем тени как грязные для пересчёта
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}
// Добавьте эту функцию в GameRenderer.cpp

// GameRenderer.cpp - ПОЛНАЯ ФУНКЦИЯ С ИСПОЛЬЗОВАНИЕМ BOUNDING SPHERE ИЗ МОДЕЛИ

bool GameRenderer::rayIntersectsModel(const Ray& ray, const Model& model,
    const glm::mat4& transform,
    float& hitDistance, glm::vec3& hitPoint,
    bool skipSphereCheck) {

    if (model.vertices.empty()) return false;

    // Проверяем сферу только если не пропущена (для оптимизации из ShadowMapper)
    if (!skipSphereCheck) {
        if (!model.boundingSphereComputed) {
            return false;
        }

        // Преобразуем центр в мировые координаты
        glm::vec4 worldCenter4 = transform * glm::vec4(model.boundingCenter, 1.0f);
        glm::vec3 worldCenter = glm::vec3(worldCenter4);

        // Вычисляем масштаб
        glm::vec3 scale;
        scale.x = glm::length(glm::vec3(transform[0]));
        scale.y = glm::length(glm::vec3(transform[1]));
        scale.z = glm::length(glm::vec3(transform[2]));
        float maxScale = std::max({ scale.x, scale.y, scale.z });
        float worldRadius = model.boundingRadius * maxScale;

        // БЫСТРАЯ ПРОВЕРКА СФЕРЫ
        float tSphere;
        if (!rayIntersectsSphere(ray, worldCenter, worldRadius, tSphere)) {
            return false; // НЕ ПОПАЛИ - ВЫХОДИМ
        }
    }

    // ПОПАЛИ В СФЕРУ - ПРОВЕРЯЕМ ТРЕУГОЛЬНИКИ С РАННИМ ВЫХОДОМ
    const float EPSILON = 0.000001f;

    for (size_t i = 0; i < model.vertices.size(); i += 3) {
        glm::vec3 v0 = glm::vec3(transform * glm::vec4(model.vertices[i].position, 1.0f));
        glm::vec3 v1 = glm::vec3(transform * glm::vec4(model.vertices[i + 1].position, 1.0f));
        glm::vec3 v2 = glm::vec3(transform * glm::vec4(model.vertices[i + 2].position, 1.0f));

        // Алгоритм Möller–Trumbore
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) continue;

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f) continue;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f) continue;

        float t = f * glm::dot(edge2, q);

        if (t > EPSILON) {
            // ПЕРВОЕ ПОПАДАНИЕ - СРАЗУ ВОЗВРАЩАЕМ!
            hitDistance = t;
            hitPoint = ray.pointAt(t);
            return true;
        }
    }

    return false;
}
void GameRenderer::setShadowTraceModeByIndex(int modeIndex) {
    switch (modeIndex) {
    case 0:
        m_currentShadowMode = ShadowMapper::TRACE_CENTER;
        m_useCornerTrace = false;
        break;
    case 1:
        m_currentShadowMode = ShadowMapper::TRACE_CORNERS;
        m_useCornerTrace = true;
        break;
    case 2:
        m_currentShadowMode = ShadowMapper::TRACE_CENTER_SUBDIVIDED;
        m_useCornerTrace = false;
        break;
    case 3:
        m_currentShadowMode = ShadowMapper::TRACE_CORNERS_SUBDIVIDED;
        m_useCornerTrace = true;
        break;
    default:
        m_currentShadowMode = ShadowMapper::TRACE_CENTER;
        m_useCornerTrace = false;
    }

    // Применяем режим ко всем ShadowMapper
    m_staticShadow.setShadowTraceMode(m_currentShadowMode);
    m_dynamicShadow.setShadowTraceMode(m_currentShadowMode);
    m_foodShadow.setShadowTraceMode(m_currentShadowMode);

    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;

    std::cout << "Shadow trace mode set to: " << m_staticShadow.getCurrentModeName() << std::endl;
}
GameRenderer::~GameRenderer() {
    for (auto& pair : m_loadedFBXModels) {
        pair.second.cleanup();
    }
    m_loadedFBXModels.clear();
}
// GameRenderer.cpp - полная реализация переключения режима

void GameRenderer::toggleShadowTraceMode() {
    // Циклическое переключение между 4 режимами
    int currentIndex = static_cast<int>(m_currentShadowMode);
    int nextIndex = (currentIndex + 1) % 4;
    setShadowTraceModeByIndex(nextIndex);
}

void GameRenderer::setShadowTraceMode(bool useCorners) {
    m_useCornerTrace = useCorners;

    ShadowMapper::ShadowTraceMode mode = m_useCornerTrace ?
        ShadowMapper::TRACE_CORNERS : ShadowMapper::TRACE_CENTER;

    m_staticShadow.setShadowTraceMode(mode);
    m_dynamicShadow.setShadowTraceMode(mode);
    m_foodShadow.setShadowTraceMode(mode);

    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}
void GameRenderer::toggleAmbient() {
    m_ambientEnabled = !m_ambientEnabled;
    setupFixedPipelineLighting();  // Перезагружаем освещение
    std::cout << "Ambient lighting: " << (m_ambientEnabled ? "ENABLED" : "DISABLED") << std::endl;
}

void GameRenderer::toggleSpecular() {
    m_specularEnabled = !m_specularEnabled;
    setupFixedPipelineLighting();  // Перезагружаем освещение
    std::cout << "Specular lighting: " << (m_specularEnabled ? "ENABLED" : "DISABLED") << std::endl;
}

void GameRenderer::toggleShowAllRays() {
    m_showAllRays = !m_showAllRays;
    std::cout << "Show all rays: " << (m_showAllRays ? "YES (show all rays)" : "NO (show only hit rays)") << std::endl;
}
void GameRenderer::initialize() {
    initOpenGLSettings();
    setupFixedPipelineLighting();
    initRayTracingResources();
}

void GameRenderer::initOpenGLSettings() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);
    glDepthMask(GL_TRUE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Временно используем серый цвет, потом он будет изменён в renderGame()
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    GLint samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (samples > 0) {
        glEnable(GL_MULTISAMPLE);
        std::cout << "MSAA enabled: " << samples << "x samples" << std::endl;
    }

    std::cout << "OpenGL initialized with Fixed Pipeline" << std::endl;
}
void GameRenderer::toggleDebugNormals() {
    m_debugNormalsEnabled = !m_debugNormalsEnabled;
    std::cout << "Debug normals: " << (m_debugNormalsEnabled ? "ENABLED" : "DISABLED") << std::endl;
}

void GameRenderer::drawDebugNormals(const GameObjects& objects) {
    if (!m_debugNormalsEnabled) return;

    //glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    //glDisable(GL_LIGHTING);
    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_DEPTH_TEST);
    //glLineWidth(1.5f);

    //float offsetX = m_gridWidth * m_cellSize / 2.0f;
    //float offsetZ = m_gridDepth * m_cellSize / 2.0f;
    //float normalLength = 0.15f;

    //// Рисуем нормали для змейки
    //for (size_t i = 0; i < objects.getSnake().size(); i++) {
    //    const Point& segment = objects.getSnake()[i];
    //    float x = segment.x * m_cellSize - offsetX;
    //    float y = segment.y * m_cellSize + 0.1f;
    //    float z = segment.z * m_cellSize - offsetZ;

    //    float scale;
    //    const Model* currentModel = nullptr;

    //    if (i == 0) {
    //        scale = m_cellSize * objects.getSnakeHeadScale();
    //        currentModel = &m_snakeHeadModel;
    //    }
    //    else if (i == objects.getSnake().size() - 1) {
    //        scale = m_cellSize * objects.getSnakeTailScale();
    //        currentModel = &m_snakeTailModel;
    //    }
    //    else {
    //        scale = m_cellSize * objects.getSnakeBodyScale();
    //        currentModel = &m_snakeBodyModel;
    //    }

    //    float rotationAngle = calculateSegmentRotation(objects.getSnake(), i);

    //    glm::mat4 transform = glm::mat4(1.0f);
    //    transform = glm::translate(transform, glm::vec3(x, y, z));
    //    transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    //    transform = glm::scale(transform, glm::vec3(scale));

    //    drawModelNormals(*currentModel, transform, normalLength);
    //}

    //// Рисуем нормали для еды (яблок)
    //for (const auto& apple : objects.getFood()) {
    //    float x = apple.x * m_cellSize - offsetX;
    //    float y = apple.y * m_cellSize + 0.1f;
    //    float z = apple.z * m_cellSize - offsetZ;
    //    float scale = m_cellSize * 0.6f;

    //    glm::mat4 transform = glm::mat4(1.0f);
    //    transform = glm::translate(transform, glm::vec3(x, y, z));
    //    transform = glm::scale(transform, glm::vec3(scale));

    //    drawModelNormals(m_appleModel, transform, normalLength);
    //}

    //// Рисуем нормали для деревьев (препятствий)
    //for (const auto& obstacle : objects.getObstacles()) {
    //    for (const auto& block : obstacle.blocks) {
    //        float x = block.x * m_cellSize - offsetX;
    //        float y = block.y * m_cellSize;
    //        float z = block.z * m_cellSize - offsetZ;
    //        float scale = m_cellSize * 1.2f;

    //        glm::mat4 transform = glm::mat4(1.0f);
    //        transform = glm::translate(transform, glm::vec3(x, y, z));
    //        transform = glm::scale(transform, glm::vec3(scale));

    //        drawModelNormals(m_treeModel, transform, normalLength);
    //    }
    //}

    //// Рисуем нормали для забора
    //for (const auto& fenceBlock : objects.getFenceBlocks()) {
    //    float x = fenceBlock.x * m_cellSize - offsetX;
    //    float z = fenceBlock.z * m_cellSize - offsetZ;
    //    float y = 0.1f;
    //    float scale = m_cellSize;

    //    glm::mat4 transform = glm::mat4(1.0f);
    //    transform = glm::translate(transform, glm::vec3(x, y, z));
    //    transform = glm::scale(transform, glm::vec3(scale, scale * 0.5f, scale));

    //    drawModelNormals(m_fenceModel, transform, normalLength);
    //}

    //// Рисуем нормали для облаков
    //for (const auto& cloud : objects.getCloudSprites()) {
    //    float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
    //    if (distanceToCenter < 8.0f) continue;

    //    glm::mat4 transform = glm::mat4(1.0f);
    //    transform = glm::translate(transform, cloud.position);
    //    transform = glm::scale(transform, glm::vec3(cloud.size));

    //    drawModelNormals(m_cloudModel, transform, normalLength);
    //}

    //// Рисуем нормали для птиц
    //for (const auto& bird : objects.getBirds()) {
    //    glm::mat4 transform = glm::mat4(1.0f);
    //    transform = glm::translate(transform, bird.position);

    //    if (glm::length(bird.direction) > 0.1f) {
    //        float angle = atan2f(bird.direction.x, bird.direction.z);
    //        transform = glm::rotate(transform, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    //        float pitch = atan2f(bird.direction.y,
    //            glm::length(glm::vec2(bird.direction.x, bird.direction.z)));
    //        transform = glm::rotate(transform, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    //    }

    //    transform = glm::scale(transform, glm::vec3(bird.size));
    //    drawModelNormals(m_birdModel, transform, normalLength);
    //}

    //// Рисуем нормали для цветов
    //for (const auto& flower : objects.getFlowerSprites()) {
    //    glm::mat4 transform = glm::mat4(1.0f);
    //    transform = glm::translate(transform, glm::vec3(flower.position.x, flower.position.y + 0.05f, flower.position.z));
    //    transform = glm::scale(transform, glm::vec3(flower.size));

    //    drawModelNormals(m_flowerModel, transform, normalLength);
    //}

    //glPopAttrib();
    //glEnable(GL_DEPTH_TEST);
}

void GameRenderer::drawModelNormals(const Model& model, const glm::mat4& transform, float normalLength) {
    if (model.vertices.empty()) return;

    glColor3f(0.0f, 1.0f, 0.0f); // Зеленый цвет для нормалей

    glBegin(GL_LINES);

    for (const auto& vertex : model.vertices) {
        // Преобразуем позицию вершины в мировые координаты
        glm::vec4 posWorld = transform * glm::vec4(vertex.position, 1.0f);

        // Преобразуем нормаль в мировые координаты (используем нормальную матрицу)
        glm::mat3 normalMatrix = glm::mat3(transform);
        // Для правильного преобразования нормалей нужно использовать обратную транспонированную матрицу
        // но для равномерного масштаба и вращения можно использовать просто верхнюю левую часть
        glm::vec3 normalWorld = glm::normalize(normalMatrix * vertex.normal);

        glm::vec3 start = glm::vec3(posWorld);
        glm::vec3 end = start + normalWorld * normalLength;

        glVertex3f(start.x, start.y, start.z);
        glVertex3f(end.x, end.y, end.z);
    }

    glEnd();
}
void GameRenderer::setupFixedPipelineLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    // Глобальная ambient - включаем/выключаем
    if (m_ambientEnabled) {
        GLfloat global_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    }
    else {
        GLfloat global_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    }

    // Включаем два источника света
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // Настраиваем в зависимости от типа
    switch (m_lightType) {
    case LightType::Directional:
        setupDirectionalLight();
        break;
    case LightType::Points:
        setupPointLight();
        break;
    case LightType::Spot:
        setupSpotLight();
        break;
    }

    std::cout << "Fixed pipeline lighting configured, type: " << (int)m_lightType
        << ", Ambient: " << (m_ambientEnabled ? "ON" : "OFF")
        << ", Specular: " << (m_specularEnabled ? "ON" : "OFF") << std::endl;
}

void GameRenderer::setupDirectionalLight() {
    // Ambient - включаем/выключаем
    GLfloat light0_ambient[] = {
        m_ambientEnabled ? 0.3f : 0.0f,
        m_ambientEnabled ? 0.3f : 0.0f,
        m_ambientEnabled ? 0.3f : 0.0f,
        1.0f
    };

    GLfloat light0_diffuse[] = { m_lightColor.r, m_lightColor.g, m_lightColor.b, 1.0f };

    // Specular - включаем/выключаем
    GLfloat light0_specular[] = {
        m_specularEnabled ? 0.5f : 0.0f,
        m_specularEnabled ? 0.5f : 0.0f,
        m_specularEnabled ? 0.5f : 0.0f,
        1.0f
    };

    GLfloat light0_position[] = { -m_lightDir.x, -m_lightDir.y, -m_lightDir.z, 0.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    // Выключаем LIGHT1
    glDisable(GL_LIGHT1);
}

void GameRenderer::setupPointLight() {
    // Ambient - включаем/выключаем
    GLfloat light0_ambient[] = {
        m_ambientEnabled ? 0.2f : 0.0f,
        m_ambientEnabled ? 0.2f : 0.0f,
        m_ambientEnabled ? 0.2f : 0.0f,
        1.0f
    };

    GLfloat light0_diffuse[] = { m_lightColor.r * 0.9f, m_lightColor.g * 0.9f, m_lightColor.b * 0.9f, 1.0f };

    // Specular - включаем/выключаем
    GLfloat light0_specular[] = {
        m_specularEnabled ? 0.4f : 0.0f,
        m_specularEnabled ? 0.4f : 0.0f,
        m_specularEnabled ? 0.4f : 0.0f,
        1.0f
    };

    GLfloat light0_position[] = { m_lightPos.x, m_lightPos.y, m_lightPos.z, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    // Мягкое затухание
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.07f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.02f);

    // Выключаем LIGHT1 для чистоты
    glDisable(GL_LIGHT1);

    std::cout << "Soft Point Light configured at: "
        << m_lightPos.x << ", " << m_lightPos.y << ", " << m_lightPos.z << std::endl;
}

void GameRenderer::setupSpotLight() {
    // Ambient - включаем/выключаем
    GLfloat light0_ambient[] = {
        m_ambientEnabled ? 0.15f : 0.0f,
        m_ambientEnabled ? 0.15f : 0.0f,
        m_ambientEnabled ? 0.15f : 0.0f,
        1.0f
    };

    GLfloat light0_diffuse[] = { m_lightColor.r * 1.5f, m_lightColor.g * 1.5f, m_lightColor.b * 1.5f, 1.0f };

    // Specular - включаем/выключаем
    GLfloat light0_specular[] = {
        m_specularEnabled ? 0.7f : 0.0f,
        m_specularEnabled ? 0.7f : 0.0f,
        m_specularEnabled ? 0.7f : 0.0f,
        1.0f
    };

    GLfloat light0_position[] = { m_lightPos.x, m_lightPos.y, m_lightPos.z, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    // НАПРАВЛЕНИЕ ПРОЖЕКТОРА - СТРОГО ВНИЗ
    GLfloat spot_direction[] = { 0.0f, -1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);

    // Угол конуса (45 градусов)
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 2.0f);

    // Затухание
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.03f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01f);

    // Заполняющий свет - ambient тоже зависит от переключателя
    GLfloat light1_ambient[] = {
        m_ambientEnabled ? 0.2f : 0.0f,
        m_ambientEnabled ? 0.2f : 0.0f,
        m_ambientEnabled ? 0.2f : 0.0f,
        1.0f
    };
    GLfloat light1_diffuse[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat light1_position[] = { 0.0f, 5.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);

    std::cout << "Spot Light configured: cutoff=45°, direction=(0,-1,0)" << std::endl;
}

void GameRenderer::updateLightPosition() {
    switch (m_lightType) {
    case LightType::Directional: {
        GLfloat light0_position[] = { -m_lightDir.x, -m_lightDir.y, -m_lightDir.z, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        break;
    }
    case LightType::Points: {
        GLfloat light0_position[] = { m_lightPos.x, m_lightPos.y, m_lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
        break;
    }
    case LightType::Spot: {
        GLfloat light0_position[] = { m_lightPos.x, m_lightPos.y, m_lightPos.z, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

        // Обновляем направление прожектора (светит вниз от позиции)
        GLfloat spot_direction[] = { 0.0f, -1.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
        break;
    }
    }
}
void GameRenderer::setSkyColor(const glm::vec3& color) {
    skyColor = color;
}

void GameRenderer::setFloorColor(const glm::vec3& color) {
    floorColor = color;
}

void GameRenderer::setGridColor(const glm::vec3& color) {
    gridColor = color;
}

void GameRenderer::setGridSettings(bool enabled, float lineWidth) {
    gridEnabled = enabled;
    gridLineWidth = lineWidth;
}

void GameRenderer::setGridDimensions(int width, int depth, float cellSize) {
    m_gridWidth = width;
    m_gridDepth = depth;
    m_cellSize = cellSize;

    // Пересчитываем размеры тайлов пола
    m_floorTileSizeX = (m_gridWidth * m_cellSize) / m_floorTilesX;
    m_floorTileSizeZ = (m_gridDepth * m_cellSize) / m_floorTilesZ;

    // Помечаем тени как грязные
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}

void GameRenderer::setShadowMapEnabled(bool enabled) {
    m_shadowMapEnabled = enabled;
    shadow_map = enabled;

    // Помечаем тени как грязные
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}

void GameRenderer::setShadowSubdivisionSize(int size) {
    if (size < 2) size = 2;
    if (size > 20) size = 20;
    m_floorSubdivisionLevel = size;

    // Помечаем тени как грязные
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}

void GameRenderer::setShadowStride(int strideX, int strideZ) {
    if (strideX < 1) strideX = 1;
    if (strideZ < 1) strideZ = 1;
    if (strideX > 8) strideX = 8;
    if (strideZ > 8) strideZ = 8;

    m_shadowStrideX = strideX;
    m_shadowStrideZ = strideZ;

    // Помечаем тени как грязные
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}

void GameRenderer::setAmbientEnabled(bool enabled) {
    m_ambientEnabled = enabled;
    setupFixedPipelineLighting();
}

void GameRenderer::setSpecularEnabled(bool enabled) {
    m_specularEnabled = enabled;
    setupFixedPipelineLighting();
}

void GameRenderer::setLightPos(const glm::vec3& pos) {
    m_lightPos = pos;
    setupFixedPipelineLighting();
    updateLightPosition();

    if (m_shadowMapEnabled) {
        updateSpheresRadii();  // <-- Добавить
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
        m_foodShadowsDirty = true;
    }
}

void GameRenderer::setLightDir(const glm::vec3& dir) {
    m_lightDir = glm::normalize(dir);
    setupFixedPipelineLighting();
    updateLightPosition();

    if (m_shadowMapEnabled) {
        updateSpheresRadii();  // <-- Добавить
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
        m_foodShadowsDirty = true;
    }
}

void GameRenderer::setLightType(LightType type) {
    m_lightType = type;
    setupFixedPipelineLighting();
    updateLightPosition();

    if (m_shadowMapEnabled) {
        updateSpheresRadii();  // <-- Добавить
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
        m_foodShadowsDirty = true;
    }
}

void GameRenderer::setLightColor(const glm::vec3& color) {
    m_lightColor = color;
    setupFixedPipelineLighting();

    // Помечаем тени как грязные
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}

void GameRenderer::markStaticShadowsDirty() {
    m_staticShadowsDirty = true;
}

void GameRenderer::forceFoodShadowsUpdate(const GameObjects& objects) {
    m_foodShadowsDirty = true;

    // Очищаем границы объектов
    m_foodShadow.clearObjectBounds();

    // Устанавливаем callback для пересечения с едой
    m_foodShadow.setIntersectCallback(
        [this, &objects](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
            float offsetX = m_gridWidth * m_cellSize / 2.0f;
            float offsetZ = m_gridDepth * m_cellSize / 2.0f;
            HitInfo hit = intersectFoodOnly(ray, objects, offsetX, offsetZ);
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

void GameRenderer::toggleRayTracing() {
    m_rayTracingEnabled = !m_rayTracingEnabled;
    std::cout << "Ray tracing " << (m_rayTracingEnabled ? "ENABLED" : "DISABLED") << std::endl;
}


void GameRenderer::setLightPosition(const glm::vec3& pos) {
    m_lightPos = pos;
    if (m_lightType != LightType::Directional) {
        updateLightPosition();

        if (m_shadowMapEnabled) {
            m_staticShadow.updateSpheresRadius(m_lightType, m_lightPos, m_lightDir);
            m_dynamicShadow.updateSpheresRadius(m_lightType, m_lightPos, m_lightDir);
            m_foodShadow.updateSpheresRadius(m_lightType, m_lightPos, m_lightDir);

            m_staticShadowsDirty = true;
            m_dynamicShadowsDirty = true;
            m_foodShadowsDirty = true;
        }
    }
}

void GameRenderer::setLightDirection(const glm::vec3& dir) {
    m_lightDir = glm::normalize(dir);
    if (m_lightType == LightType::Directional) {
        updateLightPosition();

        if (m_shadowMapEnabled) {
            m_staticShadow.updateSpheresRadius(m_lightType, m_lightPos, m_lightDir);
            m_dynamicShadow.updateSpheresRadius(m_lightType, m_lightPos, m_lightDir);
            m_foodShadow.updateSpheresRadius(m_lightType, m_lightPos, m_lightDir);

            m_staticShadowsDirty = true;
            m_dynamicShadowsDirty = true;
            m_foodShadowsDirty = true;
        }
    }
}




void GameRenderer::setMaterial(const glm::vec3& color, float shininess, float specularStrength) {
    glDisable(GL_COLOR_MATERIAL);

    GLfloat ambient[] = { color.r , color.g , color.b , 1.0f };
    GLfloat diffuse[] = { color.r , color.g , color.b , 1.0f };
    GLfloat specular[] = { specularStrength, specularStrength, specularStrength, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void GameRenderer::setupTexture(GLuint textureID) {
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }
}

void GameRenderer::resetDepthState() {
    glDepthMask(GL_TRUE);
    glClearDepth(1.0f);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

//=============================================================================
// ЗАГРУЗКА FBX МОДЕЛЕЙ
//=============================================================================

bool GameRenderer::loadFBXModelToModel(const std::string& filename, Model& outModel, const std::string& subFolder) {
    // Проверяем, не загружена ли уже эта модель
    std::string cacheKey = subFolder + "/" + filename;
    auto it = m_loadedFBXModels.find(cacheKey);
    if (it != m_loadedFBXModels.end()) {
        std::cout << "Using cached FBX model: " << cacheKey << std::endl;
        outModel = it->second;
        return true;
    }

    std::cout << "Loading FBX model: " << filename << " from folder: " << subFolder << std::endl;

    ModelData modelData;
    if (!loadFBXModel(filename, modelData, subFolder)) {
        std::cout << "Failed to load FBX model: " << filename << std::endl;
        return false;
    }

    if (!convertModelDataToModel(modelData, outModel)) {
        std::cout << "Failed to convert model data for: " << filename << std::endl;
        return false;
    }

    // Кешируем модель
    m_loadedFBXModels[cacheKey] = outModel;
    std::cout << "FBX model loaded and cached: " << cacheKey << std::endl;

    return true;
}

bool GameRenderer::convertModelDataToModel(const ModelData& modelData, Model& outModel) {
    if (modelData.vertices.empty()) {
        std::cout << "ModelData has no vertices" << std::endl;
        return false;
    }

    outModel.vertices.clear();
    outModel.hasTexture = false;
    outModel.textureID = 0;

    // Конвертация вершин
    size_t vertexCount = modelData.vertices.size() / 3;
    bool hasNormals = !modelData.normals.empty();
    bool hasTexCoords = !modelData.texCoords.empty();

    for (size_t i = 0; i < vertexCount; i++) {
        Vertex vertex;
        vertex.position.x = modelData.vertices[i * 3];
        vertex.position.y = modelData.vertices[i * 3 + 1];
        vertex.position.z = modelData.vertices[i * 3 + 2];

        if (hasNormals && i * 3 + 2 < modelData.normals.size()) {
            vertex.normal.x = modelData.normals[i * 3];
            vertex.normal.y = modelData.normals[i * 3 + 1];
            vertex.normal.z = modelData.normals[i * 3 + 2];
        }
        else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (hasTexCoords && i * 2 + 1 < modelData.texCoords.size()) {
            vertex.texCoords.x = modelData.texCoords[i * 2];
            vertex.texCoords.y = modelData.texCoords[i * 2 + 1];
        }
        else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        outModel.vertices.push_back(vertex);
    }

    if (!modelData.materials.empty() && modelData.materials[0].textureID != 0) {
        outModel.textureID = modelData.materials[0].textureID;
        outModel.hasTexture = true;
    }

    // ✅ ЭТОТ ВЫЗОВ ВЫЧИСЛИТ BOUNDING SPHERE
    outModel.setupBuffers();

    return true;
}

void GameRenderer::setupModelTexture(Model& model, GLuint textureID) {
    model.textureID = textureID;
    model.hasTexture = (textureID != 0);

    // Перекомпилируем display list с текстурой
    if (model.isCompiled && model.displayList != 0) {
        model.setupBuffers();
    }
}

void GameRenderer::drawModelWithMaterial(const Model& model, float x, float y, float z, float scale, const glm::vec3& color) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    if (model.hasTexture && model.textureID != 0) {
        // Если у модели есть текстура, используем белый цвет для материала
        setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 32.0f, 0.3f);
        setupTexture(model.textureID);
    }
    else {
        setMaterial(color, 32.0f, 0.3f);
        setupTexture(0);
    }

    model.draw();
    glPopMatrix();
}

//=============================================================================
// ЗАГРУЗКА ВСЕХ МОДЕЛЕЙ ИЗ КОНФИГА
//=============================================================================

void GameRenderer::loadModelsFromConfig(const GameObjects& objects) {
    std::cout << "\n========== LOADING MODELS FROM CONFIG ==========" << std::endl;

    // Сначала пытаемся загрузить FBX модели из конфига
    // Если не удалось - создаём примитивы как fallback

    std::cout << "\n--- Loading Apple Model ---" << std::endl;
    if (!objects.getAppleModel().empty()) {
        if (!loadFBXModelToModel(objects.getAppleModel(), m_appleModel, "food")) {
            std::cout << "  Fallback to primitive for apple" << std::endl;
            ApplePrimitive::create(m_appleModel);
        }
    }
    else {
        ApplePrimitive::create(m_appleModel);
    }

    std::cout << "\n--- Loading Tree Model ---" << std::endl;
    if (!objects.getTreeModel().empty()) {
        if (!loadFBXModelToModel(objects.getTreeModel(), m_treeModel, "obstacles")) {
            std::cout << "  Fallback to primitive for tree" << std::endl;
            TreePrimitive::create(m_treeModel);
        }
    }
    else {
        TreePrimitive::create(m_treeModel);
    }

    std::cout << "\n--- Loading Cloud Model ---" << std::endl;
    if (!objects.getCloudModel().empty()) {
        if (!loadFBXModelToModel(objects.getCloudModel(), m_cloudModel, "clouds")) {
            std::cout << "  Fallback to primitive for cloud" << std::endl;
            CloudPrimitive::create(m_cloudModel);
        }
    }
    else {
        CloudPrimitive::create(m_cloudModel);
    }

    std::cout << "\n--- Loading Bird Model ---" << std::endl;
    if (!objects.getBirdModel().empty()) {
        if (!loadFBXModelToModel(objects.getBirdModel(), m_birdModel, "birds")) {
            std::cout << "  Fallback to primitive for bird" << std::endl;
            BirdPrimitive::create(m_birdModel);
        }
    }
    else {
        BirdPrimitive::create(m_birdModel);
    }

    std::cout << "\n--- Loading Flower Model ---" << std::endl;
    if (!objects.getFlowerModel().empty()) {
        if (!loadFBXModelToModel(objects.getFlowerModel(), m_flowerModel, "flowers")) {
            std::cout << "  Fallback to primitive for flower" << std::endl;
            FlowerPrimitive::create(m_flowerModel);
        }
    }
    else {
        FlowerPrimitive::create(m_flowerModel);
    }

    std::cout << "\n--- Loading Fence Model ---" << std::endl;
    if (!objects.getFenceModel().empty()) {
        if (!loadFBXModelToModel(objects.getFenceModel(), m_fenceModel, "fence")) {
            std::cout << "  Fallback to primitive for fence" << std::endl;
            FencePrimitive::create(m_fenceModel);
        }
    }
    else {
        FencePrimitive::create(m_fenceModel);
    }

    std::cout << "\n--- Loading Snake Models ---" << std::endl;

    // Загружаем модель головы змеи
    if (!objects.getSnakeHeadModel().empty()) {
        if (!loadFBXModelToModel(objects.getSnakeHeadModel(), m_snakeHeadModel, "snake")) {
            std::cout << "  Fallback to primitive for snake head" << std::endl;
            SnakeHeadPrimitive::create(m_snakeHeadModel);
        }
    }
    else {
        SnakeHeadPrimitive::create(m_snakeHeadModel);
    }

    // Загружаем модель тела змеи
    if (!objects.getSnakeBodyModel().empty()) {
        if (!loadFBXModelToModel(objects.getSnakeBodyModel(), m_snakeBodyModel, "snake")) {
            std::cout << "  Fallback to primitive for snake body" << std::endl;
            SnakeBodyPrimitive::create(m_snakeBodyModel);
        }
    }
    else {
        SnakeBodyPrimitive::create(m_snakeBodyModel);
    }

    // Загружаем модель хвоста змеи
    if (!objects.getSnakeTailModel().empty()) {
        if (!loadFBXModelToModel(objects.getSnakeTailModel(), m_snakeTailModel, "snake")) {
            std::cout << "  Fallback to primitive for snake tail" << std::endl;
            SnakeTailPrimitive::create(m_snakeTailModel);
        }
    }
    else {
        SnakeTailPrimitive::create(m_snakeTailModel);
    }
    std::cout << "\n--- Loading Floor Model ---" << std::endl;
    if (!objects.getFloorModel().empty()) {
        if (!loadFBXModelToModel(objects.getFloorModel(), m_floorModel, "floor")) {
            std::cout << "  Fallback to textured quad for floor" << std::endl;
        }
    }
    else {
        std::cout << "  No floor model specified, using fallback" << std::endl;
    }

    std::cout << "================================================\n" << std::endl;
}

void GameRenderer::setFloorTexture(const std::string& texturePath) {
    if (texturePath.empty()) {
        useFloorTexture = false;
        std::cout << "Floor texture cleared, using color" << std::endl;
        return;
    }

    std::string fullPath = g_texturesPath + texturePath;
    std::cout << "Loading floor texture from: " << fullPath << std::endl;

    if (floorTexture.id != 0) {
        glDeleteTextures(1, &floorTexture.id);
        floorTexture.id = 0;
    }

    glGenTextures(1, &floorTexture.id);
    glBindTexture(GL_TEXTURE_2D, floorTexture.id);

    int width, height, channels;
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);

    if (data) {
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        floorTexture.width = width;
        floorTexture.height = height;
        floorTexture.path = texturePath;
        useFloorTexture = true;

        std::cout << "✓ Floor texture loaded: " << width << "x" << height << std::endl;
    }
    else {
        std::cout << "✗ Failed to load floor texture: " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &floorTexture.id);
        floorTexture.id = 0;
        useFloorTexture = false;
    }
}

//=============================================================================
// ОСНОВНОЙ РЕНДЕРИНГ
//=============================================================================

void GameRenderer::renderGame(const GameObjects& objects) {
    skyColor = objects.getSkyColor();
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    floorColor = objects.getFloorColor();
    gridColor = objects.getGridColor();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1200.0 / 800.0, 0.2, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glm::vec3 camPos = g_camera.getPosition();
    glm::vec3 camTarget = camPos + g_camera.getFront();
    glm::vec3 camUp = g_camera.getUp();

    gluLookAt(camPos.x, camPos.y, camPos.z,
        camTarget.x, camTarget.y, camTarget.z,
        camUp.x, camUp.y, camUp.z);

    updateLightPosition();

    if (m_shadowMapEnabled) {
        computeShadowsIfNeeded(objects);
    }

    if (m_floorModel.vertices.empty() || objects.getFloorModel().empty()) {
        drawFallbackFloor();
    }
    else {
        drawTiledFloor(objects);
    }
    
    drawObstaclesAsTrees(objects.getObstacles());
    drawFence(objects.getFenceBlocks());
    drawFood(objects.getFood());
    drawSnake(objects.getSnake());
    drawGroundSprites(objects.getFlowerSprites());
    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
    drawLightSource();
    drawDebugNormals(objects);
    drawDebugNormals(objects);
    drawDebugSpheres();  
    if (m_debugRaysEnabled && m_shadowMapEnabled) {
        drawDebugRaysIfEnabled();
    }
}

// GameRenderer.cpp - полная функция с синхронизацией режима

void GameRenderer::updateSpheresRadii() {
    if (!m_shadowMapEnabled) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;
    const GameObjects& objects = g_game.getGameObjects();

    // Получаем масштабы из конфига
    float treeScaleFromConfig = objects.getTreeScale();
    float appleScaleFromConfig = objects.getAppleScale();
    float snakeHeadScaleFromConfig = objects.getSnakeHeadScale();
    float snakeBodyScaleFromConfig = objects.getSnakeBodyScale();
    float snakeTailScaleFromConfig = objects.getSnakeTailScale();

    // ========== ВЫЧИСЛЯЕМ РЕАЛЬНЫЕ РАДИУСЫ МОДЕЛЕЙ (один раз) ==========
    static float treeModelRadius = 0.0f;
    static float headRadius = 0.0f, bodyRadius = 0.0f, tailRadius = 0.0f;
    static float appleModelRadius = 0.0f;
    static bool radiiComputed = false;

    if (!radiiComputed) {
        // Радиус дерева
        if (!m_treeModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;

            for (const auto& vert : m_treeModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            treeModelRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            std::cout << "Tree model radius (local): " << treeModelRadius << std::endl;
        }

        // Радиусы змейки
        if (!m_snakeHeadModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;

            for (const auto& vert : m_snakeHeadModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            headRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;

            if (!m_snakeBodyModel.vertices.empty()) {
                minX = FLT_MAX, maxX = -FLT_MAX;
                minY = FLT_MAX, maxY = -FLT_MAX;
                minZ = FLT_MAX, maxZ = -FLT_MAX;
                for (const auto& vert : m_snakeBodyModel.vertices) {
                    minX = std::min(minX, vert.position.x);
                    maxX = std::max(maxX, vert.position.x);
                    minY = std::min(minY, vert.position.y);
                    maxY = std::max(maxY, vert.position.y);
                    minZ = std::min(minZ, vert.position.z);
                    maxZ = std::max(maxZ, vert.position.z);
                }
                bodyRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            }

            if (!m_snakeTailModel.vertices.empty()) {
                minX = FLT_MAX, maxX = -FLT_MAX;
                minY = FLT_MAX, maxY = -FLT_MAX;
                minZ = FLT_MAX, maxZ = -FLT_MAX;
                for (const auto& vert : m_snakeTailModel.vertices) {
                    minX = std::min(minX, vert.position.x);
                    maxX = std::max(maxX, vert.position.x);
                    minY = std::min(minY, vert.position.y);
                    maxY = std::max(maxY, vert.position.y);
                    minZ = std::min(minZ, vert.position.z);
                    maxZ = std::max(maxZ, vert.position.z);
                }
                tailRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            }
            std::cout << "Snake radii: head=" << headRadius << ", body=" << bodyRadius << ", tail=" << tailRadius << std::endl;
        }

        // Радиус яблока
        if (!m_appleModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;

            for (const auto& vert : m_appleModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            appleModelRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            std::cout << "Apple model radius: " << appleModelRadius << std::endl;
        }

        radiiComputed = true;
    }

    // ========== СФЕРЫ ДЛЯ ДЕРЕВЬЕВ С ID ==========
    std::vector<BoundingSphere> treeSpheres;
    int treeId = 0;
    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float z = block.z * m_cellSize - offsetZ;
            float y = block.y * m_cellSize;

            // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ДЕРЕВА ИЗ КОНФИГА
            float scale = m_cellSize * treeScaleFromConfig;
            float worldRadius = treeModelRadius * scale;

            glm::vec3 center(
                x + m_cellSize * 0.5f,
                y + scale * 1.0f,
                z + m_cellSize * 0.5f
            );

            BoundingSphere sphere(center, worldRadius, BoundingSphere::MODEL_TREE, treeId++);
            sphere.updateRadius(m_lightType, m_lightPos, m_lightDir, 20.0f);
            treeSpheres.push_back(sphere);
        }
    }
    m_staticShadow.registerObjectBounds(treeSpheres);
    if (!treeSpheres.empty()) {
        std::cout << "Tree spheres: " << treeSpheres.size() << ", radius=" << treeSpheres[0].currentRadius << std::endl;
    }

    // ========== СФЕРЫ ДЛЯ ЗМЕЙКИ С ID ==========
    std::vector<BoundingSphere> snakeSpheres;
    for (size_t i = 0; i < objects.getSnake().size(); i++) {
        const Point& segment = objects.getSnake()[i];
        float x = segment.x * m_cellSize - offsetX;
        float z = segment.z * m_cellSize - offsetZ;
        float y = segment.y * m_cellSize + 0.1f;

        float scale;
        float modelRadius;
        BoundingSphere::ModelType type;

        if (i == 0) {
            scale = m_cellSize * snakeHeadScaleFromConfig;
            modelRadius = headRadius;
            type = BoundingSphere::MODEL_SNAKE_HEAD;
        }
        else if (i == objects.getSnake().size() - 1) {
            scale = m_cellSize * snakeTailScaleFromConfig;
            modelRadius = tailRadius;
            type = BoundingSphere::MODEL_SNAKE_TAIL;
        }
        else {
            scale = m_cellSize * snakeBodyScaleFromConfig;
            modelRadius = bodyRadius;
            type = BoundingSphere::MODEL_SNAKE_BODY;
        }

        float worldRadius = modelRadius * scale;

        BoundingSphere sphere(glm::vec3(x, y, z), worldRadius, type, (int)i);
        sphere.updateRadius(m_lightType, m_lightPos, m_lightDir, 20.0f);
        snakeSpheres.push_back(sphere);
    }
    m_dynamicShadow.registerObjectBounds(snakeSpheres);
    std::cout << "Snake spheres: " << snakeSpheres.size() << std::endl;

    // ========== СФЕРЫ ДЛЯ ЕДЫ С ID ==========
    std::vector<BoundingSphere> foodSpheres;
    int foodId = 0;
    for (const auto& apple : objects.getFood()) {
        float x = apple.x * m_cellSize - offsetX;
        float z = apple.z * m_cellSize - offsetZ;

        // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ЯБЛОКА ИЗ КОНФИГА
        float scale = m_cellSize * appleScaleFromConfig;
        float worldRadius = appleModelRadius * scale;

        float centerX = x + m_cellSize * 0.5f;
        float centerY = m_floorHeight + worldRadius;
        float centerZ = z + m_cellSize * 0.5f;

        BoundingSphere sphere(glm::vec3(centerX, centerY, centerZ), worldRadius,
            BoundingSphere::MODEL_APPLE, foodId++);
        sphere.updateRadius(m_lightType, m_lightPos, m_lightDir, 20.0f);
        foodSpheres.push_back(sphere);
    }
    m_foodShadow.registerObjectBounds(foodSpheres);
    if (!foodSpheres.empty()) {
        std::cout << "Food spheres: " << foodSpheres.size() << ", radius=" << foodSpheres[0].currentRadius << std::endl;
    }

    // Помечаем тени как грязные для пересчёта
    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;
}

void GameRenderer::computeShadowsIfNeeded(const GameObjects& objects) {
    // Синхронизируем режим и свет
    m_staticShadow.setShadowTraceMode(m_currentShadowMode);
    m_dynamicShadow.setShadowTraceMode(m_currentShadowMode);
    m_foodShadow.setShadowTraceMode(m_currentShadowMode);
    float groundHeight = m_floorHeight;

    m_staticShadow.setGroundHeight(groundHeight);
    m_dynamicShadow.setGroundHeight(groundHeight);
    m_foodShadow.setGroundHeight(groundHeight);

    m_staticShadow.setLightType(m_lightType);
    m_staticShadow.setLightPos(m_lightPos);
    m_staticShadow.setLightDirection(m_lightDir);

    m_dynamicShadow.setLightType(m_lightType);
    m_dynamicShadow.setLightPos(m_lightPos);
    m_dynamicShadow.setLightDirection(m_lightDir);

    m_foodShadow.setLightType(m_lightType);
    m_foodShadow.setLightPos(m_lightPos);
    m_foodShadow.setLightDirection(m_lightDir);

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    // Получаем масштабы из конфига
    float treeScaleFromConfig = objects.getTreeScale();
    float appleScaleFromConfig = objects.getAppleScale();
    float snakeHeadScaleFromConfig = objects.getSnakeHeadScale();
    float snakeBodyScaleFromConfig = objects.getSnakeBodyScale();
    float snakeTailScaleFromConfig = objects.getSnakeTailScale();

    // ========== ПРЕДВАРИТЕЛЬНО ВЫЧИСЛЯЕМ РАДИУСЫ МОДЕЛЕЙ ==========
    static float treeModelRadius = 0.0f;
    static float headRadius = 0.0f, bodyRadius = 0.0f, tailRadius = 0.0f;
    static float appleModelRadius = 0.0f;
    static bool radiiComputed = false;

    if (!radiiComputed && m_shadowMapEnabled) {
        // Радиус дерева
        if (!m_treeModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& vert : m_treeModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            treeModelRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
        }

        // Радиусы змейки
        if (!m_snakeHeadModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& vert : m_snakeHeadModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            headRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;

            if (!m_snakeBodyModel.vertices.empty()) {
                minX = FLT_MAX, maxX = -FLT_MAX;
                minY = FLT_MAX, maxY = -FLT_MAX;
                minZ = FLT_MAX, maxZ = -FLT_MAX;
                for (const auto& vert : m_snakeBodyModel.vertices) {
                    minX = std::min(minX, vert.position.x);
                    maxX = std::max(maxX, vert.position.x);
                    minY = std::min(minY, vert.position.y);
                    maxY = std::max(maxY, vert.position.y);
                    minZ = std::min(minZ, vert.position.z);
                    maxZ = std::max(maxZ, vert.position.z);
                }
                bodyRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            }

            if (!m_snakeTailModel.vertices.empty()) {
                minX = FLT_MAX, maxX = -FLT_MAX;
                minY = FLT_MAX, maxY = -FLT_MAX;
                minZ = FLT_MAX, maxZ = -FLT_MAX;
                for (const auto& vert : m_snakeTailModel.vertices) {
                    minX = std::min(minX, vert.position.x);
                    maxX = std::max(maxX, vert.position.x);
                    minY = std::min(minY, vert.position.y);
                    maxY = std::max(maxY, vert.position.y);
                    minZ = std::min(minZ, vert.position.z);
                    maxZ = std::max(maxZ, vert.position.z);
                }
                tailRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
            }
        }

        // Радиус яблока
        if (!m_appleModel.vertices.empty()) {
            float minX = FLT_MAX, maxX = -FLT_MAX;
            float minY = FLT_MAX, maxY = -FLT_MAX;
            float minZ = FLT_MAX, maxZ = -FLT_MAX;
            for (const auto& vert : m_appleModel.vertices) {
                minX = std::min(minX, vert.position.x);
                maxX = std::max(maxX, vert.position.x);
                minY = std::min(minY, vert.position.y);
                maxY = std::max(maxY, vert.position.y);
                minZ = std::min(minZ, vert.position.z);
                maxZ = std::max(maxZ, vert.position.z);
            }
            appleModelRadius = std::max({ maxX - minX, maxY - minY, maxZ - minZ }) * 0.5f;
        }

        radiiComputed = true;
        std::cout << "Model radii computed: tree=" << treeModelRadius
            << ", head=" << headRadius << ", body=" << bodyRadius
            << ", tail=" << tailRadius << ", apple=" << appleModelRadius << std::endl;
    }

    // ========== CALLBACK ДЛЯ РЕЖИМА БЕЗ СФЕР (BRUTE-FORCE) ==========
    // Эти колбэки проверяют ВСЕ треугольники ВСЕХ объектов (без сфер!)
    auto bruteForceCallbackTrees = [this, &objects, offsetX, offsetZ](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
        HitInfo hit = intersectTreesOnly(ray, objects, offsetX, offsetZ);
        if (hit.hit && hit.distance > 0.01f) {
            hitDist = hit.distance;
            hitPoint = hit.point;
            return true;
        }
        return false;
        };

    auto bruteForceCallbackSnake = [this, &objects, offsetX, offsetZ](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
        HitInfo hit = intersectSnakeOnly(ray, objects, offsetX, offsetZ);
        if (hit.hit && hit.distance > 0.01f) {
            hitDist = hit.distance;
            hitPoint = hit.point;
            return true;
        }
        return false;
        };

    auto bruteForceCallbackFood = [this, &objects, offsetX, offsetZ](const Ray& ray, float& hitDist, glm::vec3& hitPoint) -> bool {
        HitInfo hit = intersectFoodOnly(ray, objects, offsetX, offsetZ);
        if (hit.hit && hit.distance > 0.01f) {
            hitDist = hit.distance;
            hitPoint = hit.point;
            return true;
        }
        return false;
        };

    // ========== СТАТИЧЕСКИЕ ТЕНИ (деревья) ==========
    if (m_staticShadowsDirty) {
        m_staticShadow.clearObjectBounds();
        m_staticShadow.setGrid(m_gridWidth, m_gridDepth, m_cellSize, m_floorHeight);

        // Устанавливаем режим использования сфер
        m_staticShadow.setUseSpheres(m_useSpheres);

        if (m_useSpheres) {
            // ===== РЕЖИМ СО СФЕРАМИ: сфера → точная геометрия по ID =====
            std::vector<BoundingSphere> treeSpheres;
            int treeId = 0;
            for (const auto& obstacle : objects.getObstacles()) {
                for (const auto& block : obstacle.blocks) {
                    float x = block.x * m_cellSize - offsetX;
                    float z = block.z * m_cellSize - offsetZ;
                    float y = block.y * m_cellSize;

                    float scale = m_cellSize * treeScaleFromConfig;
                    float worldRadius = treeModelRadius * scale;

                    glm::vec3 center(
                        x + m_cellSize * 0.5f,
                        y + scale * 1.0f,
                        z + m_cellSize * 0.5f
                    );

                    BoundingSphere sphere(center, worldRadius, BoundingSphere::MODEL_TREE, treeId++);
                    sphere.updateRadius(m_lightType, m_lightPos, m_lightDir, 20.0f);
                    treeSpheres.push_back(sphere);
                }
            }
            m_staticShadow.registerObjectBounds(treeSpheres);

            // Точный колбэк по ID (без проверки сферы - она уже проверена)
            m_staticShadow.setIntersectCallbackExact(
                [this](const Ray& ray, float& hitDist, glm::vec3& hitPoint,
                    BoundingSphere::ModelType modelType, int instanceId) -> bool {
                        if (modelType != BoundingSphere::MODEL_TREE) return false;
                        float offsetX = m_gridWidth * m_cellSize / 2.0f;
                        float offsetZ = m_gridDepth * m_cellSize / 2.0f;
                        return intersectTreeById(ray, instanceId, offsetX, offsetZ, hitDist, hitPoint);
                }
            );

            // Для режима со сферами обычный колбэк не нужен
            m_staticShadow.setIntersectCallback(nullptr);
        }
        else {
            // ===== РЕЖИМ БЕЗ СФЕР: brute-force все треугольники всех деревьев =====
            m_staticShadow.setIntersectCallback(bruteForceCallbackTrees);
            m_staticShadow.clearObjectBounds();
        }

        m_staticShadow.computeShadows();
        m_staticShadowsDirty = false;

        std::cout << "Static shadows computed (spheres: " << (m_useSpheres ? "ON" : "OFF")
            << ", trees: " << objects.getObstacles().size() << " obstacles)" << std::endl;
    }

    // ========== ДИНАМИЧЕСКИЕ ТЕНИ (змейка) ==========
    if (m_shadowMapEnabled && m_dynamicShadowsDirty) {
        m_dynamicShadow.clearObjectBounds();
        m_dynamicShadow.setGrid(m_gridWidth, m_gridDepth, m_cellSize, m_floorHeight);

        // Устанавливаем режим использования сфер
        m_dynamicShadow.setUseSpheres(m_useSpheres);

        if (m_useSpheres) {
            // ===== РЕЖИМ СО СФЕРАМИ: сфера → точная геометрия по ID =====
            std::vector<BoundingSphere> snakeSpheres;
            for (size_t i = 0; i < objects.getSnake().size(); i++) {
                const Point& segment = objects.getSnake()[i];
                float x = segment.x * m_cellSize - offsetX;
                float z = segment.z * m_cellSize - offsetZ;
                float y = segment.y * m_cellSize + 0.1f;

                float scale;
                float modelRadius;
                BoundingSphere::ModelType type;

                if (i == 0) {
                    scale = m_cellSize * snakeHeadScaleFromConfig;
                    modelRadius = headRadius;
                    type = BoundingSphere::MODEL_SNAKE_HEAD;
                }
                else if (i == objects.getSnake().size() - 1) {
                    scale = m_cellSize * snakeTailScaleFromConfig;
                    modelRadius = tailRadius;
                    type = BoundingSphere::MODEL_SNAKE_TAIL;
                }
                else {
                    scale = m_cellSize * snakeBodyScaleFromConfig;
                    modelRadius = bodyRadius;
                    type = BoundingSphere::MODEL_SNAKE_BODY;
                }

                float worldRadius = modelRadius * scale;
                BoundingSphere sphere(glm::vec3(x, y, z), worldRadius, type, (int)i);
                sphere.updateRadius(m_lightType, m_lightPos, m_lightDir, 20.0f);
                snakeSpheres.push_back(sphere);
            }
            m_dynamicShadow.registerObjectBounds(snakeSpheres);

            // Точный колбэк по ID (без проверки сферы - она уже проверена)
            m_dynamicShadow.setIntersectCallbackExact(
                [this](const Ray& ray, float& hitDist, glm::vec3& hitPoint,
                    BoundingSphere::ModelType modelType, int instanceId) -> bool {
                        if (modelType != BoundingSphere::MODEL_SNAKE_HEAD &&
                            modelType != BoundingSphere::MODEL_SNAKE_BODY &&
                            modelType != BoundingSphere::MODEL_SNAKE_TAIL) return false;
                        float offsetX = m_gridWidth * m_cellSize / 2.0f;
                        float offsetZ = m_gridDepth * m_cellSize / 2.0f;
                        return intersectSnakeSegmentById(ray, instanceId, offsetX, offsetZ, hitDist, hitPoint);
                }
            );

            // Для режима со сферами обычный колбэк не нужен
            m_dynamicShadow.setIntersectCallback(nullptr);
        }
        else {
            // ===== РЕЖИМ БЕЗ СФЕР: brute-force все треугольники всех сегментов змейки =====
            m_dynamicShadow.setIntersectCallback(bruteForceCallbackSnake);
            m_dynamicShadow.clearObjectBounds();
        }

        m_dynamicShadow.computeShadows();
        m_dynamicShadowsDirty = false;

        std::cout << "Dynamic shadows computed (spheres: " << (m_useSpheres ? "ON" : "OFF")
            << ", snake segments: " << objects.getSnake().size() << ")" << std::endl;
    }

    // ========== ТЕНИ ДЛЯ ЕДЫ ==========
    if (m_shadowMapEnabled && m_foodShadowsDirty) {
        m_foodShadow.clearObjectBounds();
        m_foodShadow.setGrid(m_gridWidth, m_gridDepth, m_cellSize, m_floorHeight);

        // Устанавливаем режим использования сфер
        m_foodShadow.setUseSpheres(m_useSpheres);

        if (m_useSpheres) {
            // ===== РЕЖИМ СО СФЕРАМИ: сфера → точная геометрия по ID =====
            std::vector<BoundingSphere> foodSpheres;
            int foodId = 0;
            for (const auto& apple : objects.getFood()) {
                float x = apple.x * m_cellSize - offsetX;
                float z = apple.z * m_cellSize - offsetZ;

                float scale = m_cellSize * appleScaleFromConfig;
                float worldRadius = appleModelRadius * scale;

                float centerX = x + m_cellSize * 0.5f;
                float centerY = m_floorHeight + worldRadius;
                float centerZ = z + m_cellSize * 0.5f;

                BoundingSphere sphere(glm::vec3(centerX, centerY, centerZ), worldRadius,
                    BoundingSphere::MODEL_APPLE, foodId++);
                sphere.updateRadius(m_lightType, m_lightPos, m_lightDir, 20.0f);
                foodSpheres.push_back(sphere);
            }
            m_foodShadow.registerObjectBounds(foodSpheres);

            // Точный колбэк по ID (без проверки сферы - она уже проверена)
            m_foodShadow.setIntersectCallbackExact(
                [this](const Ray& ray, float& hitDist, glm::vec3& hitPoint,
                    BoundingSphere::ModelType modelType, int instanceId) -> bool {
                        if (modelType != BoundingSphere::MODEL_APPLE) return false;
                        float offsetX = m_gridWidth * m_cellSize / 2.0f;
                        float offsetZ = m_gridDepth * m_cellSize / 2.0f;
                        return intersectAppleById(ray, instanceId, offsetX, offsetZ, hitDist, hitPoint);
                }
            );

            // Для режима со сферами обычный колбэк не нужен
            m_foodShadow.setIntersectCallback(nullptr);
        }
        else {
            // ===== РЕЖИМ БЕЗ СФЕР: brute-force все треугольники всех яблок =====
            m_foodShadow.setIntersectCallback(bruteForceCallbackFood);
            m_foodShadow.clearObjectBounds();
        }

        m_foodShadow.computeShadows();
        m_foodShadowsDirty = false;

        std::cout << "Food shadows computed (spheres: " << (m_useSpheres ? "ON" : "OFF")
            << ", apples: " << objects.getFood().size() << ")" << std::endl;
    }
}
void GameRenderer::markFoodShadowsDirty() {
    m_foodShadowsDirty = true;
    std::cout << "Food shadows marked as DIRTY" << std::endl;
}
void GameRenderer::markDynamicShadowsDirty() {
    m_dynamicShadowsDirty = true;
    std::cout << "Dynamic shadows (snake) marked as DIRTY" << std::endl;
}
void GameRenderer::toggleShadowMap() {
    m_shadowMapEnabled = !m_shadowMapEnabled;
    std::cout << "Shadow mapping: " << (m_shadowMapEnabled ? "ENABLED" : "DISABLED") << std::endl;

    if (m_shadowMapEnabled) {
        resetShadows();
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
        m_foodShadowsDirty = true;
    }
}
//=============================================================================
// ОТРИСОВКА ОБЪЕКТОВ
//=============================================================================

void GameRenderer::drawFloor() {
    glPushMatrix();

    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    if (gridEnabled) {
        glDisable(GL_LIGHTING);
        glLineWidth(gridLineWidth);
        glColor3f(gridColor.r, gridColor.g, gridColor.b);

        glBegin(GL_LINES);
        for (int i = 0; i <= m_gridWidth; i++) {
            float x = i * m_cellSize - offsetX;
            glVertex3f(x, 0.01f, -offsetZ);
            glVertex3f(x, 0.01f, worldDepth - offsetZ);
        }
        for (int i = 0; i <= m_gridDepth; i++) {
            float z = i * m_cellSize - offsetZ;
            glVertex3f(-offsetX, 0.01f, z);
            glVertex3f(worldWidth - offsetX, 0.01f, z);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    }

    // Используем текстуру пола если включена
    if (useFloorTexture && floorTexture.id != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTexture.id);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    float texRepeat = worldWidth / 2.0f; // Повтор текстуры

    for (int z = 0; z < m_gridDepth; z++) {
        for (int x = 0; x < m_gridWidth; x++) {
            float posX = x * m_cellSize - offsetX;
            float posZ = z * m_cellSize - offsetZ;

            float shadowValue = 1.0f;
            if (m_shadowMapEnabled) {
                shadowValue = m_dynamicShadow.getShadowAtCell(x, z);
                float staticShadow = m_staticShadow.getShadowAtCell(x, z);
                shadowValue = std::min(shadowValue, staticShadow);
            }

            glm::vec3 finalColor = floorColor * shadowValue;
            glColor3f(finalColor.r, finalColor.g, finalColor.b);

            glBegin(GL_QUADS);
            glNormal3f(0.0f, 1.0f, 0.0f);

            if (useFloorTexture && floorTexture.id != 0) {
                float u1 = (float)x / texRepeat;
                float v1 = (float)z / texRepeat;
                float u2 = (float)(x + 1) / texRepeat;
                float v2 = (float)(z + 1) / texRepeat;

                glTexCoord2f(u1, v1); glVertex3f(posX, -0.02f, posZ);
                glTexCoord2f(u2, v1); glVertex3f(posX + m_cellSize, -0.02f, posZ);
                glTexCoord2f(u2, v2); glVertex3f(posX + m_cellSize, -0.02f, posZ + m_cellSize);
                glTexCoord2f(u1, v2); glVertex3f(posX, -0.02f, posZ + m_cellSize);
            }
            else {
                glVertex3f(posX, -0.02f, posZ);
                glVertex3f(posX + m_cellSize, -0.02f, posZ);
                glVertex3f(posX + m_cellSize, -0.02f, posZ + m_cellSize);
                glVertex3f(posX, -0.02f, posZ + m_cellSize);
            }
            glEnd();
        }
    }

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void GameRenderer::drawSnakeEyes() {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    glVertex3f(0.38f, 0.38f, 0.55f);
    glVertex3f(-0.38f, 0.38f, 0.55f);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glVertex3f(0.38f, 0.35f, 0.58f);
    glVertex3f(-0.38f, 0.35f, 0.58f);
    glEnd();
    glEnable(GL_LIGHTING);
}

float GameRenderer::calculateSegmentRotation(const std::vector<Point>& snake, size_t index) {
    if (snake.size() <= 1) return 0.0f;

    Point current = snake[index];
    Point next;

    if (index == 0) {
        next = snake[1];
    }
    else {
        next = snake[index - 1];
    }

    if (current.x > next.x) return 90.0f;
    if (current.x < next.x) return -90.0f;
    if (current.z > next.z) return 0.0f;
    if (current.z < next.z) return 180.0f;

    return 0.0f;
}

void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    if (snake.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;
    const GameObjects& gameObjects = g_game.getGameObjects();

    // Вычисляем bounding box модели головы, чтобы правильно поставить на пол
    float modelBottomOffset = 0.0f;
    if (!m_snakeHeadModel.vertices.empty()) {
        float minY_local = FLT_MAX;
        for (const auto& vert : m_snakeHeadModel.vertices) {
            minY_local = std::min(minY_local, vert.position.y);
        }
        modelBottomOffset = -minY_local;  // Смещение от центра модели до нижней точки
    }

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];

        float x = segment.x * m_cellSize - offsetX;
        float z = segment.z * m_cellSize - offsetZ;

        float scale;
        glm::vec3 color;
        const Model* currentModel = nullptr;

        if (i == 0) {
            scale = m_cellSize * gameObjects.getSnakeHeadScale();
            color = gameObjects.getSnakeHeadColor();
            currentModel = &m_snakeHeadModel;
        }
        else if (i == snake.size() - 1) {
            scale = m_cellSize * gameObjects.getSnakeTailScale();
            color = gameObjects.getSnakeTailColor();
            currentModel = &m_snakeTailModel;
        }
        else {
            scale = m_cellSize * gameObjects.getSnakeBodyScale();
            color = gameObjects.getSnakeBodyColor();
            currentModel = &m_snakeBodyModel;
        }

        // Ставим нижнюю точку модели на пол (m_floorHeight)
        float y = m_floorHeight + modelBottomOffset * scale;

        float rotationAngle = calculateSegmentRotation(snake, i);

        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
        glScalef(scale, scale, scale);

        if (currentModel->hasTexture && currentModel->textureID != 0) {
            setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 64.0f, 0.2f);
            setupTexture(currentModel->textureID);
        }
        else {
            setMaterial(color, 64.0f, 0.2f);
            setupTexture(0);
        }
        currentModel->draw();

        if (i == 0) {
            drawSnakeEyes();
        }

        glPopMatrix();
    }
}

void GameRenderer::drawFood(const std::vector<Point>& food) {
    if (food.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    if (m_appleModel.vertices.empty()) return;

    // Получаем масштаб яблока из GameObjects
    const GameObjects& objects = g_game.getGameObjects();
    float appleScaleFromConfig = objects.getAppleScale();  // Используем значение из конфига

    // Вычисляем bounding box модели один раз
    float minY_local = FLT_MAX;
    float maxY_local = -FLT_MAX;
    for (const auto& vert : m_appleModel.vertices) {
        minY_local = std::min(minY_local, vert.position.y);
        maxY_local = std::max(maxY_local, vert.position.y);
    }
    float offsetFromCenterToBottom = -minY_local;

    for (const auto& apple : food) {
        float x = apple.x * m_cellSize - offsetX;
        float z = apple.z * m_cellSize - offsetZ;

        // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ИЗ КОНФИГА!
        float scale = m_cellSize * appleScaleFromConfig;
        float worldBottomOffset = offsetFromCenterToBottom * scale;

        float centerX = x + m_cellSize * 0.5f;
        float centerY = m_floorHeight + worldBottomOffset;
        float centerZ = z + m_cellSize * 0.5f;

        glPushMatrix();
        glTranslatef(centerX, centerY, centerZ);
        glScalef(scale, scale, scale);

        if (m_appleModel.hasTexture && m_appleModel.textureID != 0) {
            setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 80.0f, 0.4f);
            setupTexture(m_appleModel.textureID);
        }
        else {
            setMaterial(glm::vec3(0.9f, 0.2f, 0.2f), 80.0f, 0.4f);
            setupTexture(0);
        }
        m_appleModel.draw();
        glPopMatrix();
    }
}

void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    if (obstacles.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;
    const GameObjects& objects = g_game.getGameObjects();

    // ✅ Получаем масштаб дерева из конфига
    float treeScaleFromConfig = objects.getTreeScale();

    // Вычисляем нижнюю точку модели дерева
    float treeBottomOffset = 0.0f;
    if (!m_treeModel.vertices.empty()) {
        float minY_local = FLT_MAX;
        for (const auto& vert : m_treeModel.vertices) {
            minY_local = std::min(minY_local, vert.position.y);
        }
        treeBottomOffset = -minY_local;
    }

    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float z = block.z * m_cellSize - offsetZ;

            // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ИЗ КОНФИГА!
            float scale = m_cellSize * treeScaleFromConfig;
            float y = m_floorHeight + treeBottomOffset * scale;

            glPushMatrix();
            glTranslatef(x + m_cellSize * 0.5f, y, z + m_cellSize * 0.5f);
            glScalef(scale, scale, scale);

            if (m_treeModel.hasTexture && m_treeModel.textureID != 0) {
                setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 30.0f, 0.2f);
                setupTexture(m_treeModel.textureID);
            }
            else {
                setMaterial(objects.getTreeColor(), 30.0f, 0.2f);
                setupTexture(0);
            }
            m_treeModel.draw();
            glPopMatrix();
        }
    }
}

void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
    const GameObjects& objects = g_game.getGameObjects();

    for (const auto& cloud : cloudSprites) {
        float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceToCenter < 8.0f) continue;

        glPushMatrix();
        glTranslatef(cloud.position.x, cloud.position.y, cloud.position.z);
        // ✅ Размер уже должен быть из конфига в Sprite::size
        glScalef(cloud.size, cloud.size, cloud.size);

        if (m_cloudModel.hasTexture && m_cloudModel.textureID != 0) {
            setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 40.0f, 0.3f);
            setupTexture(m_cloudModel.textureID);
        }
        else {
            setMaterial(objects.getCloudColor(), 40.0f, 0.3f);
            setupTexture(0);
        }

        m_cloudModel.draw();
        glPopMatrix();
    }
}

void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
    const GameObjects& objects = g_game.getGameObjects();

    for (const auto& bird : birds) {
        glPushMatrix();
        glTranslatef(bird.position.x, bird.position.y, bird.position.z);

        if (glm::length(bird.direction) > 0.1f) {
            float angle = atan2f(bird.direction.x, bird.direction.z) * 180.0f / 3.14159f;
            glRotatef(angle, 0.0f, 1.0f, 0.0f);
            float pitch = atan2f(bird.direction.y,
                glm::length(glm::vec2(bird.direction.x, bird.direction.z))) * 180.0f / 3.14159f;
            glRotatef(pitch, 1.0f, 0.0f, 0.0f);
        }

        // ✅ Размер уже должен быть из конфига в Bird::size
        glScalef(bird.size, bird.size, bird.size);

        if (m_birdModel.hasTexture && m_birdModel.textureID != 0) {
            setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 40.0f, 0.2f);
            setupTexture(m_birdModel.textureID);
        }
        else {
            setMaterial(objects.getBirdColor(), 40.0f, 0.2f);
            setupTexture(0);
        }

        m_birdModel.draw();
        glPopMatrix();
    }
}

void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    const GameObjects& objects = g_game.getGameObjects();

    for (const auto& flower : flowerSprites) {
        glPushMatrix();
        glTranslatef(flower.position.x, flower.position.y + 0.05f, flower.position.z);
        // ✅ Размер уже должен быть из конфига в Sprite::size
        glScalef(flower.size, flower.size, flower.size);

        if (m_flowerModel.hasTexture && m_flowerModel.textureID != 0) {
            setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 50.0f, 0.2f);
            setupTexture(m_flowerModel.textureID);
        }
        else {
            setMaterial(objects.getFlowerColor(), 50.0f, 0.2f);
            setupTexture(0);
        }

        m_flowerModel.draw();
        glPopMatrix();
    }
}

void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    if (fenceBlocks.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;
    const GameObjects& objects = g_game.getGameObjects();

    // Получаем масштаб забора из конфига
    float fenceScaleFromConfig = objects.getFenceScale();

    // Определяем границы мира
    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;
    float minX = -worldWidth / 2.0f;
    float maxX = worldWidth / 2.0f;
    float minZ = -worldDepth / 2.0f;
    float maxZ = worldDepth / 2.0f;

    // Толщина границы для определения краевых блоков (в клетках)
    int borderThickness = 1;

    // Вычисляем нижнюю точку модели забора
    float fenceBottomOffset = 0.0f;
    if (!m_fenceModel.vertices.empty()) {
        float minY_local = FLT_MAX;
        float maxY_local = -FLT_MAX;
        for (const auto& vert : m_fenceModel.vertices) {
            minY_local = std::min(minY_local, vert.position.y);
            maxY_local = std::max(maxY_local, vert.position.y);
        }
        fenceBottomOffset = -minY_local; // Смещение от центра модели до нижней точки
    }

    for (const auto& fenceBlock : fenceBlocks) {
        float x = fenceBlock.x * m_cellSize - offsetX;
        float z = fenceBlock.z * m_cellSize - offsetZ;

        // Масштаб забора
        float scaleX = m_cellSize * fenceScaleFromConfig;
        float scaleY = m_cellSize * fenceScaleFromConfig * 0.5f;
        float scaleZ = m_cellSize * fenceScaleFromConfig;

        // Вычисляем Y так, чтобы нижняя часть модели была на уровне пола (m_floorHeight = 0)
        float y = m_floorHeight - fenceBottomOffset * scaleY;

        // Определяем, на какой стороне находится блок
        bool isTopSide = (fenceBlock.z >= m_gridDepth - borderThickness);
        bool isBottomSide = (fenceBlock.z <= borderThickness - 1);
        bool isLeftSide = (fenceBlock.x <= borderThickness - 1);
        bool isRightSide = (fenceBlock.x >= m_gridWidth - borderThickness);

        // Является ли блок угловым (тогда не поворачиваем)
        bool isCorner = (isTopSide && isLeftSide) || (isTopSide && isRightSide) ||
            (isBottomSide && isLeftSide) || (isBottomSide && isRightSide);

        bool isSideBlock = false;
        float rotationAngle = 0.0f;

        // Сохраняем исходные координаты для модификации
        float finalX = x;
        float finalZ = z;

        // Определяем, нужно ли повернуть блок и скорректировать позицию
        if (isTopSide && !isCorner) {
            // Верхняя сторона - не поворачиваем
            rotationAngle = 0.0f;
        }
        else if (isBottomSide && !isCorner) {
            // Нижняя сторона - не поворачиваем
            rotationAngle = 0.0f;
        }
        else if (isLeftSide && !isCorner) {
            // Левая сторона - поворачиваем на 90 градусов и смещаем наружу
            rotationAngle = 90.0f;
            isSideBlock = true;
            // Смещаем левый забор влево, чтобы не заходить на поле
            finalZ = z + m_cellSize * 0.5f;
        }
        else if (isRightSide && !isCorner) {
            // Правая сторона - поворачиваем на -90 градусов и смещаем наружу
            rotationAngle = -90.0f;
            isSideBlock = true;
            // Смещаем правый забор вправо, чтобы не заходить на поле
            finalZ = z + m_cellSize * 0.5f;
        }

        glPushMatrix();
        glTranslatef(finalX, y, finalZ);

        if (isSideBlock) {
            glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
        }

        glScalef(scaleX, scaleY, scaleZ);

        if (m_fenceModel.hasTexture && m_fenceModel.textureID != 0) {
            setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 24.0f, 0.15f);
            setupTexture(m_fenceModel.textureID);
        }
        else {
            setMaterial(objects.getFenceColor(), 24.0f, 0.15f);
            setupTexture(0);
        }

        m_fenceModel.draw();
        glPopMatrix();
    }
}

void GameRenderer::drawLightSource() {
    glPushMatrix();
    glDisable(GL_LIGHTING);

    switch (m_lightType) {
    case LightType::Directional: {
        glColor3f(1.0f, 0.9f, 0.6f);
        glm::vec3 lightPos = -m_lightDir * 10.0f;
        glTranslatef(lightPos.x, lightPos.y, lightPos.z);
        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.4f, 16, 16);
        gluDeleteQuadric(quad);
        break;
    }
    case LightType::Points: {
        glTranslatef(m_lightPos.x, m_lightPos.y, m_lightPos.z);
        GLUquadric* quad = gluNewQuadric();
        glColor3f(1.0f, 0.8f, 0.4f);
        gluSphere(quad, 0.3f, 16, 16);
        gluDeleteQuadric(quad);
        break;
    }
    case LightType::Spot: {
        // Прожектор: рисуем конус, указывающий НАПРАВЛЕНИЕ СВЕТА
        // Если светит вниз - конус рисуем вниз
        glTranslatef(m_lightPos.x, m_lightPos.y, m_lightPos.z);

        GLUquadric* quad = gluNewQuadric();
        glColor3f(1.0f, 0.7f, 0.2f);

        // Сфера на вершине
        gluSphere(quad, 0.2f, 12, 12);
        gluDeleteQuadric(quad);
        break;
    }
    }

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void GameRenderer::drawModel(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color) {

    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    setMaterial(color, 32.0f, 0.3f);
    setupTexture(model.hasTexture ? model.textureID : 0);
    model.draw();

    glPopMatrix();
}

void GameRenderer::drawModelWithRotation(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color, float rotationAngle) {

    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    setMaterial(color, 32.0f, 0.3f);
    setupTexture(model.hasTexture ? model.textureID : 0);
    model.draw();

    glPopMatrix();
}

//=============================================================================
// ТЕНИ
//=============================================================================


void GameRenderer::toggleDebugRays() {
    m_debugRaysEnabled = !m_debugRaysEnabled;
    std::cout << "Debug rays: " << (m_debugRaysEnabled ? "ENABLED" : "DISABLED") << std::endl;

    if (m_debugRaysEnabled && m_shadowMapEnabled) {
        m_staticShadow.enableDebugRays(true);
        m_dynamicShadow.enableDebugRays(true);
        m_foodShadow.enableDebugRays(true);
        
        // НЕ СБРАСЫВАЕМ m_showAllRays, сохраняем текущую настройку
        std::cout << "Show all rays: " << (m_showAllRays ? "YES" : "NO (only hit rays)") << std::endl;
        
        // Пересчитываем тени для записи всех лучей
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
        m_foodShadowsDirty = true;
    }
    else if (!m_debugRaysEnabled) {
        m_staticShadow.enableDebugRays(false);
        m_dynamicShadow.enableDebugRays(false);
        m_foodShadow.enableDebugRays(false);
    }
}

// GameRenderer.cpp - полная функция с установкой режима

// В GameRenderer::resetShadows()
void GameRenderer::resetShadows() {
    m_staticShadow = ShadowMapper(
        m_gridWidth, m_gridDepth, m_cellSize, m_floorHeight,
        m_lightDir, m_lightColor, m_lightType, m_lightPos,
        m_shadowStrideX, m_shadowStrideZ
    );

    m_dynamicShadow = ShadowMapper(
        m_gridWidth, m_gridDepth, m_cellSize, m_floorHeight,
        m_lightDir, m_lightColor, m_lightType, m_lightPos,
        m_shadowStrideX, m_shadowStrideZ
    );

    m_foodShadow = ShadowMapper(
        m_gridWidth, m_gridDepth, m_cellSize, m_floorHeight,
        m_lightDir, m_lightColor, m_lightType, m_lightPos,
        m_shadowStrideX, m_shadowStrideZ
    );

    // Устанавливаем режим
    m_staticShadow.setShadowTraceMode(m_currentShadowMode);
    m_dynamicShadow.setShadowTraceMode(m_currentShadowMode);
    m_foodShadow.setShadowTraceMode(m_currentShadowMode);

    // ВАЖНО: ОТКЛЮЧАЕМ СФЕРЫ!
    m_staticShadow.setUseSpheres(m_useSpheres);
    m_dynamicShadow.setUseSpheres(m_useSpheres);
    m_foodShadow.setUseSpheres(m_useSpheres);

    // ОЧИЩАЕМ СФЕРЫ!
    m_staticShadow.clearObjectBounds();
    m_dynamicShadow.clearObjectBounds();
    m_foodShadow.clearObjectBounds();

    // Настраиваем свет
    m_staticShadow.setLightType(m_lightType);
    m_staticShadow.setLightPos(m_lightPos);
    m_staticShadow.setLightDirection(m_lightDir);

    m_dynamicShadow.setLightType(m_lightType);
    m_dynamicShadow.setLightPos(m_lightPos);
    m_dynamicShadow.setLightDirection(m_lightDir);

    m_foodShadow.setLightType(m_lightType);
    m_foodShadow.setLightPos(m_lightPos);
    m_foodShadow.setLightDirection(m_lightDir);

    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;
    m_foodShadowsDirty = true;

    std::cout << "Shadows reset with mode: " << m_staticShadow.getCurrentModeName()
        << " (spheres DISABLED, using exact geometry)" << std::endl;
}


void GameRenderer::renderShadowMap() {
    // Упрощённая версия теней для fixed pipeline
}

//=============================================================================
// ОТЛАДКА
//=============================================================================

void GameRenderer::drawSphereImmediate(const glm::vec3& center, float radius) {
    GLUquadric* quad = gluNewQuadric();

    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    gluSphere(quad, radius, 12, 12);
    glPopMatrix();

    gluDeleteQuadric(quad);
}

void GameRenderer::drawDebugRaysIfEnabled() {
    if (!m_debugRaysEnabled || !m_shadowMapEnabled) return;

    const auto& rays = m_staticShadow.getDebugRays();

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.5f);

    for (const auto& ray : rays) {
        // Фильтр по типу лучей
        if (!m_showAllRays && !ray.hit) continue;

        // Выбираем цвет в зависимости от типа луча
        switch (ray.rayType) {
        case DebugRay::RAY_CENTER:
            glColor3f(1.0f, 1.0f, 0.0f);  // Жёлтый - центральные лучи
            break;
        case DebugRay::RAY_CORNER:
            glColor3f(1.0f, 0.5f, 0.0f);  // Оранжевый - угловые лучи
            break;
        case DebugRay::RAY_SUB_CENTER:
            glColor3f(0.0f, 1.0f, 0.0f);  // Зелёный - подклетки центр
            break;
        case DebugRay::RAY_SUB_CORNER:
            glColor3f(0.0f, 0.5f, 1.0f);  // Голубой - подклетки углы
            break;
        }

        // Если луч попал в объект - красный цвет поверх
        if (ray.hit) {
            glColor3f(1.0f, 0.0f, 0.0f);
        }

        // Рисуем луч (от источника к точке)
        glBegin(GL_LINES);
        glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
        glVertex3f(ray.hitPoint.x, ray.hitPoint.y, ray.hitPoint.z);
        glEnd();
    }

    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
}

void GameRenderer::drawDebugRays(const std::vector<DebugRay>& rays, float lineWidth) {
    // Реализация для отладки
}

void GameRenderer::drawRay(const DebugRay& ray, const glm::vec3& color) {
    // Реализация для отладки
}

// В GameRenderer.cpp - исправленный intersectScene

HitInfo GameRenderer::intersectScene(const Ray& ray, const GameObjects& objects,
    float offsetX, float offsetZ, bool treesOnly) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;
    float maxDistance = 100.0f;
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    // Получаем масштабы из конфига
    float treeScaleFromConfig = objects.getTreeScale();
    float appleScaleFromConfig = objects.getAppleScale();

    // Пол (всегда проверяем)
    float tGround = -ray.origin.y / ray.direction.y;
    if (tGround > 0.01f && tGround < maxDistance && tGround < closestHit.distance) {
        glm::vec3 hitPoint = ray.pointAt(tGround);
        if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
            closestHit.hit = true;
            closestHit.distance = tGround;
            closestHit.point = hitPoint;
            closestHit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // ===== ДЕРЕВЬЯ =====
    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float z = block.z * m_cellSize - offsetZ;
            float y = block.y * m_cellSize;

            // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ДЕРЕВА ИЗ КОНФИГА
            float scale = m_cellSize * treeScaleFromConfig;
            float treeHeight = scale * 1.5f;

            glm::vec3 treeCenter(
                x + m_cellSize * 0.5f,
                y + treeHeight * 0.5f,
                z + m_cellSize * 0.5f
            );
            float boundingRadius = scale * 0.8f;

            float tSphere;
            if (rayIntersectsSphere(ray, treeCenter, boundingRadius, tSphere)) {
                if (!m_treeModel.vertices.empty()) {
                    glm::mat4 transform = glm::mat4(1.0f);
                    transform = glm::translate(transform, glm::vec3(x + m_cellSize * 0.5f, y, z + m_cellSize * 0.5f));
                    transform = glm::scale(transform, glm::vec3(scale));

                    float hitDist;
                    glm::vec3 hitPt;
                    if (rayIntersectsModel(ray, m_treeModel, transform, hitDist, hitPt)) {
                        if (hitDist > 0.01f && hitDist < closestHit.distance) {
                            closestHit.hit = true;
                            closestHit.distance = hitDist;
                            closestHit.point = hitPt;
                            closestHit.normal = glm::normalize(hitPt - treeCenter);
                        }
                    }
                }
            }
        }
    }

    if (treesOnly) {
        return closestHit;
    }

    // ===== ЗМЕЙКА =====
    for (size_t i = 0; i < objects.getSnake().size(); i++) {
        const Point& segment = objects.getSnake()[i];
        float x = segment.x * m_cellSize - offsetX;
        float z = segment.z * m_cellSize - offsetZ;
        float y = segment.y * m_cellSize + 0.1f;

        float scale;
        const Model* model = nullptr;
        if (i == 0) {
            scale = m_cellSize * objects.getSnakeHeadScale();
            model = &m_snakeHeadModel;
        }
        else if (i == objects.getSnake().size() - 1) {
            scale = m_cellSize * objects.getSnakeTailScale();
            model = &m_snakeTailModel;
        }
        else {
            scale = m_cellSize * objects.getSnakeBodyScale();
            model = &m_snakeBodyModel;
        }

        float segmentRadius = scale * 0.6f;
        glm::vec3 center(x, y, z);

        float tSphere;
        if (rayIntersectsSphere(ray, center, segmentRadius, tSphere)) {
            if (!model->vertices.empty()) {
                float rotationAngle = calculateSegmentRotation(objects.getSnake(), i);
                glm::mat4 transform = glm::mat4(1.0f);
                transform = glm::translate(transform, glm::vec3(x, y, z));
                transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                transform = glm::scale(transform, glm::vec3(scale));

                float hitDist;
                glm::vec3 hitPt;
                if (rayIntersectsModel(ray, *model, transform, hitDist, hitPt)) {
                    if (hitDist > 0.01f && hitDist < closestHit.distance) {
                        closestHit.hit = true;
                        closestHit.distance = hitDist;
                        closestHit.point = hitPt;
                        closestHit.normal = glm::normalize(hitPt - center);
                    }
                }
            }
        }
    }

    // ===== ЯБЛОКИ =====
    if (m_appleModel.vertices.empty()) return closestHit;

    static float minY_local = FLT_MAX;
    static float maxY_local = -FLT_MAX;
    static float offsetFromCenterToBottom = 0.0f;
    static bool boundsComputed = false;

    if (!boundsComputed) {
        for (const auto& vert : m_appleModel.vertices) {
            minY_local = std::min(minY_local, vert.position.y);
            maxY_local = std::max(maxY_local, vert.position.y);
        }
        offsetFromCenterToBottom = -minY_local;
        boundsComputed = true;
    }

    for (const auto& apple : objects.getFood()) {
        float x = apple.x * m_cellSize - offsetX;
        float z = apple.z * m_cellSize - offsetZ;

        // ✅ ИСПОЛЬЗУЕМ МАСШТАБ ЯБЛОКА ИЗ КОНФИГА
        float scale = m_cellSize * appleScaleFromConfig;
        float worldBottomOffset = offsetFromCenterToBottom * scale;

        float centerX = x + m_cellSize * 0.5f;
        float centerY = m_floorHeight + worldBottomOffset;
        float centerZ = z + m_cellSize * 0.5f;

        float modelRadius = (maxY_local - minY_local) * 0.5f * scale;
        glm::vec3 center(centerX, centerY, centerZ);

        float tSphere;
        if (rayIntersectsSphere(ray, center, modelRadius, tSphere)) {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, center);
            transform = glm::scale(transform, glm::vec3(scale));

            float hitDist;
            glm::vec3 hitPt;
            if (rayIntersectsModel(ray, m_appleModel, transform, hitDist, hitPt)) {
                if (hitDist > 0.01f && hitDist < closestHit.distance) {
                    closestHit.hit = true;
                    closestHit.distance = hitDist;
                    closestHit.point = hitPt;
                    closestHit.normal = glm::normalize(hitPt - center);
                }
            }
        }
    }

    return closestHit;
}
// ============================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ: intersectFoodOnly
// ============================================================================

HitInfo GameRenderer::intersectFoodOnly(const Ray& ray, const GameObjects& objects,
    float offsetX, float offsetZ) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;
    float maxDistance = 100.0f;

    // Получаем масштаб яблока из GameObjects
    float appleScaleFromConfig = objects.getAppleScale();

    // ===== Проверяем пол =====
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    float tGround = -ray.origin.y / ray.direction.y;
    if (tGround > 0.01f && tGround < maxDistance && tGround < closestHit.distance) {
        glm::vec3 hitPoint = ray.pointAt(tGround);
        if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
            closestHit.hit = true;
            closestHit.distance = tGround;
            closestHit.point = hitPoint;
            closestHit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Проверяем яблоки
    if (m_appleModel.vertices.empty()) return closestHit;

    // Вычисляем bounding box модели один раз
    static float minY_local = FLT_MAX;
    static float maxY_local = -FLT_MAX;
    static float offsetFromCenterToBottom = 0.0f;
    static bool boundsComputed = false;

    if (!boundsComputed) {
        for (const auto& vert : m_appleModel.vertices) {
            minY_local = std::min(minY_local, vert.position.y);
            maxY_local = std::max(maxY_local, vert.position.y);
        }
        offsetFromCenterToBottom = -minY_local;
        boundsComputed = true;
    }

    for (const auto& apple : objects.getFood()) {
        float x = apple.x * m_cellSize - offsetX;
        float z = apple.z * m_cellSize - offsetZ;

        float scale = m_cellSize * appleScaleFromConfig;
        float worldBottomOffset = offsetFromCenterToBottom * scale;

        float centerX = x + m_cellSize * 0.5f;
        float centerY = m_floorHeight + worldBottomOffset;
        float centerZ = z + m_cellSize * 0.5f;

        glm::vec3 center(centerX, centerY, centerZ);
        float modelRadius = (maxY_local - minY_local) * 0.5f * scale;

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, center);
        transform = glm::scale(transform, glm::vec3(scale));

        if (m_useSpheres) {
            // ===== РЕЖИМ СО СФЕРАМИ: сфера → точные полигоны =====
            float tSphere;
            if (rayIntersectsSphere(ray, center, modelRadius, tSphere)) {
                float hitDist;
                glm::vec3 hitPt;
                if (rayIntersectsModel(ray, m_appleModel, transform, hitDist, hitPt, true)) {
                    if (hitDist > 0.01f && hitDist < closestHit.distance) {
                        closestHit.hit = true;
                        closestHit.distance = hitDist;
                        closestHit.point = hitPt;
                        closestHit.normal = glm::normalize(hitPt - center);
                    }
                }
            }
        }
        else {
            // ===== РЕЖИМ БЕЗ СФЕР: brute-force все треугольники =====
            float hitDist;
            glm::vec3 hitPt;
            if (rayIntersectsModel(ray, m_appleModel, transform, hitDist, hitPt, true)) {
                if (hitDist > 0.01f && hitDist < closestHit.distance) {
                    closestHit.hit = true;
                    closestHit.distance = hitDist;
                    closestHit.point = hitPt;
                    closestHit.normal = glm::normalize(hitPt - center);
                }
            }
        }
    }

    return closestHit;
}

// ============================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ: intersectTreesOnly
// ============================================================================

HitInfo GameRenderer::intersectTreesOnly(const Ray& ray, const GameObjects& objects,
    float offsetX, float offsetZ) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;
    float maxDistance = 100.0f;
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    // Получаем масштаб дерева из конфига
    float treeScaleFromConfig = objects.getTreeScale();

    // Пол (всегда нужно для теней на земле)
    float tGround = -ray.origin.y / ray.direction.y;
    if (tGround > 0.01f && tGround < maxDistance) {
        glm::vec3 hitPoint = ray.pointAt(tGround);
        if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
            closestHit.hit = true;
            closestHit.distance = tGround;
            closestHit.point = hitPoint;
            closestHit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Только деревья
    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float z = block.z * m_cellSize - offsetZ;
            float y = block.y * m_cellSize;

            float scale = m_cellSize * treeScaleFromConfig;
            float treeHeight = scale * 1.5f;

            glm::vec3 treeCenter(
                x + m_cellSize * 0.5f,
                y + treeHeight * 0.5f,
                z + m_cellSize * 0.5f
            );
            float boundingRadius = scale * 0.8f;

            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(x + m_cellSize * 0.5f, y, z + m_cellSize * 0.5f));
            transform = glm::scale(transform, glm::vec3(scale));

            if (m_useSpheres) {
                // ===== РЕЖИМ СО СФЕРАМИ: сфера → точные полигоны =====
                float tSphere;
                if (rayIntersectsSphere(ray, treeCenter, boundingRadius, tSphere)) {
                    if (!m_treeModel.vertices.empty()) {
                        float hitDist;
                        glm::vec3 hitPt;
                        if (rayIntersectsModel(ray, m_treeModel, transform, hitDist, hitPt, true)) {
                            if (hitDist > 0.01f && hitDist < closestHit.distance) {
                                closestHit.hit = true;
                                closestHit.distance = hitDist;
                                closestHit.point = hitPt;
                                return closestHit;
                            }
                        }
                    }
                }
            }
            else {
                // ===== РЕЖИМ БЕЗ СФЕР: brute-force все треугольники =====
                if (!m_treeModel.vertices.empty()) {
                    float hitDist;
                    glm::vec3 hitPt;
                    if (rayIntersectsModel(ray, m_treeModel, transform, hitDist, hitPt, true)) {
                        if (hitDist > 0.01f && hitDist < closestHit.distance) {
                            closestHit.hit = true;
                            closestHit.distance = hitDist;
                            closestHit.point = hitPt;
                            return closestHit;
                        }
                    }
                }
            }
        }
    }

    return closestHit;
}

// ============================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ: intersectSnakeOnly
// ============================================================================

HitInfo GameRenderer::intersectSnakeOnly(const Ray& ray, const GameObjects& objects,
    float offsetX, float offsetZ) {

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;

    // Только змейка - масштабы уже из конфига
    for (size_t i = 0; i < objects.getSnake().size(); i++) {
        const Point& segment = objects.getSnake()[i];
        float x = segment.x * m_cellSize - offsetX;
        float z = segment.z * m_cellSize - offsetZ;
        float y = segment.y * m_cellSize + 0.1f;

        float scale;
        const Model* model = nullptr;
        if (i == 0) {
            scale = m_cellSize * objects.getSnakeHeadScale();
            model = &m_snakeHeadModel;
        }
        else if (i == objects.getSnake().size() - 1) {
            scale = m_cellSize * objects.getSnakeTailScale();
            model = &m_snakeTailModel;
        }
        else {
            scale = m_cellSize * objects.getSnakeBodyScale();
            model = &m_snakeBodyModel;
        }

        float segmentRadius = scale * 0.6f;
        glm::vec3 center(x, y, z);
        float rotationAngle = calculateSegmentRotation(objects.getSnake(), i);

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(x, y, z));
        transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(scale));

        if (m_useSpheres) {
            // ===== РЕЖИМ СО СФЕРАМИ: сфера → точные полигоны =====
            float tSphere;
            if (rayIntersectsSphere(ray, center, segmentRadius, tSphere)) {
                if (!model->vertices.empty()) {
                    float hitDist;
                    glm::vec3 hitPt;
                    if (rayIntersectsModel(ray, *model, transform, hitDist, hitPt, true)) {
                        if (hitDist > 0.01f && hitDist < closestHit.distance) {
                            closestHit.hit = true;
                            closestHit.distance = hitDist;
                            closestHit.point = hitPt;
                        }
                    }
                }
            }
        }
        else {
            // ===== РЕЖИМ БЕЗ СФЕР: brute-force все треугольники =====
            if (!model->vertices.empty()) {
                float hitDist;
                glm::vec3 hitPt;
                if (rayIntersectsModel(ray, *model, transform, hitDist, hitPt, true)) {
                    if (hitDist > 0.01f && hitDist < closestHit.distance) {
                        closestHit.hit = true;
                        closestHit.distance = hitDist;
                        closestHit.point = hitPt;
                    }
                }
            }
        }
    }

    return closestHit;
}
void GameRenderer::checkGLError(const char* functionName) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL error " << error << " in " << functionName << std::endl;
    }
}

//=============================================================================
// ТРАССИРОВКА ЛУЧЕЙ
//=============================================================================

void GameRenderer::initRayTracingResources() {
    std::cout << "Ray tracing resources initialized (simplified for fixed pipeline)" << std::endl;
}

void GameRenderer::cleanupRayTracingResources() {
    // Очистка ресурсов
}

void GameRenderer::renderWithRayTracing(const GameObjects& objects) {
    std::cout << "Ray tracing not fully implemented in fixed pipeline mode" << std::endl;
}

glm::vec3 GameRenderer::traceRay(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ, int depth) {
    return skyColor;
}


bool GameRenderer::rayIntersectsSphere(const Ray& ray, const glm::vec3& center, float radius, float& tHit) {
    glm::vec3 oc = ray.origin - center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float sqrtD = sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    if (t1 > 0) {
        tHit = t1;
        return true;
    }
    if (t2 > 0) {
        tHit = t2;
        return true;
    }

    return false;
}

glm::vec3 GameRenderer::computeNormal(const glm::vec3& point, const glm::vec3& min, const glm::vec3& max) {
    glm::vec3 normal(0.0f);
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 localPoint = point - center;

    float dx = abs(localPoint.x);
    float dy = abs(localPoint.y);
    float dz = abs(localPoint.z);

    if (dx > dy && dx > dz) {
        normal = glm::vec3(localPoint.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
    }
    else if (dy > dx && dy > dz) {
        normal = glm::vec3(0.0f, localPoint.y > 0 ? 1.0f : -1.0f, 0.0f);
    }
    else {
        normal = glm::vec3(0.0f, 0.0f, localPoint.z > 0 ? 1.0f : -1.0f);
    }

    return normal;
}

//=============================================================================
// ПОЗИЦИОНИРОВАНИЕ ОБЪЕКТОВ
//=============================================================================

glm::vec3 GameRenderer::getSnakeSegmentPosition(const Point& segment, size_t index) {
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    float x = segment.x * m_cellSize - offsetX;
    float z = segment.z * m_cellSize - offsetZ;
    float y = segment.y * m_cellSize + 0.1f;

    return glm::vec3(x, y, z);
}

glm::vec3 GameRenderer::getSnakeSegmentScale(const Point& segment, size_t index) {
    const GameObjects& objects = g_game.getGameObjects();
    float baseScale = m_cellSize * 0.8f;
    float scale = baseScale;

    if (index == 0) scale *= objects.getSnakeHeadScale();
    else if (index == g_game.getSnake().size() - 1) scale *= objects.getSnakeTailScale();
    else scale *= objects.getSnakeBodyScale();

    return glm::vec3(scale);
}

glm::vec3 GameRenderer::getFoodPosition(const Point& food) {
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    float x = food.x * m_cellSize - offsetX;
    float z = food.z * m_cellSize - offsetZ;
    float y = food.y * m_cellSize + 0.1f;

    return glm::vec3(x, y, z);
}

glm::vec3 GameRenderer::getObstaclePosition(const Point& block) {
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    float x = block.x * m_cellSize - offsetX;
    float z = block.z * m_cellSize - offsetZ;
    float y = block.y * m_cellSize;

    return glm::vec3(x, y, z);
}

glm::vec3 GameRenderer::getBirdPosition(const Bird& bird) {
    return bird.position;
}

glm::vec3 GameRenderer::getCloudPosition(const Sprite& cloud) {
    return cloud.position;
}

glm::vec3 GameRenderer::getFlowerPosition(const Sprite& flower) {
    glm::vec3 pos = flower.position;
    pos.y += 0.05f;
    return pos;
}

//=============================================================================
// СТАТИЧЕСКИЕ МЕТОДЫ ДЛЯ GLFW
//=============================================================================

void GameRenderer::setupGLFWHints() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
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
    glGetError();
    return true;
}

void GameRenderer::printGraphicsInfo() {
    std::cout << "=== GRAPHICS SYSTEM INFO ===" << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLU version: " << gluGetString(GLU_VERSION) << std::endl;
    std::cout << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "===========================" << std::endl;
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

//=============================================================================
// КОЛБЭКИ GLFW
//=============================================================================

namespace {
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            g_game.handleKeyPress(key);

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
            if (key == GLFW_KEY_T && action == GLFW_PRESS) {
                g_game.toggleRayTracing();
                std::cout << "Ray tracing toggled" << std::endl;
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
//=============================================================================
// ПРИМИТИВЫ (FALLBACK ДЛЯ СЛУЧАЯ, КОГДА FBX МОДЕЛИ НЕ ЗАГРУЗИЛИСЬ)
//=============================================================================

void GameRenderer::createPrimitives() {
    std::cout << "Creating primitive models (fallback)..." << std::endl;

    // Создаём примитивы на случай, если FBX модели не загрузятся
    ApplePrimitive::create(m_appleModel);
    TreePrimitive::create(m_treeModel);
    CloudPrimitive::create(m_cloudModel);
    BirdPrimitive::create(m_birdModel);
    FlowerPrimitive::create(m_flowerModel);
    FencePrimitive::create(m_fenceModel);

    SnakeHeadPrimitive::create(m_snakeHeadModel);
    SnakeBodyPrimitive::create(m_snakeBodyModel);
    SnakeTailPrimitive::create(m_snakeTailModel);

    std::cout << "All primitive models created successfully" << std::endl;
}
void GameRenderer::setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}
//=============================================================================
// ОТРИСОВКА ТАЙЛОВОГО ПОЛА ИЗ FBX МОДЕЛЕЙ (С ПРИНУДИТЕЛЬНЫМ ПОДЪЁМОМ)
//=============================================================================
void GameRenderer::drawTiledFloor(const GameObjects& objects) {
    // Проверяем, есть ли модель пола
    if (m_floorModel.vertices.empty()) {
        std::string floorModelPath = objects.getFloorModel();
        if (!floorModelPath.empty()) {
            if (!loadFBXModelToModel(floorModelPath, m_floorModel, "floor")) {
                std::cout << "Failed to load floor model, using fallback" << std::endl;
                drawFallbackFloor();
                return;
            }
        }
        else {
            drawFallbackFloor();
            return;
        }
    }

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(5.0f, 10.0f);

    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;

    // Вычисляем реальные габариты модели
    float modelMinX = 999999.0f, modelMaxX = -999999.0f;
    float modelMinY = 999999.0f, modelMaxY = -999999.0f;
    float modelMinZ = 999999.0f, modelMaxZ = -999999.0f;

    for (const auto& vertex : m_floorModel.vertices) {
        modelMinX = std::min(modelMinX, vertex.position.x);
        modelMaxX = std::max(modelMaxX, vertex.position.x);
        modelMinY = std::min(modelMinY, vertex.position.y);
        modelMaxY = std::max(modelMaxY, vertex.position.y);
        modelMinZ = std::min(modelMinZ, vertex.position.z);
        modelMaxZ = std::max(modelMaxZ, vertex.position.z);
    }

    float modelCenterX = (modelMinX + modelMaxX) / 2.0f;
    float modelCenterY = (modelMinY + modelMaxY) / 2.0f;
    float modelCenterZ = (modelMinZ + modelMaxZ) / 2.0f;

    float modelSizeX = modelMaxX - modelMinX;
    float modelSizeY = modelMaxY - modelMinY;
    float modelSizeZ = modelMaxZ - modelMinZ;

    int tilesX = 8;
    int tilesZ = 8;

    float tileWidth = worldWidth / tilesX;
    float tileDepth = worldDepth / tilesZ;

    float scaleX = tileWidth / modelSizeX;
    float scaleZ = tileDepth / modelSizeZ;
    float scale = std::min(scaleX, scaleZ);

    float startX = -worldWidth / 2.0f;
    float startZ = -worldDepth / 2.0f;

    float manualRaise = 0.5f;
    float baseY = manualRaise;
    m_floorHeight = baseY;

    bool hasTexture = (m_floorModel.hasTexture && m_floorModel.textureID != 0);
    glm::vec3 floorColorObj = objects.getFloorColor();

    glm::vec3 darkColor = floorColorObj * 0.15f;
    glm::vec3 lightColor = floorColorObj;

    ShadowMapper::ShadowTraceMode currentMode = m_staticShadow.getShadowTraceMode();
    bool isSubdividedMode = (currentMode == ShadowMapper::TRACE_CENTER_SUBDIVIDED ||
        currentMode == ShadowMapper::TRACE_CORNERS_SUBDIVIDED);
    bool useGradient = (currentMode == ShadowMapper::TRACE_CORNERS ||
        currentMode == ShadowMapper::TRACE_CORNERS_SUBDIVIDED);

    int subDivSize = SHADOW_SUBDIVISION_SIZE;

    if (gridEnabled) {
        drawFloorGrid();
    }

    for (int z = 0; z < m_gridDepth; z++) {
        for (int x = 0; x < m_gridWidth; x++) {

            float posX = startX + x * m_cellSize;
            float posZ = startZ + z * m_cellSize;

            float shadowValue = 1.0f;
            if (m_shadowMapEnabled) {
                if (isSubdividedMode) {
                    // Для subdivided режимов - более сложная логика
                    int shadowX = x / m_shadowStrideX;
                    int shadowZ = z / m_shadowStrideZ;

                    if (shadowX >= 0 && shadowX < m_staticShadow.getTotalCellsX() &&
                        shadowZ >= 0 && shadowZ < m_staticShadow.getTotalCellsZ()) {

                        const auto& shadowGrid = m_staticShadow.getShadowGrid();
                        const auto& sample = shadowGrid[shadowZ][shadowX];

                        if (sample.hasSubCells()) {
                            // Для каждого тайла нужно усреднить значения подклеток
                            float sum = 0.0f;
                            int count = 0;

                            int subCellPerTileX = subDivSize / tilesX;
                            int subCellPerTileZ = subDivSize / tilesZ;
                            if (subCellPerTileX < 1) subCellPerTileX = 1;
                            if (subCellPerTileZ < 1) subCellPerTileZ = 1;

                            int startSubX = (x % m_shadowStrideX) * subDivSize / m_shadowStrideX;
                            int startSubZ = (z % m_shadowStrideZ) * subDivSize / m_shadowStrideZ;
                            int endSubX = startSubX + subDivSize / m_shadowStrideX;
                            int endSubZ = startSubZ + subDivSize / m_shadowStrideZ;

                            for (int subZ = startSubZ; subZ < endSubZ && subZ < subDivSize; subZ++) {
                                for (int subX = startSubX; subX < endSubX && subX < subDivSize; subX++) {
                                    sum += sample.subCellValues[subZ][subX];
                                    count++;
                                }
                            }

                            if (count > 0) {
                                shadowValue = sum / count;
                            }
                        }
                        else {
                            shadowValue = sample.value;
                        }
                    }
                }
                else {
                    float snakeShadow = m_dynamicShadow.getShadowAtCell(x, z);
                    float foodShadow = m_foodShadow.getShadowAtCell(x, z);
                    float staticShadow = m_staticShadow.getShadowAtCell(x, z);
                    shadowValue = std::min({ snakeShadow, foodShadow, staticShadow });
                }
            }

            if (currentMode == ShadowMapper::TRACE_CENTER_SUBDIVIDED) {
                shadowValue = glm::smoothstep(0.3f, 0.7f, shadowValue);
            }

            glm::vec3 finalColor;
            if (useGradient) {
                finalColor = glm::mix(darkColor, lightColor, shadowValue);
            }
            else {
                shadowValue = (shadowValue >= 0.5f) ? 1.0f : 0.0f;
                finalColor = floorColorObj * shadowValue;
            }

            // Рисуем тайл
            glPushMatrix();
            glTranslatef(posX + tileWidth / 2, baseY, posZ + tileDepth / 2);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glTranslatef(-modelCenterX, -modelCenterY, -modelCenterZ);
            glScalef(scale, scale, scale);

            if (hasTexture) {
                setMaterial(glm::vec3(1.0f, 1.0f, 1.0f), 32.0f, 0.3f);
                setupTexture(m_floorModel.textureID);
            }
            else {
                setMaterial(finalColor, 32.0f, 0.3f);
                setupTexture(0);
            }

            m_floorModel.draw();
            glPopMatrix();
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}
void GameRenderer::drawFloorGrid() {
    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    glDisable(GL_LIGHTING);
    glLineWidth(gridLineWidth);
    glColor3f(gridColor.r, gridColor.g, gridColor.b);

    glBegin(GL_LINES);
    // Вертикальные линии сетки
    for (int i = 0; i <= m_gridWidth; i++) {
        float x = i * m_cellSize - offsetX;
        glVertex3f(x, 0.01f, -offsetZ);
        glVertex3f(x, 0.01f, worldDepth - offsetZ);
    }
    // Горизонтальные линии сетки
    for (int i = 0; i <= m_gridDepth; i++) {
        float z = i * m_cellSize - offsetZ;
        glVertex3f(-offsetX, 0.01f, z);
        glVertex3f(worldWidth - offsetX, 0.01f, z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void GameRenderer::drawFallbackFloor() {
    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;
    m_floorHeight = 0.0f;

    if (gridEnabled) {
        drawFloorGrid();
    }

    if (useFloorTexture && floorTexture.id != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTexture.id);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    float texRepeat = worldWidth / 2.0f;

    glm::vec3 darkColor = floorColor * 0.15f;
    glm::vec3 lightColor = floorColor;

    ShadowMapper::ShadowTraceMode currentMode = m_staticShadow.getShadowTraceMode();
    bool isSubdividedMode = (currentMode == ShadowMapper::TRACE_CENTER_SUBDIVIDED ||
        currentMode == ShadowMapper::TRACE_CORNERS_SUBDIVIDED);
    bool useGradient = (currentMode == ShadowMapper::TRACE_CORNERS ||
        currentMode == ShadowMapper::TRACE_CORNERS_SUBDIVIDED);
    int subDivSize = SHADOW_SUBDIVISION_SIZE;

    for (int z = 0; z < m_gridDepth; z++) {
        for (int x = 0; x < m_gridWidth; x++) {

            float posX = x * m_cellSize - offsetX;
            float posZ = z * m_cellSize - offsetZ;

            if (isSubdividedMode && m_shadowMapEnabled) {
                float subCellSizeX = m_cellSize / subDivSize;
                float subCellSizeZ = m_cellSize / subDivSize;

                for (int subZ = 0; subZ < subDivSize; subZ++) {
                    for (int subX = 0; subX < subDivSize; subX++) {

                        float subPosX = posX + subX * subCellSizeX;
                        float subPosZ = posZ + subZ * subCellSizeZ;
                        float subCenterX = subPosX + subCellSizeX / 2.0f;
                        float subCenterZ = subPosZ + subCellSizeZ / 2.0f;
                        glm::vec3 subCenter(subCenterX, 0.05f, subCenterZ);

                        // Получаем тени с подразбиением для каждой точки
                        float staticShadow = m_staticShadow.getShadowAtPointWithSubdivision(subCenter, nullptr);
                        float snakeShadow = m_dynamicShadow.getShadowAtPointWithSubdivision(subCenter, nullptr);
                        float foodShadow = m_foodShadow.getShadowAtPointWithSubdivision(subCenter, nullptr);

                        float shadowValue = std::min({ staticShadow, snakeShadow, foodShadow });

                        glm::vec3 finalColor;
                        if (useGradient) {
                            finalColor = glm::mix(darkColor, lightColor, shadowValue);
                        }
                        else {
                            finalColor = (shadowValue >= 0.5f) ? lightColor : darkColor;
                        }

                        glColor3f(finalColor.r, finalColor.g, finalColor.b);

                        glBegin(GL_QUADS);
                        glNormal3f(0.0f, 1.0f, 0.0f);

                        if (useFloorTexture && floorTexture.id != 0) {
                            float u = (float)(x * subDivSize + subX) / (texRepeat * subDivSize);
                            float v = (float)(z * subDivSize + subZ) / (texRepeat * subDivSize);
                            float u2 = (float)(x * subDivSize + subX + 1) / (texRepeat * subDivSize);
                            float v2 = (float)(z * subDivSize + subZ + 1) / (texRepeat * subDivSize);
                            glTexCoord2f(u, v); glVertex3f(subPosX, -0.02f, subPosZ);
                            glTexCoord2f(u2, v); glVertex3f(subPosX + subCellSizeX, -0.02f, subPosZ);
                            glTexCoord2f(u2, v2); glVertex3f(subPosX + subCellSizeX, -0.02f, subPosZ + subCellSizeZ);
                            glTexCoord2f(u, v2); glVertex3f(subPosX, -0.02f, subPosZ + subCellSizeZ);
                        }
                        else {
                            glVertex3f(subPosX, -0.02f, subPosZ);
                            glVertex3f(subPosX + subCellSizeX, -0.02f, subPosZ);
                            glVertex3f(subPosX + subCellSizeX, -0.02f, subPosZ + subCellSizeZ);
                            glVertex3f(subPosX, -0.02f, subPosZ + subCellSizeZ);
                        }
                        glEnd();
                    }
                }
            }
            else {
                // Не-subdivided режимы (оставляем как было)
                float shadowValue = 1.0f;
                if (m_shadowMapEnabled) {
                    float snakeShadow = m_dynamicShadow.getShadowAtCell(x, z);
                    float foodShadow = m_foodShadow.getShadowAtCell(x, z);
                    float staticShadow = m_staticShadow.getShadowAtCell(x, z);
                    shadowValue = std::min({ snakeShadow, foodShadow, staticShadow });
                }

                glm::vec3 finalColor;
                if (useGradient) {
                    finalColor = glm::mix(darkColor, lightColor, shadowValue);
                }
                else {
                    shadowValue = (shadowValue >= 0.5f) ? 1.0f : 0.0f;
                    finalColor = floorColor * shadowValue;
                }

                glColor3f(finalColor.r, finalColor.g, finalColor.b);

                glBegin(GL_QUADS);
                glNormal3f(0.0f, 1.0f, 0.0f);
                if (useFloorTexture && floorTexture.id != 0) {
                    float u1 = (float)x / texRepeat;
                    float v1 = (float)z / texRepeat;
                    float u2 = (float)(x + 1) / texRepeat;
                    float v2 = (float)(z + 1) / texRepeat;
                    glTexCoord2f(u1, v1); glVertex3f(posX, -0.02f, posZ);
                    glTexCoord2f(u2, v1); glVertex3f(posX + m_cellSize, -0.02f, posZ);
                    glTexCoord2f(u2, v2); glVertex3f(posX + m_cellSize, -0.02f, posZ + m_cellSize);
                    glTexCoord2f(u1, v2); glVertex3f(posX, -0.02f, posZ + m_cellSize);
                }
                else {
                    glVertex3f(posX, -0.02f, posZ);
                    glVertex3f(posX + m_cellSize, -0.02f, posZ);
                    glVertex3f(posX + m_cellSize, -0.02f, posZ + m_cellSize);
                    glVertex3f(posX, -0.02f, posZ + m_cellSize);
                }
                glEnd();
            }
        }
    }

    glDisable(GL_TEXTURE_2D);
}
// ============================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ: intersectTreeById (без сфер - сразу полигоны)
// ============================================================================

bool GameRenderer::intersectTreeById(const Ray& ray, int treeId,
    float offsetX, float offsetZ, float& hitDist, glm::vec3& hitPoint)
{
    const GameObjects& objects = g_game.getGameObjects();
    const auto& obstacles = objects.getObstacles();

    int currentId = 0;
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            if (currentId == treeId) {
                float x = block.x * m_cellSize - offsetX;
                float z = block.z * m_cellSize - offsetZ;
                float y = block.y * m_cellSize;

                float treeScale = objects.getTreeScale();
                float scale = m_cellSize * treeScale;

                glm::mat4 transform = glm::mat4(1.0f);
                transform = glm::translate(transform,
                    glm::vec3(x + m_cellSize * 0.5f, y, z + m_cellSize * 0.5f));
                transform = glm::scale(transform, glm::vec3(scale));

                // skipSphereCheck = true - НЕ ИСПОЛЬЗУЕМ СФЕРУ!
                return rayIntersectsModel(ray, m_treeModel, transform, hitDist, hitPoint, true);
            }
            currentId++;
        }
    }
    return false;
}

// ============================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ: intersectSnakeSegmentById (без сфер - сразу полигоны)
// ============================================================================

bool GameRenderer::intersectSnakeSegmentById(const Ray& ray, int segmentId,
    float offsetX, float offsetZ, float& hitDist, glm::vec3& hitPoint) {

    const GameObjects& objects = g_game.getGameObjects();
    const auto& snake = objects.getSnake();

    if (segmentId < 0 || segmentId >= (int)snake.size()) return false;

    const Point& segment = snake[segmentId];
    float x = segment.x * m_cellSize - offsetX;
    float z = segment.z * m_cellSize - offsetZ;
    float y = segment.y * m_cellSize + 0.1f;

    float scale;
    const Model* model;
    if (segmentId == 0) {
        scale = m_cellSize * objects.getSnakeHeadScale();
        model = &m_snakeHeadModel;
    }
    else if (segmentId == (int)snake.size() - 1) {
        scale = m_cellSize * objects.getSnakeTailScale();
        model = &m_snakeTailModel;
    }
    else {
        scale = m_cellSize * objects.getSnakeBodyScale();
        model = &m_snakeBodyModel;
    }

    float rotationAngle = calculateSegmentRotation(snake, segmentId);
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(x, y, z));
    transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(scale));

    // skipSphereCheck = true - НЕ ИСПОЛЬЗУЕМ СФЕРУ!
    return rayIntersectsModel(ray, *model, transform, hitDist, hitPoint, true);
}

// ============================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ: intersectAppleById (без сфер - сразу полигоны)
// ============================================================================

bool GameRenderer::intersectAppleById(const Ray& ray, int appleId,
    float offsetX, float offsetZ, float& hitDist, glm::vec3& hitPoint) {

    const GameObjects& objects = g_game.getGameObjects();
    const auto& food = objects.getFood();

    if (appleId < 0 || appleId >= (int)food.size()) return false;

    const Point& apple = food[appleId];
    float x = apple.x * m_cellSize - offsetX;
    float z = apple.z * m_cellSize - offsetZ;

    float appleScale = objects.getAppleScale();
    float scale = m_cellSize * appleScale;
    float centerX = x + m_cellSize * 0.5f;

    // Вычисляем высоту для яблока
    static float appleHeight = 0.0f;
    static bool heightComputed = false;
    if (!heightComputed && !m_appleModel.vertices.empty()) {
        float minY = FLT_MAX, maxY = -FLT_MAX;
        for (const auto& vert : m_appleModel.vertices) {
            minY = std::min(minY, vert.position.y);
            maxY = std::max(maxY, vert.position.y);
        }
        appleHeight = maxY - minY;
        heightComputed = true;
    }

    float centerY = m_floorHeight + (appleHeight * scale * 0.5f);
    float centerZ = z + m_cellSize * 0.5f;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(centerX, centerY, centerZ));
    transform = glm::scale(transform, glm::vec3(scale));

    // skipSphereCheck = true - НЕ ИСПОЛЬЗУЕМ СФЕРУ!
    return rayIntersectsModel(ray, m_appleModel, transform, hitDist, hitPoint, true);
}