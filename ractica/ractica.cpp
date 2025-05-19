#define _CRT_SECURE_NO_WARNINGS
#include <glew.h>
#include <glfw3.h>
#include <time.h>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cmath>

// Размеры игрового поля (логические)
const int N = 30;
const int M = 20;
const int scale = 25;
const int gameWidth = N * scale;
const int gameHeight = M * scale;

// Состояние игры
bool gameOver = false;
bool gameStarted = false;
bool showScoresWindow = false;
int dir = 0, newDir = 0;
int num = 4;
struct Segment { int x; int y; } s[100];

// Время и скорость
double lastTickTime = 0;
double tickInterval = 0.1;
double lastSpeedIncreaseTime = 0;
const double speedIncreaseInterval = 30.0;

// Система очков
int score = 0;
std::vector<int> highScores(10, 0);

// Шрифт
GLuint fontBase;

// Функция проверки: находится ли заданная точка на змейке
bool IsOnSnake(int x, int y) {
    for (int i = 0; i < num; i++)
        if (s[i].x == x && s[i].y == y)
            return true;
    return false;
}

// Класс "Фрукт"
class Fruct {
public:
    int x, y;

    void New() {
        do {
            x = rand() % N;
            y = rand() % M;
        } while (IsOnSnake(x, y));
    }

    void Draw() {
        glColor3f(0.0f, 1.0f, 1.0f);
        glRectf(x * scale, y * scale, (x + 1) * scale, (y + 1) * scale);
    }
} fruits[5];

// Работа с рейтингом
void LoadHighScores() {
    std::ifstream file("highscores.txt");
    if (file.is_open()) {
        for (int i = 0; i < 10 && file >> highScores[i]; i++);
        file.close();
        std::sort(highScores.rbegin(), highScores.rend());
    }
}

void SaveHighScores() {
    std::ofstream file("highscores.txt");
    if (file.is_open()) {
        for (int sc : highScores)
            file << sc << " ";
        file.close();
    }
}

void UpdateHighScores() {
    if (score > highScores.back()) {
        highScores.back() = score;
        std::sort(highScores.rbegin(), highScores.rend());
        SaveHighScores();
    }
}

// Функция для вычисления ширины текста (в пикселях) с использованием текущего контекста устройства (HDC)
float GetTextWidth(const char* text) {
    SIZE size;
    HDC hdc = wglGetCurrentDC();
    GetTextExtentPoint32A(hdc, text, (int)strlen(text), &size);
    return (float)size.cx;
}

// Отрисовка текста с указанной позицией и цветом, используя оконные координаты
void DrawText(float x, float y, const char* text, float r = 1.0f, float g = 1.0f, float b = 1.0f) {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int windowWidth = vp[2];
    int windowHeight = vp[3];

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(r, g, b);
    glRasterPos2f(x, y);

    glPushAttrib(GL_LIST_BIT);
    glListBase(fontBase);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Отрисовка текста по центру по горизонтали (окошные координаты)
void DrawCenteredText(float y, const char* text, float r = 1.0f, float g = 1.0f, float b = 1.0f) {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int windowWidth = vp[2];
    float textWidth = GetTextWidth(text);
    float x = (windowWidth - textWidth) / 2;
    DrawText(x, y, text, r, g, b);
}

// Отрисовка текста по центру для окна с рейтингом
void DrawCenteredTextWindow(int windowWidth, float y, const char* text, GLuint fontList, float r = 1.0f, float g = 1.0f, float b = 1.0f) {
    SIZE size;
    HDC hdc = wglGetCurrentDC();
    GetTextExtentPoint32A(hdc, text, (int)strlen(text), &size);
    float textWidth = (float)size.cx;
    float x = (windowWidth - textWidth) / 2;

    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(fontList);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

// Инициализация шрифта
void InitFont() {
    fontBase = glGenLists(256);
    HFONT font = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Arial");
    HDC hdc = wglGetCurrentDC();
    HFONT oldfont = (HFONT)SelectObject(hdc, font);
    wglUseFontBitmaps(hdc, 0, 256, fontBase);
    SelectObject(hdc, oldfont);
    DeleteObject(font);
}

// Игровая логика: сброс состояния игры
void ResetGame() {
    gameOver = false;
    gameStarted = false;
    showScoresWindow = false;
    num = 4;
    dir = newDir = 0;
    score = 0;
    tickInterval = 0.1;

    for (int i = 0; i < num; i++) {
        s[i].x = 10 - i;
        s[i].y = 10;
    }

    for (int i = 0; i < 5; i++)
        fruits[i].New();

    lastTickTime = glfwGetTime();
    lastSpeedIncreaseTime = lastTickTime;
}

// Главная функция обновления логики игры
void Tick() {
    if (gameOver || !gameStarted)
        return;

    double currentTime = glfwGetTime();
    if (currentTime - lastSpeedIncreaseTime >= speedIncreaseInterval) {
        tickInterval /= 1.2;
        lastSpeedIncreaseTime = currentTime;
    }

    // Избегаем разворота на 180°
    if ((newDir == 0 && dir != 3) || (newDir == 3 && dir != 0) ||
        (newDir == 1 && dir != 2) || (newDir == 2 && dir != 1)) {
        dir = newDir;
    }

    // Передвигаем сегменты змейки: каждый сегмент переходит в положение предыдущего пункта
    for (int i = num; i > 0; --i) {
        s[i].x = s[i - 1].x;
        s[i].y = s[i - 1].y;
    }

    if (dir == 0) s[0].y += 1;
    if (dir == 1) s[0].x -= 1;
    if (dir == 2) s[0].x += 1;
    if (dir == 3) s[0].y -= 1;

    // Проверка столкновения со стенами
    if (s[0].x < 0 || s[0].x >= N || s[0].y < 0 || s[0].y >= M) {
        gameOver = true;
        UpdateHighScores();
        showScoresWindow = true;
        return;
    }

    // Проверка столкновения с собственным телом
    for (int i = 4; i < num; i++) {
        if (s[0].x == s[i].x && s[0].y == s[i].y) {
            gameOver = true;
            UpdateHighScores();
            showScoresWindow = true;
            return;
        }
    }

    // Проверка столкновения с фруктами
    for (int i = 0; i < 5; i++) {
        if (s[0].x == fruits[i].x && s[0].y == fruits[i].y) {
            num++;
            score += 10;
            fruits[i].New();
        }
    }
}

// Отрисовка сетки игрового поля (игровые координаты)
void DrawGrid() {
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 0; i <= N; i++) {
        glVertex2f(i * scale, 0);
        glVertex2f(i * scale, M * scale);
    }
    for (int j = 0; j <= M; j++) {
        glVertex2f(0, j * scale);
        glVertex2f(N * scale, j * scale);
    }
    glEnd();
}

// Отрисовка змейки (игровые координаты)
void DrawSnake() {
    glColor3f(0.1f, 1.0f, 0.0f);
    for (int i = 0; i < num; i++) {
        glRectf(s[i].x * scale, s[i].y * scale, (s[i].x + 0.9f) * scale, (s[i].y + 0.9f) * scale);
    }
}

// Отрисовка текущего счета (выводится в оконных координатах)
void DrawCurrentScore() {
    char text[50];
    snprintf(text, sizeof(text), "Score: %d", score);
    // Выводим текст чуть ниже верхней границы окна
    DrawCenteredText(30, text);
}

// Отрисовка экрана Game Over (в оконных координатах)
void DrawGameOverScreen() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int windowWidth = vp[2], windowHeight = vp[3];

    // Затемняем экран
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glRectf(0, 0, windowWidth, windowHeight);

    // Заголовок "GAME OVER"
    DrawCenteredText(windowHeight - 50, "GAME OVER");

    // Вывод текущего счета
    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Your score: %d", score);
    DrawCenteredText(windowHeight - 100, scoreText);

    // Инструкции
    DrawCenteredText(50, "Press ENTER to restart");
    DrawCenteredText(20, "Press ESC to exit");
}

// Экран старта игры (выводится в оконных координатах)
// При старте игры на заднем плане отрисовывается игровая сетка с масштабированием, а поверх неё — инструкции.
void DrawStartScreen() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int windowWidth = vp[2], windowHeight = vp[3];

    // Затемняем экран поверх отрисованного игрового поля
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glRectf(0, 0, windowWidth, windowHeight);
    DrawCenteredText(windowHeight / 2, "Press ENTER to start");
    DrawCenteredText(windowHeight / 2 - 40, "Use arrow keys to control");
}

// Отрисовка окна с рейтингом (не менялась)
void ShowScoresWindow(GLFWwindow* mainWindow) {
    GLFWwindow* scoresWindow = glfwCreateWindow(400, 600, "Top 10 Scores", NULL, NULL);
    if (!scoresWindow) return;

    glfwMakeContextCurrent(scoresWindow);

    // Инициализация шрифта для окна с рейтингом
    GLuint scoresFontBase = glGenLists(256);
    HFONT font = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Arial");
    HDC hdc = wglGetCurrentDC();
    HFONT oldfont = (HFONT)SelectObject(hdc, font);
    wglUseFontBitmaps(hdc, 0, 256, scoresFontBase);
    SelectObject(hdc, oldfont);
    DeleteObject(font);

    bool windowShouldClose = false;

    while (!windowShouldClose && !glfwWindowShouldClose(mainWindow)) {
        glfwPollEvents();

        // Закрытие окна рейтингов по нажатию ENTER
        if (glfwGetKey(scoresWindow, GLFW_KEY_ENTER) == GLFW_PRESS) {
            windowShouldClose = true;
        }

        int width, height;
        glfwGetFramebufferSize(scoresWindow, &width, &height);
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, 0, height, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Фон окна рейтингов
        glColor3f(0.1f, 0.1f, 0.1f);
        glRectf(0, 0, width, height);

        // Заголовок "Top 10 Scores"
        DrawCenteredTextWindow(width, height - 50, "Top 10 Scores", scoresFontBase, 1.0f, 1.0f, 1.0f);

        // Текущий счет игрока
        char scoreText[50];
        snprintf(scoreText, sizeof(scoreText), "Your score: %d", score);
        DrawCenteredTextWindow(width, height - 100, scoreText, scoresFontBase, 1.0f, 1.0f, 1.0f);

        // Вывод списка рекордов
        for (int i = 0; i < 10; i++) {
            char buf[50];
            snprintf(buf, sizeof(buf), "%d. %d", i + 1, highScores[i]);
            DrawCenteredTextWindow(width, height - 150 - 30 * i, buf, scoresFontBase, 1.0f, 1.0f, 1.0f);
        }

        // Инструкция по закрытию окна рейтингов
        DrawCenteredTextWindow(width, 50, "Press ENTER to close", scoresFontBase, 1.0f, 1.0f, 1.0f);

        glfwSwapBuffers(scoresWindow);
    }

    glDeleteLists(scoresFontBase, 256);
    glfwDestroyWindow(scoresWindow);
    glfwMakeContextCurrent(mainWindow);
}

// Обработчик изменения размера окна – теперь только обновляет viewport
void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Обработчик нажатий клавиш
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (!gameStarted) {
        if (key == GLFW_KEY_ENTER) {
            gameStarted = true;
            // Начинаем движение вправо
            dir = newDir = 2;
        }
        return;
    }

    if (gameOver && key == GLFW_KEY_ENTER) {
        ResetGame();
        return;
    }

    if (!gameOver) {
        switch (key) {
        case GLFW_KEY_UP:    if (dir != 3) newDir = 0; break;
        case GLFW_KEY_LEFT:  if (dir != 2) newDir = 1; break;
        case GLFW_KEY_RIGHT: if (dir != 1) newDir = 2; break;
        case GLFW_KEY_DOWN:  if (dir != 0) newDir = 3; break;
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }

    // Начальный размер окна равен игровым размерам
    GLFWwindow* window = glfwCreateWindow(gameWidth, gameHeight, "Snake Game", NULL, NULL);
    if (!window) {
        std::cerr << "Window creation failed" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetKeyCallback(window, KeyCallback);

    LoadHighScores();
    InitFont();
    ResetGame();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        if (gameStarted && !gameOver && currentTime - lastTickTime >= tickInterval) {
            Tick();
            lastTickTime = currentTime;
        }

        // Если игра окончена и необходимо показать окно с рейтингом,
        // открываем отдельное окно для отображения рекордов.
        if (gameOver && showScoresWindow) {
            ShowScoresWindow(window);
            showScoresWindow = false;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glClear(GL_COLOR_BUFFER_BIT);

        // Устанавливаем проекцию в оконных координатах
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, 0, height, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // Рендер игрового поля с масштабированием:
        // Вычисляем коэффициент масштабирования для вписывания gameWidth x gameHeight в текущее окно
        
        float  factor = min(static_cast<float>(width) / static_cast<float>(gameWidth),
            static_cast<float>(height) / static_cast<float>(gameHeight));
        float offsetX = (width - gameWidth * factor) / 2.0f;
        float offsetY = (height - gameHeight * factor) / 2.0f;
        glPushMatrix();
//glTranslatef(offsetX, offsetY, 0);    
        float scaleX = static_cast<float>(width) / gameWidth;
        float scaleY = static_cast<float>(height) / gameHeight;
        glScalef(scaleX, scaleY, 1.0f);
        // Рендер элементов игрового поля
        DrawGrid();
        DrawSnake();
        for (int i = 0; i < 5; i++) {
            fruits[i].Draw();
        }
        glPopMatrix();

        // Отрисовка текущего счета (в оконных координатах)
        DrawCurrentScore();

        // Если игра ещё не начата, выводим стартовый экран (накладываясь поверх игрового поля)
        if (!gameStarted) {
            DrawStartScreen();
        }
        // Если игра окончена — показываем экран Game Over
        else if (gameOver) {
            DrawGameOverScreen();
        }

        glfwSwapBuffers(window);
    }

    // Освобождаем ресурсы: удаляем списки шрифтов и завершаем работу GLFW
    glDeleteLists(fontBase, 256);
    glfwTerminate();

    return 0;
}

