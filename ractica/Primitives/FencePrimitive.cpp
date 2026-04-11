#include "../pch.h"
#include "FencePrimitive.h"
#include "PrimitiveBase.h"

void FencePrimitive::create(Model& model) {
    model.vertices.clear();
    model.vertices.reserve(5000);

    // Размеры одной секции забора (для одной клетки)
    float postHeight = 0.6f;      // Высота столба
    float postWidth = 0.08f;      // Ширина столба
    float railThickness = 0.06f;  // Толщина рейки
    float railWidth = 0.05f;      // Ширина рейки
    float sectionWidth = 0.9f;    // Ширина секции (чуть меньше cellSize)

    // Центр секции в (0,0,0)
    float halfSection = sectionWidth / 2.0f;

    // === СТОЛБЫ ===
    // Левый столб
    createPost(model, -halfSection, 0.0f, 0.0f, postWidth, postHeight);
    // Правый столб
    createPost(model, halfSection, 0.0f, 0.0f, postWidth, postHeight);

    // === ВЕРХНЯЯ РЕЙКА ===
    createRailHorizontal(model, 0.0f, postHeight * 0.7f, 0.0f,
        sectionWidth - postWidth, railThickness);

    // === НИЖНЯЯ РЕЙКА ===
    createRailHorizontal(model, 0.0f, postHeight * 0.25f, 0.0f,
        sectionWidth - postWidth, railThickness);

    // === ВЕРТИКАЛЬНЫЕ РЕЙКИ (между столбами) ===
    // Средняя вертикальная рейка
    createRailVertical(model, 0.0f, postHeight / 2.0f, 0.0f,
        postHeight * 0.55f, railWidth);

    model.hasTexture = false;
    model.setupBuffers();

    std::cout << "Fence primitive created with " << model.vertices.size() << " vertices" << std::endl;
}

void FencePrimitive::createPost(Model& model, float x, float y, float z, float width, float height) {
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    float halfDepth = width / 2.0f;

    glm::vec3 min(x - halfWidth, y, z - halfDepth);
    glm::vec3 max(x + halfWidth, y + height, z + halfDepth);

    // Передняя грань (Z+)
    addQuad(model,
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // Задняя грань (Z-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(0.0f, 0.0f, -1.0f)
    );

    // Левая грань (X-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(-1.0f, 0.0f, 0.0f)
    );

    // Правая грань (X+)
    addQuad(model,
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    // Верхняя грань (Y+)
    addQuad(model,
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Нижняя грань (Y-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(0.0f, -1.0f, 0.0f)
    );
}

void FencePrimitive::createRailHorizontal(Model& model, float x, float y, float z, float length, float thickness) {
    float halfLength = length / 2.0f;
    float halfThickness = thickness / 2.0f;
    float railDepth = 0.05f;
    float halfDepth = railDepth / 2.0f;

    glm::vec3 min(x - halfLength, y - halfThickness, z - halfDepth);
    glm::vec3 max(x + halfLength, y + halfThickness, z + halfDepth);

    // Передняя грань (Z+)
    addQuad(model,
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // Задняя грань (Z-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(0.0f, 0.0f, -1.0f)
    );

    // Левая грань (X-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(-1.0f, 0.0f, 0.0f)
    );

    // Правая грань (X+)
    addQuad(model,
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    // Верхняя грань (Y+)
    addQuad(model,
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Нижняя грань (Y-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(0.0f, -1.0f, 0.0f)
    );
}

void FencePrimitive::createRailVertical(Model& model, float x, float y, float z, float length, float thickness) {
    float halfLength = length / 2.0f;
    float halfThickness = thickness / 2.0f;
    float railDepth = 0.05f;
    float halfDepth = railDepth / 2.0f;

    glm::vec3 min(x - halfThickness, y - halfLength, z - halfDepth);
    glm::vec3 max(x + halfThickness, y + halfLength, z + halfDepth);

    // Передняя грань (Z+)
    addQuad(model,
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // Задняя грань (Z-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(0.0f, 0.0f, -1.0f)
    );

    // Левая грань (X-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(-1.0f, 0.0f, 0.0f)
    );

    // Правая грань (X+)
    addQuad(model,
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    // Верхняя грань (Y+)
    addQuad(model,
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Нижняя грань (Y-)
    addQuad(model,
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(0.0f, -1.0f, 0.0f)
    );
}

void FencePrimitive::addQuad(Model& model,
    const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4,
    const glm::vec3& normal) {

    // Треугольник 1
    Vertex vert1, vert2, vert3;
    vert1.position = v1;
    vert2.position = v2;
    vert3.position = v3;

    vert1.normal = normal;
    vert2.normal = normal;
    vert3.normal = normal;

    vert1.texCoords = glm::vec2(0.0f, 0.0f);
    vert2.texCoords = glm::vec2(1.0f, 0.0f);
    vert3.texCoords = glm::vec2(1.0f, 1.0f);

    model.vertices.push_back(vert1);
    model.vertices.push_back(vert2);
    model.vertices.push_back(vert3);

    // Треугольник 2
    Vertex vert4;
    vert4.position = v4;
    vert4.normal = normal;
    vert4.texCoords = glm::vec2(0.0f, 1.0f);

    model.vertices.push_back(vert3);
    model.vertices.push_back(vert4);
    model.vertices.push_back(vert1);
}