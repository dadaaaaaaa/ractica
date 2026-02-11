#include "../pch.h"
#include "ShaderManager.h"
#include <iostream>

// Шейдеры для 3D (оставьте без изменений)
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

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    
    uniform vec3 color;
    uniform bool useTexture;
    uniform bool isFloor;
    uniform float cellSize;
    
    void main() {
        vec3 result;
        
        if (isFloor) {
            // АБСОЛЮТНО ЧЕТКАЯ СЕТКА БЕЗ МЕРЦАНИЯ И ПЛЫВУЩЕСТИ
            
            // 1. Используем МИРОВЫЕ координаты без преобразований
            // Добавляем небольшое смещение для стабильности
            float stableX = FragPos.x + 1000.0; // Смещение для стабильности floor()
            float stableZ = FragPos.z + 1000.0;
            
            // 2. Вычисляем индексы клеток с высокой точностью
            // Используем инвариантную относительно погрешности формулу
            ivec2 cellIdx = ivec2(
                int(floor((stableX + 0.001) / cellSize)),
                int(floor((stableZ + 0.001) / cellSize))
            );
            
            // 3. Вычисляем позицию внутри клетки с фиксированной точностью
            vec2 cellPos = vec2(
                (stableX - float(cellIdx.x) * cellSize) / cellSize,
                (stableZ - float(cellIdx.y) * cellSize) / cellSize
            );
            
            // 4. ТОЛСТЫЕ линии сетки - 10% от размера клетки
            float gridLineWidth = 0.1;
            
            // 5. Определяем линии сетки с ЗАПАСОМ для устранения погрешности
            float epsilon = 0.001; // Маленькое значение для устойчивости
            
            // Расстояние до ближайшей линии
            float distToVertical = min(cellPos.x, 1.0 - cellPos.x);
            float distToHorizontal = min(cellPos.y, 1.0 - cellPos.y);
            
            // Являемся ли мы частью вертикальной линии?
            float isVerticalLine = step(distToVertical, gridLineWidth + epsilon);
            
            // Являемся ли мы частью горизонтальной линии?
            float isHorizontalLine = step(distToHorizontal, gridLineWidth + epsilon);
            
            // Находимся ли мы на любой линии сетки?
            float isAnyLine = min(1.0, isVerticalLine + isHorizontalLine);
            
            // 6. Цвета с ХОРОШИМ КОНТРАСТОМ
            vec3 lightCellColor = vec3(0.45f, 0.75f, 0.35f);   // Ярче
            vec3 darkCellColor = vec3(0.35f, 0.65f, 0.25f);    // Темнее
            vec3 gridColor = vec3(0.2f, 0.5f, 0.15f);          // Контрастный для линий
            
            // 7. Шахматный паттерн - используем четность суммы координат
            // Добавляем большие числа для стабильности при отрицательных координатах
            int patternX = cellIdx.x + 10000;
            int patternZ = cellIdx.y + 10000;
            bool isDarkCell = ((patternX + patternZ) & 1) == 0;
            
            // 8. Выбираем цвет клетки
            vec3 cellColor = isDarkCell ? darkCellColor : lightCellColor;
            
            // 9. Финальный цвет - если на линии, то цвет линии, иначе цвет клетки
            // Используем smoothstep для небольшого сглаживания (но не мерцания)
            float lineBlend = smoothstep(gridLineWidth - 0.005, gridLineWidth + 0.005, 
                                        min(distToVertical, distToHorizontal));
            result = mix(gridColor, cellColor, lineBlend);
            
            // 10. ПРОСТОЕ И СТАБИЛЬНОЕ ОСВЕЩЕНИЕ
            // Используем фиксированное значение для устранения мерцания
            float lightFactor = 0.9 + 0.1 * clamp(Normal.y, 0.0, 1.0);
            result *= lightFactor;
            
            // 11. Добавляем очень слабую текстуру для клеток (не для линий)
            if (isAnyLine < 0.1) {
                // Детерминированный паттерн без тригонометрии
                float texturePattern = 0.95 + 0.05 * 
                    fract(sin(float(cellIdx.x) * 12.9898 + float(cellIdx.y) * 78.233) * 43758.5453);
                result *= texturePattern;
            }
            
        } else if (useTexture) {
            // Существующая логика для текстурных объектов
            if (color.r > 0.8 && color.g < 0.2) {
                float woodPattern = sin(TexCoords.x * 30.0) * 0.3 + 0.7;
                float ringPattern = sin(TexCoords.y * 15.0) * 0.2 + 0.8;
                result = color * woodPattern * ringPattern;
            }
            else if (color.g > 0.8 && color.b < 0.3) {
                float grassPattern = sin(TexCoords.x * 50.0) * sin(TexCoords.y * 50.0) * 0.4 + 0.6;
                result = color * grassPattern;
            }
            else if (color.r > 0.8 && color.g > 0.8) {
                float spots = step(0.8, sin(TexCoords.x * 40.0) * sin(TexCoords.y * 40.0));
                result = color * (0.8 + spots * 0.2);
            }
            else if (color.r > 0.9 && color.g > 0.9 && color.b > 0.9) {
                float cloud = sin(TexCoords.x * 25.0) * sin(TexCoords.y * 25.0) * 0.5 + 0.5;
                result = color * cloud;
            }
            else if (color.b > 0.8) {
                float birdPattern = sin(TexCoords.x * 60.0) * 0.4 + 0.6;
                result = color * birdPattern;
            }
            else if (color.r > 0.8 || color.g > 0.8 || color.b > 0.8) {
                float flowerPattern = sin(TexCoords.x * 35.0) * cos(TexCoords.y * 35.0) * 0.3 + 0.7;
                result = color * flowerPattern;
            }
            else {
                float pattern = sin(TexCoords.x * 20.0) * sin(TexCoords.y * 20.0) * 0.3 + 0.7;
                result = color * pattern;
            }
            
            // Освещение для текстурных объектов
            vec3 lightDir = vec3(0.0, -1.0, 0.0);
            float diff = max(dot(normalize(Normal), -lightDir), 0.3);
            result = result * (0.7 + 0.3 * diff);
            
        } else {
            // Для нетекстурных объектов
            vec3 lightDir = vec3(0.0, -1.0, 0.0);
            float diff = max(dot(normalize(Normal), -lightDir), 0.2);
            vec3 ambient = 0.6 * color;
            vec3 diffuse = diff * color;
            result = ambient + diffuse * 0.4;
        }
        
        FragColor = vec4(result, 1.0);
    }
)";

// Шейдеры для 2D интерфейса - ИСПРАВЬТЕ ИМЕНА UNIFORM ПЕРЕМЕННЫХ
const char* uiVertexShaderSource = R"(
    #version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 projection;
uniform mat4 model;

void main() {
    // Для 2D используем только projection * model
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";

const char* uiFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec3 color;
    uniform float alpha;
    
    void main() {
        FragColor = vec4(color, alpha);
    }
)";

ShaderManager::ShaderManager() : shaderProgram(0), uiShaderProgram(0) {}
void ShaderManager::setIsFloor(bool isFloor) const {
    GLint isFloorLoc = glGetUniformLocation(shaderProgram, "isFloor");
    if (isFloorLoc != -1) {
        glUniform1i(isFloorLoc, isFloor);
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
void ShaderManager::setCellSize(float cellSize) const {
    GLint cellSizeLoc = glGetUniformLocation(shaderProgram, "cellSize");
    if (cellSizeLoc != -1) {
        glUniform1f(cellSizeLoc, cellSize);
    }
}
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
    // ИСПРАВЛЕНО: используем сохраненный uiAlphaLoc вместо поиска каждый раз
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

    return true;
}

bool ShaderManager::createUIShaderProgram() {
    GLuint uiVertexShader = compileShader(GL_VERTEX_SHADER, uiVertexShaderSource);
    GLuint uiFragmentShader = compileShader(GL_FRAGMENT_SHADER, uiFragmentShaderSource);

    // Проверка компиляции шейдеров
    GLint success;
    glGetShaderiv(uiVertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(uiVertexShader, 512, NULL, infoLog);
        std::cerr << "UI Vertex Shader compilation error:\n" << infoLog << std::endl;
        return false;
    }

    glGetShaderiv(uiFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(uiFragmentShader, 512, NULL, infoLog);
        std::cerr << "UI Fragment Shader compilation error:\n" << infoLog << std::endl;
        return false;
    }

    uiShaderProgram = glCreateProgram();
    glAttachShader(uiShaderProgram, uiVertexShader);
    glAttachShader(uiShaderProgram, uiFragmentShader);
    glLinkProgram(uiShaderProgram);

    glGetProgramiv(uiShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(uiShaderProgram, 512, NULL, infoLog);
        std::cerr << "UI Shader program linking error:\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(uiVertexShader);
    glDeleteShader(uiFragmentShader);

    // Получаем uniform locations с проверкой
    uiProjectionLoc = glGetUniformLocation(uiShaderProgram, "projection");
    uiModelLoc = glGetUniformLocation(uiShaderProgram, "model");
    uiColorLoc = glGetUniformLocation(uiShaderProgram, "color");
    uiAlphaLoc = glGetUniformLocation(uiShaderProgram, "alpha");

    // Проверяем что все uniform найдены
    if (uiProjectionLoc == -1 || uiModelLoc == -1 || uiColorLoc == -1 || uiAlphaLoc == -1) {
        std::cerr << "Error: Failed to find some UI shader uniforms!" << std::endl;
        std::cerr << "Projection: " << uiProjectionLoc << ", Model: " << uiModelLoc
            << ", Color: " << uiColorLoc << ", Alpha: " << uiAlphaLoc << std::endl;
        return false;
    }

    std::cout << "UI Shader Uniform Locations (all found):" << std::endl;
    std::cout << "projection: " << uiProjectionLoc << std::endl;
    std::cout << "model: " << uiModelLoc << std::endl;
    std::cout << "color: " << uiColorLoc << std::endl;
    std::cout << "alpha: " << uiAlphaLoc << std::endl;

    return true;
}