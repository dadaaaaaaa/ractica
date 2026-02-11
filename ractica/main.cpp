#include "pch.h"
#include "Game/Game.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Camera.h"
#include "Graphics/GameRenderer.h"
#include "Core/Constants.h"

// Глобальные экземпляры
ShaderManager g_shaderManager;
Camera g_camera;
Game g_game;
GLuint uiVAO = 0, uiVBO = 0;
bool g_shouldExitGame = false;
GLFWwindow* g_mainWindow = nullptr; // ДОБАВЛЕНО
static bool g_useDoubleBuffer = true;

int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        
        return -1;
    }

    // Настройка окна с помощью GameRenderer
    GameRenderer::setupGLFWHints();
    GLFWwindow* window = glfwCreateWindow(1200, 800, "3D Snake Game", NULL, NULL);
    g_mainWindow = window; // Сохраняем ссылку на окно
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Инициализация GLEW
    if (!GameRenderer::initGLEW()) {
        glfwTerminate();
        return -1;
    }

    // Проверка системы
    GameRenderer::checkDoubleBufferSupport(window);
    GameRenderer::setupVSync(window, true);
    GameRenderer::printGraphicsInfo();

    // Проверка поддержки OpenGL
    if (!GLEW_VERSION_3_3 || !glGenVertexArrays) {
        glfwTerminate();
        return -1;
    }

    // Настройка колбэков через GameRenderer
    GameRenderer::setupCallbacks(window);

    // Настройка OpenGL
    GameRenderer::initOpenGLSettings();
    GameRenderer::checkGLError("OpenGL setup");

    // Инициализация сид рандома
    srand(static_cast<unsigned int>(time(0)));

    // Инициализация шейдеров
    if (!g_shaderManager.initialize()) {
        glfwTerminate();
        return -1;
    }
    g_game.initialize();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_game.render();
    glfwSwapBuffers(window);
    double lastUpdateTime = glfwGetTime();
    double lastFrameTime = glfwGetTime();
    int frameCount = 0;
    double fpsUpdateTime = lastFrameTime;
    double frameTimeAccumulator = 0.0;
    const double fixedDeltaTime = 1.0 / 60.0;

    while (!glfwWindowShouldClose(window) && !g_shouldExitGame) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        // Фиксированный таймстеп
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

        // Обновление FPS в заголовке
        frameCount++;
        if (currentTime - fpsUpdateTime >= 1.0) {
            double fps = frameCount / (currentTime - fpsUpdateTime);
            frameCount = 0;
            fpsUpdateTime = currentTime;

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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        g_game.render();
        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // Очистка ресурсов
    std::cout << "Cleaning up resources..." << std::endl;
    if (uiVAO) glDeleteVertexArrays(1, &uiVAO);
    if (uiVBO) glDeleteBuffers(1, &uiVBO);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Game terminated successfully." << std::endl;
    return 0;
}