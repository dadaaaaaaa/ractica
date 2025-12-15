// Graphics/ShadowRenderer.h
#ifndef SHADOW_RENDERER_H
#define SHADOW_RENDERER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

class ShadowRenderer {
public:
    ShadowRenderer();
    ~ShadowRenderer();

    bool initialize();

    void beginShadowPass(const glm::vec3& lightPos);
    void endShadowPass();

    GLuint getShadowTexture() const;
    const glm::mat4& getLightSpaceMatrix() const;

    void renderSimpleCube(const glm::mat4& modelMatrix);

private:
    bool compileShadowShader();
    bool createShadowMap();
    void setupCubeVAO();

    GLuint depthMapFBO;
    GLuint depthMap;
    GLuint shadowShaderProgram;
    glm::mat4 lightSpaceMatrix;

    GLuint cubeVAO;
    GLuint cubeVBO;
};

#endif // SHADOW_RENDERER_H