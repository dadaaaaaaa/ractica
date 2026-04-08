#include "../pch.h"
#include "GameRenderer.h"
#include "../Graphics/ShaderManager.h"
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
#include "RayTracer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>

extern ShaderManager g_shaderManager;
extern Camera g_camera;
extern Game g_game;
extern std::string g_modelsPath;
extern std::string g_texturesPath;

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
    , shadow_map(false)
    , m_shadowStrideX(20)
    , m_shadowStrideZ(20)
{
    m_lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.5f));
    m_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_lightType = LightType::Directional;
    m_lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
}

void GameRenderer::resetShadows() {
    m_staticShadow = ShadowMapper(
        m_gridWidth,
        m_gridDepth,
        m_cellSize,
        0.0f,
        m_lightDir,
        m_lightColor,
        m_shadowStrideX,  // stride по X
        m_shadowStrideZ   // stride по Z
    );

    m_dynamicShadow = ShadowMapper(
        m_gridWidth,
        m_gridDepth,
        m_cellSize,
        0.0f,
        m_lightDir,
        m_lightColor,
        m_shadowStrideX,
        m_shadowStrideZ
    );

    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;

    std::cout << "Shadows reset with ratio: "
        << m_shadowStrideX << ":" << m_shadowStrideZ << std::endl;
}
// Добавить в GameRenderer.cpp после traceRay метода
HitInfo GameRenderer::intersectScene(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ) {
    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;

    float maxDistance = 50.0f;

    // Проверка пола
    float tGround = -ray.origin.y / ray.direction.y;
    if (tGround > 0 && tGround < maxDistance && tGround < closestHit.distance) {
        glm::vec3 hitPoint = ray.pointAt(tGround);
        float halfWidth = m_gridWidth * m_cellSize / 2.0f;
        float halfDepth = m_gridDepth * m_cellSize / 2.0f;

        if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
            closestHit.hit = true;
            closestHit.distance = tGround;
            closestHit.point = hitPoint;
            closestHit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Проверка деревьев
    float treeSize = m_cellSize * 1.5f;
    float treeHalf = treeSize / 2.0f;

    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            glm::vec3 pos = getObstaclePosition(block);
            if (glm::length(pos - ray.origin) > maxDistance) continue;

            glm::vec3 boxMin(pos.x - treeHalf, pos.y, pos.z - treeHalf);
            glm::vec3 boxMax(pos.x + treeHalf, pos.y + treeSize, pos.z + treeHalf);

            float tMin, tMax;
            if (rayIntersectsAABB(ray, boxMin, boxMax, tMin, tMax)) {
                if (tMin > 0.01f && tMin < closestHit.distance) {
                    closestHit.hit = true;
                    closestHit.distance = tMin;
                    closestHit.point = ray.pointAt(tMin);
                    closestHit.normal = computeNormal(closestHit.point, boxMin, boxMax);
                }
            }
        }
    }

    // Проверка змейки
    const auto& snake = objects.getSnake();
    for (size_t i = 0; i < snake.size(); i++) {
        glm::vec3 pos = getSnakeSegmentPosition(snake[i], i);
        glm::vec3 scale = getSnakeSegmentScale(snake[i], i);
        float halfSize = scale.x / 2.0f;

        glm::vec3 boxMin(pos.x - halfSize, pos.y - halfSize, pos.z - halfSize);
        glm::vec3 boxMax(pos.x + halfSize, pos.y + halfSize, pos.z + halfSize);

        float tMin, tMax;
        if (rayIntersectsAABB(ray, boxMin, boxMax, tMin, tMax)) {
            if (tMin > 0.01f && tMin < closestHit.distance) {
                closestHit.hit = true;
                closestHit.distance = tMin;
                closestHit.point = ray.pointAt(tMin);
                closestHit.normal = computeNormal(closestHit.point, boxMin, boxMax);
            }
        }
    }

    // Проверка еды
    float foodRadius = m_cellSize * 0.4f;
    for (const auto& apple : objects.getFood()) {
        glm::vec3 pos = getFoodPosition(apple);
        float tHit;
        if (rayIntersectsSphere(ray, pos, foodRadius, tHit)) {
            if (tHit > 0.01f && tHit < closestHit.distance) {
                closestHit.hit = true;
                closestHit.distance = tHit;
                closestHit.point = ray.pointAt(tHit);
                closestHit.normal = glm::normalize(closestHit.point - pos);
            }
        }
    }

    return closestHit;
}
void GameRenderer::initialize() {
    m_staticShadow = ShadowMapper(
        m_gridWidth,
        m_gridDepth,
        m_cellSize,
        0.0f,
        m_lightDir,
        m_lightColor
    );

    m_dynamicShadow = m_staticShadow;

    initOpenGLSettings();
    initRayTracingResources();
}

void GameRenderer::renderGame(const GameObjects& objects) {
    auto frameStartTime = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ
    static int frameCount = 0;
    static float totalFrameTime = 0.0f;

    if (m_rayTracingEnabled) {
        renderWithRayTracing(objects);
        return;
    }

    resetDepthState();

    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    g_shaderManager.use3DShader();

    GLuint shader = g_shaderManager.getShaderProgram();

    glUniform3fv(glGetUniformLocation(shader, "lightDir"), 1, &m_lightDir[0]);
    glUniform3fv(glGetUniformLocation(shader, "lightColor"), 1, &m_lightColor[0]);

    if (shadow_map != 0) {
        m_staticShadow.setLightType(m_lightType);
        m_staticShadow.setLightPos(m_lightPos);
        m_dynamicShadow.setLightType(m_lightType);
        m_dynamicShadow.setLightPos(m_lightPos);

        // ===== СТАТИЧЕСКИЕ ТЕНИ (деревья) =====
        if (m_staticShadowsDirty) {
            auto staticStart = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ

            std::cout << "\n[STATIC SHADOWS] Starting computation..." << std::endl;

            m_staticShadow.clearObjectBounds();
            m_staticShadow.setGrid(m_gridWidth, m_gridDepth, m_cellSize, 0.0f);
            std::vector<BoundingSphere> staticSpheres;

            int treeCount = 0;
            for (const auto& obstacle : objects.getObstacles()) {
                for (const auto& block : obstacle.blocks) {
                    float x = block.x * m_cellSize - (m_gridWidth * m_cellSize / 2.0f);
                    float z = block.z * m_cellSize - (m_gridDepth * m_cellSize / 2.0f);

                    float treeSize = m_cellSize * 1.5f;
                    float radius = treeSize * 0.4f;

                    staticSpheres.emplace_back(glm::vec3(x, treeSize / 2.0f, z), radius);
                    treeCount++;
                }
            }
            std::cout << "  Registered " << treeCount << " trees for static shadows" << std::endl;

            m_staticShadow.registerObjectBounds(staticSpheres);

            m_staticShadow.setIntersectCallback(
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

            m_staticShadow.computeShadows();
            m_staticShadowsDirty = false;

            auto staticEnd = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ
            auto staticMs = std::chrono::duration_cast<std::chrono::milliseconds>(staticEnd - staticStart).count();  // ДОБАВИТЬ
            std::cout << "[STATIC SHADOWS] Completed in " << staticMs << " ms\n" << std::endl;  // ДОБАВИТЬ
        }

        // ===== ДИНАМИЧЕСКИЕ ТЕНИ (змейка + яблоки) =====
        if (m_dynamicShadowsDirty) {
            auto dynamicStart = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ

            std::cout << "[DYNAMIC SHADOWS] Starting computation..." << std::endl;

            m_dynamicShadow.clearObjectBounds();
            std::vector<BoundingSphere> dynamicSpheres;

            // Змейка
            int snakeSegments = 0;
            for (size_t i = 0; i < objects.getSnake().size(); i++) {
                const auto& segment = objects.getSnake()[i];
                float x = segment.x * m_cellSize - (m_gridWidth * m_cellSize / 2.0f);
                float z = segment.z * m_cellSize - (m_gridDepth * m_cellSize / 2.0f);
                float scale = m_cellSize * 0.8f;
                float radius = scale * 0.4f;

                dynamicSpheres.emplace_back(glm::vec3(x, 0.15f, z), radius);
                snakeSegments++;
            }
            std::cout << "  Registered " << snakeSegments << " snake segments" << std::endl;

            // Яблоки
            int appleCount = 0;
            for (const auto& apple : objects.getFood()) {
                float x = apple.x * m_cellSize - (m_gridWidth * m_cellSize / 2.0f);
                float z = apple.z * m_cellSize - (m_gridDepth * m_cellSize / 2.0f);
                float radius = m_cellSize * 0.25f;

                dynamicSpheres.emplace_back(glm::vec3(x, 0.1f, z), radius);
                appleCount++;
            }
            std::cout << "  Registered " << appleCount << " apples" << std::endl;
            std::cout << "  Total dynamic objects: " << dynamicSpheres.size() << std::endl;

            m_dynamicShadow.registerObjectBounds(dynamicSpheres);

            m_dynamicShadow.setIntersectCallback(
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

            m_dynamicShadow.computeShadows();
            m_dynamicShadowsDirty = false;

            auto dynamicEnd = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ
            auto dynamicMs = std::chrono::duration_cast<std::chrono::milliseconds>(dynamicEnd - dynamicStart).count();  // ДОБАВИТЬ
            std::cout << "[DYNAMIC SHADOWS] Completed in " << dynamicMs << " ms\n" << std::endl;  // ДОБАВИТЬ
        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        1200.0f / 800.0f,
        0.2f,
        100.0f);

    glm::mat4 view = glm::lookAt(g_camera.getPosition(),
        g_camera.getPosition() + g_camera.getFront(),
        g_camera.getUp());

    g_shaderManager.setViewMatrix(view);
    g_shaderManager.setProjectionMatrix(projection);

    // Замер времени отрисовки
    auto drawStart = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ

    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
    drawObstaclesAsTrees(objects.getObstacles());
    drawFence(objects.getFenceBlocks());
    drawGroundSprites(objects.getFlowerSprites());
    drawFloor();
    drawLightSource();
    drawFood(objects.getFood());
    drawSnake(objects.getSnake());

    auto drawEnd = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ
    auto drawMs = std::chrono::duration_cast<std::chrono::milliseconds>(drawEnd - drawStart).count();  // ДОБАВИТЬ

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto frameEndTime = std::chrono::high_resolution_clock::now();  // ДОБАВИТЬ
    auto frameMs = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - frameStartTime).count();  // ДОБАВИТЬ

    // Статистика по кадрам (каждые 60 кадров)
    frameCount++;
    totalFrameTime += frameMs;
    if (frameCount >= 60) {
        float avgFrameTime = totalFrameTime / frameCount;
        float fps = 1000.0f / avgFrameTime;
        std::cout << "[PERFORMANCE] Frame: " << frameMs << " ms, "
            << "Draw: " << drawMs << " ms, "
            << "FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
        frameCount = 0;
        totalFrameTime = 0.0f;
    }
}

void GameRenderer::createPrimitives() {
    std::cout << "Creating primitive fallback models..." << std::endl;

    PrimitiveBase::createCube(cubePrimitive);
    PrimitiveBase::createSphere(spherePrimitive);
    ApplePrimitive::create(applePrimitive);
    TreePrimitive::create(treePrimitive);
    CloudPrimitive::create(cloudPrimitive);
    BirdPrimitive::create(birdPrimitive);
    FlowerPrimitive::create(flowerPrimitive);

    std::cout << "Primitives created successfully" << std::endl;
}

bool GameRenderer::loadTextureForModel(Model& model, const std::string& texturePath, const std::string& modelFolder) {
    if (texturePath.empty()) {
        model.hasTexture = false;
        return false;
    }

    std::string fullPath;

    // Сначала пробуем в папке с моделью
    fullPath = g_modelsPath + modelFolder + "\\" + texturePath;
    std::cout << "  Trying texture in model folder: " << fullPath << std::endl;

    if (!std::filesystem::exists(fullPath)) {
        // Пробуем в папке текстур
        fullPath = g_texturesPath + texturePath;
        std::cout << "  Trying texture in textures folder: " << fullPath << std::endl;
    }

    if (!std::filesystem::exists(fullPath)) {
        std::cout << "  ✗ Texture not found: " << texturePath << std::endl;
        model.hasTexture = false;
        return false;
    }

    // Загружаем текстуру
    int width, height, channels;
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);

    if (data) {
        if (model.textureID != 0) {
            glDeleteTextures(1, &model.textureID);
        }

        glGenTextures(1, &model.textureID);
        glBindTexture(GL_TEXTURE_2D, model.textureID);

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        model.hasTexture = true;
        std::cout << "  ✓ Texture loaded: " << texturePath << " (" << width << "x" << height << ")" << std::endl;
        return true;
    }
    else {
        std::cout << "  ✗ Failed to load texture: " << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
        model.hasTexture = false;
        return false;
    }
}

bool GameRenderer::loadModelWithFallback(Model& model, const std::string& modelPath,
    const std::string& subFolder,
    std::function<void(Model&)> fallbackCreator) {

    // Очищаем предыдущую модель
    model.cleanup();
    model.hasTexture = false;

    // Если путь пустой - сразу используем примитив
    if (modelPath.empty()) {
        std::cout << "Model path is empty, using primitive fallback" << std::endl;
        fallbackCreator(model);
        return false;
    }

    // Пытаемся загрузить из файла
    std::string fullPath = g_modelsPath + subFolder + "\\" + modelPath;

    std::cout << "Attempting to load model: " << fullPath << std::endl;

    if (std::filesystem::exists(fullPath)) {
        ModelData tempData;
        if (loadFBXModel(modelPath, tempData, subFolder)) {
            // Конвертируем ModelData в Model
            model.vertices.clear();

            // Используем индексы для правильного построения треугольников
            size_t numTriangles = tempData.materialIndices.size();
            size_t vertexIdx = 0;

            for (size_t i = 0; i < numTriangles; i++) {
                for (int j = 0; j < 3; j++) {
                    Vertex v;

                    // Позиция
                    v.position = glm::vec3(
                        tempData.vertices[vertexIdx * 3],
                        tempData.vertices[vertexIdx * 3 + 1],
                        tempData.vertices[vertexIdx * 3 + 2]
                    );

                    // Нормаль
                    if (vertexIdx < tempData.normals.size() / 3) {
                        v.normal = glm::vec3(
                            tempData.normals[vertexIdx * 3],
                            tempData.normals[vertexIdx * 3 + 1],
                            tempData.normals[vertexIdx * 3 + 2]
                        );
                    }
                    else {
                        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    }

                    // Текстурные координаты
                    if (vertexIdx < tempData.texCoords.size() / 2) {
                        v.texCoords = glm::vec2(
                            tempData.texCoords[vertexIdx * 2],
                            tempData.texCoords[vertexIdx * 2 + 1]
                        );
                    }
                    else {
                        v.texCoords = glm::vec2(0.0f, 0.0f);
                    }

                    model.vertices.push_back(v);
                    vertexIdx++;
                }
            }

            // Копируем текстуры из материалов
            if (!tempData.materials.empty() && tempData.materials[0].textureID != 0) {
                model.textureID = tempData.materials[0].textureID;
                model.hasTexture = true;
                std::cout << "  ✓ Model has texture: ID=" << model.textureID << std::endl;
            }

            model.setupBuffers();
            std::cout << "✓ Model loaded from file: " << modelPath << std::endl;
            std::cout << "  Vertices: " << model.vertices.size() << std::endl;
            return true;
        }
    }

    // Если не удалось - используем примитив
    std::cout << "✗ Model file not found, using primitive fallback" << std::endl;
    fallbackCreator(model);
    return false;
}

void GameRenderer::loadModelsFromConfig(const GameObjects& objects) {
    std::cout << "\n========== LOADING MODELS FROM CONFIG ==========" << std::endl;

    // Сначала создаем примитивы для фолбэков
    createPrimitives();

    // Загружаем модели змейки
    loadModelWithFallback(snakeHeadModel, objects.getSnakeHeadModel(), "snake_head",
        [this](Model& m) { PrimitiveBase::createCube(m); });

    loadModelWithFallback(snakeBodyModel, objects.getSnakeBodyModel(), "snake_body",
        [this](Model& m) { PrimitiveBase::createCube(m); });

    loadModelWithFallback(snakeTailModel, objects.getSnakeTailModel(), "snake_tail",
        [this](Model& m) { PrimitiveBase::createCube(m); });

    // Загружаем модели окружения
    loadModelWithFallback(appleModel, objects.getAppleModel(), "food",
        [this](Model& m) { ApplePrimitive::create(m); });

    loadModelWithFallback(treeModel, objects.getTreeModel(), "obstacles",
        [this](Model& m) { TreePrimitive::create(m); });

    loadModelWithFallback(cloudModel, objects.getCloudModel(), "clouds",
        [this](Model& m) { CloudPrimitive::create(m); });

    loadModelWithFallback(birdModel, objects.getBirdModel(), "birds",
        [this](Model& m) { BirdPrimitive::create(m); });

    loadModelWithFallback(flowerModel, objects.getFlowerModel(), "flowers",
        [this](Model& m) { FlowerPrimitive::create(m); });

    // Загружаем модель пола
    if (!objects.getFloorModel().empty()) {
        std::string floorPath = g_modelsPath + "floor\\" + objects.getFloorModel();
        std::cout << "Loading floor model from: " << floorPath << std::endl;

        if (std::filesystem::exists(floorPath)) {
            ModelData tempData;
            if (loadFBXModel(objects.getFloorModel(), tempData, "floor")) {
                // Конвертируем ModelData в Model
                floorModel.vertices.clear();

                size_t numTriangles = tempData.materialIndices.size();
                size_t vertexIdx = 0;

                for (size_t i = 0; i < numTriangles; i++) {
                    for (int j = 0; j < 3; j++) {
                        Vertex v;

                        v.position = glm::vec3(
                            tempData.vertices[vertexIdx * 3],
                            tempData.vertices[vertexIdx * 3 + 1],
                            tempData.vertices[vertexIdx * 3 + 2]
                        );

                        if (vertexIdx < tempData.normals.size() / 3) {
                            v.normal = glm::vec3(
                                tempData.normals[vertexIdx * 3],
                                tempData.normals[vertexIdx * 3 + 1],
                                tempData.normals[vertexIdx * 3 + 2]
                            );
                        }
                        else {
                            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                        }

                        if (vertexIdx < tempData.texCoords.size() / 2) {
                            v.texCoords = glm::vec2(
                                tempData.texCoords[vertexIdx * 2],
                                tempData.texCoords[vertexIdx * 2 + 1]
                            );
                        }
                        else {
                            v.texCoords = glm::vec2(0.0f, 0.0f);
                        }

                        floorModel.vertices.push_back(v);
                        vertexIdx++;
                    }
                }

                // Копируем текстуры из материалов
                floorModel.hasTexture = false;
                for (const auto& mat : tempData.materials) {
                    if (mat.textureID != 0) {
                        floorModel.textureID = mat.textureID;
                        floorModel.hasTexture = true;
                        std::cout << "  ✓ Floor model has texture: ID=" << floorModel.textureID << std::endl;
                        break;
                    }
                }

                if (!floorModel.hasTexture) {
                    std::cout << "  ✗ Floor model has no texture" << std::endl;
                }

                floorModel.setupBuffers();
                std::cout << "✓ Floor model loaded from file: " << objects.getFloorModel() << std::endl;
                std::cout << "  Vertices: " << floorModel.vertices.size() << std::endl;
            }
            else {
                std::cout << "✗ Failed to load floor model, creating simple floor" << std::endl;
                createTexturedFloorModel(floorModel);
            }
        }
        else {
            std::cout << "✗ Floor model file not found: " << floorPath << std::endl;
            createTexturedFloorModel(floorModel);
        }
    }
    else {
        std::cout << "No floor model specified, creating simple floor" << std::endl;
        createTexturedFloorModel(floorModel);
    }


    std::cout << "================================================\n" << std::endl;
}

void GameRenderer::drawModel(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 2.0f);

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    if (snake.empty()) return;

    static bool firstDraw = true;


    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    // Используем размеры из конфига
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];

        // Конвертируем координаты сетки в мировые координаты
        float x = segment.x * m_cellSize - offsetX;
        float y = segment.y * m_cellSize + 0.1f;
        float z = segment.z * m_cellSize - offsetZ;

        const Model* modelToDraw = &snakeBodyModel;
        glm::vec3 color;
        float scale = m_cellSize * 0.8f; // Базовый масштаб от размера ячейки

        if (i == 0) {
            modelToDraw = &snakeHeadModel;
            color = g_game.getSnakeHeadColor();
            scale *= g_game.getSnakeHeadScale();
        }
        else if (i == snake.size() - 1) {
            modelToDraw = &snakeTailModel;
            color = g_game.getSnakeTailColor();
            scale *= g_game.getSnakeTailScale();
        }
        else {
            modelToDraw = &snakeBodyModel;
            color = g_game.getSnakeBodyColor();
            scale *= g_game.getSnakeBodyScale();
        }

        float rotationAngle = calculateSegmentRotation(snake, i);
        drawModelWithRotation(*modelToDraw, x, y, z, scale, color, rotationAngle);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
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

void GameRenderer::drawModelWithRotation(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color, float rotationAngle) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 2.0f);
    g_shaderManager.setIsFloor(false);
    g_shaderManager.setGridEnabled(false);
    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawFood(const std::vector<Point>& food) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 3.0f);

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    for (const auto& apple : food) {
        float x = apple.x * m_cellSize - offsetX;
        float y = apple.y * m_cellSize + 0.1f;
        float z = apple.z * m_cellSize - offsetZ;

        drawModel(appleModel, x, y, z, m_cellSize * 0.8f, glm::vec3(1.0f, 0.8f, 0.2f));
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.2f, 2.5f);

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float y = block.y * m_cellSize;
            float z = block.z * m_cellSize - offsetZ;

            drawModel(treeModel, x, y, z, m_cellSize * 1.5f, glm::vec3(0.1f, 0.4f, 0.1f));
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    if (fenceBlocks.empty()) return;

    std::vector<Vertex> allFenceVertices;

    // Используем размеры из конфига
    float cellSize = m_cellSize;
    float gridWidth = m_gridWidth;
    float gridDepth = m_gridDepth;

    float offsetX = gridWidth * cellSize / 2.0f;
    float offsetZ = gridDepth * cellSize / 2.0f;

    for (const auto& fenceBlock : fenceBlocks) {
        // Конвертируем координаты сетки в мировые координаты
        float x = fenceBlock.x * cellSize - offsetX;
        float y = fenceBlock.y * cellSize;
        float z = fenceBlock.z * cellSize - offsetZ;

        glm::vec3 fenceColor(0.55f, 0.27f, 0.07f);
        glm::vec3 darkColor(0.45f, 0.17f, 0.05f);

        bool isCorner = (fenceBlock.x == 0 && fenceBlock.z == 0) ||
            (fenceBlock.x == gridWidth - 1 && fenceBlock.z == 0) ||
            (fenceBlock.x == 0 && fenceBlock.z == gridDepth - 1) ||
            (fenceBlock.x == gridWidth - 1 && fenceBlock.z == gridDepth - 1);

        bool isNorth = (fenceBlock.z == 0);
        bool isSouth = (fenceBlock.z == gridDepth - 1);
        bool isWest = (fenceBlock.x == 0);
        bool isEast = (fenceBlock.x == gridWidth - 1);

        if (isCorner) {
            float postSize = cellSize * 0.8f;
            createFencePost(allFenceVertices, x, y, z,
                postSize, 0.28f, fenceColor);

            if (fenceBlock.x == 0) {
                createFenceRailHorizontal(allFenceVertices,
                    x + cellSize / 2.0f, y + 0.2f, z,
                    cellSize / 2.0f, 0.03f, darkColor);
            }
            else {
                createFenceRailHorizontal(allFenceVertices,
                    x - cellSize / 2.0f, y + 0.2f, z,
                    cellSize / 2.0f, 0.03f, darkColor);
            }

            if (fenceBlock.z == 0) {
                createFenceRailVertical(allFenceVertices,
                    x, y + 0.2f, z + cellSize / 2.0f,
                    cellSize / 2.0f, 0.03f, darkColor);
            }
            else {
                createFenceRailVertical(allFenceVertices,
                    x, y + 0.2f, z - cellSize / 2.0f,
                    cellSize / 2.0f, 0.03f, darkColor);
            }
        }
        else if (isNorth || isSouth || isWest || isEast) {
            float postSize = cellSize * 0.6f;
            createFencePost(allFenceVertices, x, y, z,
                postSize, 0.25f, fenceColor);

            if (isNorth || isSouth) {
                createFenceRailHorizontal(allFenceVertices, x, y + 0.2f, z,
                    cellSize, 0.03f, darkColor);
                createFenceRailHorizontal(allFenceVertices, x, y + 0.1f, z,
                    cellSize, 0.03f, darkColor);
            }
            else if (isWest || isEast) {
                createFenceRailVertical(allFenceVertices, x, y + 0.2f, z,
                    cellSize, 0.03f, darkColor);
                createFenceRailVertical(allFenceVertices, x, y + 0.1f, z,
                    cellSize, 0.03f, darkColor);
            }
        }
    }

    if (!allFenceVertices.empty()) {
        Model monolithicFenceModel;
        monolithicFenceModel.vertices = allFenceVertices;
        monolithicFenceModel.hasTexture = false;
        monolithicFenceModel.setupBuffers();

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        g_shaderManager.setModelMatrix(modelMatrix);
        g_shaderManager.setIsFloor(false);
        g_shaderManager.setGridEnabled(false);
        g_shaderManager.setColor(glm::vec3(0.55f, 0.27f, 0.07f));
        g_shaderManager.setUseTexture(false);

        monolithicFenceModel.draw();
    }
}

void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.1f, 0.5f);

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
        g_shaderManager.setIsFloor(false);
        g_shaderManager.setGridEnabled(false);
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
    g_shaderManager.setIsFloor(false);
    g_shaderManager.setGridEnabled(false);
    g_shaderManager.setColor(bird.color);
    g_shaderManager.setUseTexture(birdModel.hasTexture);

    birdModel.draw();
}

void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.8f, 1.5f);

    for (const auto& bird : birds) {
        drawBird(bird);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.7f, 1.2f);

    for (const auto& flower : flowerSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 position = flower.position;
        position.y += 0.05f;
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(flower.size));

        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(flower.color);
        g_shaderManager.setIsFloor(false);
        g_shaderManager.setGridEnabled(false);
        g_shaderManager.setUseTexture(flowerModel.hasTexture);

        flowerModel.draw();
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
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

void GameRenderer::drawFloor() {

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(5.0f, 10.0f);

    // Размер игрового поля (для справки)
    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;

    // Рисуем плитки ТОЛЬКО если есть модель пола
    if (floorModel.textureID > 0) {

        // Вычисляем границы модели
        float modelMinX = 999999, modelMaxX = -999999;
        float modelMinY = 999999, modelMaxY = -999999;
        float modelMinZ = 999999, modelMaxZ = -999999;

        for (const auto& vertex : floorModel.vertices) {
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

        float scale = 0.01f;

        float tileWidth = modelSizeX * scale;
        float tileDepth = modelSizeY * scale;


        int tilesX = 50;
        int tilesZ = 50;

        float startX = -(tilesX * tileWidth) / 2.0f;
        float startZ = -(tilesZ * tileDepth) / 2.0f;

        int drawnTiles = 0;
        float baseY = -0.1f;

        for (int i = 0; i < tilesX; i++) {
            for (int j = 0; j < tilesZ; j++) {
                float posX = startX + i * tileWidth + tileWidth / 2.0f;
                float posZ = startZ + j * tileDepth + tileDepth / 2.0f;

                glm::mat4 modelMatrix = glm::mat4(1.0f);

                modelMatrix = glm::translate(modelMatrix, glm::vec3(posX, baseY, posZ));
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-modelCenterX, -modelCenterY, -modelCenterZ));
                modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, scale, scale));

                g_shaderManager.setModelMatrix(modelMatrix);
                g_shaderManager.setColor(glm::vec3(1.0f, 1.0f, 1.0f)); // Белый для текстуры
                g_shaderManager.setUseTexture(floorModel.hasTexture);
                g_shaderManager.setIsFloor(true);
                g_shaderManager.setCellSize(m_cellSize);
                g_shaderManager.setGridEnabled(gridEnabled);
                g_shaderManager.setGridLineWidth(gridLineWidth);
                g_shaderManager.setGridWidth(m_gridWidth);
                g_shaderManager.setGridDepth(m_gridDepth);
                g_shaderManager.setGridColor(gridColor);

                floorModel.draw();
                drawnTiles++;
            }
        }

    }
    // В drawFloor() - упрощённая версия
    else if (shadow_map != 0) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(5.0f, 10.0f);

        int tilesX = m_gridWidth * 2;
        int tilesZ = m_gridDepth * 2;
        float tileSize = m_cellSize * 0.5f;
        float startX = -(tilesX * tileSize) / 2.0f;
        float startZ = -(tilesZ * tileSize) / 2.0f;
        float baseY = 0.0f;

        for (int i = 0; i < tilesX; i++) {
            for (int j = 0; j < tilesZ; j++) {
                float posX = startX + i * tileSize + tileSize * 0.5f;
                float posZ = startZ + j * tileSize + tileSize * 0.5f;

                // Получаем тень (простое значение 0.3 или 1.0)
                float staticShadow = m_staticShadow.getShadowAtWorldPos(posX, posZ);
                float dynamicShadow = m_dynamicShadow.getShadowAtWorldPos(posX, posZ);
                float shadow = std::min(staticShadow, dynamicShadow);

                // Цвет с учётом тени
                glm::vec3 finalColor = floorColor * shadow;

                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(posX, baseY, posZ));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(tileSize, 1.0f, tileSize));

                g_shaderManager.setModelMatrix(modelMatrix);
                g_shaderManager.setColor(finalColor);
                g_shaderManager.setUseTexture(false);
                g_shaderManager.setIsFloor(true);

                floorModel.draw();
            }
        }
    }
    else {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(5.0f, 10.0f);

        glm::mat4 model = glm::mat4(1.0f);

        float floorScale = GRID_WIDTH * CELL_SIZE * 3.0f;
        model = glm::scale(model, glm::vec3(floorScale, 1.0f, floorScale));
        model = glm::translate(model, glm::vec3(0.0f, -0.05f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(floorColor);
        g_shaderManager.setUseTexture(false);
        g_shaderManager.setIsFloor(true);
        g_shaderManager.setCellSize(CELL_SIZE);

        floorModel.draw();
    }
    
    glDisable(GL_POLYGON_OFFSET_FILL);

}
void GameRenderer::markDynamicShadowsDirty() {
    m_dynamicShadowsDirty = true;
}
void GameRenderer::drawLightSource() {

    // позиция "солнца" (просто далеко в сторону света)
    glm::vec3 lightPos = -m_lightDir * 5.0f;

    // === РИСУЕМ СФЕРУ (солнце) ===
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(glm::vec3(1.0f, 1.0f, 0.2f)); // жёлтое солнце
    g_shaderManager.setUseTexture(false);
    g_shaderManager.setIsFloor(false);

    spherePrimitive.draw(); // или cubePrimitive если нет сферы

    // === РИСУЕМ ЛУЧ (линия направления) ===
    glm::vec3 start = lightPos;
    glm::vec3 end = lightPos + m_lightDir * 2.0f;

    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(start.x, start.y, start.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
}
void GameRenderer::createFencePost(std::vector<Vertex>& vertices, float x, float y, float z,
    float width, float height, const glm::vec3& color) {
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
}

void GameRenderer::createFenceRailHorizontal(std::vector<Vertex>& vertices, float x, float y, float z,
    float length, float thickness, const glm::vec3& color) {
    float halfLength = length / 2.0f;
    float halfThickness = thickness / 2.0f;
    float railWidth = 0.04f;

    // Передняя грань
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z - railWidth}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });

    // Задняя грань
    vertices.push_back({ {x - halfLength, y - halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y - halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfLength, y + halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y + halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfLength, y - halfThickness, z + railWidth}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });
}

void GameRenderer::createFenceRailVertical(std::vector<Vertex>& vertices, float x, float y, float z,
    float length, float thickness, const glm::vec3& color) {
    float halfLength = length / 2.0f;
    float halfThickness = thickness / 2.0f;
    float railWidth = 0.04f;

    // Левая грань
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z + halfLength}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z + halfLength}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y + halfThickness, z - halfLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - railWidth, y - halfThickness, z - halfLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });

    // Правая грань
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z + halfLength}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z + halfLength}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y + halfThickness, z - halfLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + railWidth, y - halfThickness, z - halfLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
}

void GameRenderer::createFenceCorner(std::vector<Vertex>& vertices, float x, float y, float z,
    const glm::vec3& color) {
    createFencePost(vertices, x, y, z, 0.1f, 0.3f, color);
    createFenceRailHorizontal(vertices, x + m_cellSize / 2.0f, y + 0.25f, z,
        m_cellSize, 0.04f, color);
    createFenceRailHorizontal(vertices, x - m_cellSize / 2.0f, y + 0.25f, z,
        m_cellSize, 0.04f, color);
    createFenceRailVertical(vertices, x, y + 0.25f, z + m_cellSize / 2.0f,
        m_cellSize, 0.04f, color);
    createFenceRailVertical(vertices, x, y + 0.25f, z - m_cellSize / 2.0f,
        m_cellSize, 0.04f, color);
}

// Заглушки для остальных методов
void GameRenderer::createSnakeHeadModel(Model& model) { PrimitiveBase::createCube(model); }
void GameRenderer::createSnakeBodyModel(Model& model) { PrimitiveBase::createCube(model); }
void GameRenderer::createSnakeTailModel(Model& model) { PrimitiveBase::createCube(model); }
void GameRenderer::loadAllModels() { createPrimitives(); }
bool GameRenderer::loadModelFromFile(Model& model, const std::string& modelPath, const std::string& texturePath) { return false; }
bool GameRenderer::loadOBJModel(Model& model, const std::string& path) { return false; }
bool GameRenderer::loadTexture(Model& model, const std::string& path) { return false; }
bool GameRenderer::loadTextureFromFile(Model& model, const std::string& path) { return false; }
bool GameRenderer::createProceduralTexture(Model& model, const std::string& name) { return false; }
void GameRenderer::createSnakeTexture(std::vector<unsigned char>& data, int size) {}
void GameRenderer::createCheckerboardTexture(std::vector<unsigned char>& data, int size) {}
void GameRenderer::createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal) {}
void GameRenderer::createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color) {}
void GameRenderer::createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color) {}
void GameRenderer::createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius) {}
void GameRenderer::createCloudModel(Model& model) { CloudPrimitive::create(model); }
void GameRenderer::createAnimatedBirdModel(Model& model) { BirdPrimitive::create(model); }
void GameRenderer::createFlowerModel(Model& model) { FlowerPrimitive::create(model); }
void GameRenderer::createTreeModel(Model& model) { TreePrimitive::create(model); }
void GameRenderer::createDetailedAppleModel(Model& model) { ApplePrimitive::create(model); }
void GameRenderer::createTexturedCubeModel(Model& model) { PrimitiveBase::createCube(model); }
void GameRenderer::createTexturedSphereModel(Model& model) { PrimitiveBase::createSphere(model); }
void GameRenderer::createTexturedFloorModel(Model& model) {
    model.vertices.clear();
    float floorSize = 1.0f;

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(-floorSize, 0.0f, -floorSize);
    v2.position = glm::vec3(-floorSize, 0.0f, floorSize);
    v3.position = glm::vec3(floorSize, 0.0f, -floorSize);
    v4.position = glm::vec3(floorSize, 0.0f, floorSize);

    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    v1.normal = v2.normal = v3.normal = v4.normal = normal;

    v1.texCoords = glm::vec2(0.0f, 0.0f);
    v2.texCoords = glm::vec2(0.0f, 1.0f);
    v3.texCoords = glm::vec2(1.0f, 0.0f);
    v4.texCoords = glm::vec2(1.0f, 1.0f);

    model.vertices.push_back(v1);
    model.vertices.push_back(v3);
    model.vertices.push_back(v4);
    model.vertices.push_back(v4);
    model.vertices.push_back(v2);
    model.vertices.push_back(v1);

    model.hasTexture = false;
    model.setupBuffers();
}
void GameRenderer::createFenceModel(Model& model) {}

//=============================================================================
// RAY TRACING METHODS
//=============================================================================

bool GameRenderer::rayIntersectsAABB(
    const Ray& ray,
    const glm::vec3& min,
    const glm::vec3& max,
    float& tMin,
    float& tMax)
{
    tMin = 0.0f;
    tMax = 1000.0f;

    for (int i = 0; i < 3; i++) {
        float origin = ray.origin[i];
        float dir = ray.direction[i];

        if (abs(dir) < 1e-6f) {
            if (origin < min[i] || origin > max[i]) {
                return false;
            }
        }
        else {
            float t1 = (min[i] - origin) / dir;
            float t2 = (max[i] - origin) / dir;

            if (t1 > t2) std::swap(t1, t2);

            tMin = glm::max(tMin, t1);
            tMax = glm::min(tMax, t2);

            if (tMin > tMax) return false;
        }
    }

    return tMin > 0.0f;
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

void GameRenderer::initRayTracingResources() {
    glGenTextures(1, &m_rayTracingTexture);
    glBindTexture(GL_TEXTURE_2D, m_rayTracingTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_rayTracingVAO);
    glGenBuffers(1, &m_rayTracingVBO);
    glBindVertexArray(m_rayTracingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_rayTracingVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const char* vertexShader = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    const char* fragmentShader = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        void main() {
            FragColor = texture(uTexture, TexCoord);
        }
    )";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShader, nullptr);
    glCompileShader(vs);

    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vs, 512, nullptr, infoLog);
        std::cout << "Vertex shader error: " << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShader, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fs, 512, nullptr, infoLog);
        std::cout << "Fragment shader error: " << infoLog << std::endl;
    }

    m_rayTracingShader = glCreateProgram();
    glAttachShader(m_rayTracingShader, vs);
    glAttachShader(m_rayTracingShader, fs);
    glLinkProgram(m_rayTracingShader);

    glGetProgramiv(m_rayTracingShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_rayTracingShader, 512, nullptr, infoLog);
        std::cout << "Shader program link error: " << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    std::cout << "Ray tracing resources initialized. Shader ID: " << m_rayTracingShader << std::endl;
}

void GameRenderer::cleanupRayTracingResources() {
    if (m_rayTracingTexture) glDeleteTextures(1, &m_rayTracingTexture);
    if (m_rayTracingVAO) glDeleteVertexArrays(1, &m_rayTracingVAO);
    if (m_rayTracingVBO) glDeleteBuffers(1, &m_rayTracingVBO);
    if (m_rayTracingShader) glDeleteProgram(m_rayTracingShader);
}

void GameRenderer::renderWithRayTracing(const GameObjects& objects)
{
    if (!m_rayTracingEnabled) return;

    int width = g_game.getWindowWidth();
    int height = g_game.getWindowHeight();
    if (width <= 0 || height <= 0) {
        width = 1200;
        height = 800;
    }

    std::vector<unsigned char> pixels(width * height * 3);
    std::vector<bool> computed(width * height, false);

    glm::vec3 camPos = g_camera.getPosition();
    glm::vec3 camDir = glm::normalize(g_camera.getFront());
    glm::vec3 camUp = g_camera.getUp();

    float fov = 60.0f;
    float aspect = (float)width / (float)height;
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    // Вывод параметров рендеринга
    std::cout << "Ray Tracing: Step size = " << m_rayTracingStepSize
        << ", Adaptive = " << (m_rayTracingUseAdaptive ? "ON" : "OFF") << std::endl;

    // ПЕРВЫЙ ПРОХОД: рендерим с шагом
#pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; y += m_rayTracingStepSize) {

        for (int x = 0; x < width; x += m_rayTracingStepSize) {
            float screenX = (2.0f * (x + 0.5f) / width - 1.0f);
            float screenY = (1.0f - 2.0f * (y + 0.5f) / height);

            Ray ray = m_rayTracer.getRayFromCamera(
                camPos, camDir, camUp, fov, aspect, screenX, screenY
            );

            glm::vec3 color = traceRay(ray, objects, offsetX, offsetZ);

            int index = ((height - 1 - y) * width + x) * 3;
            pixels[index + 0] = (unsigned char)(glm::clamp(color.r, 0.0f, 1.0f) * 255);
            pixels[index + 1] = (unsigned char)(glm::clamp(color.g, 0.0f, 1.0f) * 255);
            pixels[index + 2] = (unsigned char)(glm::clamp(color.b, 0.0f, 1.0f) * 255);
            computed[y * width + x] = true;
        }
    }

    // ВТОРОЙ ПРОХОД: интерполяция пропущенных пикселей

    if (m_rayTracingStepSize > 1) {
#pragma omp parallel for
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (computed[y * width + x]) continue;

                int nearestX = ((x + m_rayTracingStepSize / 2) / m_rayTracingStepSize) * m_rayTracingStepSize;
                int nearestY = ((y + m_rayTracingStepSize / 2) / m_rayTracingStepSize) * m_rayTracingStepSize;

                nearestX = std::min(nearestX, width - 1);
                nearestY = std::min(nearestY, height - 1);

                int nearestIdx = ((height - 1 - nearestY) * width + nearestX) * 3;

                pixels[((height - 1 - y) * width + x) * 3 + 0] = pixels[nearestIdx + 0];
                pixels[((height - 1 - y) * width + x) * 3 + 1] = pixels[nearestIdx + 1];
                pixels[((height - 1 - y) * width + x) * 3 + 2] = pixels[nearestIdx + 2];
            }
        }
    }

    // АДАПТИВНАЯ ВЫБОРКА (опционально)
    if (m_rayTracingUseAdaptive && m_rayTracingStepSize > 1) {
        for (int y = 0; y < height - m_rayTracingStepSize; y += m_rayTracingStepSize) {
            for (int x = 0; x < width - m_rayTracingStepSize; x += m_rayTracingStepSize) {
                int idx1 = ((height - 1 - y) * width + x) * 3;
                int idx2 = ((height - 1 - y) * width + x + m_rayTracingStepSize) * 3;
                int idx3 = ((height - 1 - (y + m_rayTracingStepSize)) * width + x) * 3;

                float diff1 = abs(pixels[idx1 + 0] - pixels[idx2 + 0]) +
                    abs(pixels[idx1 + 1] - pixels[idx2 + 1]) +
                    abs(pixels[idx1 + 2] - pixels[idx2 + 2]);
                float diff2 = abs(pixels[idx1 + 0] - pixels[idx3 + 0]) +
                    abs(pixels[idx1 + 1] - pixels[idx3 + 1]) +
                    abs(pixels[idx1 + 2] - pixels[idx3 + 2]);

                if (diff1 > 100 || diff2 > 100) {
                    int centerX = x + m_rayTracingStepSize / 2;
                    int centerY = y + m_rayTracingStepSize / 2;

                    if (centerX < width && centerY < height && !computed[centerY * width + centerX]) {
                        float screenX = (2.0f * (centerX + 0.5f) / width - 1.0f);
                        float screenY = (1.0f - 2.0f * (centerY + 0.5f) / height);

                        Ray ray = m_rayTracer.getRayFromCamera(
                            camPos, camDir, camUp, fov, aspect, screenX, screenY
                        );

                        glm::vec3 color = traceRay(ray, objects, offsetX, offsetZ);

                        int idx = ((height - 1 - centerY) * width + centerX) * 3;
                        pixels[idx + 0] = (unsigned char)(glm::clamp(color.r, 0.0f, 1.0f) * 255);
                        pixels[idx + 1] = (unsigned char)(glm::clamp(color.g, 0.0f, 1.0f) * 255);
                        pixels[idx + 2] = (unsigned char)(glm::clamp(color.b, 0.0f, 1.0f) * 255);
                        computed[centerY * width + centerX] = true;
                    }
                }
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_rayTracingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(m_rayTracingShader);
    glBindVertexArray(m_rayTracingVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}


glm::vec3 GameRenderer::traceRay(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ, int depth) {
    // Ограничение глубины рекурсии
    if (depth > 2) return skyColor;

    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;

    float maxDistance = 30.0f;
    glm::vec3 rayOrigin = ray.origin;
    glm::vec3 rayDir = ray.direction;

    // ===== ПОЛ =====
    float halfWidth = m_gridWidth * m_cellSize;
    float halfDepth = m_gridDepth * m_cellSize;
    float tGround = -rayOrigin.y / rayDir.y;
    if (tGround > 0 && tGround < maxDistance && tGround < closestHit.distance) {
        glm::vec3 hitPoint = ray.pointAt(tGround);
        if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
            closestHit.hit = true;
            closestHit.distance = tGround;
            closestHit.point = hitPoint;
            closestHit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            closestHit.color = floorColor;
        }
    }

    // ===== ЗМЕЙКА =====
    const auto& snake = objects.getSnake();
    for (size_t i = 0; i < snake.size(); i++) {
        glm::vec3 pos = getSnakeSegmentPosition(snake[i], i);
        if (glm::length(pos - rayOrigin) > maxDistance) continue;

        glm::vec3 scale = getSnakeSegmentScale(snake[i], i);
        float halfSize = scale.x / 2.0f;
        glm::vec3 boxMin(pos.x - halfSize, pos.y - halfSize, pos.z - halfSize);
        glm::vec3 boxMax(pos.x + halfSize, pos.y + halfSize, pos.z + halfSize);

        float tMin, tMax;
        if (rayIntersectsAABB(ray, boxMin, boxMax, tMin, tMax)) {
            if (tMin > 0 && tMin < closestHit.distance && tMin < maxDistance) {
                closestHit.hit = true;
                closestHit.distance = tMin;
                closestHit.point = ray.pointAt(tMin);
                closestHit.normal = computeNormal(closestHit.point, boxMin, boxMax);

                if (i == 0) closestHit.color = g_game.getSnakeHeadColor();
                else if (i == snake.size() - 1) closestHit.color = g_game.getSnakeTailColor();
                else closestHit.color = g_game.getSnakeBodyColor();
            }
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // ===== ЕДА =====
    const auto& food = objects.getFood();
    float foodRadius = (m_cellSize * 0.8f) / 2.0f;
    for (const auto& apple : food) {
        glm::vec3 pos = getFoodPosition(apple);
        if (glm::length(pos - rayOrigin) > maxDistance) continue;

        float tHit;
        if (rayIntersectsSphere(ray, pos, foodRadius, tHit)) {
            if (tHit > 0 && tHit < closestHit.distance && tHit < maxDistance) {
                closestHit.hit = true;
                closestHit.distance = tHit;
                closestHit.point = ray.pointAt(tHit);
                closestHit.normal = glm::normalize(closestHit.point - pos);
                closestHit.color = glm::vec3(1.0f, 0.8f, 0.2f);
            }
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // ===== ПРЕПЯТСТВИЯ =====
    const auto& obstacles = objects.getObstacles();
    float treeScale = m_cellSize * 1.5f;
    float treeHalf = treeScale / 2.0f;
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            glm::vec3 pos = getObstaclePosition(block);
            if (glm::length(pos - rayOrigin) > maxDistance) continue;

            glm::vec3 boxMin(pos.x - treeHalf, pos.y, pos.z - treeHalf);
            glm::vec3 boxMax(pos.x + treeHalf, pos.y + treeScale, pos.z + treeHalf);

            float tMin, tMax;
            if (rayIntersectsAABB(ray, boxMin, boxMax, tMin, tMax)) {
                if (tMin > 0 && tMin < closestHit.distance && tMin < maxDistance) {
                    closestHit.hit = true;
                    closestHit.distance = tMin;
                    closestHit.point = ray.pointAt(tMin);
                    closestHit.normal = computeNormal(closestHit.point, boxMin, boxMax);
                    closestHit.color = glm::vec3(0.1f, 0.4f, 0.1f);
                }
            }
            if (closestHit.hit && closestHit.distance < 0.5f) break;
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // ===== ПТИЦЫ =====
    const auto& birds = objects.getBirds();
    float birdRadius = 0.15f;
    for (const auto& bird : birds) {
        glm::vec3 pos = getBirdPosition(bird);
        if (glm::length(pos - rayOrigin) > maxDistance) continue;

        float tHit;
        if (rayIntersectsSphere(ray, pos, birdRadius, tHit)) {
            if (tHit > 0 && tHit < closestHit.distance && tHit < maxDistance) {
                closestHit.hit = true;
                closestHit.distance = tHit;
                closestHit.point = ray.pointAt(tHit);
                closestHit.normal = glm::normalize(closestHit.point - pos);
                closestHit.color = bird.color;
            }
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // ===== ОБЛАКА =====
    const auto& clouds = objects.getCloudSprites();
    float cloudRadius = 0.5f;
    for (const auto& cloud : clouds) {
        float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceToCenter < 8.0f) continue;

        glm::vec3 pos = getCloudPosition(cloud);
        if (glm::length(pos - rayOrigin) > maxDistance) continue;

        float tHit;
        if (rayIntersectsSphere(ray, pos, cloudRadius, tHit)) {
            if (tHit > 0 && tHit < closestHit.distance && tHit < maxDistance) {
                closestHit.hit = true;
                closestHit.distance = tHit;
                closestHit.point = ray.pointAt(tHit);
                closestHit.normal = glm::normalize(closestHit.point - pos);
                closestHit.color = cloud.color;
            }
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // ===== ЦВЕТЫ =====
    const auto& flowers = objects.getFlowerSprites();
    float flowerRadius = 0.08f;
    for (const auto& flower : flowers) {
        glm::vec3 pos = getFlowerPosition(flower);
        if (glm::length(pos - rayOrigin) > maxDistance) continue;

        float tHit;
        if (rayIntersectsSphere(ray, pos, flowerRadius, tHit)) {
            if (tHit > 0 && tHit < closestHit.distance && tHit < maxDistance) {
                closestHit.hit = true;
                closestHit.distance = tHit;
                closestHit.point = ray.pointAt(tHit);
                closestHit.normal = glm::normalize(closestHit.point - pos);
                closestHit.color = flower.color;
            }
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // ===== ЗАБОР =====
    const auto& fenceBlocks = objects.getFenceBlocks();
    float fenceSize = m_cellSize * 0.6f;
    float fenceHalf = fenceSize / 2.0f;
    for (const auto& block : fenceBlocks) {
        float x = block.x * m_cellSize - offsetX;
        float z = block.z * m_cellSize - offsetZ;
        float y = block.y * m_cellSize;

        glm::vec3 pos(x, y + fenceHalf, z);
        if (glm::length(pos - rayOrigin) > maxDistance) continue;

        glm::vec3 boxMin(x - fenceHalf, y, z - fenceHalf);
        glm::vec3 boxMax(x + fenceHalf, y + fenceSize, z + fenceHalf);

        float tMin, tMax;
        if (rayIntersectsAABB(ray, boxMin, boxMax, tMin, tMax)) {
            if (tMin > 0 && tMin < closestHit.distance && tMin < maxDistance) {
                closestHit.hit = true;
                closestHit.distance = tMin;
                closestHit.point = ray.pointAt(tMin);
                closestHit.normal = computeNormal(closestHit.point, boxMin, boxMax);
                closestHit.color = glm::vec3(0.55f, 0.27f, 0.07f);
            }
        }
        if (closestHit.hit && closestHit.distance < 0.5f) break;
    }

    // Если ничего не нашли - возвращаем небо
    if (!closestHit.hit) {
        float t = 0.5f * (rayDir.y + 1.0f);
        glm::vec3 skyTop = glm::vec3(0.1f, 0.2f, 0.5f);
        glm::vec3 skyBottom = glm::vec3(0.6f, 0.8f, 1.0f);
        return glm::mix(skyBottom, skyTop, t);
    }

    // ===== ОСВЕЩЕНИЕ =====
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 0.2f, 0.5f));
    glm::vec3 lightColor(0.9f, 0.85f, 0.75f);

    // Ambient - базовое освещение
    glm::vec3 ambient = closestHit.color * 0.35f;

    // Диффузное освещение
    float diff = glm::max(glm::dot(closestHit.normal, lightDir), 0.2f);

    // ===== ПРОВЕРКА ТЕНИ (УПРОЩЕННАЯ, НО ДЛЯ ВСЕХ) =====
    float shadow = 1.0f;

    // Луч к солнцу
    glm::vec3 shadowRayOrigin = closestHit.point + closestHit.normal * 0.1f;
    Ray shadowRay(shadowRayOrigin, lightDir);

    // Проверяем все объекты подряд без условий
    // 1. Деревья
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            glm::vec3 pos = getObstaclePosition(block);
            if (glm::length(pos - closestHit.point) > 10.0f) continue;

            glm::vec3 boxMin(pos.x - treeHalf, pos.y, pos.z - treeHalf);
            glm::vec3 boxMax(pos.x + treeHalf, pos.y + treeScale, pos.z + treeHalf);

            float tMin, tMax;
            if (rayIntersectsAABB(shadowRay, boxMin, boxMax, tMin, tMax)) {
                if (tMin > 0.05f && tMin < 20.0f) {
                    shadow = 0.4f;
                }
            }
        }
    }

    // 2. Змейка
    for (size_t i = 0; i < snake.size(); i++) {
        glm::vec3 pos = getSnakeSegmentPosition(snake[i], i);
        if (glm::length(pos - closestHit.point) > 8.0f) continue;
        if (glm::length(pos - shadowRayOrigin) < 0.2f) continue;

        glm::vec3 scale = getSnakeSegmentScale(snake[i], i);
        float halfSize = scale.x / 2.0f;
        glm::vec3 boxMin(pos.x - halfSize, pos.y - halfSize, pos.z - halfSize);
        glm::vec3 boxMax(pos.x + halfSize, pos.y + halfSize, pos.z + halfSize);

        float tMin, tMax;
        if (rayIntersectsAABB(shadowRay, boxMin, boxMax, tMin, tMax)) {
            if (tMin > 0.05f && tMin < 15.0f) {
                shadow = 0.4f;
            }
        }
    }

    // 3. Забор
    for (const auto& block : fenceBlocks) {
        float x = block.x * m_cellSize - offsetX;
        float z = block.z * m_cellSize - offsetZ;
        float y = block.y * m_cellSize;

        glm::vec3 pos(x, y + fenceHalf, z);
        if (glm::length(pos - closestHit.point) > 8.0f) continue;

        glm::vec3 boxMin(x - fenceHalf, y, z - fenceHalf);
        glm::vec3 boxMax(x + fenceHalf, y + fenceSize, z + fenceHalf);

        float tMin, tMax;
        if (rayIntersectsAABB(shadowRay, boxMin, boxMax, tMin, tMax)) {
            if (tMin > 0.05f && tMin < 15.0f) {
                shadow = 0.4f;
            }
        }
    }

    // 4. Еда
    for (const auto& apple : food) {
        glm::vec3 pos = getFoodPosition(apple);
        if (glm::length(pos - closestHit.point) > 5.0f) continue;
        if (glm::length(pos - shadowRayOrigin) < 0.2f) continue;

        float tHit;
        if (rayIntersectsSphere(shadowRay, pos, foodRadius, tHit)) {
            if (tHit > 0.05f && tHit < 10.0f) {
                shadow = 0.4f;
            }
        }
    }

    glm::vec3 diffuse = closestHit.color * lightColor * diff * shadow;

    // ===== ОТРАЖЕНИЯ (блики) =====
    glm::vec3 reflection(0.0f);
    if (depth < 1) {
        bool isReflective = (closestHit.color.r > 0.8f && closestHit.color.g > 0.6f) ||
            (closestHit.color == g_game.getSnakeHeadColor());
        if (isReflective) {
            glm::vec3 reflectDir = glm::reflect(rayDir, closestHit.normal);
            Ray reflectRay(closestHit.point + closestHit.normal * 0.05f, reflectDir);
            reflection = traceRay(reflectRay, objects, offsetX, offsetZ, depth + 1) * 0.3f;
        }
    }

    return ambient + diffuse + reflection;
}// СТАТИЧЕСКИЕ МЕТОДЫ GameRenderer
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
    float baseScale = m_cellSize * 0.8f;
    float scale = baseScale;

    if (index == 0) scale *= g_game.getSnakeHeadScale();
    else if (index == g_game.getSnake().size() - 1) scale *= g_game.getSnakeTailScale();
    else scale *= g_game.getSnakeBodyScale();

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
void GameRenderer::setupGLFWHints() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
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
    glGetError(); // Игнорируем первую ошибку
    return true;
}

void GameRenderer::initOpenGLSettings() {
    // Базовые настройки OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
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

void GameRenderer::resetDepthState() {
    // Сбрасываем состояние глубины между кадрами
    glDepthMask(GL_TRUE);
    glClearDepth(1.0f);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    // Отключаем все смещения полигонов
    glDisable(GL_POLYGON_OFFSET_FILL);
}

//=============================================================================
// КОЛБЭКИ GLFW
//=============================================================================

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
            if (key == GLFW_KEY_T && action == GLFW_PRESS) {
                g_game.toggleRayTracing(); // нужно добавить этот метод в Game
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

void GameRenderer::setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}