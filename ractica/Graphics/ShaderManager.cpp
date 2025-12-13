#include "../pch.h"
#include "ShaderManager.h"
#include <iostream>

// Шейдеры для 3D (оставьте без изменений)
// Новый вершинный шейдер с поддержкой теней
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 lightSpaceMatrix;
    
    out vec3 Normal;
    out vec3 FragPos;
    out vec2 TexCoords;
    out vec4 FragPosLightSpace;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoords = aTexCoords;
        FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

// Новый фрагментный шейдер с тенями и клетчатым полом
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    in vec4 FragPosLightSpace;
    
    uniform vec3 color;
    uniform bool useTexture;
    uniform bool useShadows;
    uniform vec3 lightPos;
    uniform sampler2D shadowMap;
    
    // Функция для расчета теней
    float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
        // Преобразование в NDC
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        
        if (projCoords.z > 1.0) return 0.0;
        
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        float currentDepth = projCoords.z;
        
        // Bias для предотвращения shadow acne
        float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
        
        // PCF для сглаживания
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for(int x = -1; x <= 1; ++x) {
            for(int y = -1; y <= 1; ++y) {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
        
        return shadow;
    }
    
    // Функция для создания клетчатого узора (как в тетради)
    vec3 createNotebookGridPattern(vec2 texCoords, vec3 baseColor) {
        float cellSize = 0.2; // Размер клетки
        float lineWidth = 0.015; // Толщина линий
        
        // Увеличиваем масштаб для более четких линий
        vec2 scaledCoords = texCoords * 8.0; // 8x8 клеток
        
        // Основной цвет клетки (зеленый как в тетради)
        vec3 cellColor = vec3(0.7, 0.95, 0.7); // Светло-зеленый
        
        // Определяем границы клеток
        vec2 gridLines = abs(fract(scaledCoords) - 0.5) * 2.0;
        float lineFactor = smoothstep(0.5 - lineWidth, 0.5 + lineWidth, max(gridLines.x, gridLines.y));
        
        // Более темные основные линии каждые 5 клеток
        vec2 majorLines = mod(scaledCoords, 5.0);
        bool isMajorLine = (majorLines.x < 0.05 || majorLines.x > 4.95 || 
                           majorLines.y < 0.05 || majorLines.y > 4.95);
        
        if (isMajorLine) {
            cellColor = vec3(0.4, 0.7, 0.4); // Темно-зеленые основные линии
            lineWidth *= 1.5; // Основные линии толще
        }
        
        // Чередование оттенков для шахматного узора
        vec2 cellIndex = floor(scaledCoords);
        float isEvenCell = mod(cellIndex.x + cellIndex.y, 2.0);
        
        // Если это клетка (не линия), применяем шахматный узор
        if (lineFactor < 0.5) {
            if (isEvenCell > 0.5) {
                cellColor *= 0.95; // Чуть темнее для чередования
            }
        }
        
        // Плавный переход между линиями и клетками
        return mix(cellColor, vec3(0.3, 0.6, 0.3), lineFactor);
    }
    
    void main() {
        vec3 result;
        vec3 lightDir = normalize(lightPos - FragPos);
        
        // Расчет освещения
        float ambientStrength = 0.4;
        float diff = max(dot(normalize(Normal), lightDir), 0.0);
        
        // Тени
        float shadow = 0.0;
        if (useShadows) {
            shadow = ShadowCalculation(FragPosLightSpace, normalize(Normal), lightDir);
        }
        
        if (useTexture) {
            // Проверяем, это пол или другой объект
            // Для пола используем Y-нормаль для определения
            if (abs(Normal.y) > 0.9 && color.g > 0.5) { // Зеленый цвет + горизонтальная поверхность
                // Это пол - применяем клетчатый узор тетради
                result = createNotebookGridPattern(TexCoords, color);
            } else {
                // Остальные объекты с их текстурами
                // ... существующая логика текстур ...
                float pattern = sin(TexCoords.x * 20.0) * sin(TexCoords.y * 20.0) * 0.3 + 0.7;
                result = color * pattern;
            }
            
            // Применяем освещение и тени
            vec3 ambient = ambientStrength * result;
            vec3 diffuse = diff * result;
            result = ambient + (1.0 - shadow * 0.7) * diffuse;
        } else {
            vec3 ambient = ambientStrength * color;
            vec3 diffuse = diff * color;
            result = ambient + (1.0 - shadow * 0.5) * diffuse;
        }
        
        // Легкое затенение по краям (rim lighting)
        float rim = 1.0 - abs(dot(normalize(Normal), normalize(-FragPos)));
        result += rim * 0.1 * color;
        
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
    lightSpaceMatrixLoc = glGetUniformLocation(shaderProgram, "lightSpaceMatrix");
    shadowMapLoc = glGetUniformLocation(shaderProgram, "shadowMap");
    useShadowsLoc = glGetUniformLocation(shaderProgram, "useShadows");
    lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");

    return true;
}
void ShaderManager::setLightSpaceMatrix(const glm::mat4& lightSpaceMatrix) const {
    if (lightSpaceMatrixLoc != -1) {
        glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    }
}

void ShaderManager::setShadowMap(GLuint textureID) const {
    if (shadowMapLoc != -1) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(shadowMapLoc, 1);
    }
}

void ShaderManager::setUseShadows(bool useShadows) const {
    if (useShadowsLoc != -1) {
        glUniform1i(useShadowsLoc, useShadows);
    }
}

void ShaderManager::setLightPosition(const glm::vec3& lightPos) const {
    if (lightPosLoc != -1) {
        glUniform3fv(lightPosLoc, 1, &lightPos[0]);
    }
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