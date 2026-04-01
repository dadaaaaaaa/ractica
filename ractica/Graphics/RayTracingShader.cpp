#include "../pch.h"
#include "RayTracingShader.h"
#include "../Objects/GameObjects.h"
#include "../Game/Game.h"
#include <glm/gtc/type_ptr.hpp>

// Внешние переменные (определены в Game.cpp)
extern Game g_game;
extern GameConfig g_config;
extern std::string g_configPath;  // Убираем конфликт, объявляем как extern

// Compute shader source
const char* computeShaderSource = R"(
#version 430 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, binding = 0) uniform image2D outputImage;

uniform int u_width;
uniform int u_height;
uniform int u_frameCount;
uniform vec3 u_cameraPos;
uniform vec3 u_cameraFront;
uniform vec3 u_cameraUp;

// Object buffers
struct SnakeSegment {
    vec3 position;
    float halfSize;
    int type;
    vec3 color;
};

struct Sphere {
    vec3 position;
    float radius;
    vec3 color;
    int type;
};

struct AABB {
    vec3 min;
    vec3 max;
    vec3 color;
    int type;
};

layout(std430, binding = 1) readonly buffer SnakeBuffer {
    SnakeSegment snakeSegments[];
};

layout(std430, binding = 2) readonly buffer FoodBuffer {
    Sphere foodItems[];
};

layout(std430, binding = 3) readonly buffer ObstacleBuffer {
    AABB obstacles[];
};

layout(std430, binding = 4) readonly buffer FenceBuffer {
    AABB fences[];
};

layout(std430, binding = 5) readonly buffer BirdBuffer {
    Sphere birds[];
};

layout(std430, binding = 6) readonly buffer CloudBuffer {
    Sphere clouds[];
};

layout(std430, binding = 7) readonly buffer FlowerBuffer {
    Sphere flowers[];
};

// Параметры сцены
uniform vec3 u_skyColor;
uniform vec3 u_floorColor;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_gridWidth;
uniform float u_gridDepth;
uniform float u_cellSize;

struct HitInfo {
    bool hit;
    float distance;
    vec3 point;
    vec3 normal;
    vec3 color;
};

bool rayIntersectSphere(vec3 origin, vec3 dir, Sphere sphere, out float t) {
    vec3 oc = origin - sphere.position;
    float a = dot(dir, dir);
    float b = 2.0 * dot(oc, dir);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float disc = b * b - 4.0 * a * c;
    
    if (disc < 0.0) return false;
    
    float sqrtDisc = sqrt(disc);
    float t1 = (-b - sqrtDisc) / (2.0 * a);
    float t2 = (-b + sqrtDisc) / (2.0 * a);
    
    t = min(t1, t2);
    if (t < 0.0) t = max(t1, t2);
    
    return t > 0.0;
}

bool rayIntersectAABB(vec3 origin, vec3 dir, AABB box, out float tMin) {
    tMin = 0.0;
    float tMax = 1000.0;
    
    for (int i = 0; i < 3; i++) {
        float invDir = 1.0 / dir[i];
        float t1 = (box.min[i] - origin[i]) * invDir;
        float t2 = (box.max[i] - origin[i]) * invDir;
        
        if (t1 > t2) {
            float temp = t1;
            t1 = t2;
            t2 = temp;
        }
        
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        
        if (tMin > tMax) return false;
    }
    
    return tMin > 0.0;
}

vec3 computeAABBNormal(vec3 point, AABB box) {
    vec3 center = (box.min + box.max) * 0.5;
    vec3 localPoint = point - center;
    
    float dx = abs(localPoint.x);
    float dy = abs(localPoint.y);
    float dz = abs(localPoint.z);
    
    if (dx > dy && dx > dz) {
        return vec3(sign(localPoint.x), 0.0, 0.0);
    } else if (dy > dx && dy > dz) {
        return vec3(0.0, sign(localPoint.y), 0.0);
    } else {
        return vec3(0.0, 0.0, sign(localPoint.z));
    }
}

HitInfo traceScene(vec3 origin, vec3 dir) {
    HitInfo hit;
    hit.hit = false;
    hit.distance = 1000.0;
    
    // Пол (большая плоскость)
    float tGround = -origin.y / dir.y;
    if (tGround > 0 && tGround < hit.distance) {
        vec3 hitPoint = origin + dir * tGround;
        float halfWidth = u_gridWidth * u_cellSize / 2.0;
        float halfDepth = u_gridDepth * u_cellSize / 2.0;
        if (abs(hitPoint.x) <= halfWidth && abs(hitPoint.z) <= halfDepth) {
            hit.hit = true;
            hit.distance = tGround;
            hit.point = hitPoint;
            hit.normal = vec3(0.0, 1.0, 0.0);
            hit.color = u_floorColor;
        }
    }
    
    // Змейка
    for (int i = 0; i < snakeSegments.length(); i++) {
        SnakeSegment seg = snakeSegments[i];
        vec3 minBox = seg.position - vec3(seg.halfSize);
        vec3 maxBox = seg.position + vec3(seg.halfSize);
        AABB box;
        box.min = minBox;
        box.max = maxBox;
        box.color = seg.color;
        
        float t;
        if (rayIntersectAABB(origin, dir, box, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = computeAABBNormal(hit.point, box);
                hit.color = seg.color;
            }
        }
    }
    
    // Еда
    for (int i = 0; i < foodItems.length(); i++) {
        Sphere food = foodItems[i];
        float t;
        if (rayIntersectSphere(origin, dir, food, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = normalize(hit.point - food.position);
                hit.color = food.color;
            }
        }
    }
    
    // Препятствия
    for (int i = 0; i < obstacles.length(); i++) {
        AABB obs = obstacles[i];
        float t;
        if (rayIntersectAABB(origin, dir, obs, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = computeAABBNormal(hit.point, obs);
                hit.color = obs.color;
            }
        }
    }
    
    // Забор
    for (int i = 0; i < fences.length(); i++) {
        AABB fence = fences[i];
        float t;
        if (rayIntersectAABB(origin, dir, fence, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = computeAABBNormal(hit.point, fence);
                hit.color = fence.color;
            }
        }
    }
    
    // Птицы
    for (int i = 0; i < birds.length(); i++) {
        Sphere bird = birds[i];
        float t;
        if (rayIntersectSphere(origin, dir, bird, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = normalize(hit.point - bird.position);
                hit.color = bird.color;
            }
        }
    }
    
    // Облака
    for (int i = 0; i < clouds.length(); i++) {
        Sphere cloud = clouds[i];
        float t;
        if (rayIntersectSphere(origin, dir, cloud, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = normalize(hit.point - cloud.position);
                hit.color = cloud.color;
            }
        }
    }
    
    // Цветы
    for (int i = 0; i < flowers.length(); i++) {
        Sphere flower = flowers[i];
        float t;
        if (rayIntersectSphere(origin, dir, flower, t)) {
            if (t > 0 && t < hit.distance) {
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = normalize(hit.point - flower.position);
                hit.color = flower.color;
            }
        }
    }
    
    return hit;
}

vec3 computeLighting(HitInfo hit, vec3 origin, vec3 dir) {
    vec3 ambient = hit.color * 0.35;
    
    // Проверка тени
    vec3 lightDir = normalize(u_lightDir);
    vec3 shadowRayOrigin = hit.point + hit.normal * 0.01;
    
    HitInfo shadowHit = traceScene(shadowRayOrigin, lightDir);
    float shadow = 1.0;
    if (shadowHit.hit && shadowHit.distance < 10.0) {
        shadow = 0.4;
    }
    
    float diff = max(dot(hit.normal, lightDir), 0.2);
    vec3 diffuse = hit.color * u_lightColor * diff * shadow;
    
    return ambient + diffuse;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= u_width || pixelCoord.y >= u_height) return;
    
    // Генерация луча
    float aspect = float(u_width) / float(u_height);
    float fov = 60.0;
    float tanHalfFov = tan(radians(fov * 0.5));
    float viewportHeight = 2.0 * tanHalfFov;
    float viewportWidth = viewportHeight * aspect;
    
    float x = (float(pixelCoord.x) / float(u_width)) * 2.0 - 1.0;
    float y = 1.0 - (float(pixelCoord.y) / float(u_height)) * 2.0;
    
    vec3 forward = normalize(u_cameraFront);
    vec3 up = normalize(u_cameraUp);
    vec3 right = normalize(cross(forward, up));
    vec3 realUp = normalize(cross(right, forward));
    
    vec3 rayDir = normalize(forward + x * viewportWidth * right + y * viewportHeight * realUp);
    vec3 rayOrigin = u_cameraPos;
    
    // Трассировка
    HitInfo hit = traceScene(rayOrigin, rayDir);
    
    vec3 color;
    if (!hit.hit) {
        // Небо с градиентом
        float t = 0.5 * (rayDir.y + 1.0);
        vec3 skyBottom = vec3(0.6, 0.8, 1.0);
        vec3 skyTop = vec3(0.1, 0.2, 0.5);
        color = mix(skyBottom, skyTop, t);
    } else {
        color = computeLighting(hit, rayOrigin, rayDir);
    }
    
    imageStore(outputImage, pixelCoord, vec4(color, 1.0));
}
)";

RayTracingShader::RayTracingShader()
    : m_programID(0)
    , m_rayTracingTexture(0)
    , m_rayTracingVAO(0)
    , m_rayTracingVBO(0)
    , m_width(0)
    , m_height(0)
    , m_frameCount(0)
    , m_snakeBuffer(0)
    , m_foodBuffer(0)
    , m_obstacleBuffer(0)
    , m_fenceBuffer(0)
    , m_birdBuffer(0)
    , m_cloudBuffer(0)
    , m_flowerBuffer(0)
{
}

RayTracingShader::~RayTracingShader() {
    cleanup();
}

void RayTracingShader::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    // Создаем текстуру для вывода
    glGenTextures(1, &m_rayTracingTexture);
    glBindTexture(GL_TEXTURE_2D, m_rayTracingTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, m_rayTracingTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Создаем VAO для отображения текстуры
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

    // Компилируем compute shader
    unsigned int computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderSource, nullptr);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
        std::cout << "Compute shader compilation error: " << infoLog << std::endl;
    }

    m_programID = glCreateProgram();
    glAttachShader(m_programID, computeShader);
    glLinkProgram(m_programID);

    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        std::cout << "Shader program link error: " << infoLog << std::endl;
    }

    glDeleteShader(computeShader);

    // Получаем uniform locations
    m_widthLoc = glGetUniformLocation(m_programID, "u_width");
    m_heightLoc = glGetUniformLocation(m_programID, "u_height");
    m_frameCountLoc = glGetUniformLocation(m_programID, "u_frameCount");
    m_cameraPosLoc = glGetUniformLocation(m_programID, "u_cameraPos");
    m_cameraFrontLoc = glGetUniformLocation(m_programID, "u_cameraFront");
    m_cameraUpLoc = glGetUniformLocation(m_programID, "u_cameraUp");

    // Создаем буферы для объектов
    glGenBuffers(1, &m_snakeBuffer);
    glGenBuffers(1, &m_foodBuffer);
    glGenBuffers(1, &m_obstacleBuffer);
    glGenBuffers(1, &m_fenceBuffer);
    glGenBuffers(1, &m_birdBuffer);
    glGenBuffers(1, &m_cloudBuffer);
    glGenBuffers(1, &m_flowerBuffer);

    std::cout << "GPU Ray Tracing shader initialized" << std::endl;
}

void RayTracingShader::cleanup() {
    if (m_rayTracingTexture) glDeleteTextures(1, &m_rayTracingTexture);
    if (m_rayTracingVAO) glDeleteVertexArrays(1, &m_rayTracingVAO);
    if (m_rayTracingVBO) glDeleteBuffers(1, &m_rayTracingVBO);
    if (m_programID) glDeleteProgram(m_programID);

    if (m_snakeBuffer) glDeleteBuffers(1, &m_snakeBuffer);
    if (m_foodBuffer) glDeleteBuffers(1, &m_foodBuffer);
    if (m_obstacleBuffer) glDeleteBuffers(1, &m_obstacleBuffer);
    if (m_fenceBuffer) glDeleteBuffers(1, &m_fenceBuffer);
    if (m_birdBuffer) glDeleteBuffers(1, &m_birdBuffer);
    if (m_cloudBuffer) glDeleteBuffers(1, &m_cloudBuffer);
    if (m_flowerBuffer) glDeleteBuffers(1, &m_flowerBuffer);
}

void RayTracingShader::updateObjects(const GameObjects& objects) {
    // Получаем параметры из глобальной конфигурации
    float cellSize = g_config.cellSize;
    float gridWidth = static_cast<float>(g_config.gridWidth);
    float gridDepth = static_cast<float>(g_config.gridDepth);
    float offsetX = gridWidth * cellSize / 2.0f;
    float offsetZ = gridDepth * cellSize / 2.0f;

    // Структуры данных для шейдера
    struct SnakeSegmentData {
        glm::vec3 position;
        float halfSize;
        int type;
        glm::vec3 color;
    };

    struct SphereData {
        glm::vec3 position;
        float radius;
        glm::vec3 color;
        int type;
    };

    struct AABBData {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 color;
        int type;
    };

    // Змейка
    std::vector<SnakeSegmentData> snakeData;
    const auto& snake = objects.getSnake();

    for (size_t i = 0; i < snake.size(); i++) {
        SnakeSegmentData seg;
        seg.position.x = snake[i].x * cellSize - offsetX;
        seg.position.z = snake[i].z * cellSize - offsetZ;
        seg.position.y = snake[i].y * cellSize + 0.1f;

        float scale = cellSize * 0.8f;
        if (i == 0) {
            seg.type = 0;
            seg.color = objects.getSnakeHeadColor();
            scale *= objects.getSnakeHeadScale();
        }
        else if (i == snake.size() - 1) {
            seg.type = 2;
            seg.color = objects.getSnakeTailColor();
            scale *= objects.getSnakeTailScale();
        }
        else {
            seg.type = 1;
            seg.color = objects.getSnakeBodyColor();
            scale *= objects.getSnakeBodyScale();
        }
        seg.halfSize = scale / 2.0f;
        snakeData.push_back(seg);
    }

    if (!snakeData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_snakeBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, snakeData.size() * sizeof(SnakeSegmentData),
            snakeData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_snakeBuffer);
    }

    // Еда
    std::vector<SphereData> foodData;
    const auto& food = objects.getFood();
    float foodRadius = (cellSize * 0.8f) / 2.0f;

    for (const auto& f : food) {
        SphereData sphere;
        sphere.position.x = f.x * cellSize - offsetX;
        sphere.position.z = f.z * cellSize - offsetZ;
        sphere.position.y = f.y * cellSize + 0.1f;
        sphere.radius = foodRadius;
        sphere.color = glm::vec3(1.0f, 0.8f, 0.2f);
        sphere.type = 0;
        foodData.push_back(sphere);
    }

    if (!foodData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_foodBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, foodData.size() * sizeof(SphereData),
            foodData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_foodBuffer);
    }

    // Препятствия (деревья)
    std::vector<AABBData> obstacleData;
    float treeScale = cellSize * 1.5f;
    float treeHalf = treeScale / 2.0f;

    for (const auto& obstacle : objects.getObstacles()) {
        for (const auto& block : obstacle.blocks) {
            AABBData aabb;
            aabb.min.x = block.x * cellSize - offsetX - treeHalf;
            aabb.min.z = block.z * cellSize - offsetZ - treeHalf;
            aabb.min.y = block.y * cellSize;
            aabb.max.x = block.x * cellSize - offsetX + treeHalf;
            aabb.max.z = block.z * cellSize - offsetZ + treeHalf;
            aabb.max.y = block.y * cellSize + treeScale;
            aabb.color = glm::vec3(0.1f, 0.4f, 0.1f);
            aabb.type = 0;
            obstacleData.push_back(aabb);
        }
    }

    if (!obstacleData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_obstacleBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, obstacleData.size() * sizeof(AABBData),
            obstacleData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_obstacleBuffer);
    }

    // Забор
    std::vector<AABBData> fenceData;
    float fenceSize = cellSize * 0.6f;
    float fenceHalf = fenceSize / 2.0f;

    for (const auto& block : objects.getFenceBlocks()) {
        AABBData aabb;
        aabb.min.x = block.x * cellSize - offsetX - fenceHalf;
        aabb.min.z = block.z * cellSize - offsetZ - fenceHalf;
        aabb.min.y = block.y * cellSize;
        aabb.max.x = block.x * cellSize - offsetX + fenceHalf;
        aabb.max.z = block.z * cellSize - offsetZ + fenceHalf;
        aabb.max.y = block.y * cellSize + fenceSize;
        aabb.color = glm::vec3(0.55f, 0.27f, 0.07f);
        aabb.type = 1;
        fenceData.push_back(aabb);
    }

    if (!fenceData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fenceBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, fenceData.size() * sizeof(AABBData),
            fenceData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_fenceBuffer);
    }

    // Птицы
    std::vector<SphereData> birdData;
    for (const auto& bird : objects.getBirds()) {
        SphereData sphere;
        sphere.position = bird.position;
        sphere.radius = 0.25f;
        sphere.color = bird.color;
        sphere.type = 1;
        birdData.push_back(sphere);
    }

    if (!birdData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_birdBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, birdData.size() * sizeof(SphereData),
            birdData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_birdBuffer);
    }

    // Облака
    std::vector<SphereData> cloudData;
    for (const auto& cloud : objects.getCloudSprites()) {
        SphereData sphere;
        sphere.position = cloud.position;
        sphere.radius = 0.8f;
        sphere.color = cloud.color;
        sphere.type = 2;
        cloudData.push_back(sphere);
    }

    if (!cloudData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_cloudBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, cloudData.size() * sizeof(SphereData),
            cloudData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_cloudBuffer);
    }

    // Цветы
    std::vector<SphereData> flowerData;
    for (const auto& flower : objects.getFlowerSprites()) {
        SphereData sphere;
        sphere.position = flower.position;
        sphere.position.y += 0.05f;
        sphere.radius = 0.1f;
        sphere.color = flower.color;
        sphere.type = 3;
        flowerData.push_back(sphere);
    }

    if (!flowerData.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_flowerBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, flowerData.size() * sizeof(SphereData),
            flowerData.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, m_flowerBuffer);
    }
}

void RayTracingShader::render(const glm::vec3& cameraPos, const glm::vec3& cameraFront, const glm::vec3& cameraUp) {
    glUseProgram(m_programID);

    // Устанавливаем uniform'ы
    glUniform1i(m_widthLoc, m_width);
    glUniform1i(m_heightLoc, m_height);
    glUniform1i(m_frameCountLoc, m_frameCount++);
    glUniform3f(m_cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(m_cameraFrontLoc, cameraFront.x, cameraFront.y, cameraFront.z);
    glUniform3f(m_cameraUpLoc, cameraUp.x, cameraUp.y, cameraUp.z);

    // Получаем locations для uniform'ов
    GLint skyColorLoc = glGetUniformLocation(m_programID, "u_skyColor");
    GLint floorColorLoc = glGetUniformLocation(m_programID, "u_floorColor");
    GLint lightDirLoc = glGetUniformLocation(m_programID, "u_lightDir");
    GLint lightColorLoc = glGetUniformLocation(m_programID, "u_lightColor");
    GLint gridWidthLoc = glGetUniformLocation(m_programID, "u_gridWidth");
    GLint gridDepthLoc = glGetUniformLocation(m_programID, "u_gridDepth");
    GLint cellSizeLoc = glGetUniformLocation(m_programID, "u_cellSize");

    // Устанавливаем цвета из конфига
    if (skyColorLoc != -1) {
        glUniform3f(skyColorLoc, g_config.skyColor.r, g_config.skyColor.g, g_config.skyColor.b);
    }
    if (floorColorLoc != -1) {
        glUniform3f(floorColorLoc, g_config.floorColor.r, g_config.floorColor.g, g_config.floorColor.b);
    }

    // Свет
    if (lightDirLoc != -1) {
        glUniform3f(lightDirLoc, 1.0f, 0.2f, 0.5f);
    }
    if (lightColorLoc != -1) {
        glUniform3f(lightColorLoc, 0.9f, 0.85f, 0.75f);
    }

    // Размеры мира
    if (gridWidthLoc != -1) {
        glUniform1f(gridWidthLoc, static_cast<float>(g_config.gridWidth));
    }
    if (gridDepthLoc != -1) {
        glUniform1f(gridDepthLoc, static_cast<float>(g_config.gridDepth));
    }
    if (cellSizeLoc != -1) {
        glUniform1f(cellSizeLoc, g_config.cellSize);
    }

    // Запускаем compute shader
    int workGroupX = (m_width + 7) / 8;
    int workGroupY = (m_height + 7) / 8;
    glDispatchCompute(workGroupX, workGroupY, 1);

    // Ждем завершения
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Отображаем текстуру на экран
    glDisable(GL_DEPTH_TEST);

    GLuint screenShader = glCreateProgram();
    const char* vs = "#version 330 core\nlayout(location=0) in vec2 aPos;layout(location=1) in vec2 aTexCoord;out vec2 TexCoord;void main(){gl_Position=vec4(aPos,0,1);TexCoord=aTexCoord;}";
    const char* fs = "#version 330 core\nin vec2 TexCoord;out vec4 FragColor;uniform sampler2D uTexture;void main(){FragColor=texture(uTexture,TexCoord);}";

    GLuint vsShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsShader, 1, &vs, nullptr);
    glCompileShader(vsShader);

    GLuint fsShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsShader, 1, &fs, nullptr);
    glCompileShader(fsShader);

    glAttachShader(screenShader, vsShader);
    glAttachShader(screenShader, fsShader);
    glLinkProgram(screenShader);

    glUseProgram(screenShader);
    glUniform1i(glGetUniformLocation(screenShader, "uTexture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_rayTracingTexture);

    glBindVertexArray(m_rayTracingVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDeleteShader(vsShader);
    glDeleteShader(fsShader);
    glDeleteProgram(screenShader);

    glEnable(GL_DEPTH_TEST);
}
