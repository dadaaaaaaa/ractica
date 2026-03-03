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
    out vec3 ModelColor; // Для передачи цвета модели
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoords = aTexCoords;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    
    uniform vec3 color;              // Цвет модели из конфига
    uniform bool useTexture;          // Использовать ли текстуру
    uniform bool isFloor;             // Специальный флаг для пола
    uniform float cellSize;           // Размер клетки для сетки пола
    
    uniform bool hasTexture;          // Есть ли текстура у модели
    uniform sampler2D modelTexture;   // Текстура модели
    
    void main() {
        vec3 result;
        vec3 finalColor = color;       // Базовый цвет из конфига
        
        // Если есть текстура - используем её
        if (hasTexture) {
            vec4 texColor = texture(modelTexture, TexCoords);
            finalColor = texColor.rgb;
        }
        
        // Специальная обработка для пола (сетка)
        if (isFloor) {
            // АБСОЛЮТНО ЧЕТКАЯ СЕТКА БЕЗ МЕРЦАНИЯ
            float stableX = FragPos.x + 1000.0;
            float stableZ = FragPos.z + 1000.0;
            
            ivec2 cellIdx = ivec2(
                int(floor((stableX + 0.001) / cellSize)),
                int(floor((stableZ + 0.001) / cellSize))
            );
            
            vec2 cellPos = vec2(
                (stableX - float(cellIdx.x) * cellSize) / cellSize,
                (stableZ - float(cellIdx.y) * cellSize) / cellSize
            );
            
            float gridLineWidth = 0.1;
            float epsilon = 0.001;
            
            float distToVertical = min(cellPos.x, 1.0 - cellPos.x);
            float distToHorizontal = min(cellPos.y, 1.0 - cellPos.y);
            
            float isVerticalLine = step(distToVertical, gridLineWidth + epsilon);
            float isHorizontalLine = step(distToHorizontal, gridLineWidth + epsilon);
            float isAnyLine = min(1.0, isVerticalLine + isHorizontalLine);
            
            vec3 lightCellColor = vec3(0.45f, 0.75f, 0.35f);
            vec3 darkCellColor = vec3(0.35f, 0.65f, 0.25f);
            vec3 gridColor = vec3(0.2f, 0.5f, 0.15f);
            
            int patternX = cellIdx.x + 10000;
            int patternZ = cellIdx.y + 10000;
            bool isDarkCell = ((patternX + patternZ) & 1) == 0;
            
            vec3 cellColor = isDarkCell ? darkCellColor : lightCellColor;
            
            float lineBlend = smoothstep(gridLineWidth - 0.005, gridLineWidth + 0.005, 
                                        min(distToVertical, distToHorizontal));
            result = mix(gridColor, cellColor, lineBlend);
            
            float lightFactor = 0.9 + 0.1 * clamp(Normal.y, 0.0, 1.0);
            result *= lightFactor;
            
        } else {
            // Обычное освещение для всех 3D моделей
            vec3 lightDir = vec3(0.5, -1.0, 0.3);  // Направление света
            lightDir = normalize(lightDir);
            
            vec3 ambient = 0.3 * finalColor;       // Фоновое освещение
            float diff = max(dot(normalize(Normal), -lightDir), 0.0);
            vec3 diffuse = diff * finalColor;       // Диффузное освещение
            
            result = ambient + diffuse * 0.8;
            
            // Добавляем небольшой блеск
            vec3 viewDir = normalize(-FragPos);
            vec3 reflectDir = reflect(lightDir, normalize(Normal));
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            result += spec * 0.3;
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
    std::cout << "ShaderManager::setColor: ("
        << color.r << ", "
        << color.g << ", "
        << color.b << ")" << std::endl;
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