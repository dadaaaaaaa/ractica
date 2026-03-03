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

    // ћетоды дл€ установки uniform переменных 3D шейдера
    void setModelMatrix(const glm::mat4& model) const;
    void setViewMatrix(const glm::mat4& view) const;
    void setProjectionMatrix(const glm::mat4& projection) const;
    void setColor(const glm::vec3& color) const;
    void setUseTexture(bool useTexture) const;

    // Ќовый метод дл€ получени€ ID шейдерной программы
    GLuint getShaderProgram() const { return shaderProgram; }
    GLuint getUIShaderProgram() const { return uiShaderProgram; }

    // ћетоды дл€ установки uniform переменных UI шейдера
    void setUIProjection(const glm::mat4& projection) const;
    void setUIModel(const glm::mat4& model) const;
    void setUIColor(const glm::vec3& color) const;
    void setUIAlpha(float alpha) const;
    void setIsFloor(bool isFloor) const;
    void setCellSize(float cellSize) const;
    void setGridWidth(float width) const;
    void setGridDepth(float depth) const;
private:
    GLuint shaderProgram;
    GLuint uiShaderProgram;

    // Uniform locations дл€ 3D шейдера
    GLint modelLoc;
    GLint viewLoc;
    GLint projectionLoc;
    GLint colorLoc;
    GLint useTextureLoc;

    // Uniform locations дл€ UI шейдера
    GLint uiProjectionLoc;
    GLint uiModelLoc;
    GLint uiColorLoc;
    GLint uiAlphaLoc;

    GLuint compileShader(GLenum type, const char* source);
    bool createShaderProgram();
    bool createUIShaderProgram();

};

#endif // SHADER_MANAGER_H