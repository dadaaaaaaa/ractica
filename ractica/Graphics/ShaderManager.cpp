#include "../pch.h"
#include "ShaderManager.h"
#include <iostream>

// Шейдеры для 3D объектов (моделей)
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec3 Normal;
    out vec3 FragPos;
    out vec2 TexCoords;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoords = aTexCoords;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

// ИСПРАВЛЕННЫЙ ФРАГМЕНТНЫЙ ШЕЙДЕР
// ПРОСТЕЙШАЯ ВЕРСИЯ С ФИКСИРОВАННЫМ РАЗМЕРОМ КЛЕТКИ
// ИСПРАВЛЕННЫЙ ФРАГМЕНТНЫЙ ШЕЙДЕР
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec2 TexCoords;
    in vec3 FragPos;
    
    uniform vec3 color;              // Цвет модели из конфига
    uniform vec3 gridColor;           // Цвет сетки из конфига
    uniform bool useTexture;          // Использовать ли текстуру
    uniform bool gridEnabled;         // Включена ли сетка
    uniform float cellSize;           // Размер клетки
    uniform float gridWidth;          // Ширина сетки в клетках
    uniform float gridDepth;          // Глубина сетки в клетках
    uniform bool isFloor=false;             // Флаг: это пол или другой объект
    
    uniform sampler2D modelTexture;   // Текстура модели
    
    void main() {
        vec3 finalColor = color;
        
        // Если есть текстура - используем её
        if (useTexture) {
            finalColor = texture(modelTexture, TexCoords).rgb;
        }
        
        // Начинаем с цвета текстуры
        vec3 result = finalColor;
        
        // Рисуем сетку ТОЛЬКО если это пол И сетка включена
        if (isFloor && gridEnabled) {
            // Границы игрового поля
            float halfWidth = (gridWidth * cellSize) / 2.0;
            float halfDepth = (gridDepth * cellSize) / 2.0;
            
            // Проверяем, находимся ли мы в пределах игрового поля
            if (abs(FragPos.x) <= halfWidth && abs(FragPos.z) <= halfDepth) {
                // Вычисляем позицию в клетке
                float x = FragPos.x + halfWidth;
                float z = FragPos.z + halfDepth;
                
                float posInCellX = mod(x, cellSize);
                float posInCellZ = mod(z, cellSize);
                
                float lineWidth = 0.05;
                
                // Если мы близко к границе клетки
                if (posInCellX < lineWidth || posInCellX > cellSize - lineWidth ||
                    posInCellZ < lineWidth || posInCellZ > cellSize - lineWidth) {
                    result = gridColor;
                }
            }
        }
        
        FragColor = vec4(result, 1.0);
    }
)";

// Шейдеры для 2D интерфейса
const char* uiVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    
    uniform mat4 projection;
    uniform mat4 model;
    
    void main() {
        gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    }
)";

const char* uiFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec3 color;
    uniform float alpha;
    uniform bool hasTexture;
    uniform sampler2D uiTexture;
    
    void main() {
        FragColor = vec4(color, alpha);
        if (hasTexture) {
            FragColor *= texture(uiTexture, gl_PointCoord);
        }
    }
)";

ShaderManager::ShaderManager() : shaderProgram(0), uiShaderProgram(0) {}

ShaderManager::~ShaderManager() {
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (uiShaderProgram) glDeleteProgram(uiShaderProgram);
}

bool ShaderManager::initialize() {
    return createShaderProgram() && createUIShaderProgram();
}

void ShaderManager::use3DShader() const {
    glUseProgram(shaderProgram);
}

void ShaderManager::useUIShader() const {
    glUseProgram(uiShaderProgram);
}

void ShaderManager::setModelMatrix(const glm::mat4& model) const {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
}

void ShaderManager::setViewMatrix(const glm::mat4& view) const {
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void ShaderManager::setProjectionMatrix(const glm::mat4& projection) const {
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void ShaderManager::setColor(const glm::vec3& color) const {
    glUniform3fv(colorLoc, 1, &color[0]);
}

void ShaderManager::setUseTexture(bool useTexture) const {
    glUniform1i(useTextureLoc, useTexture);
}

void ShaderManager::setIsFloor(bool isFloor) const {
    GLint isFloorLoc = glGetUniformLocation(shaderProgram, "isFloor");
    if (isFloorLoc != -1) {
        glUniform1i(isFloorLoc, isFloor);
    }
}

void ShaderManager::setCellSize(float cellSize) const {
    GLint cellSizeLoc = glGetUniformLocation(shaderProgram, "cellSize");
    if (cellSizeLoc != -1) {
        glUniform1f(cellSizeLoc, cellSize);
    }
}

void ShaderManager::setGridEnabled(bool enabled) const {
    GLint gridEnabledLoc = glGetUniformLocation(shaderProgram, "gridEnabled");
    if (gridEnabledLoc != -1) {
        glUniform1i(gridEnabledLoc, enabled);
    }
}

void ShaderManager::setGridLineWidth(float width) const {
    GLint gridLineWidthLoc = glGetUniformLocation(shaderProgram, "gridLineWidth");
    if (gridLineWidthLoc != -1) {
        glUniform1f(gridLineWidthLoc, width);
    }
}

void ShaderManager::setGridWidth(float width) const {
    GLint gridWidthLoc = glGetUniformLocation(shaderProgram, "gridWidth");
    if (gridWidthLoc != -1) {
        glUniform1f(gridWidthLoc, width);
    }
}

void ShaderManager::setGridDepth(float depth) const {
    GLint gridDepthLoc = glGetUniformLocation(shaderProgram, "gridDepth");
    if (gridDepthLoc != -1) {
        glUniform1f(gridDepthLoc, depth);
    }
}

void ShaderManager::setGridColor(const glm::vec3& color) const {
    GLint gridColorLoc = glGetUniformLocation(shaderProgram, "gridColor");
    if (gridColorLoc != -1) {
        glUniform3fv(gridColorLoc, 1, glm::value_ptr(color));
    }
}

void ShaderManager::setUIProjection(const glm::mat4& projection) const {
    glUniformMatrix4fv(uiProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void ShaderManager::setUIModel(const glm::mat4& model) const {
    glUniformMatrix4fv(uiModelLoc, 1, GL_FALSE, glm::value_ptr(model));
}

void ShaderManager::setUIColor(const glm::vec3& color) const {
    glUniform3fv(uiColorLoc, 1, glm::value_ptr(color));
}

void ShaderManager::setUIAlpha(float alpha) const {
    glUniform1f(uiAlphaLoc, alpha);
}

GLuint ShaderManager::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }
    return shader;
}

bool ShaderManager::createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking error:\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    modelLoc = glGetUniformLocation(shaderProgram, "model");
    viewLoc = glGetUniformLocation(shaderProgram, "view");
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    colorLoc = glGetUniformLocation(shaderProgram, "color");
    useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");

    std::cout << "3D Shader Uniform Locations:" << std::endl;
    std::cout << "  model: " << modelLoc << std::endl;
    std::cout << "  view: " << viewLoc << std::endl;
    std::cout << "  projection: " << projectionLoc << std::endl;
    std::cout << "  color: " << colorLoc << std::endl;
    std::cout << "  useTexture: " << useTextureLoc << std::endl;

    return true;
}

bool ShaderManager::createUIShaderProgram() {
    GLuint uiVertexShader = compileShader(GL_VERTEX_SHADER, uiVertexShaderSource);
    GLuint uiFragmentShader = compileShader(GL_FRAGMENT_SHADER, uiFragmentShaderSource);

    uiShaderProgram = glCreateProgram();
    glAttachShader(uiShaderProgram, uiVertexShader);
    glAttachShader(uiShaderProgram, uiFragmentShader);
    glLinkProgram(uiShaderProgram);

    GLint success;
    glGetProgramiv(uiShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(uiShaderProgram, 512, NULL, infoLog);
        std::cerr << "UI Shader program linking error:\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(uiVertexShader);
    glDeleteShader(uiFragmentShader);

    uiProjectionLoc = glGetUniformLocation(uiShaderProgram, "projection");
    uiModelLoc = glGetUniformLocation(uiShaderProgram, "model");
    uiColorLoc = glGetUniformLocation(uiShaderProgram, "color");
    uiAlphaLoc = glGetUniformLocation(uiShaderProgram, "alpha");

    if (uiProjectionLoc == -1 || uiModelLoc == -1 || uiColorLoc == -1 || uiAlphaLoc == -1) {
        std::cerr << "Error: Failed to find some UI shader uniforms!" << std::endl;
        return false;
    }

    std::cout << "UI Shader Uniform Locations:" << std::endl;
    std::cout << "  projection: " << uiProjectionLoc << std::endl;
    std::cout << "  model: " << uiModelLoc << std::endl;
    std::cout << "  color: " << uiColorLoc << std::endl;
    std::cout << "  alpha: " << uiAlphaLoc << std::endl;

    return true;
}