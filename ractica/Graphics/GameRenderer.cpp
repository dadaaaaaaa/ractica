#include "../pch.h"
#include "GameRenderer.h"
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
#include "Model.h"
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
    , shadow_map(false)
    , m_shadowStrideX(8)
    , m_shadowStrideZ(8)
    , m_staticShadowsDirty(true)
    , m_dynamicShadowsDirty(true)
    , m_debugRaysEnabled(false)
    , m_showGroundRays(false)
{
    m_lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.5f));
    m_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_lightType = LightType::Directional;
    m_lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
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

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Настройка сглаживания
    GLint samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (samples > 0) {
        glEnable(GL_MULTISAMPLE);
        std::cout << "MSAA enabled: " << samples << "x samples" << std::endl;
    }

    std::cout << "OpenGL initialized with Fixed Pipeline" << std::endl;
}

void GameRenderer::setupFixedPipelineLighting() {
    // Включаем освещение
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);

    // Настройка материала
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 32);

    // Глобальная ambient освещенность
    GLfloat global_ambient[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    // Настройка направленного света (солнце) - LIGHT0
    GLfloat light0_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light0_diffuse[] = { 0.9f, 0.9f, 0.85f, 1.0f };
    GLfloat light0_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    // Дополнительный рассеянный свет снизу - LIGHT1
    GLfloat light1_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat light1_diffuse[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light1_specular[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat light1_position[] = { 0.0f, -1.0f, 0.0f, 0.0f };

    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);

    std::cout << "Fixed pipeline lighting configured" << std::endl;
}

void GameRenderer::updateLightPosition() {
    if (m_lightType == LightType::Directional) {
        // Направленный свет (бесконечность)
        GLfloat light0_position[] = {
            -m_lightDir.x, -m_lightDir.y, -m_lightDir.z, 0.0f
        };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    }
    else {
        // Точечный свет или прожектор
        GLfloat light0_position[] = {
            m_lightPos.x, m_lightPos.y, m_lightPos.z, 1.0f
        };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

        if (m_lightType == LightType::Spot) {
            GLfloat spot_direction[] = { m_lightDir.x, m_lightDir.y, m_lightDir.z };
            glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
            glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0f);
            glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 2.0f);
        }
    }
}

void GameRenderer::setupMaterial(const glm::vec3& color, float shininess) {
    GLfloat ambient[] = { color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, 1.0f };
    GLfloat diffuse[] = { color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 1.0f };
    GLfloat specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };

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
    auto frameStartTime = std::chrono::high_resolution_clock::now();
    static int frameCount = 0;
    static float totalFrameTime = 0.0f;

    if (m_rayTracingEnabled) {
        renderWithRayTracing(objects);
        return;
    }

    resetDepthState();

    // Очистка буферов
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Включаем тест глубины
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Настройка проекции
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1200.0 / 800.0, 0.2, 100.0);

    // Настройка вида (камера)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glm::vec3 camPos = g_camera.getPosition();
    glm::vec3 camTarget = camPos + g_camera.getFront();
    glm::vec3 camUp = g_camera.getUp();

    gluLookAt(camPos.x, camPos.y, camPos.z,
        camTarget.x, camTarget.y, camTarget.z,
        camUp.x, camUp.y, camUp.z);

    // Обновляем позицию источника света
    updateLightPosition();

    // Рисуем все объекты
    auto drawStart = std::chrono::high_resolution_clock::now();

    drawFloor();
    drawObstaclesAsTrees(objects.getObstacles());
    drawFence(objects.getFenceBlocks());
    drawFood(objects.getFood());
    drawSnake(objects.getSnake());
    drawGroundSprites(objects.getFlowerSprites());
    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
    drawLightSource();

    auto drawEnd = std::chrono::high_resolution_clock::now();
    auto drawMs = std::chrono::duration_cast<std::chrono::milliseconds>(drawEnd - drawStart).count();

    // Отладочные лучи
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawDebugRaysIfEnabled();

    // Статистика производительности
    auto frameEndTime = std::chrono::high_resolution_clock::now();
    auto frameMs = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - frameStartTime).count();

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

//=============================================================================
// ОТРИСОВКА ОБЪЕКТОВ
//=============================================================================

void GameRenderer::drawFloor() {
    glPushMatrix();

    float worldWidth = m_gridWidth * m_cellSize;
    float worldDepth = m_gridDepth * m_cellSize;
    float offsetX = worldWidth / 2.0f;
    float offsetZ = worldDepth / 2.0f;

    // Рисуем сетку пола
    if (gridEnabled) {
        glDisable(GL_LIGHTING);
        glLineWidth(gridLineWidth);
        glColor3f(gridColor.r, gridColor.g, gridColor.b);

        glBegin(GL_LINES);
        // Линии по X
        for (int i = 0; i <= m_gridWidth; i++) {
            float x = i * m_cellSize - offsetX;
            glVertex3f(x, 0.01f, -offsetZ);
            glVertex3f(x, 0.01f, worldDepth - offsetZ);
        }
        // Линии по Z
        for (int i = 0; i <= m_gridDepth; i++) {
            float z = i * m_cellSize - offsetZ;
            glVertex3f(-offsetX, 0.01f, z);
            glVertex3f(worldWidth - offsetX, 0.01f, z);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    }

    // Рисуем сам пол
    setupMaterial(floorColor, 16.0f);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-offsetX, -0.05f, -offsetZ);
    glVertex3f(worldWidth - offsetX, -0.05f, -offsetZ);
    glVertex3f(worldWidth - offsetX, -0.05f, worldDepth - offsetZ);
    glVertex3f(-offsetX, -0.05f, worldDepth - offsetZ);
    glEnd();

    glPopMatrix();
}

void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    if (snake.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];

        float x = segment.x * m_cellSize - offsetX;
        float y = segment.y * m_cellSize + 0.1f;
        float z = segment.z * m_cellSize - offsetZ;

        glm::vec3 color;
        float scale = m_cellSize * 0.8f;

        if (i == 0) {
            color = g_game.getSnakeHeadColor();
            scale *= g_game.getSnakeHeadScale();
        }
        else if (i == snake.size() - 1) {
            color = g_game.getSnakeTailColor();
            scale *= g_game.getSnakeTailScale();
        }
        else {
            color = g_game.getSnakeBodyColor();
            scale *= g_game.getSnakeBodyScale();
        }

        float rotationAngle = calculateSegmentRotation(snake, i);

        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
        glScalef(scale, scale, scale);

        setupMaterial(color, 64.0f);
        glDisable(GL_TEXTURE_2D);

        // Рисуем куб для сегмента змейки
        glBegin(GL_QUADS);
        // Передняя грань (Z+)
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);

        // Задняя грань (Z-)
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);

        // Левая грань (X-)
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);

        // Правая грань (X+)
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);

        // Верхняя грань (Y+)
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);

        // Нижняя грань (Y-)
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glEnd();

        // Глаза для головы
        if (i == 0) {
            glDisable(GL_LIGHTING);
            glColor3f(1.0f, 1.0f, 1.0f);
            glPointSize(4.0f);
            glBegin(GL_POINTS);
            glVertex3f(0.35f, 0.35f, 0.51f);
            glVertex3f(-0.35f, 0.35f, 0.51f);
            glEnd();
            glEnable(GL_LIGHTING);
        }

        glPopMatrix();
    }
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

void GameRenderer::drawFood(const std::vector<Point>& food) {
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    GLUquadric* quad = gluNewQuadric();

    for (const auto& apple : food) {
        float x = apple.x * m_cellSize - offsetX;
        float y = apple.y * m_cellSize + 0.1f;
        float z = apple.z * m_cellSize - offsetZ;

        glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(m_cellSize * 0.6f, m_cellSize * 0.6f, m_cellSize * 0.6f);

        setupMaterial(glm::vec3(1.0f, 0.8f, 0.2f), 32.0f);
        glDisable(GL_TEXTURE_2D);

        gluSphere(quad, 0.5, 16, 16);

        glPopMatrix();
    }

    gluDeleteQuadric(quad);
}

void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    GLUquadric* quad = gluNewQuadric();

    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = block.x * m_cellSize - offsetX;
            float y = block.y * m_cellSize;
            float z = block.z * m_cellSize - offsetZ;

            glPushMatrix();
            glTranslatef(x, y, z);

            // Ствол
            glPushMatrix();
            glScalef(m_cellSize * 0.4f, m_cellSize * 0.8f, m_cellSize * 0.4f);
            setupMaterial(glm::vec3(0.55f, 0.27f, 0.07f), 16.0f);
            gluCylinder(quad, 0.5, 0.5, 1.0, 8, 8);
            glPopMatrix();

            // Крона (нижний ярус)
            glPushMatrix();
            glTranslatef(0.0f, m_cellSize * 0.6f, 0.0f);
            glScalef(m_cellSize * 0.9f, m_cellSize * 0.7f, m_cellSize * 0.9f);
            setupMaterial(glm::vec3(0.1f, 0.5f, 0.1f), 32.0f);
            gluSphere(quad, 0.6, 12, 12);
            glPopMatrix();

            // Крона (верхний ярус)
            glPushMatrix();
            glTranslatef(0.0f, m_cellSize * 1.0f, 0.0f);
            glScalef(m_cellSize * 0.7f, m_cellSize * 0.6f, m_cellSize * 0.7f);
            gluSphere(quad, 0.5, 12, 12);
            glPopMatrix();

            glPopMatrix();
        }
    }

    gluDeleteQuadric(quad);
}

void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    if (fenceBlocks.empty()) return;

    float offsetX = m_gridWidth * m_cellSize / 2.0f;
    float offsetZ = m_gridDepth * m_cellSize / 2.0f;

    for (const auto& fenceBlock : fenceBlocks) {
        float x = fenceBlock.x * m_cellSize - offsetX;
        float y = fenceBlock.y * m_cellSize;
        float z = fenceBlock.z * m_cellSize - offsetZ;

        bool isCorner = (fenceBlock.x == 0 && fenceBlock.z == 0) ||
            (fenceBlock.x == m_gridWidth - 1 && fenceBlock.z == 0) ||
            (fenceBlock.x == 0 && fenceBlock.z == m_gridDepth - 1) ||
            (fenceBlock.x == m_gridWidth - 1 && fenceBlock.z == m_gridDepth - 1);

        bool isNorth = (fenceBlock.z == 0);
        bool isSouth = (fenceBlock.z == m_gridDepth - 1);
        bool isWest = (fenceBlock.x == 0);
        bool isEast = (fenceBlock.x == m_gridWidth - 1);

        glPushMatrix();

        if (isCorner) {
            // Угловой столб - рисуем куб через glBegin/glEnd
            glTranslatef(x, y + m_cellSize * 0.2f, z);
            glScalef(m_cellSize * 0.8f, m_cellSize * 0.4f, m_cellSize * 0.8f);
            setupMaterial(glm::vec3(0.55f, 0.27f, 0.07f), 16.0f);
            glDisable(GL_TEXTURE_2D);

            drawCube();
        }
        else if (isNorth || isSouth || isWest || isEast) {
            // Обычный столб забора
            glTranslatef(x, y + m_cellSize * 0.2f, z);
            glScalef(m_cellSize * 0.6f, m_cellSize * 0.4f, m_cellSize * 0.6f);
            setupMaterial(glm::vec3(0.55f, 0.27f, 0.07f), 16.0f);
            glDisable(GL_TEXTURE_2D);

            drawCube();

            // Горизонтальные перекладины
            glPushMatrix();
            if (isNorth || isSouth) {
                glTranslatef(0.0f, -0.3f, 0.0f);
                glScalef(1.6f, 0.2f, 0.3f);
            }
            else {
                glTranslatef(0.0f, -0.3f, 0.0f);
                glScalef(0.3f, 0.2f, 1.6f);
            }
            setupMaterial(glm::vec3(0.45f, 0.17f, 0.05f), 16.0f);
            drawCube();
            glPopMatrix();
        }

        glPopMatrix();
    }
}

// Добавить этот метод в класс GameRenderer:
void GameRenderer::drawCube() {
    glBegin(GL_QUADS);
    // Передняя грань
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);

    // Задняя грань
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);

    // Левая грань
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);

    // Правая грань
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);

    // Верхняя грань
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);

    // Нижняя грань
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glEnd();
}

// Вспомогательная функция для куба через GLU
static void gluCube(GLUquadric* quad, float size) {
    // Простая реализация куба через glBegin/glEnd
    float h = size / 2.0f;
    glBegin(GL_QUADS);
    // Передняя грань
    glNormal3f(0, 0, 1);
    glVertex3f(-h, -h, h);
    glVertex3f(h, -h, h);
    glVertex3f(h, h, h);
    glVertex3f(-h, h, h);
    // Задняя грань
    glNormal3f(0, 0, -1);
    glVertex3f(-h, -h, -h);
    glVertex3f(-h, h, -h);
    glVertex3f(h, h, -h);
    glVertex3f(h, -h, -h);
    // Левая грань
    glNormal3f(-1, 0, 0);
    glVertex3f(-h, -h, -h);
    glVertex3f(-h, -h, h);
    glVertex3f(-h, h, h);
    glVertex3f(-h, h, -h);
    // Правая грань
    glNormal3f(1, 0, 0);
    glVertex3f(h, -h, -h);
    glVertex3f(h, h, -h);
    glVertex3f(h, h, h);
    glVertex3f(h, -h, h);
    // Верхняя грань
    glNormal3f(0, 1, 0);
    glVertex3f(-h, h, -h);
    glVertex3f(-h, h, h);
    glVertex3f(h, h, h);
    glVertex3f(h, h, -h);
    // Нижняя грань
    glNormal3f(0, -1, 0);
    glVertex3f(-h, -h, -h);
    glVertex3f(h, -h, -h);
    glVertex3f(h, -h, h);
    glVertex3f(-h, -h, h);
    glEnd();
}

void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
    GLUquadric* quad = gluNewQuadric();

    for (const auto& cloud : cloudSprites) {
        float distanceToCenter = glm::length(glm::vec2(cloud.position.x, cloud.position.z));
        if (distanceToCenter < 8.0f) continue;

        glPushMatrix();
        glTranslatef(cloud.position.x, cloud.position.y, cloud.position.z);
        glScalef(cloud.size, cloud.size, cloud.size);

        setupMaterial(cloud.color, 16.0f);
        glDisable(GL_TEXTURE_2D);

        // Облако из нескольких сфер
        glPushMatrix();
        glTranslatef(-0.6f, 0.0f, 0.0f);
        gluSphere(quad, 0.4, 12, 12);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.6f, 0.0f, 0.0f);
        gluSphere(quad, 0.4, 12, 12);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, 0.3f, 0.0f);
        gluSphere(quad, 0.5, 12, 12);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, -0.2f, 0.0f);
        gluSphere(quad, 0.45, 12, 12);
        glPopMatrix();

        glPopMatrix();
    }

    gluDeleteQuadric(quad);
}

void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
    for (const auto& bird : birds) {
        glPushMatrix();
        glTranslatef(bird.position.x, bird.position.y, bird.position.z);

        // Поворот в направлении полёта
        if (glm::length(bird.direction) > 0.1f) {
            float angle = atan2f(bird.direction.x, bird.direction.z) * 180.0f / 3.14159f;
            glRotatef(angle, 0.0f, 1.0f, 0.0f);

            float pitch = atan2f(bird.direction.y,
                glm::length(glm::vec2(bird.direction.x, bird.direction.z))) * 180.0f / 3.14159f;
            glRotatef(pitch, 1.0f, 0.0f, 0.0f);
        }

        glScalef(bird.size, bird.size, bird.size);
        setupMaterial(bird.color, 32.0f);
        glDisable(GL_TEXTURE_2D);

        // ИСПРАВЛЕНО: добавил объявление wingOffset
        float wingOffset = sin(glfwGetTime() * 8.0f) * 0.15f;

        // Простая птица из треугольников
        glBegin(GL_TRIANGLES);
        // Тело
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-0.3f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.2f, 0.0f);
        glVertex3f(0.3f, 0.0f, 0.0f);

        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-0.3f, 0.0f, 0.0f);
        glVertex3f(0.0f, -0.2f, 0.0f);
        glVertex3f(0.3f, 0.0f, 0.0f);

        // Крылья с анимацией - ИСПРАВЛЕНО: используем wingOffset
        glNormal3f(-0.5f, 0.5f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(-0.4f, 0.15f + wingOffset, -0.2f);
        glVertex3f(-0.2f, 0.0f, -0.1f);

        glNormal3f(0.5f, 0.5f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.4f, 0.15f + wingOffset, -0.2f);
        glVertex3f(0.2f, 0.0f, -0.1f);
        glEnd();

        glPopMatrix();
    }
}

void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    GLUquadric* quad = gluNewQuadric();

    for (const auto& flower : flowerSprites) {
        glPushMatrix();
        glTranslatef(flower.position.x, flower.position.y + 0.05f, flower.position.z);
        glScalef(flower.size, flower.size, flower.size);

        setupMaterial(flower.color, 32.0f);
        glDisable(GL_TEXTURE_2D);

        // Стебель
        glPushMatrix();
        glScalef(0.05f, 0.5f, 0.05f);
        gluCylinder(quad, 0.5, 0.3, 1.0, 4, 4);
        glPopMatrix();

        // Лепестки
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < 6; i++) {
            float angle = i * 60.0f * 3.14159f / 180.0f;
            float x = cos(angle) * 0.2f;
            float z = sin(angle) * 0.2f;

            glNormal3f(x, 0.5f, z);
            glVertex3f(0.0f, 0.25f, 0.0f);
            glVertex3f(x, 0.05f, z);
            glVertex3f(0.0f, 0.05f, 0.0f);
        }
        glEnd();

        // Серединка
        glColor3f(1.0f, 0.8f, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, 0.18f, 0.0f);
        gluSphere(quad, 0.08, 6, 6);
        glPopMatrix();

        glPopMatrix();
    }

    gluDeleteQuadric(quad);
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

    setupMaterial(color);
    setupTexture(model.hasTexture ? model.textureID : 0);

    if (model.vertices.size() > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        if (model.hasTexture) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &model.vertices[0].position);
        glNormalPointer(GL_FLOAT, sizeof(Vertex), &model.vertices[0].normal);
        if (model.hasTexture) {
            glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &model.vertices[0].texCoords);
        }

        glDrawArrays(GL_TRIANGLES, 0, model.vertices.size());

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        if (model.hasTexture) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }

    glPopMatrix();
}

void GameRenderer::drawModelWithRotation(const Model& model, float x, float y, float z,
    float scale, const glm::vec3& color, float rotationAngle) {

    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    setupMaterial(color);
    setupTexture(model.hasTexture ? model.textureID : 0);

    if (model.vertices.size() > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        if (model.hasTexture) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &model.vertices[0].position);
        glNormalPointer(GL_FLOAT, sizeof(Vertex), &model.vertices[0].normal);
        if (model.hasTexture) {
            glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &model.vertices[0].texCoords);
        }

        glDrawArrays(GL_TRIANGLES, 0, model.vertices.size());

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        if (model.hasTexture) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }

    glPopMatrix();
}

//=============================================================================
// ПРИМИТИВЫ
//=============================================================================

void GameRenderer::createPrimitives() {
    std::cout << "Creating primitive fallback models..." << std::endl;

    PrimitiveBase::createCube(cubePrimitive);
    PrimitiveBase::createSphere(spherePrimitive);
    ApplePrimitive::create(appleModel);
    TreePrimitive::create(treeModel);
    CloudPrimitive::create(cloudModel);
    BirdPrimitive::create(birdModel);
    FlowerPrimitive::create(flowerModel);

    std::cout << "Primitives created successfully" << std::endl;
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

    // Здесь можно добавить загрузку моделей из FBX/OBJ файлов
    // Для fixed pipeline используем только примитивы

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

void GameRenderer::resetShadows() {
    m_staticShadow = ShadowMapper(
        m_gridWidth, m_gridDepth, m_cellSize, gridLineWidth,
        m_lightDir, m_lightColor, m_lightType, m_lightPos,
        m_shadowStrideX, m_shadowStrideZ
    );

    m_dynamicShadow = m_staticShadow;

    m_staticShadowsDirty = true;
    m_dynamicShadowsDirty = true;

    std::cout << "Shadows reset. Total samples: "
        << (m_gridWidth * m_gridDepth) << std::endl;
}

void GameRenderer::markDynamicShadowsDirty() {
    m_dynamicShadowsDirty = true;
}

void GameRenderer::renderShadowMap() {
    // Упрощённая версия теней для fixed pipeline
    // Просто затемняем область в зависимости от позиции
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
    if (!m_debugRaysEnabled) return;

    auto staticRays = m_staticShadow.getDebugRays();
    auto dynamicRays = m_dynamicShadow.getDebugRays();

    if (staticRays.empty() && dynamicRays.empty()) return;

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);

    // Рисуем красные линии (попавшие в объекты)
    for (const auto& ray : staticRays) {
        if (ray.hit) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
            glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
            glVertex3f(ray.hitPoint.x, ray.hitPoint.y, ray.hitPoint.z);
            glEnd();
        }
        else if (m_showGroundRays) {
            glColor3f(0.2f, 0.6f, 1.0f);
            glBegin(GL_LINES);
            glVertex3f(ray.origin.x, ray.origin.y, ray.origin.z);
            glVertex3f(ray.hitPoint.x, ray.hitPoint.y, ray.hitPoint.z);
            glEnd();
        }
    }

    glEnable(GL_LIGHTING);
}

void GameRenderer::drawDebugRays(const std::vector<DebugRay>& rays, float lineWidth) {
    // Реализация для отладки
}

void GameRenderer::drawRay(const DebugRay& ray, const glm::vec3& color) {
    // Реализация для отладки
}

void GameRenderer::toggleDebugRays() {
    m_debugRaysEnabled = !m_debugRaysEnabled;

    m_staticShadow.enableDebugRays(m_debugRaysEnabled);
    m_dynamicShadow.enableDebugRays(m_debugRaysEnabled);

    if (m_debugRaysEnabled) {
        m_staticShadowsDirty = true;
        m_dynamicShadowsDirty = true;
        shadow_map = true;
    }
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
    // Упрощённая инициализация для fixed pipeline
    std::cout << "Ray tracing resources initialized (simplified for fixed pipeline)" << std::endl;
}

void GameRenderer::cleanupRayTracingResources() {
    // Очистка ресурсов
}

void GameRenderer::renderWithRayTracing(const GameObjects& objects) {
    // Упрощённая версия для fixed pipeline
    std::cout << "Ray tracing not fully implemented in fixed pipeline mode" << std::endl;
}

glm::vec3 GameRenderer::traceRay(const Ray& ray, const GameObjects& objects, float offsetX, float offsetZ, int depth) {
    // Упрощённая версия
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
    float offsetX = 120 * 0.1f / 2.0f;  // m_gridWidth * m_cellSize / 2
    float offsetZ = 120 * 0.1f / 2.0f;

    float x = segment.x * 0.1f - offsetX;
    float z = segment.z * 0.1f - offsetZ;
    float y = segment.y * 0.1f + 0.1f;

    return glm::vec3(x, y, z);
}

glm::vec3 GameRenderer::getSnakeSegmentScale(const Point& segment, size_t index) {
    float baseScale = 0.1f * 0.8f;
    float scale = baseScale;

    if (index == 0) scale *= 1.2f;
    else if (index == g_game.getSnake().size() - 1) scale *= 0.9f;
    else scale *= 1.0f;

    return glm::vec3(scale);
}

glm::vec3 GameRenderer::getFoodPosition(const Point& food) {
    float offsetX = 120 * 0.1f / 2.0f;
    float offsetZ = 120 * 0.1f / 2.0f;

    float x = food.x * 0.1f - offsetX;
    float z = food.z * 0.1f - offsetZ;
    float y = food.y * 0.1f + 0.1f;

    return glm::vec3(x, y, z);
}

glm::vec3 GameRenderer::getObstaclePosition(const Point& block) {
    float offsetX = 120 * 0.1f / 2.0f;
    float offsetZ = 120 * 0.1f / 2.0f;

    float x = block.x * 0.1f - offsetX;
    float z = block.z * 0.1f - offsetZ;
    float y = block.y * 0.1f;

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
    glGetError(); // Игнорируем первую ошибку
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