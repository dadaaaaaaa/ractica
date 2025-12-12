#include "pch.h"
#include "Game/Game.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Camera.h"
#include "Core/Constants.h"

// Глобальные экземпляры
ShaderManager g_shaderManager;
Camera g_camera;
Game g_game;
GLuint uiVAO = 0, uiVBO = 0;
bool g_shouldExitGame = false;

// Колбэки GLFW
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        g_game.handleKeyPress(key);

        // Обработка выхода
        if (key == GLFW_KEY_ESCAPE) {
            if (g_game.getGameState() == PLAYING) {
                g_game.setGameState(PAUSED);
            }
            else if (g_game.getGameState() == PAUSED) {
                g_game.setGameState(PLAYING);
            }
            else if (g_game.getGameState() == MAIN_MENU) {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }
            else {
                g_game.setGameState(MAIN_MENU);
            }
        }
    }
}

void charCallback(GLFWwindow* window, unsigned int codepoint) {
    if (g_game.getGameState() == SETTINGS && g_game.isNameInputActive()) {
        // Конвертируем Unicode в символ и добавляем к имени
        if (codepoint < 128) {
            char c = static_cast<char>(codepoint);
            // Фильтруем только разрешенные символы
            if (isalnum(c) || c == ' ' || c == '-' || c == '_') {
                g_game.addCharacterToName(c);
            }
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    g_game.handleMouseScroll(yoffset);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        g_game.setMousePressed(true);
        g_game.handleMouseClick();
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        g_game.setMousePressed(false);
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    g_game.setMousePosition(xpos, ypos);
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
    g_game.setWindowSize(width, height);
    glViewport(0, 0, width, height);
}

// Функция для проверки ошибок OpenGL
void checkGLError(const char* functionName) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL error " << error << " in " << functionName << std::endl;
    }
}

// Вспомогательная функция для проверки поддержки двойной буферизации
void checkDoubleBufferSupport(GLFWwindow* window) {
    int doubleBuffer = glfwGetWindowAttrib(window, GLFW_DOUBLEBUFFER);
    if (doubleBuffer) {
        std::cout << "Double buffering is ENABLED" << std::endl;
    }
    else {
        std::cout << "WARNING: Double buffering is DISABLED" << std::endl;
    }
}

// Вспомогательная функция для настройки вертикальной синхронизации (VSync)
void setupVSync(GLFWwindow* window, bool enabled = true) {
    if (enabled) {
        glfwSwapInterval(1); // Включить VSync (60 FPS)
        std::cout << "VSync ENABLED" << std::endl;
    }
    else {
        glfwSwapInterval(0); // Выключить VSync (неограниченный FPS)
        std::cout << "VSync DISABLED" << std::endl;
    }
}

// Вспомогательная функция для вывода информации о графической системе
void printGraphicsInfo() {
    std::cout << "=== GRAPHICS SYSTEM INFO ===" << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Проверяем доступные расширения
    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    bool hasDoubleBuffer = false;

    for (GLint i = 0; i < numExtensions; i++) {
        const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (strstr(extension, "double_buffer") != nullptr) {
            hasDoubleBuffer = true;
        }
    }
    std::cout << "Double buffer support: " << (hasDoubleBuffer ? "YES" : "NO") << std::endl;
    std::cout << "===========================" << std::endl;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // ★ НАСТРОЙКА ОКНА С ДВОЙНОЙ БУФЕРИЗАЦИЕЙ ★
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // ★ Включение двойной буферизации
    glfwWindowHint(GLFW_SAMPLES, 4); // Многократное сглаживание (MSAA)
    glfwWindowHint(GLFW_DEPTH_BITS, 24); // 24-битный буфер глубины
    glfwWindowHint(GLFW_STENCIL_BITS, 8); // 8-битный буфер трафарета
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "3D Snake Game", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Инициализируем GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        glfwTerminate();
        return -1;
    }

    // Игнорируем первую ошибку (если есть) из-за glewExperimental
    glGetError();

    // Проверяем и выводим информацию о системе
    checkDoubleBufferSupport(window);
    setupVSync(window, true);
    printGraphicsInfo();

    // Проверяем поддержку необходимых функций
    if (!GLEW_VERSION_3_3) {
        std::cerr << "OpenGL 3.3 not supported!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Проверяем, что функции загружены
    if (!glGenVertexArrays) {
        std::cerr << "glGenVertexArrays not loaded!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Устанавливаем колбэки
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetCharCallback(window, charCallback);

    // Настраиваем OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Включаем MSAA если поддерживается
    GLint samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (samples > 0) {
        glEnable(GL_MULTISAMPLE);
        std::cout << "MSAA enabled: " << samples << "x samples" << std::endl;
    }

    // Настраиваем смешивание для прозрачности
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Настраиваем очистку
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    checkGLError("OpenGL setup");

    srand(static_cast<unsigned int>(time(0)));

    // Инициализируем системы
    std::cout << "Initializing shaders..." << std::endl;
    if (!g_shaderManager.initialize()) {
        std::cerr << "Failed to initialize shaders!" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "Initializing game..." << std::endl;
    g_game.initialize();

    // ★ ОЧИСТКА И ОТРИСОВКА ПЕРВОГО КАДРА ★
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_game.render();
    glfwSwapBuffers(window);

    std::cout << "Game started successfully!" << std::endl;

    // Переменные для управления временем
    double lastUpdateTime = glfwGetTime();
    double lastFrameTime = glfwGetTime();
    int frameCount = 0;
    double fpsUpdateTime = lastFrameTime;
    double frameTimeAccumulator = 0.0;
    const double fixedDeltaTime = 1.0 / 60.0; // 60 FPS для фиксированного обновления

    // ★ ОСНОВНОЙ ЦИКЛ РЕНДЕРИНГА С ДВОЙНОЙ БУФЕРИЗАЦИЕЙ ★
    while (!glfwWindowShouldClose(window) && !g_shouldExitGame) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        // Обновление с фиксированным временным шагом
        frameTimeAccumulator += deltaTime;
        while (frameTimeAccumulator >= fixedDeltaTime) {
            // Обновление логики игры
            if (currentTime - lastUpdateTime > g_game.getGameSpeed()) {
                g_game.update();
                lastUpdateTime = currentTime;
            }

            // Обновление камеры
            if (g_game.getGameState() == PLAYING ||
                g_game.getGameState() == PAUSED ||
                g_game.getGameState() == GAME_OVER) {
                if (!g_game.getSnake().empty()) {
                    const auto& head = g_game.getSnake()[0];
                    glm::vec3 headPos = glm::vec3(
                        (head.x - GRID_WIDTH / 2.0f) * CELL_SIZE,
                        head.y * CELL_SIZE,
                        (head.z - GRID_DEPTH / 2.0f) * CELL_SIZE
                    );
                    g_camera.update(headPos, fixedDeltaTime);
                }
            }

            frameTimeAccumulator -= fixedDeltaTime;
        }

        // Обновление FPS в заголовке окна каждую секунду
        frameCount++;
        if (currentTime - fpsUpdateTime >= 1.0) {
            double fps = frameCount / (currentTime - fpsUpdateTime);
            frameCount = 0;
            fpsUpdateTime = currentTime;

            // Обновление заголовка окна
            std::string title = "3D Snake Game";
            if (fps > 0) {
                title += " | FPS: " + std::to_string(static_cast<int>(fps));
            }

            GameState currentState = g_game.getGameState();
            if (currentState == PLAYING) {
                title += " | Score: " + std::to_string(g_game.getScore());
            }
            else if (currentState == PAUSED) {
                title += " | PAUSED | Score: " + std::to_string(g_game.getScore());
            }
            else if (currentState == GAME_OVER) {
                title += " | GAME OVER | Score: " + std::to_string(g_game.getScore());
            }
            glfwSetWindowTitle(window, title.c_str());
        }

        // ★ КРИТИЧЕСКАЯ ЧАСТЬ: РЕНДЕРИНГ С ДВОЙНОЙ БУФЕРИЗАЦИЕЙ ★
        // 1. Очищаем буферы
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. Рендерим сцену в задний буфер
        g_game.render();

        // 3. Меняем буферы местами (отображаем задний буфер)
        glfwSwapBuffers(window);

        // 4. Обрабатываем события
        glfwPollEvents();
    }

    // Очистка ресурсов
    std::cout << "Cleaning up resources..." << std::endl;
    if (uiVAO) {
        glDeleteVertexArrays(1, &uiVAO);
        uiVAO = 0;
    }
    if (uiVBO) {
        glDeleteBuffers(1, &uiVBO);
        uiVBO = 0;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Game terminated successfully." << std::endl;
    return 0;
}