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
    , m_showGroundRays(true)
    , m_shadowStrideX(1)
    , m_shadowStrideZ(1)
    , m_staticShadowsDirty(true)
    , m_dynamicShadowsDirty(true)
    , shadow_map(false)
{
    m_lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.5f));
    m_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_lightType = LightType::Directional;
    m_lightPos = glm::vec3(0.0f, 100.0f, 0.0f);
}

void GameRenderer::initialize() {
    initOpenGLSettings();
    setupFixedPipelineLighting();
    initRayTracingResources();
    createPrimitives();
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

void GameRenderer::setupFixedPipelineLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);


    glEnable(GL_NORMALIZE);

    GLfloat global_ambient[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    GLfloat light0_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light0_diffuse[] = { m_lightColor.r, m_lightColor.g, m_lightColor.b, 1.0f };
    GLfloat light0_specular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat light0_position[] = { -m_lightDir.x, -m_lightDir.y, -m_lightDir.z, 0.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    std::cout << "Fixed pipeline lighting configured" << std::endl;
}

void GameRenderer::updateLightPosition() {
    GLfloat light0_position[] = { -m_lightDir.x, -m_lightDir.y, -m_lightDir.z, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
}

void GameRenderer::setMaterial(const glm::vec3& color, float shininess, float specularStrength) {
    glDisable(GL_COLOR_MATERIAL);

    GLfloat ambient[] = { color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, 1.0f };
    GLfloat diffuse[] = { color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 1.0f };
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
// ОСНОВНОЙ РЕНДЕРИНГ
//=============================================================================

void GameRenderer::renderGame(const GameObjects& objects) {
    // Используем цвет неба из параметра objects
    skyColor = objects.getSkyColor();  // Сохраняем в член класса
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Также обновляем другие цвета из конфига
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

    drawFloor();
    drawObstaclesAsTrees(objects.getObstacles());
    drawFence(objects.getFenceBlocks());
    drawFood(objects.getFood());
    drawSnake(objects.getSnake());
    drawGroundSprites(objects.getFlowerSprites());
    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
    drawLightSource();

    if (m_debugRaysEnabled && m_shadowMapEnabled) {
        drawDebugRaysIfEnabled();
    }
}

void GameRenderer::computeShadowsIfNeeded(const GameObjects& objects) {
    if (m_staticShadowsDirty) {
        std::cout << "[STATIC SHADOWS] Computing..." << std::endl;

        m_staticShadow.clearObjectBounds();
        m_staticShadow.setGrid(m_gridWidth, m_gridDepth, m_cellSize, 0.0f);
        std::vector<BoundingSphere> staticSpheres;

        for (const auto& obstacle : objects.getObstacles()) {
            for (const auto& block : obstacle.blocks) {
                float x = block.x * m_cellSize - (m_gridWidth * m_cellSize / 2.0f);
                float z = block.z * m_cellSize - (m_gridDepth * m_cellSize / 2.0f);
                float treeSize = m_cellSize * 1.5f;
                float radius = treeSize * 0.4f;
                staticSpheres.emplace_back(glm::vec3(x, treeSize / 2.0f, z), radius);
            }
        }

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
        std::cout << "[STATIC SHADOWS] Completed" << std::endl;
    }

    if (m_dynamicShadowsDirty) {
        std::cout << "[DYNAMIC SHADOWS] Computing..." << std::endl;

        m_dynamicShadow.clearObjectBounds();
        std::vector<BoundingSphere> dynamicSpheres;

        for (size_t i = 0; i < objects.getSnake().size(); i++) {
            const auto& segment = objects.getSnake()[i];
            float x = segment.x * m_cellSize - (m_gridWidth * m_cellSize / 2.0f);
            float z = segment.z * m_cellSize - (m_gridDepth * m_cellSize / 2.0f);
            float radius = m_cellSize * 0.4f;
            dynamicSpheres.emplace_back(glm::vec3(x, 0.15f, z), radius);
        }

        for (const auto& apple : objects.getFood()) {
            float x = apple.x * m_cellSize - (m_gridWidth * m_cellSize / 2.0f);
            float z = apple.z * m_cellSize - (m_gridDepth * m_cellSize / 2.0f);
            float radius = m_cellSize * 0.25f;
            dynamicSpheres.emplace_back(glm::vec3(x, 0.1f, z), radius);
        }

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
        std::cout << "[DYNAMIC SHADOWS] Completed" << std::endl;
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

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Рисуем пол на ВСЕХ клетках, включая границы
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
            // Опускаем пол чуть ниже, чтобы забор не перекрывал
            glVertex3f(posX, -0.02f, posZ);
            glVertex3f(posX + m_cellSize, -0.02f, posZ);
            glVertex3f(posX + m_cellSize, -0.02f, posZ + m_cellSize);
            glVertex3f(posX, -0.02f, posZ + m_cellSize);
            glEnd();
        }
    }

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

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];

        float x = segment.x * m_cellSize - offsetX;
        float y = segment.y * m_cellSize + 0.1f;
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

        float rotationAngle = calculateSegmentRotation(snake, i);

        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
        glScalef(scale, scale, scale);

        // setMaterial уже отключает COLOR_MATERIAL
        setMaterial(color, 64.0f, 0.2f);

        if (currentModel) {
            currentModel->draw();
        }

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

    for (const auto& apple : food) {
        float x = apple.x * m_cellSize - offsetX;
        float y = apple.y * m_cellSize + 0.1f;
        float z = apple.z * m_cellSize - offsetZ;

        glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(m_cellSize * 0.6f, m_cellSize * 0.6f, m_cellSize * 0.6f);

        // Красный цвет для яблока
        setMaterial(glm::vec3(0.9f, 0.2f, 0.2f), 80.0f, 0.4f);
        m_appleModel.draw();

        glPopMatrix();
    }
}

void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    if (obstacles.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float y = block.y * m_cellSize;
            float z = block.z * m_cellSize - offsetZ;

            glPushMatrix();
            glTranslatef(x, y, z);
            glScalef(m_cellSize * 1.2f, m_cellSize * 1.2f, m_cellSize * 1.2f);

            // Зелёный цвет для дерева
            setMaterial(glm::vec3(0.2f, 0.6f, 0.2f), 30.0f, 0.2f);
            m_treeModel.draw();

            glPopMatrix();
        }
    }
}

void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
    for (const auto& cloud : cloudSprites) {
        float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceToCenter < 8.0f) continue;

        glPushMatrix();
        glTranslatef(cloud.position.x, cloud.position.y, cloud.position.z);
        glScalef(cloud.size, cloud.size, cloud.size);

        // Цвет облака из конфига
        setMaterial(cloud.color, 40.0f, 0.3f);
        m_cloudModel.draw();

        glPopMatrix();
    }
}

void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
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

        glScalef(bird.size, bird.size, bird.size);

        // Цвет птицы из конфига
        setMaterial(bird.color, 40.0f, 0.2f);
        m_birdModel.draw();

        glPopMatrix();
    }
}

void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    for (const auto& flower : flowerSprites) {
        glPushMatrix();
        glTranslatef(flower.position.x, flower.position.y + 0.05f, flower.position.z);
        glScalef(flower.size, flower.size, flower.size);

        // Цвет цветка из конфига
        setMaterial(flower.color, 50.0f, 0.2f);
        m_flowerModel.draw();

        glPopMatrix();
    }
}

void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    if (fenceBlocks.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    // Временно отключаем COLOR_MATERIAL для забора
    glDisable(GL_COLOR_MATERIAL);

    setMaterial(glm::vec3(0.55f, 0.27f, 0.07f), 24.0f, 0.15f);

    for (const auto& fenceBlock : fenceBlocks) {
        float x = fenceBlock.x * m_cellSize - offsetX;
        float z = fenceBlock.z * m_cellSize - offsetZ;

        bool isNorth = (fenceBlock.z == 0);
        bool isSouth = (fenceBlock.z == m_gridDepth - 1);
        bool isWest = (fenceBlock.x == 0);
        bool isEast = (fenceBlock.x == m_gridWidth - 1);

        bool isCorner = (isNorth || isSouth) && (isWest || isEast);

        float postHeight = m_cellSize * 0.5f;
        float postWidth = m_cellSize * 0.08f;
        float railHeight = m_cellSize * 0.05f;

        if (isCorner) {
            // Угловой столб
            glPushMatrix();
            glTranslatef(x, postHeight / 2.0f, z);
            glScalef(postWidth * 1.2f, postHeight, postWidth * 1.2f);
PrimitiveBase::drawCube();
            glPopMatrix();
        }
        else if (isNorth || isSouth) {
            // Север/Юг - забор вдоль X
            float railLength = m_cellSize * 0.92f;

            // Левый столб
            glPushMatrix();
            glTranslatef(x - m_cellSize / 2.0f + postWidth, postHeight / 2.0f, z);
            glScalef(postWidth, postHeight, postWidth);
PrimitiveBase::drawCube();
            glPopMatrix();

            // Правый столб
            glPushMatrix();
            glTranslatef(x + m_cellSize / 2.0f - postWidth, postHeight / 2.0f, z);
            glScalef(postWidth, postHeight, postWidth);
PrimitiveBase::drawCube();
            glPopMatrix();

            // Верхняя рейка
            glPushMatrix();
            glTranslatef(x, postHeight * 0.7f, z);
            glScalef(railLength, railHeight, postWidth);
PrimitiveBase::drawCube();
            glPopMatrix();

            // Нижняя рейка
            glPushMatrix();
            glTranslatef(x, postHeight * 0.25f, z);
            glScalef(railLength, railHeight, postWidth);
PrimitiveBase::drawCube();
            glPopMatrix();
        }
        else if (isWest || isEast) {
            // Запад/Восток - забор вдоль Z
            float railLength = m_cellSize * 0.92f;

            // Нижний столб
            glPushMatrix();
            glTranslatef(x, postHeight / 2.0f, z - m_cellSize / 2.0f + postWidth);
            glScalef(postWidth, postHeight, postWidth);
PrimitiveBase::drawCube();
            glPopMatrix();

            // Верхний столб
            glPushMatrix();
            glTranslatef(x, postHeight / 2.0f, z + m_cellSize / 2.0f - postWidth);
            glScalef(postWidth, postHeight, postWidth);
PrimitiveBase::drawCube();
            glPopMatrix();

            // Верхняя рейка
            glPushMatrix();
            glTranslatef(x, postHeight * 0.7f, z);
            glScalef(postWidth, railHeight, railLength);
PrimitiveBase::drawCube();
            glPopMatrix();

            // Нижняя рейка
            glPushMatrix();
            glTranslatef(x, postHeight * 0.25f, z);
            glScalef(postWidth, railHeight, railLength);
PrimitiveBase::drawCube();
            glPopMatrix();
        }
    }

    // Включаем обратно
    glEnable(GL_COLOR_MATERIAL);
}

void GameRenderer::drawLightSource() {
    glPushMatrix();

    if (m_lightType == LightType::Directional) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 0.8f);

        glm::vec3 lightPos = -m_lightDir * 5.0f;
        glTranslatef(lightPos.x, lightPos.y, lightPos.z);

        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.2, 8, 8);
        gluDeleteQuadric(quad);

        glEnable(GL_LIGHTING);
    }
    else {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.8f, 0.3f);
        glTranslatef(m_lightPos.x, m_lightPos.y, m_lightPos.z);

        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.25, 8, 8);
        gluDeleteQuadric(quad);

        glEnable(GL_LIGHTING);
    }

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
// ПРИМИТИВЫ
//=============================================================================

void GameRenderer::createPrimitives() {
    std::cout << "Creating primitive models..." << std::endl;

    ApplePrimitive::create(m_appleModel);
    TreePrimitive::create(m_treeModel);
    CloudPrimitive::create(m_cloudModel);
    BirdPrimitive::create(m_birdModel);
    FlowerPrimitive::create(m_flowerModel);
    FencePrimitive::create(m_fenceModel);

    SnakeHeadPrimitive::create(m_snakeHeadModel);
    SnakeBodyPrimitive::create(m_snakeBodyModel);
    SnakeTailPrimitive::create(m_snakeTailModel);

    std::cout << "All primitives created successfully" << std::endl;
}

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

//=============================================================================
// ЗАГРУЗКА МОДЕЛЕЙ
//=============================================================================

void GameRenderer::loadModelsFromConfig(const GameObjects& objects) {
    std::cout << "\n========== LOADING MODELS FROM CONFIG ==========" << std::endl;

    createPrimitives();

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
// ТЕНИ
//=============================================================================

void GameRenderer::toggleShadowMap() {
    m_shadowMapEnabled = !m_shadowMapEnabled;
    std::cout << "Shadow mapping: " << (m_shadowMapEnabled ? "ENABLED" : "DISABLED") << std::endl;

    if (m_shadowMapEnabled) {
        resetShadows();
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
    }
}

void GameRenderer::toggleDebugRays() {
    m_debugRaysEnabled = !m_debugRaysEnabled;
    std::cout << "Debug rays: " << (m_debugRaysEnabled ? "ENABLED" : "DISABLED") << std::endl;

    if (m_debugRaysEnabled && m_shadowMapEnabled) {
        m_staticShadow.enableDebugRays(true);
        m_dynamicShadow.enableDebugRays(true);
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
    }
    else if (!m_debugRaysEnabled) {
        m_staticShadow.enableDebugRays(false);
        m_dynamicShadow.enableDebugRays(false);
    }
}

void GameRenderer::resetShadows() {
    m_staticShadow = ShadowMapper(
        m_gridWidth, m_gridDepth, m_cellSize, 0.0f,
        m_lightDir, m_lightColor, m_lightType, m_lightPos,
        1, 1
    );

    m_dynamicShadow = ShadowMapper(
        m_gridWidth, m_gridDepth, m_cellSize, 0.0f,
        m_lightDir, m_lightColor, m_lightType, m_lightPos,
        1, 1
    );

    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;

    std::cout << "Shadows reset. Total cells: " << (m_gridWidth * m_gridDepth) << std::endl;
}

void GameRenderer::markDynamicShadowsDirty() {
    m_dynamicShadowsDirty = true;
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

    auto staticRays = m_staticShadow.getDebugRays();
    auto dynamicRays = m_dynamicShadow.getDebugRays();

    if (staticRays.empty() && dynamicRays.empty()) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    for (const auto& ray : staticRays) {
        if (ray.hit) {
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        else if (m_showGroundRays) {
            glColor3f(0.2f, 0.6f, 1.0f);
        }
        else {
            continue;
        }

        glBegin(GL_LINES);
        glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
        glVertex3f(ray.hitPoint.x, ray.hitPoint.y, ray.hitPoint.z);
        glEnd();

        if (ray.hit) {
            glColor3f(1.0f, 0.0f, 0.0f);
            drawSphereImmediate(ray.hitPoint, 0.08f);
        }
    }

    for (const auto& ray : dynamicRays) {
        if (ray.hit) {
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        else if (m_showGroundRays) {
            glColor3f(0.2f, 0.6f, 1.0f);
        }
        else {
            continue;
        }

        glBegin(GL_LINES);
        glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
        glVertex3f(ray.hitPoint.x, ray.hitPoint.y, ray.hitPoint.z);
        glEnd();

        if (ray.hit) {
            glColor3f(1.0f, 0.0f, 0.0f);
            drawSphereImmediate(ray.hitPoint, 0.08f);
        }
    }

    glColor3f(0.0f, 1.0f, 0.0f);
    for (const auto& ray : staticRays) {
        drawSphereImmediate(ray.origin, 0.05f);
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

HitInfo GameRenderer::intersectScene(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ) {
    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.distance = 1000.0f;

    float maxDistance = 50.0f;
    float treeSize = m_cellSize * 1.5f;
    float treeHalf = treeSize / 2.0f;

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

    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float z = block.z * m_cellSize - offsetZ;
            float y = block.y * m_cellSize;

            glm::vec3 boxMin(x - treeHalf, y, z - treeHalf);
            glm::vec3 boxMax(x + treeHalf, y + treeSize, z + treeHalf);

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

    const auto& snake = objects.getSnake();
    float snakeSize = m_cellSize * 0.8f;
    float snakeHalf = snakeSize / 2.0f;

    for (size_t i = 0; i < snake.size(); i++) {
        float x = snake[i].x * m_cellSize - offsetX;
        float z = snake[i].z * m_cellSize - offsetZ;
        float y = snake[i].y * m_cellSize + 0.1f;

        glm::vec3 boxMin(x - snakeHalf, y - snakeHalf, z - snakeHalf);
        glm::vec3 boxMax(x + snakeHalf, y + snakeHalf, z + snakeHalf);

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

    float foodRadius = m_cellSize * 0.4f;
    for (const auto& apple : objects.getFood()) {
        float x = apple.x * m_cellSize - offsetX;
        float z = apple.z * m_cellSize - offsetZ;
        float y = apple.y * m_cellSize + 0.1f;

        float tHit;
        if (rayIntersectsSphere(ray, glm::vec3(x, y, z), foodRadius, tHit)) {
            if (tHit > 0.01f && tHit < closestHit.distance) {
                closestHit.hit = true;
                closestHit.distance = tHit;
                closestHit.point = ray.pointAt(tHit);
                closestHit.normal = glm::normalize(closestHit.point - glm::vec3(x, y, z));
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

bool GameRenderer::rayIntersectsAABB(const Ray& ray, const glm::vec3& min, const glm::vec3& max, float& tMin, float& tMax) {
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

void GameRenderer::setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}