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

public:
    ShaderManager();
    ~ShaderManager();

    bool initialize();
    void use3DShader() const;
    void useUIShader() const;
    void setUIAlpha(float alpha) const;

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