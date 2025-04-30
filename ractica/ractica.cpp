#define GLEW_STATIC
#include <glew.h>
#include <glfw3.h>
#include <time.h>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h> // Для работы с шрифтами Windows

// Размеры игрового поля в клетках
int N = 30, M = 20;
// Размер одной клетки в пикселях
int scale = 25;
// Размеры окна в пикселях
int w = scale * N;
int h = scale * M;

// Флаги состояния игры
bool gameOver = false;      // Игра закончена
bool gameStarted = false;   // Игра начата
// Направление движения змейки (0-вверх, 1-влево, 2-вправо, 3-вниз)
int dir = 0, newDir = 0;
// Длина змейки и массив сегментов
int num = 4;
struct { int x; int y; } s[100];

// Переменные для управления временем
double lastTickTime = 0; // Время последнего обновления
double tickInterval = 0.1; // Интервал обновления (в секундах)

// Глобальные переменные для работы с шрифтами
GLuint fontBase;

// Класс для фруктов
class Fruct
{
public:
    int x, y; // Позиция фрукта

    // Генерация нового фрукта
    void New()
    {
        x = rand() % N;
        y = rand() % M;
    }

    // Отрисовка фрукта
    void DrawFruct()
    {
        glColor3f(0.0, 1.0, 1.0); // Голубой цвет
        glRectf(x * scale, y * scale, (x + 1) * scale, (y + 1) * scale);
    }
} m[5]; // Массив из 5 фруктов

// Инициализация шрифтов
void InitFont()
{
    // Создаем 256 дисплейных списков
    fontBase = glGenLists(256);

    // Создаем шрифт
    HFONT font = CreateFont(
        24,                        // Высота шрифта
        0,                         // Ширина (0 - авто)
        0,                         // Угол наклона
        0,                         // Угол ориентации
        FW_NORMAL,                 // Толщина
        FALSE,                     // Курсив
        FALSE,                     // Подчеркивание
        FALSE,                     // Зачеркивание
        ANSI_CHARSET,              // Кодировка
        OUT_TT_PRECIS,             // Точность вывода
        CLIP_DEFAULT_PRECIS,       // Точность отсечения
        ANTIALIASED_QUALITY,       // Качество
        FF_DONTCARE | DEFAULT_PITCH, // Семейство
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
    glOrtho(0, w, 0, h, -1, 1);

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

// Отрисовка игровой сетки
void DrawGrid()
{
    glColor3f(1.0, 0.0, 0.0); // Красный цвет
    glBegin(GL_LINES);
    // Вертикальные линии
    for (int i = 0; i < w; i += scale)
    {
        glVertex2f(i, 0); glVertex2f(i, h);
    }
    // Горизонтальные линии
    for (int j = 0; j < h; j += scale)
    {
        glVertex2f(0, j); glVertex2f(w, j);
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

    // Установка начальной позиции змейки
    for (int i = 0; i < num; i++)
    {
        s[i].x = 10 - i; // Горизонтальная змейка
        s[i].y = 10;
    }

    // Генерация фруктов
    for (int i = 0; i < 5; i++)
        m[i].New();

    lastTickTime = glfwGetTime(); // Обнуляем время обновления
}

// Логика игры (движение, столкновения)
void tick()
{
    // Если игра не начата или закончена - ничего не делаем
    if (gameOver || !gameStarted) return;

    // Обновляем направление только если оно допустимо (не противоположно текущему)
    if ((newDir == 0 && dir != 3) || // Вверх, если не двигались вниз
        (newDir == 3 && dir != 0) || // Вниз, если не двигались вверх
        (newDir == 1 && dir != 2) || // Влево, если не двигались вправо
        (newDir == 2 && dir != 1))   // Вправо, если не двигались влево
    {
        dir = newDir;
    }

    // Движение змейки - каждый сегмент занимает позицию предыдущего
    for (int i = num; i > 0; --i)
    {
        s[i].x = s[i - 1].x;
        s[i].y = s[i - 1].y;
    }

    // Перемещение головы в зависимости от направления
    if (dir == 0) s[0].y += 1; // Вверх
    if (dir == 1) s[0].x -= 1; // Влево
    if (dir == 2) s[0].x += 1; // Вправо
    if (dir == 3) s[0].y -= 1; // Вниз

    // Проверка столкновения с границами
    if (s[0].x >= N || s[0].x < 0 || s[0].y >= M || s[0].y < 0)
    {
        gameOver = true; // Игра окончена
        return;
    }

    // Проверка столкновения с собой (начинаем с 4, чтобы не проверять первые 3 сегмента)
    for (int i = 4; i < num; i++)
    {
        if (s[0].x == s[i].x && s[0].y == s[i].y)
        {
            gameOver = true; // Игра окончена
            return;
        }
    }

    // Проверка съедания фруктов
    for (int i = 0; i < 5; i++)
    {
        if (s[0].x == m[i].x && s[0].y == m[i].y)
        {
            num++; // Увеличиваем длину змейки
            m[i].New(); // Создаем новый фрукт
        }
    }
}

// Отрисовка змейки
void DrawSnake()
{
    glColor3f(0.1, 1.0, 0.0); // Зеленый цвет
    for (int i = 0; i < num; i++)
    {
        // Рисуем каждый сегмент змейки
        glRectf(s[i].x * scale, s[i].y * scale,
            (s[i].x + 0.9) * scale, (s[i].y + 0.9) * scale);
    }
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) // Обрабатываем только нажатие (без REPEAT)
    {
        // ESC - выход в любом состоянии
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
            return;
        }

        // ENTER - старт/рестарт игры с движением вправо
        if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && !gameStarted)
        {
            gameStarted = true;
            dir = newDir = 2; // Движение вправо
            return;
        }

        // Если игра не начата - начинаем при нажатии стрелки
        if (!gameStarted)
        {
            switch (key)
            {
            case GLFW_KEY_UP:    gameStarted = true; dir = newDir = 0; break;
            case GLFW_KEY_LEFT:   gameStarted = true; dir = newDir = 1; break;
            case GLFW_KEY_RIGHT:  gameStarted = true; dir = newDir = 2; break;
            case GLFW_KEY_DOWN:   gameStarted = true; dir = newDir = 3; break;
            }
            return;
        }

        // Управление во время игры (с защитой от разворота на 180°)
        if (gameStarted && !gameOver)
        {
            switch (key)
            {
            case GLFW_KEY_UP:    if (dir != 3) newDir = 0; break;
            case GLFW_KEY_LEFT:  if (dir != 2) newDir = 1; break;
            case GLFW_KEY_RIGHT: if (dir != 1) newDir = 2; break;
            case GLFW_KEY_DOWN:  if (dir != 0) newDir = 3; break;
            }
        }

        // ENTER при Game Over - рестарт
        if (gameOver && (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER))
        {
            ResetGame();
            return;
        }
    }
}// Отрисовка экрана завершения игры
void DrawGameOver()
{
    // Черный прямоугольник для фона
    glColor3f(0.0, 0.0, 0.0);
    glRectf(w / 2 - 150, h / 2 - 50, w / 2 + 150, h / 2 + 50);
    // Отображение текста "Game Over"
    DrawText(w / 2 - 50, h / 2 + 10, "Game Over");
    DrawText(w / 2 - 130, h / 2 - 10, "Press ENTER to restart");
    DrawText(w / 2 - 100, h / 2 - 30, "Press ESC to exit");
}

// Отрисовка стартового экрана
void DrawStartScreen()
{
    // Черный прямоугольник для фона
    glColor3f(0.0, 0.0, 0.0);
    glRectf(w / 2 - 150, h / 2 - 50, w / 2 + 150, h / 2 + 50);

    // Отображение текста "Press ENTER to start"
    DrawText(w / 2 - 130, h / 2, "Press ENTER to start");
}

// Освобождение ресурсов шрифтов
void CleanUpFonts()
{
    glDeleteLists(fontBase, 256);
}

int main()
{
    // Инициализация GLFW и создание окна
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(w, h, "Snake Game", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Инициализация шрифтов
    InitFont();

    // Установка обработчика событий клавиатуры
    glfwSetKeyCallback(window, key_callback);

    // Сброс игры перед началом
    ResetGame();

    // Основной игровой цикл
    while (!glfwWindowShouldClose(window))
    {
        // Обработка ввода
        glfwPollEvents();

        // Управление скоростью змейки
        double currentTime = glfwGetTime();
        if (gameStarted && !gameOver && (currentTime - lastTickTime) >= tickInterval) {
            tick();
            lastTickTime = currentTime; // Обновление времени последнего тика
        }

        // Отрисовка
        glClear(GL_COLOR_BUFFER_BIT);
        DrawGrid(); // Отрисовка сетки
        if (gameOver)
        {
            DrawGameOver(); // Отрисовка экрана "Game Over"
        }
        else if (!gameStarted)
        {
            DrawStartScreen(); // Отрисовка стартового экрана
        }
        else
        {
            DrawSnake(); // Отрисовка змейки
            for (int i = 0; i < 5; i++)
            {
                m[i].DrawFruct(); // Отрисовка фруктов
            }
        }
        glfwSwapBuffers(window);
    }

    // Освобождение ресурсов
    CleanUpFonts();
    glfwTerminate();
    return 0;
}