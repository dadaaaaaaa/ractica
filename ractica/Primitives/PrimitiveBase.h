#pragma once
#include "../Graphics/Model.h"
#include <vector>

class PrimitiveBase {
public:
    static void createCube(Model& model);
    static void createCube(Model& model, const glm::vec3& color);  // Новая перегрузка с цветом
    static void createSphere(Model& model, int segments = 16, int rings = 16);
    static void createCylinder(Model& model, float radius = 0.5f, float height = 1.0f, int segments = 12);
    static void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments);

    // Вспомогательные методы для отрисовки (без VAO/VBO, для Fixed Pipeline)
    static void drawCube();
    static void drawSphere(float radius = 0.5f, int segments = 16, int rings = 16);
    static void drawCylinder(float radius = 0.5f, float height = 1.0f, int segments = 12);
};