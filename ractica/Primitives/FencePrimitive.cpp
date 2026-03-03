#include "../pch.h"
#include "FencePrimitive.h"

void FencePrimitive::createPost(std::vector<Vertex>& vertices, float x, float y, float z,
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

    // Левая грань
    vertices.push_back({ {x - halfWidth, y + height, z + halfWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z - halfWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z - halfWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x - halfWidth, y, z + halfWidth}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x - halfWidth, y + height, z + halfWidth}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });

    // Правая грань
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z - halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z - halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z - halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
    vertices.push_back({ {x + halfWidth, y, z + halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
    vertices.push_back({ {x + halfWidth, y + height, z + halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
}

void FencePrimitive::createRailHorizontal(std::vector<Vertex>& vertices, float x, float y, float z,
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

void FencePrimitive::createRailVertical(std::vector<Vertex>& vertices, float x, float y, float z,
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