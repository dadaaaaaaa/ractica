#include "pch.h"
#include "Game/Game.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Camera.h"

// Глобальные экземпляры
ShaderManager g_shaderManager;
Camera g_camera;
Game g_game;
GLuint uiVAO = 0, uiVBO = 0;

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

    // Инициализируем GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cout << "Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Устанавливаем колбэки
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    glEnable(GL_DEPTH_TEST);

    srand(static_cast<unsigned int>(time(0)));

    // Инициализируем системы
    std::cout << "Initializing shaders..." << std::endl;
    if (!g_shaderManager.initialize()) {
        std::cerr << "Failed to initialize shaders!" << std::endl;
        return -1;
    }

    std::cout << "Initializing game..." << std::endl;
    g_game.initialize();

    std::cout << "Game started successfully!" << std::endl;

    double lastUpdateTime = glfwGetTime();
    double lastSpriteUpdateTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
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
            g_game.updateClouds();
            g_game.updateBirds();
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
        // В главном цикле после g_game.render();
        std::cout << "Game state: " << static_cast<int>(g_game.getGameState()) << std::endl;
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