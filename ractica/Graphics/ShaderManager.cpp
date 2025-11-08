#include "../pch.h"
#include "ShaderManager.h"
#include <iostream>

// ============================================================================
// 3D ШЕЙДЕРЫ ДЛЯ ОСНОВНОЙ ГРАФИКИ
// ============================================================================

// Вершинный шейдер для 3D объектов
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

// Фрагментный шейдер для 3D объектов с процедурными текстурами
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    
    uniform vec3 color;
    uniform bool useTexture;
    
    void main() {
        vec3 result;
        
        if (useTexture) {
            // Процедурные текстуры для разных типов объектов
            if (color.r > 0.8 && color.g < 0.2) {
                // Текстура дерева
                float woodPattern = sin(TexCoords.x * 30.0) * 0.3 + 0.7;
                float ringPattern = sin(TexCoords.y * 15.0) * 0.2 + 0.8;
                result = color * woodPattern * ringPattern;
            }
            else if (color.g > 0.8 && color.b < 0.3) {
                // Текстура травы
                float grassPattern = sin(TexCoords.x * 50.0) * sin(TexCoords.y * 50.0) * 0.4 + 0.6;
                result = color * grassPattern;
            }
            else if (color.r > 0.8 && color.g > 0.8) {
                // Текстура с пятнами
                float spots = step(0.8, sin(TexCoords.x * 40.0) * sin(TexCoords.y * 40.0));
                result = color * (0.8 + spots * 0.2);
            }
            else if (color.r > 0.9 && color.g > 0.9 && color.b > 0.9) {
                // Текстура облаков
                float cloud = sin(TexCoords.x * 25.0) * sin(TexCoords.y * 25.0) * 0.5 + 0.5;
                result = color * cloud;
            }
            else if (color.b > 0.8) {
                // Текстура птиц
                float birdPattern = sin(TexCoords.x * 60.0) * 0.4 + 0.6;
                result = color * birdPattern;
            }
            else if (color.r > 0.8 || color.g > 0.8 || color.b > 0.8) {
                // Текстура цветов
                float flowerPattern = sin(TexCoords.x * 35.0) * cos(TexCoords.y * 35.0) * 0.3 + 0.7;
                result = color * flowerPattern;
            }
            else {
                // Общая процедурная текстура
                float pattern = sin(TexCoords.x * 20.0) * sin(TexCoords.y * 20.0) * 0.3 + 0.7;
                result = color * pattern;
            }
            
            // Простое освещение
            vec3 lightDir = vec3(0.0, -1.0, 0.0);
            float diff = max(dot(normalize(Normal), -lightDir), 0.3);
            result = result * (0.7 + 0.3 * diff);
        } else {
            // Без текстуры - простое освещение
            vec3 lightDir = vec3(0.0, -1.0, 0.0);
            float diff = max(dot(normalize(Normal), -lightDir), 0.2);
            vec3 ambient = 0.6 * color;
            vec3 diffuse = diff * color;
            result = ambient + diffuse * 0.4;
        }
        
        FragColor = vec4(result, 1.0);
    }
)";

// ============================================================================
// 2D ШЕЙДЕРЫ ДЛЯ ПОЛЬЗОВАТЕЛЬСКОГО ИНТЕРФЕЙСА
// ============================================================================

// Вершинный шейдер для UI элементов
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

// Фрагментный шейдер для UI элементов с поддержкой прозрачности
const char* uiFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec3 color;
    uniform float alpha;
    
    void main() {
        FragColor = vec4(color, alpha);
    }
)";

// ============================================================================
// РЕАЛИЗАЦИЯ КЛАССА SHADERMANAGER
// ============================================================================

// Конструктор - инициализирует шейдерные программы нулевыми значениями
ShaderManager::ShaderManager()
    : shaderProgram(0), uiShaderProgram(0),
    modelLoc(-1), viewLoc(-1), projectionLoc(-1), colorLoc(-1), useTextureLoc(-1),
    uiProjectionLoc(-1), uiModelLoc(-1), uiColorLoc(-1), uiAlphaLoc(-1) {
}

// Деструктор - очищает шейдерные программы
ShaderManager::~ShaderManager() {
    cleanup();
}

// Инициализирует все шейдерные программы
bool ShaderManager::initialize() {
    std::cout << "🎮 Initializing shader manager..." << std::endl;

    bool success = createShaderProgram() && createUIShaderProgram();

    if (success) {
        std::cout << "✅ Shader manager initialized successfully" << std::endl;
    }
    else {
        std::cerr << "❌ Failed to initialize shader manager" << std::endl;
    }

    return success;
}

// Очищает все шейдерные ресурсы
void ShaderManager::cleanup() {
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        std::cout << "🗑️  Deleted 3D shader program" << std::endl;
    }

    if (uiShaderProgram) {
        glDeleteProgram(uiShaderProgram);
        uiShaderProgram = 0;
        std::cout << "🗑️  Deleted UI shader program" << std::endl;
    }
}

// Активирует 3D шейдер для рендеринга игровых объектов
void ShaderManager::use3DShader() const {
    if (shaderProgram) {
        glUseProgram(shaderProgram);
    }
}

// Активирует UI шейдер для рендеринга интерфейса
void ShaderManager::useUIShader() const {
    if (uiShaderProgram) {
        glUseProgram(uiShaderProgram);
    }
}

// ============================================================================
// МЕТОДЫ ДЛЯ 3D ШЕЙДЕРА
// ============================================================================

// Устанавливает матрицу модели для 3D объектов
void ShaderManager::setModelMatrix(const glm::mat4& model) const {
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    }
}

// Устанавливает матрицу вида для 3D сцены
void ShaderManager::setViewMatrix(const glm::mat4& view) const {
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
}

// Устанавливает матрицу проекции для 3D сцены
void ShaderManager::setProjectionMatrix(const glm::mat4& projection) const {
    if (projectionLoc != -1) {
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
}

// Устанавливает цвет для 3D объектов
void ShaderManager::setColor(const glm::vec3& color) const {
    if (colorLoc != -1) {
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));
    }
}

// Включает/выключает использование текстур для 3D объектов
void ShaderManager::setUseTexture(bool useTexture) const {
    if (useTextureLoc != -1) {
        glUniform1i(useTextureLoc, useTexture);
    }
}

// ============================================================================
// МЕТОДЫ ДЛЯ UI ШЕЙДЕРА
// ============================================================================

// Устанавливает матрицу проекции для UI элементов
void ShaderManager::setUIProjectionMatrix(const glm::mat4& projection) const {
    if (uiProjectionLoc != -1) {
        glUniformMatrix4fv(uiProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
}

// Устанавливает матрицу модели для UI элементов
void ShaderManager::setUIModelMatrix(const glm::mat4& model) const {
    if (uiModelLoc != -1) {
        glUniformMatrix4fv(uiModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    }
}

// Устанавливает цвет для UI элементов
void ShaderManager::setUIColor(const glm::vec3& color) const {
    if (uiColorLoc != -1) {
        glUniform3fv(uiColorLoc, 1, glm::value_ptr(color));
    }
}

// Устанавливает прозрачность для UI элементов
void ShaderManager::setUIAlpha(float alpha) const {
    if (uiAlphaLoc != -1) {
        glUniform1f(uiAlphaLoc, alpha);
    }
}

// ============================================================================
// ПРИВАТНЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// ============================================================================

// Компилирует шейдер из исходного кода
GLuint ShaderManager::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Проверяем успешность компиляции
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "❌ Shader compilation error ("
            << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") << "):\n"
            << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Создает шейдерную программу для 3D графики
bool ShaderManager::createShaderProgram() {
    std::cout << "🔧 Creating 3D shader program..." << std::endl;

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (!vertexShader || !fragmentShader) {
        std::cerr << "❌ Failed to compile 3D shaders" << std::endl;
        return false;
    }

    // Создаем и линкуем шейдерную программу
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Проверяем успешность линковки
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "❌ 3D Shader program linking error:\n" << infoLog << std::endl;

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Очищаем скомпилированные шейдеры (они больше не нужны)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Получаем location uniform переменных
    modelLoc = glGetUniformLocation(shaderProgram, "model");
    viewLoc = glGetUniformLocation(shaderProgram, "view");
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    colorLoc = glGetUniformLocation(shaderProgram, "color");
    useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");

    // Проверяем что все uniform найдены
    if (modelLoc == -1 || viewLoc == -1 || projectionLoc == -1 ||
        colorLoc == -1 || useTextureLoc == -1) {
        std::cerr << "❌ Failed to find some 3D shader uniforms" << std::endl;
        return false;
    }

    std::cout << "✅ 3D shader program created successfully" << std::endl;
    return true;
}

// Создает шейдерную программу для UI элементов
bool ShaderManager::createUIShaderProgram() {
    std::cout << "🔧 Creating UI shader program..." << std::endl;

    GLuint uiVertexShader = compileShader(GL_VERTEX_SHADER, uiVertexShaderSource);
    GLuint uiFragmentShader = compileShader(GL_FRAGMENT_SHADER, uiFragmentShaderSource);

    if (!uiVertexShader || !uiFragmentShader) {
        std::cerr << "❌ Failed to compile UI shaders" << std::endl;
        return false;
    }

    // Создаем и линкуем UI шейдерную программу
    uiShaderProgram = glCreateProgram();
    glAttachShader(uiShaderProgram, uiVertexShader);
    glAttachShader(uiShaderProgram, uiFragmentShader);
    glLinkProgram(uiShaderProgram);

    // Проверяем успешность линковки
    GLint success;
    glGetProgramiv(uiShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(uiShaderProgram, 512, NULL, infoLog);
        std::cerr << "❌ UI Shader program linking error:\n" << infoLog << std::endl;

        glDeleteShader(uiVertexShader);
        glDeleteShader(uiFragmentShader);
        return false;
    }

    // Очищаем скомпилированные шейдеры
    glDeleteShader(uiVertexShader);
    glDeleteShader(uiFragmentShader);

    // Получаем location uniform переменных для UI
    uiProjectionLoc = glGetUniformLocation(uiShaderProgram, "projection");
    uiModelLoc = glGetUniformLocation(uiShaderProgram, "model");
    uiColorLoc = glGetUniformLocation(uiShaderProgram, "color");
    uiAlphaLoc = glGetUniformLocation(uiShaderProgram, "alpha");

    // Проверяем что все UI uniform найдены
    if (uiProjectionLoc == -1 || uiModelLoc == -1 || uiColorLoc == -1 || uiAlphaLoc == -1) {
        std::cerr << "❌ Failed to find some UI shader uniforms!" << std::endl;
        std::cerr << "Projection: " << uiProjectionLoc
            << ", Model: " << uiModelLoc
            << ", Color: " << uiColorLoc
            << ", Alpha: " << uiAlphaLoc << std::endl;
        return false;
    }

    std::cout << "✅ UI shader program created successfully" << std::endl;
    std::cout << "📊 UI Uniform Locations - Projection: " << uiProjectionLoc
        << ", Model: " << uiModelLoc
        << ", Color: " << uiColorLoc
        << ", Alpha: " << uiAlphaLoc << std::endl;

    return true;
}