// ShadowRenderer.h
#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include "pch.h"
class ShadowRenderer {
public:
    ShadowRenderer();
    ~ShadowRenderer();

    bool initialize();
    void beginShadowPass();
    void endShadowPass();
    void renderShadows(const std::vector<glm::mat4>& modelMatrices);

    GLuint getShadowMap() const { return depthMap; }
    glm::mat4 getLightSpaceMatrix() const { return lightSpaceMatrix; }

private:
    GLuint depthMapFBO;
    GLuint depthMap;
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    glm::mat4 lightSpaceMatrix;

    void createShadowMap();
};