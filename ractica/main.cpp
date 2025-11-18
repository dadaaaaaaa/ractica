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

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "3D Snake Game", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Инициализируем GLEW ПРАВИЛЬНО
    glewExperimental = GL_TRUE; // Это важно для core profile
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cout << "Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        glfwTerminate();
        return -1;
    }

    // Игнорируем первую ошибку (если есть) из-за glewExperimental
    glGetError();

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;

    // Проверяем поддержку необходимых функций
    if (!GLEW_VERSION_3_3) {
        std::cout << "OpenGL 3.3 not supported!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Проверяем, что функции загружены
    if (!glGenVertexArrays) {
        std::cout << "glGenVertexArrays not loaded!" << std::endl;
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
    glEnable(GL_DEPTH_TEST);
    checkGLError("glEnable");

    srand(static_cast<unsigned int>(time(0)));

    // Теперь инициализируем системы ПОСЛЕ проверки GLEW
    std::cout << "Initializing shaders..." << std::endl;
    if (!g_shaderManager.initialize()) {
        std::cerr << "Failed to initialize shaders!" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "Initializing game..." << std::endl;
    g_game.initialize();

    // Принудительно вызываем отрисовку главного меню один раз
    g_game.render();
    glfwSwapBuffers(window);

    std::cout << "Game started successfully!" << std::endl;

    double lastUpdateTime = glfwGetTime();
    double lastSpriteUpdateTime = glfwGetTime();

    while (!glfwWindowShouldClose(window) && !g_shouldExitGame) {
        double currentTime = glfwGetTime();

        // Обновление заголовка окна
        std::string title = "3D Snake Game";
        GameState currentState = g_game.getGameState();
        if (currentState == PLAYING) {
            title += " - Score: " + std::to_string(g_game.getScore());
        }
        else if (currentState == PAUSED) {
            title += " - PAUSED - Score: " + std::to_string(g_game.getScore());
        }
        else if (currentState == GAME_OVER) {
            title += " - GAME OVER - Score: " + std::to_string(g_game.getScore());
        }
        glfwSetWindowTitle(window, title.c_str());

        // Обновление игры
        if (currentState == PLAYING && currentTime - lastUpdateTime > g_game.getGameSpeed()) {
            g_game.update();
            lastUpdateTime = currentTime;
        }

        // Обновление спрайтов
        if (currentTime - lastSpriteUpdateTime > 0.05) {
            lastSpriteUpdateTime = currentTime;
        }

        // Обновление камеры
        if (currentState == PLAYING || currentState == PAUSED || currentState == GAME_OVER) {
            if (!g_game.getSnake().empty()) {
                const auto& head = g_game.getSnake()[0];
                glm::vec3 headPos = glm::vec3(
                    (head.x - GRID_WIDTH / 2.0f) * CELL_SIZE,
                    head.y * CELL_SIZE,
                    (head.z - GRID_DEPTH / 2.0f) * CELL_SIZE
                );
                g_camera.update(headPos, 0.016f);
            }
        }

        // Рендеринг
        g_game.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка ресурсов
    if (uiVAO) glDeleteVertexArrays(1, &uiVAO);
    if (uiVBO) glDeleteBuffers(1, &uiVBO);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}