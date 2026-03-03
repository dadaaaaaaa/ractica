#pragma once
#include "../Graphics/Model.h"

// Базовый класс для всех примитивов
class PrimitiveBase {
public:
    static void createCube(Model& model);
    static void createSphere(Model& model, int segments = 16, int rings = 16);
    static void createCylinder(Model& model, float radius, float height, int segments = 8);
    static void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments);
};