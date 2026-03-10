#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <GL/glew.h>
#include <glm/glm.hpp>

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    bool initialize();

    void use3DShader() const;
    void useUIShader() const;

    // Методы для установки uniform переменных 3D шейдера
    void setModelMatrix(const glm::mat4& model) const;
    void setViewMatrix(const glm::mat4& view) const;
    void setProjectionMatrix(const glm::mat4& projection) const;
    void setColor(const glm::vec3& color) const;
    void setUseTexture(bool useTexture) const;

    // Новые методы для управления сеткой
    void setIsFloor(bool isFloor) const;
    void setCellSize(float cellSize) const;
    void setGridEnabled(bool enabled) const;
    void setGridLineWidth(float width) const;
    void setGridWidth(float width) const;
    void setGridDepth(float depth) const;

    // Методы для UI шейдера
    void setUIProjection(const glm::mat4& projection) const;
    void setUIModel(const glm::mat4& model) const;
    void setUIColor(const glm::vec3& color) const;
    void setUIAlpha(float alpha) const;

    // Геттеры для ID шейдерных программ
    GLuint getShaderProgram() const { return shaderProgram; }
    GLuint getUIShaderProgram() const { return uiShaderProgram; }

private:
    GLuint shaderProgram;
    GLuint uiShaderProgram;

    // Uniform locations для 3D шейдера
    GLint modelLoc;
    GLint viewLoc;
    GLint projectionLoc;
    GLint colorLoc;
    GLint useTextureLoc;

    // Uniform locations для UI шейдера
    GLint uiProjectionLoc;
    GLint uiModelLoc;
    GLint uiColorLoc;
    GLint uiAlphaLoc;

    GLuint compileShader(GLenum type, const char* source);
    bool createShaderProgram();
    bool createUIShaderProgram();
};

#endif // SHADER_MANAGER_H