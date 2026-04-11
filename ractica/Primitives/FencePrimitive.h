#pragma once
#include "../Graphics/Model.h"
#include <vector>
#include <glm/glm.hpp>

class FencePrimitive {
public:
    static void create(Model& model);

private:
    static void createPost(Model& model, float x, float y, float z, float width, float height);
    static void createRailHorizontal(Model& model, float x, float y, float z, float length, float thickness);
    static void createRailVertical(Model& model, float x, float y, float z, float length, float thickness);
    static void addQuad(Model& model,
        const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4,
        const glm::vec3& normal);
};