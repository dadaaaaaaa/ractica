#include "../pch.h"
#include "GameRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Camera.h"
#include "../Core/Constants.h"
#include "../Utils/MathUtils.h"

// √лобальные экземпл€ры
extern ShaderManager g_shaderManager;
extern Camera g_camera;

GameRenderer::GameRenderer() {
   
}

void GameRenderer::initialize() {
    loadAllModels();
}

void GameRenderer::renderGame(const GameObjects& objects) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    g_shaderManager.use3DShader();

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(g_camera.getPosition(), g_camera.getPosition() + g_camera.getFront(), g_camera.getUp());

    g_shaderManager.setViewMatrix(view);
    g_shaderManager.setProjectionMatrix(projection);

    drawFloor();
    drawGroundSprites(objects.getFlowerSprites());
    drawFence(objects.getFenceBlocks());
    drawObstaclesAsTrees(objects.getObstacles());
    drawSnake(objects.getSnake());
    drawFood(objects.getFood());
    drawClouds(objects.getCloudSprites());
    drawBirds(objects.getBirds());
}

void GameRenderer::drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    g_shaderManager.setModelMatrix(modelMatrix);
    g_shaderManager.setColor(color);
    g_shaderManager.setUseTexture(model.hasTexture);

    model.draw();
}

void GameRenderer::drawFloor() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
    model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));

    g_shaderManager.setModelMatrix(model);
    g_shaderManager.setColor(glm::vec3(0.3f, 0.6f, 0.2f));
    g_shaderManager.setUseTexture(floorModel.hasTexture);

    floorModel.draw();
}

void GameRenderer::drawSnake(const std::vector<Point>& snake) {
    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];
        float x = (segment.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = segment.y * CELL_SIZE + 0.05f;
        float z = (segment.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 color = (i == 0) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.7f, 0.0f);
        drawModel(snakeModel, x, y, z, CELL_SIZE * 0.8f, color);
    }
}

void GameRenderer::drawFood(const std::vector<Point>& food) {
    for (const auto& apple : food) {
        float x = (apple.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = apple.y * CELL_SIZE + 0.05f;
        float z = (apple.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        drawModel(appleModel, x, y, z, CELL_SIZE * 0.8f, glm::vec3(1.0f, 0.8f, 0.2f));
    }
}

void GameRenderer::drawObstaclesAsTrees(const std::vector<Obstacle>& obstacles) {
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = (block.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
            float y = block.y * CELL_SIZE;
            float z = (block.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

            drawModel(treeModel, x, y, z, CELL_SIZE * 1.5f, glm::vec3(0.1f, 0.4f, 0.1f));
        }
    }
}

void GameRenderer::drawFence(const std::vector<Point>& fenceBlocks) {
    for (const auto& fenceBlock : fenceBlocks) {
        float x = (fenceBlock.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = fenceBlock.y * CELL_SIZE;
        float z = (fenceBlock.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 fenceColor(0.55f, 0.27f, 0.07f);
        drawModel(fenceModel, x, y, z, CELL_SIZE * 1.2f, fenceColor);
    }
}

void GameRenderer::drawClouds(const std::vector<Sprite>& cloudSprites) {
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
        g_shaderManager.setUseTexture(cloudModel.hasTexture);

        cloudModel.draw();
    }
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
    g_shaderManager.setColor(bird.color);
    g_shaderManager.setUseTexture(birdModel.hasTexture);

    birdModel.draw();
}

void GameRenderer::drawBirds(const std::vector<Bird>& birds) {
    for (const auto& bird : birds) {
        drawBird(bird);
    }
}

void GameRenderer::drawGroundSprites(const std::vector<Sprite>& flowerSprites) {
    for (const auto& flower : flowerSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, flower.position);
        model = glm::scale(model, glm::vec3(flower.size));

        glm::vec3 toCamera = glm::normalize(g_camera.getPosition() - flower.position);
        float angle = atan2f(toCamera.x, toCamera.z);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        g_shaderManager.setModelMatrix(model);
        g_shaderManager.setColor(flower.color);
        g_shaderManager.setUseTexture(flowerModel.hasTexture);

        flowerModel.draw();
    }
}

void GameRenderer::loadAllModels() {
    std::cout << "Loading models..." << std::endl;

    createTexturedCubeModel(snakeModel);
    createTexturedSphereModel(foodModel);
    createTexturedCubeModel(obstacleModel);
    createTexturedFloorModel(floorModel);
    createFenceModel(fenceModel);
    createCloudModel(cloudModel);
    createAnimatedBirdModel(birdModel);
    createFlowerModel(flowerModel);
    createTreeModel(treeModel);
    createDetailedAppleModel(appleModel);

    std::cout << "All models loaded successfully!" << std::endl;
}

void GameRenderer::createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal) {
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        Vertex v1, v2, v3;
        v1.position = glm::vec3(cx, cy, 0.0f);
        v2.position = glm::vec3(cx + cos(angle1) * radius, cy + sin(angle1) * radius, 0.0f);
        v3.position = glm::vec3(cx + cos(angle2) * radius, cy + sin(angle2) * radius, 0.0f);

        v1.normal = v2.normal = v3.normal = normal;

        v1.texCoords = glm::vec2(0.5f, 0.5f);
        v2.texCoords = glm::vec2(0.5f + cos(angle1) * 0.5f, 0.5f + sin(angle1) * 0.5f);
        v3.texCoords = glm::vec2(0.5f + cos(angle2) * 0.5f, 0.5f + sin(angle2) * 0.5f);

        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
    }
}

void GameRenderer::createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color) {
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        glm::vec3 p1(x + cos(angle1) * radius, y, z + sin(angle1) * radius);
        glm::vec3 p2(x + cos(angle2) * radius, y, z + sin(angle2) * radius);
        glm::vec3 p3(x + cos(angle1) * radius, y + height, z + sin(angle1) * radius);
        glm::vec3 p4(x + cos(angle2) * radius, y + height, z + sin(angle2) * radius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }
}

void GameRenderer::createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color) {
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                cx + radius * sin(theta1) * cos(phi1),
                cy + radius * cos(theta1),
                cz + radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                cx + radius * sin(theta1) * cos(phi2),
                cy + radius * cos(theta1),
                cz + radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                cx + radius * sin(theta2) * cos(phi2),
                cy + radius * cos(theta2),
                cz + radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                cx + radius * sin(theta2) * cos(phi1),
                cy + radius * cos(theta2),
                cz + radius * sin(theta2) * sin(phi1)
            );

            glm::vec3 normal1 = glm::normalize(v1 - glm::vec3(cx, cy, cz));
            glm::vec3 normal2 = glm::normalize(v2 - glm::vec3(cx, cy, cz));
            glm::vec3 normal3 = glm::normalize(v3 - glm::vec3(cx, cy, cz));
            glm::vec3 normal4 = glm::normalize(v4 - glm::vec3(cx, cy, cz));

            glm::vec2 t1 = glm::vec2((float)j / segments, (float)i / rings);
            glm::vec2 t2 = glm::vec2((float)(j + 1) / segments, (float)i / rings);
            glm::vec2 t3 = glm::vec2((float)(j + 1) / segments, (float)(i + 1) / rings);
            glm::vec2 t4 = glm::vec2((float)j / segments, (float)(i + 1) / rings);

            vertices.push_back({ v1, normal1, t1 });
            vertices.push_back({ v2, normal2, t2 });
            vertices.push_back({ v3, normal3, t3 });

            vertices.push_back({ v3, normal3, t3 });
            vertices.push_back({ v4, normal4, t4 });
            vertices.push_back({ v1, normal1, t1 });
        }
    }
}

void GameRenderer::createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius) {
    createSpherePart(vertices, x, y, z, radius, 8, 4, glm::vec3(1.0f));
}

void GameRenderer::createCloudModel(Model& model) {
    createCloudPart(model.vertices, 0.0f, 0.0f, 0.0f, 0.4f);
    createCloudPart(model.vertices, 0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, -0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, 0.0f, 0.3f, 0.0f, 0.25f);
    createCloudPart(model.vertices, 0.2f, -0.1f, 0.0f, 0.25f);
    createCloudPart(model.vertices, -0.2f, -0.1f, 0.0f, 0.25f);

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createAnimatedBirdModel(Model& model) {
    createSpherePart(model.vertices, 0.0f, 0.0f, 0.0f, 0.3f, 12, 8, glm::vec3(1.0f));
    createSpherePart(model.vertices, 0.4f, 0.1f, 0.0f, 0.15f, 8, 6, glm::vec3(1.0f));

    Vertex beak[] = {
        {{0.55f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.7f, 0.1f, -0.05f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.7f, 0.1f, 0.05f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.55f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.7f, 0.15f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        {{0.7f, 0.1f, -0.05f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : beak) {
        model.vertices.push_back(v);
    }

    Vertex tail[] = {
        {{-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.6f, 0.0f, -0.15f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.6f, 0.0f, 0.15f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.6f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        {{-0.6f, 0.0f, -0.15f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : tail) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createFlowerModel(Model& model) {
    createCircle(model.vertices, 0.0f, 0.0f, 0.1f, 8);

    for (int i = 0; i < 6; i++) {
        float angle = i * 3.14159f / 3.0f;
        float x = cos(angle) * 0.25f;
        float y = sin(angle) * 0.25f;
        createCircle(model.vertices, x, y, 0.08f, 6);
    }

    Vertex stem[] = {
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : stem) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createTreeModel(Model& model) {
    createCylinder(model.vertices, 0.0f, 0.0f, 0.0f, 0.08f, 0.6f, 8, glm::vec3(0.4f, 0.2f, 0.1f));
    createSpherePart(model.vertices, 0.0f, 0.8f, 0.0f, 0.3f, 12, 8, glm::vec3(0.1f, 0.4f, 0.1f));

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createDetailedAppleModel(Model& model) {
    createSpherePart(model.vertices, 0.0f, 0.0f, 0.0f, 0.5f, 16, 12, glm::vec3(1.0f, 0.0f, 0.0f));
    createSpherePart(model.vertices, 0.0f, 0.3f, 0.0f, 0.1f, 8, 4, glm::vec3(0.3f, 0.2f, 0.1f));

    Vertex stem[] = {
        {{-0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : stem) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createTexturedCubeModel(Model& model) {
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (int i = 0; i < 288; i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex.normal = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        vertex.texCoords = glm::vec2(vertices[i + 6], vertices[i + 7]);
        model.vertices.push_back(vertex);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createTexturedSphereModel(Model& model) {
    const int segments = 16;
    const int rings = 16;
    const float radius = 0.5f;

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                radius * sin(theta1) * cos(phi1),
                radius * cos(theta1),
                radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                radius * sin(theta1) * cos(phi2),
                radius * cos(theta1),
                radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                radius * sin(theta2) * cos(phi2),
                radius * cos(theta2),
                radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                radius * sin(theta2) * cos(phi1),
                radius * cos(theta2),
                radius * sin(theta2) * sin(phi1)
            );

            glm::vec2 t1 = glm::vec2((float)j / segments, (float)i / rings);
            glm::vec2 t2 = glm::vec2((float)(j + 1) / segments, (float)i / rings);
            glm::vec2 t3 = glm::vec2((float)(j + 1) / segments, (float)(i + 1) / rings);
            glm::vec2 t4 = glm::vec2((float)j / segments, (float)(i + 1) / rings);

            model.vertices.push_back({ v1, glm::normalize(v1), t1 });
            model.vertices.push_back({ v2, glm::normalize(v2), t2 });
            model.vertices.push_back({ v3, glm::normalize(v3), t3 });

            model.vertices.push_back({ v3, glm::normalize(v3), t3 });
            model.vertices.push_back({ v4, glm::normalize(v4), t4 });
            model.vertices.push_back({ v1, glm::normalize(v1), t1 });
        }
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createTexturedFloorModel(Model& model) {
    float floorSize = 8.0f;

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(-floorSize, -0.1f, -floorSize);
    v2.position = glm::vec3(-floorSize, -0.1f, floorSize);
    v3.position = glm::vec3(floorSize, -0.1f, -floorSize);
    v4.position = glm::vec3(floorSize, -0.1f, floorSize);

    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    v1.normal = v2.normal = v3.normal = v4.normal = normal;

    v1.texCoords = glm::vec2(0.0f, 0.0f);
    v2.texCoords = glm::vec2(0.0f, 4.0f);
    v3.texCoords = glm::vec2(4.0f, 0.0f);
    v4.texCoords = glm::vec2(4.0f, 4.0f);

    model.vertices.push_back(v1);
    model.vertices.push_back(v3);
    model.vertices.push_back(v4);

    model.vertices.push_back(v4);
    model.vertices.push_back(v2);
    model.vertices.push_back(v1);

    model.hasTexture = true;
    model.setupBuffers();
}

void GameRenderer::createFenceModel(Model& model) {
    float vertices[] = {
        -0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 0.0f,
         0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 3.0f,
         0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 3.0f,
        -0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 3.0f,
        -0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 0.0f,
         0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 3.0f,
         0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 3.0f,
        -0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 3.0f,
        -0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.3f,  1.5f,  0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.3f,  1.5f, -0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.3f, -0.5f, -0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.3f, -0.5f, -0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.3f, -0.5f,  0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.3f,  1.5f,  0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.3f,  1.5f,  0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.3f,  1.5f, -0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.3f, -0.5f, -0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.3f, -0.5f, -0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.3f, -0.5f,  0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.3f,  1.5f,  0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         -0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (int i = 0; i < 240; i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex.normal = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        vertex.texCoords = glm::vec2(vertices[i + 6], vertices[i + 7]);
        model.vertices.push_back(vertex);
    }

    model.hasTexture = true;
    model.setupBuffers();
}