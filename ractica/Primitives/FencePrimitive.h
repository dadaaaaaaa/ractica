#pragma once
#include "../Graphics/Model.h"

class FencePrimitive {
public:
    static void createPost(std::vector<Vertex>& vertices, float x, float y, float z,
        float width, float height, const glm::vec3& color);
    static void createRailHorizontal(std::vector<Vertex>& vertices, float x, float y, float z,
        float length, float thickness, const glm::vec3& color);
    static void createRailVertical(std::vector<Vertex>& vertices, float x, float y, float z,
        float length, float thickness, const glm::vec3& color);
};