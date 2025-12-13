#pragma once
#include <GLEW/glew.h>
#include <glm/glm.hpp>
#include <string>

class ShaderManager {
private:
    GLuint shaderProgram;
    GLuint uiShaderProgram;

    // Uniform locations
    GLuint modelLoc, viewLoc, projectionLoc, colorLoc, useTextureLoc;
    GLuint uiProjectionLoc, uiModelLoc, uiColorLoc, uiAlphaLoc;  // днаюбэре uiAlphaLoc
    GLint lightSpaceMatrixLoc;
    GLint shadowMapLoc;
    GLint useShadowsLoc;
    GLint lightPosLoc;
public:
    ShaderManager();
    ~ShaderManager();

    bool initialize();
    void use3DShader() const;
    void useUIShader() const;
    void setUIAlpha(float alpha) const;
    void setLightSpaceMatrix(const glm::mat4& lightSpaceMatrix) const;
    void setShadowMap(GLuint textureID) const;
    void setUseShadows(bool useShadows) const;
    void setLightPosition(const glm::vec3& lightPos) const;
    // Setters for uniforms
    void setModelMatrix(const glm::mat4& model) const;
    void setViewMatrix(const glm::mat4& view) const;
    void setProjectionMatrix(const glm::mat4& projection) const;
    void setColor(const glm::vec3& color) const;
    void setUseTexture(bool useTexture) const;

    void setUIProjection(const glm::mat4& projection) const;
    void setUIModel(const glm::mat4& model) const;
    void setUIColor(const glm::vec3& color) const;

private:
    GLuint compileShader(GLenum type, const char* source);
    bool createShaderProgram();
    bool createUIShaderProgram();
};