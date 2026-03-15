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
    , m_cellSize(0.1f) {
}

void GameRenderer::initialize() {
    std::cout << "GameRenderer::initialize() - Models will be loaded from config in Game::loadConfig()" << std::endl;
    initOpenGLSettings();
}

void GameRenderer::renderGame(const GameObjects& objects) {
    resetDepthState();

    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    g_shaderManager.use3DShader();

    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        1200.0f / 800.0f,
        0.2f,
        100.0f);

    glm::mat4 view = glm::lookAt(g_camera.getPosition(),
        g_camera.getPosition() + g_camera.getFront(),
        g_camera.getUp());

    g_shaderManager.setViewMatrix(view);
    g_shaderManager.setProjectionMatrix(projection);

    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
    drawObstaclesAsTrees(objects.getObstacles());
    drawFence(objects.getFenceBlocks());
    drawGroundSprites(objects.getFlowerSprites());

    // Рисуем 3D пол
    drawFloor();

    drawFood(objects.getFood());
    drawSnake(objects.getSnake());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
float GameRenderer::getFloorHeightAt(float x, float z) const {
    if (floorModel.vertices.empty()) return 0.0f;

    // Статические переменные для хранения диапазона высот
    static float minY = 0.0f;
    static float maxY = 0.0f;
    static bool firstCall = true;

    // При первом вызове вычисляем диапазон высот модели
    if (firstCall) {
        minY = 999999;
        maxY = -999999;
        for (const auto& vertex : floorModel.vertices) {
            minY = std::min(minY, vertex.position.y);
            maxY = std::max(maxY, vertex.position.y);
        }
        std::cout << "  Floor height range: " << minY << " to " << maxY << std::endl;
        firstCall = false;
    }

    float bestDist = 1e9;
    float bestY = 0.0f;

    // Ищем ближайшую вершину
    for (size_t i = 0; i < floorModel.vertices.size(); i++) {
        float dx = floorModel.vertices[i].position.x - x;
        float dz = floorModel.vertices[i].position.z - z;
        float dist = dx * dx + dz * dz;

        if (dist < bestDist) {
            bestDist = dist;
            bestY = floorModel.vertices[i].position.y;
        }
    }

    // Нормализуем высоту: делаем так, чтобы минимальная высота была 0
    // и масштабируем до разумных размеров (0-1)
    float normalizedY = (bestY - minY) / (maxY - minY);

    return normalizedY;
}
void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    if (snake.empty()) return;

    // Временно отключаем полигон оффсет
    glDisable(GL_POLYGON_OFFSET_FILL);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    static bool firstDraw = true;
    if (firstDraw) {
        std::cout << "\n=== SNAKE POSITIONS ===" << std::endl;
        firstDraw = false;
    }

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];

        float worldX = segment.x * m_cellSize - offsetX;
        float worldZ = segment.z * m_cellSize - offsetZ;

        // Получаем нормализованную высоту пола (0-1)
        float floorY = 0.0f;
        if (floorModel.vertices.size() > 0) {
            floorY = getFloorHeightAt(worldX, worldZ);
        }

        // Теперь floorY в диапазоне 0-1, добавляем небольшое смещение
        float y = floorY + 0.02f;

        const Model* modelToDraw = &snakeBodyModel;
        glm::vec3 color;
        float scale = m_cellSize * 0.8f;

        if (i == 0) {
            modelToDraw = &snakeHeadModel;
            color = g_game.getSnakeHeadColor();
            scale *= g_game.getSnakeHeadScale();

            // Отладка для головы
            static int frameCount = 0;
            if (frameCount++ % 60 == 0) {
                std::cout << "  Head at: (" << worldX << ", " << worldZ << ") "
                    << "normY=" << floorY << " finalY=" << y << std::endl;
            }
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

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(worldX, y, worldZ));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

        g_shaderManager.setModelMatrix(modelMatrix);
        g_shaderManager.setIsFloor(false);
        g_shaderManager.setGridEnabled(false);
        g_shaderManager.setColor(color);
        g_shaderManager.setUseTexture(modelToDraw->hasTexture);

        modelToDraw->draw();
    }

    glDepthFunc(GL_LESS);
    glEnable(GL_POLYGON_OFFSET_FILL);
}float GameRenderer::calculateSegmentRotation(const std::vector<Point>& snake, size_t index) {
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
    std::cout << "\n=== DRAW FLOOR DEBUG ===" << std::endl;

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(5.0f, 10.0f);

    // Размер игрового поля (для справки)
    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;

    std::cout << "  World size: " << worldWidth << " x " << worldDepth << std::endl;
    std::cout << "  Floor model exists: " << (floorModel.vertices.size() > 0 ? "YES" : "NO") << std::endl;
    std::cout << "  Floor color: (" << floorColor.r << ", " << floorColor.g << ", " << floorColor.b << ")" << std::endl;
    std::cout << "  Floor has texture: " << (floorModel.hasTexture ? "YES" : "NO") << std::endl;
    std::cout << "  Grid enabled: " << (gridEnabled ? "YES" : "NO") << std::endl;

    // Рисуем плитки ТОЛЬКО если есть модель пола
    if (floorModel.vertices.size() > 0) {
        std::cout << "  Drawing tiles over base floor" << std::endl;

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

        std::cout << "  Tile size: " << tileWidth << " x " << tileDepth << std::endl;

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

        std::cout << "  Tiles drawn: " << drawnTiles << std::endl;
    }

    glDisable(GL_POLYGON_OFFSET_FILL);

    std::cout << "=== END DRAW FLOOR ===\n" << std::endl;
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
// СТАТИЧЕСКИЕ МЕТОДЫ GameRenderer
//=============================================================================

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