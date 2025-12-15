// ShadowRenderer.cpp
#include "ShadowRenderer.h"
#include <iostream>

// Конструктор
ShadowRenderer::ShadowRenderer() {
    : depthMapFBO(0),
        depthMap(0),
        shadowShaderProgram(0),
        cubeVAO(0),
        cubeVBO(0)
    {
        // Инициализация матрицы света
        lightSpaceMatrix = glm::mat4(1.0f);
    }
}
// Деструктор
ShadowRenderer::~ShadowRenderer() {
    // Освобождаем ресурсы OpenGL
    if (depthMapFBO != 0) {
        glDeleteFramebuffers(1, &depthMapFBO);
    }
    if (depthMap != 0) {
        glDeleteTextures(1, &depthMap);
    }
    if (shadowShaderProgram != 0) {
        glDeleteProgram(shadowShaderProgram);
    }
    if (cubeVAO != 0) {
        glDeleteVertexArrays(1, &cubeVAO);
    }
    if (cubeVBO != 0) {
        glDeleteBuffers(1, &cubeVBO);
    }
}

// Инициализация
bool ShadowRenderer::initialize() {
    std::cout << "Initializing ShadowRenderer..." << std::endl;

    if (!compileShadowShader()) {
        std::cerr << "ERROR: Failed to compile shadow shader!" << std::endl;
        return false;
    }

    if (!createShadowMap()) {
        std::cerr << "ERROR: Failed to create shadow map!" << std::endl;
        return false;
    }

    setupCubeVAO();

    std::cout << "ShadowRenderer initialized successfully!" << std::endl;
    return true;
}

// Компиляция шейдера теней
bool ShadowRenderer::compileShadowShader() {
    const char* shadowVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 lightSpaceMatrix;
        uniform mat4 model;
        
        void main() {
            gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
        }
    )";

    const char* shadowFragmentShaderSource = R"(
        #version 330 core
        
        void main() {
            // Фрагментный шейдер для теней - ничего не делает
        }
    )";

    // Вершинный шейдер
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &shadowVertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Shadow vertex shader compilation failed:\n" << infoLog << std::endl;
        return false;
    }

    // Фрагментный шейдер
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &shadowFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Shadow fragment shader compilation failed:\n" << infoLog << std::endl;
        return false;
    }

    // Создание программы шейдера
    shadowShaderProgram = glCreateProgram();
    glAttachShader(shadowShaderProgram, vertexShader);
    glAttachShader(shadowShaderProgram, fragmentShader);
    glLinkProgram(shadowShaderProgram);

    glGetProgramiv(shadowShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shadowShaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shadow shader linking failed:\n" << infoLog << std::endl;
        return false;
    }

    // Удаляем шейдеры после линковки
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "Shadow shader compiled successfully!" << std::endl;
    return true;
}

// Создание карты теней
bool ShadowRenderer::createShadowMap() {
    // Создаем FBO
    glGenFramebuffers(1, &depthMapFBO);

    // Создаем текстуру глубины
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Настройка параметров текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Привязываем текстуру к FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    // Отключаем вывод цвета
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Проверяем статус FBO
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Shadow framebuffer is not complete!" << std::endl;
        return false;
    }

    // Возвращаемся к дефолтному FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "Shadow map created: " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << std::endl;
    return true;
}

// Настройка VAO для куба
void ShadowRenderer::setupCubeVAO() {
    // Вершины куба
    float vertices[] = {
        // back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // left face
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // right face
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

         // bottom face
         -0.5f, -0.5f, -0.5f,
          0.5f, -0.5f, -0.5f,
          0.5f, -0.5f,  0.5f,
          0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f, -0.5f,

         // top face
         -0.5f,  0.5f, -0.5f,
          0.5f,  0.5f, -0.5f,
          0.5f,  0.5f,  0.5f,
          0.5f,  0.5f,  0.5f,
         -0.5f,  0.5f,  0.5f,
         -0.5f,  0.5f, -0.5f,
    };

    // Создаем VAO и VBO
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    // Настраиваем VAO
    glBindVertexArray(cubeVAO);

    // Настраиваем VBO
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Настраиваем атрибуты вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Отвязываем буферы
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Cube VAO created for shadow rendering" << std::endl;
}

// Начало рендера в карту теней
void ShadowRenderer::beginShadowPass(const glm::vec3& lightPos) {
    // Устанавливаем viewport для карты теней
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // Привязываем FBO теней
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Вычисляем матрицу света
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 30.0f);
    glm::mat4 lightView = glm::lookAt(lightPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceMatrix = lightProjection * lightView;

    // Используем шейдер теней
    glUseProgram(shadowShaderProgram);

    // Устанавливаем uniform матрицы света
    GLuint lightSpaceLoc = glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix");
    if (lightSpaceLoc != -1) {
        glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    }

    // Включаем culling для уменьшения артефактов
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

// Конец рендера в карту теней
void ShadowRenderer::endShadowPass() {
    // Отвязываем FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Возвращаем culling в нормальное состояние
    glCullFace(GL_BACK);
}

// Получение текстуры теней
GLuint ShadowRenderer::getShadowTexture() const {
    return depthMap;
}

// Получение матрицы света
const glm::mat4& ShadowRenderer::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

// Рендер простого куба для теней
void ShadowRenderer::renderSimpleCube(const glm::mat4& modelMatrix) {
    // Устанавливаем uniform матрицы модели
    GLuint modelLoc = glGetUniformLocation(shadowShaderProgram, "model");
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }

    // Рендерим куб
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}