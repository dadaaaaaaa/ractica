#include "pch.h"
#include "Game/Game.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Camera.h"
#include "Core/Constants.h"
#include "Core/Types.h"

// Глобальные экземпляры
ShaderManager g_shaderManager;
Camera g_camera;
Game g_game;
GLuint uiVAO = 0, uiVBO = 0;
bool g_shouldExitGame = false;

// ============================================================================
// КОЛБЭКИ GLFW
// ============================================================================

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        g_game.handleKeyPress(key);

        // Обработка выхода через ESC
        if (key == GLFW_KEY_ESCAPE) {
            GameState currentState = g_game.getGameState();

            if (currentState == PLAYING) {
                g_game.setGameState(PAUSED);
                std::cout << "⏸️ Game paused" << std::endl;
            }
            else if (currentState == PAUSED) {
                g_game.setGameState(PLAYING);
                std::cout << "▶️ Game resumed" << std::endl;
            }
            else if (currentState == MAIN_MENU) {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }
            else {
                g_game.setGameState(MAIN_MENU);
                std::cout << "🏠 Returning to main menu" << std::endl;
            }
        }

        // Выход из игры через Alt+F4 или закрытие окна
        if (key == GLFW_KEY_F4 && (mods & GLFW_MOD_ALT)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    g_game.handleMouseScroll(yoffset);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_game.setMousePressed(true);
            g_game.handleMouseClick();
        }
        else if (action == GLFW_RELEASE) {
            g_game.setMousePressed(false);
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    g_game.setMousePosition(xpos, ypos);
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
    g_game.setWindowSize(width, height);
    glViewport(0, 0, width, height);

    if (ENABLE_DEBUG_INFO) {
        std::cout << "🔄 Window resized to: " << width << "x" << height << std::endl;
    }
}

void windowCloseCallback(GLFWwindow* window) {
    std::cout << "⚠️ Window close requested" << std::endl;
    g_shouldExitGame = true;
}

// ============================================================================
// УТИЛИТЫ
// ============================================================================

void checkGLError(const char* functionName) {
    if (!ENABLE_DEBUG_INFO) return;

    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cout << "❌ OpenGL error " << error << " in " << functionName << std::endl;
    }
}

void printSystemInfo() {
    std::cout << "=== SYSTEM INFO ===" << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "GLFW version: " << glfwGetVersionString() << std::endl;
    std::cout << "==================" << std::endl;
}

bool initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "❌ Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Настройка OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Для Mac OS X
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ GLFW initialized successfully" << std::endl;
    }

    return true;
}

bool initializeGLEW() {
    // Включаем экспериментальный режим для core profile
    glewExperimental = GL_TRUE;

    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "❌ Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        return false;
    }

    // Игнорируем первую ошибку (если есть) из-за glewExperimental
    glGetError();

    // Проверяем поддержку необходимых функций
    if (!GLEW_VERSION_3_3) {
        std::cerr << "❌ OpenGL 3.3 not supported!" << std::endl;
        return false;
    }

    if (!glGenVertexArrays) {
        std::cerr << "❌ glGenVertexArrays not loaded!" << std::endl;
        return false;
    }

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ GLEW initialized successfully" << std::endl;
    }

    return true;
}

bool createWindow(GLFWwindow*& window) {
    window = glfwCreateWindow(
        DEFAULT_WINDOW_WIDTH,
        DEFAULT_WINDOW_HEIGHT,
        "3D Snake Game",
        NULL, NULL
    );

    if (!window) {
        std::cerr << "❌ Failed to create GLFW window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(window);

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ Window created successfully: "
            << DEFAULT_WINDOW_WIDTH << "x" << DEFAULT_WINDOW_HEIGHT << std::endl;
    }

    return true;
}

void setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ GLFW callbacks set up" << std::endl;
    }
}

void setupOpenGL() {
    // Основные настройки OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Цвет очистки
    glClearColor(
        BACKGROUND_COLOR_R,
        BACKGROUND_COLOR_G,
        BACKGROUND_COLOR_B,
        BACKGROUND_COLOR_A
    );

    checkGLError("OpenGL setup");

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ OpenGL configured successfully" << std::endl;
    }
}

void updateWindowTitle(GLFWwindow* window) {
    std::string title = "3D Snake Game";
    GameState currentState = g_game.getGameState();

    switch (currentState) {
    case PLAYING:
        title += " - Score: " + std::to_string(g_game.getScore());
        break;
    case PAUSED:
        title += " - PAUSED - Score: " + std::to_string(g_game.getScore());
        break;
    case GAME_OVER:
        title += " - GAME OVER - Score: " + std::to_string(g_game.getScore());
        break;
    case MAIN_MENU:
        title += " - Main Menu";
        break;
    case SETTINGS:
        title += " - Settings";
        break;
    case HIGH_SCORES:
        title += " - High Scores";
        break;
    case CONTROLS:
        title += " - Controls";
        break;
    }

    glfwSetWindowTitle(window, title.c_str());
}

void cleanup() {
    if (ENABLE_DEBUG_INFO) {
        std::cout << "🧹 Cleaning up resources..." << std::endl;
    }

    if (uiVAO) glDeleteVertexArrays(1, &uiVAO);
    if (uiVBO) glDeleteBuffers(1, &uiVBO);

    g_game.cleanup();

    glfwTerminate();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "✅ Cleanup completed" << std::endl;
    }
}

// ============================================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================================

int main() {
    std::cout << "🚀 Starting 3D Snake Game..." << std::endl;

    // Инициализация GLFW
    if (!initializeGLFW()) {
        return -1;
    }

    // Создание окна
    GLFWwindow* window = nullptr;
    if (!createWindow(window)) {
        glfwTerminate();
        return -1;
    }

    // Инициализация GLEW
    if (!initializeGLEW()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Вывод информации о системе
    printSystemInfo();

    // Настройка колбэков
    setupCallbacks(window);

    // Настройка OpenGL
    setupOpenGL();

    // Инициализация генератора случайных чисел
    srand(static_cast<unsigned int>(time(nullptr)));

    // Инициализация шейдеров
    std::cout << "🎨 Initializing shaders..." << std::endl;
    if (!g_shaderManager.initialize()) {
        std::cerr << "❌ Failed to initialize shaders!" << std::endl;
        cleanup();
        return -1;
    }

    // Инициализация игры
    std::cout << "🎮 Initializing game..." << std::endl;
    if (!g_game.isRunning()) {
        g_game.initialize();
    }

    // Первоначальная отрисовка
    g_game.render();
    glfwSwapBuffers(window);

    std::cout << "✅ Game started successfully!" << std::endl;

    // Главный игровой цикл
    double lastUpdateTime = glfwGetTime();
    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window) && !g_shouldExitGame) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;

        // Обновление заголовка окна
        updateWindowTitle(window);

        // Обновление игры с фиксированным временным шагом
        GameState currentState = g_game.getGameState();
        if (currentState == PLAYING && currentTime - lastUpdateTime > g_game.getGameSpeed()) {
            g_game.update();
            lastUpdateTime = currentTime;
        }

        // Рендеринг
        g_game.render();

        // Обмен буферов и обработка событий
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Ограничение FPS для экономии ресурсов
        if (deltaTime < MIN_FRAME_TIME) {
            double sleepTime = MIN_FRAME_TIME - deltaTime;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }

        lastFrameTime = currentTime;
    }

    // Очистка ресурсов
    cleanup();

    std::cout << "👋 Game exited successfully" << std::endl;
    return 0;
}