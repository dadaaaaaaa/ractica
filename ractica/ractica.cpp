#define _CRT_SECURE_NO_WARNINGS
#include <glew.h>
#include <glfw3.h>
#include <time.h>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <fstream> // Для работы с файлами
#include <cstring>

// Размеры игрового поля в клетках
int N = 30, M = 20;
// Размер одной клетки в пикселях
int scale = 25;
// Фиксированная игровая зона (логические координаты)
int gameWidth = scale * N;
int gameHeight = scale * M;

// Флаги состояния игры
bool gameOver = false; // Игра закончена
bool gameStarted = false; // Игра начата
// Направление движения змейки (0 – вверх, 1 – влево, 2 – вправо, 3 – вниз)
int dir = 0, newDir = 0;
// Длина змейки и массив сегментов
int num = 4;
struct Segment { int x; int y; } s[100];

// Переменные для управления временем
double lastTickTime = 0; // Время последнего обновления
double tickInterval = 0.1; // Интервал обновления (в секундах)
double lastSpeedIncreaseTime = 0; // Время последнего увеличения скорости
double speedIncreaseInterval = 30.0; // Интервал увеличения скорости (30 секунд)

// Система очков
int score = 0;
int highScore = 0;

// Глобальная переменная для работы со шрифтами
GLuint fontBase;

// Функция для проверки, находится ли данная позиция на змейке
bool IsOnSnake(int x, int y)
{
    for (int i = 0; i < num; i++)
    {
        if (s[i].x == x && s[i].y == y)
            return true;
    }
    return false;
}

// Класс для фруктов
class Fruct
{
public:
    int x, y; // Позиция фрукта

    // Генерация нового фрукта с проверкой, чтобы он не появлялся на змейке
    void New()
    {
        do {
            x = rand() % N;
            y = rand() % M;
        } while (IsOnSnake(x, y));
    }

    // Отрисовка фрукта
    void DrawFruct()
    {
        glColor3f(0.0f, 1.0f, 1.0f); // Голубой цвет
        glRectf(x * scale, y * scale, (x + 1) * scale, (y + 1) * scale);
    }
} m[5]; // Массив из 5 фруктов

// Загрузка рекорда из файла
void LoadHighScore()
{
    std::ifstream file("highscore.txt");
    if (file.is_open())
    {
        file >> highScore;
        file.close();
    }
}

// Сохранение рекорда в файл
void SaveHighScore()
{
    std::ofstream file("highscore.txt");
    if (file.is_open())
    {
        file << highScore;
        file.close();
    }
}

// Инициализация шрифтов
void InitFont()
{
    // Создаем 256 дисплейных списков
    fontBase = glGenLists(256);

    // Создаем шрифт
    HFONT font = CreateFont(
        24,                         // Высота шрифта
        0,                          // Ширина (0 - авто)
        0,                          // Угол наклона
        0,                          // Угол ориентации
        FW_NORMAL,                  // Толщина
        FALSE,                      // Курсив
        FALSE,                      // Подчеркивание
        FALSE,                      // Зачеркивание
        ANSI_CHARSET,               // Кодировка
        OUT_TT_PRECIS,              // Точность вывода
        CLIP_DEFAULT_PRECIS,        // Точность отсечения
        ANTIALIASED_QUALITY,        // Качество
        FF_DONTCARE | DEFAULT_PITCH,// Семейство
        L"Arial"                    // Имя шрифта
    );

    // Получаем контекст устройства
    HDC hdc = wglGetCurrentDC();

    // Выбираем шрифт
    HFONT oldfont = (HFONT)SelectObject(hdc, font);

    // Создаем битовые карты символов
    wglUseFontBitmaps(hdc, 0, 256, fontBase);

    // Восстанавливаем старый шрифт
    SelectObject(hdc, oldfont);

    // Удаляем созданный шрифт
    DeleteObject(font);
    
}

// Отрисовка текста
void DrawText(float x, float y, const char* text, float r = 1.0f, float g = 1.0f, float b = 1.0f)
{
    // Сохраняем текущие матрицы
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gameWidth, 0, gameHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Устанавливаем цвет текста
    glColor3f(r, g, b);

    // Позиция текста
    glRasterPos2f(x, y);

    // Рисуем текст
    glPushAttrib(GL_LIST_BIT);
    glListBase(fontBase);
    glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();

    // Восстанавливаем матрицы
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
}

// Отрисовка игровой сетки внутри фиксированной логической зоны
void DrawGrid()
{
    glColor3f(1.0f, 0.0f, 0.0f); // Красный цвет
    glBegin(GL_LINES);
    // Вертикальные линии
    for (int i = 0; i <= gameWidth; i += scale)
    {
        glVertex2f(i, 0);
        glVertex2f(i, gameHeight);
    }
    // Горизонтальные линии
    for (int j = 0; j <= gameHeight; j += scale)
    {
        glVertex2f(0, j);
        glVertex2f(gameWidth, j);
    }
    glEnd();
}

// Сброс игры в начальное состояние
void ResetGame()
{
    gameOver = false;
    gameStarted = false;
    num = 4; // Начальная длина змейки
    dir = 0; // Начальное направление
    newDir = 0;
    score = 0; // Сброс очков
    tickInterval = 0.1; // Сброс скорости

    // Установка начальной позиции змейки (начинаем примерно с середины игрового поля)
    for (int i = 0; i < num; i++)
    {
        s[i].x = 10 - i;
        s[i].y = 10;
    }

    // Генерация фруктов, убеждаясь, что они не появляются на змейке
    for (int i = 0; i < 5; i++)
        m[i].New();

    lastTickTime = glfwGetTime();
    lastSpeedIncreaseTime = lastTickTime;
}

// Логика игры (движение, столкновения)
void tick()
{
    if (gameOver || !gameStarted) return;

    double currentTime = glfwGetTime();
    if ((currentTime - lastSpeedIncreaseTime) >= speedIncreaseInterval)
    {
        tickInterval /= 1.2;
        lastSpeedIncreaseTime = currentTime;
    }

    if ((newDir == 0 && dir != 3) ||
        (newDir == 3 && dir != 0) ||
        (newDir == 1 && dir != 2) ||
        (newDir == 2 && dir != 1))
    {
        dir = newDir;
    }

    // Сдвиг сегментов змейки
    for (int i = num; i > 0; --i)
    {
        s[i].x = s[i - 1].x;
        s[i].y = s[i - 1].y;
    }

    // Перемещение головы
    if (dir == 0) s[0].y += 1;
    if (dir == 1) s[0].x -= 1;
    if (dir == 2) s[0].x += 1;
    if (dir == 3) s[0].y -= 1;

    // Столкновение с границами игровой зоны
    if (s[0].x >= N || s[0].x < 0 || s[0].y >= M || s[0].y < 0)
    {
        gameOver = true;
        if (score > highScore)
        {
            highScore = score;
            SaveHighScore();
        }
        return;
    }

    // Проверка столкновения с телом змейки (начиная с 4-го сегмента)
    for (int i = 4; i < num; i++)
    {
        if (s[0].x == s[i].x && s[0].y == s[i].y)
        {
            gameOver = true;
            if (score > highScore)
            {
                highScore = score;
                SaveHighScore();
            }
            return;
        }
    }

    // Проверка столкновения с фруктами
    for (int i = 0; i < 5; i++)
    {
        if (s[0].x == m[i].x && s[0].y == m[i].y)
        {
            num++;
            score += 10;
            m[i].New();
        }
    }
    
}

// Отрисовка змейки
void DrawSnake()
{
    glColor3f(0.1f, 1.0f, 0.0f);
    for (int i = 0; i < num; i++)
    {
        glRectf(s[i].x * scale, s[i].y * scale,
            (s[i].x + 0.9f) * scale, (s[i].y + 0.9f) * scale);
    }
}

// Обработчик изменения размера окна
// Теперь весь viewport используется, и фиксированная логическая зона растягивается для заполнения окна
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Проекция остаётся по логическим координатам фиксированной зоны,
    // но растягивается на весь размер окна за счёт viewport
    glOrtho(0, gameWidth, 0, gameHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Обработчик клавиатуры
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
            return;
        }
        if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && !gameStarted)
        {
            gameStarted = true;
            dir = newDir = 2; // Стартуем вправо
            return;
        }
        if (!gameStarted)
        {
            switch (key)
            {
            case GLFW_KEY_UP: gameStarted = true; dir = newDir = 0; break;
            case GLFW_KEY_LEFT: gameStarted = true; dir = newDir = 1; break;
            case GLFW_KEY_RIGHT: gameStarted = true; dir = newDir = 2; break;
            case GLFW_KEY_DOWN: gameStarted = true; dir = newDir = 3; break;
            }
            return;
        }
        if (gameStarted && !gameOver)
        {
            switch (key)
            {
            case GLFW_KEY_UP: if (dir != 3) newDir = 0; break;
            case GLFW_KEY_LEFT: if (dir != 2) newDir = 1; break;
            case GLFW_KEY_RIGHT: if (dir != 1) newDir = 2; break;
            case GLFW_KEY_DOWN: if (dir != 0) newDir = 3; break;
            }
        }
        if (gameOver && (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER))
        {
            ResetGame();
            return;
        }
    }
}

// Отрисовка экрана завершения игры
void DrawGameOver()
{
    glColor3f(0.0f, 0.0f, 0.0f);
    glRectf(gameWidth / 2 - 150, gameHeight / 2 - 50, gameWidth / 2 + 150, gameHeight / 2 + 50);
    DrawText(gameWidth / 2 - 50, gameHeight / 2 + 10, "Game Over");
    DrawText(gameWidth / 2 - 130, gameHeight / 2 - 10, "Press ENTER to restart");
    DrawText(gameWidth / 2 - 100, gameHeight / 2 - 30, "Press ESC to exit");
}

// Отрисовка стартового экрана
void DrawStartScreen()
{
    glColor3f(0.0f, 0.0f, 0.0f);
    glRectf(gameWidth / 2 - 150, gameHeight / 2 - 50, gameWidth / 2 + 150, gameHeight / 2 + 50);
    DrawText(gameWidth / 2 - 130, gameHeight / 2, "Press ENTER to start");
}

// Отрисовка счета
void DrawScore()
{
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);
    DrawText(10, gameHeight - 30, scoreText);

    char highScoreText[50];
    sprintf(highScoreText, "High Score: %d", highScore);
    DrawText(10, gameHeight - 60, highScoreText);
}

// Освобождение ресурсов шрифтов
void CleanUpFonts()
{
    glDeleteLists(fontBase, 256);
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Создаём окно с начальными размерами, равными игровой зоне
    GLFWwindow* window = glfwCreateWindow(gameWidth, gameHeight, "Snake Game", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    // Устанавливаем обработчик изменения размера окна
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // Для первоначальной инициализации вызываем наш framebuffer_size_callback,
    // передавая текущие размеры окна
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    LoadHighScore();
    InitFont();
    ResetGame();

    // Основной игровой цикл
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        if (gameStarted && !gameOver && (currentTime - lastTickTime) >= tickInterval)
        {
            tick();
            lastTickTime = currentTime;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        DrawGrid();

        if (gameOver)
            DrawGameOver();
        else if (!gameStarted)
            DrawStartScreen();
        else
        {
            DrawScore();
            for (int i = 0; i < 5; i++)
                m[i].DrawFruct();
            DrawSnake();
        }

        glfwSwapBuffers(window);
    }

    CleanUpFonts();
    glfwTerminate();
    return 0;
    
}