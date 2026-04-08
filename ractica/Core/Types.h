#pragma once
#include <glm/glm.hpp>
#include <string>

// Структура для представления точки/сегмента змейки
struct Point {
    int x, y, z;
    Point(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Структура для вершины модели
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};
enum class LightType {
    Directional,
    Points,
    Spot
};
// Структура для рекордов
struct HighScore {
    std::string playerName;
    int score;
    std::string date;
    float gameSpeed;
    int snakeLength;
    int gameDuration;

    HighScore() : playerName(""), score(0), date(""), gameSpeed(1.0f), snakeLength(0), gameDuration(0) {}

    HighScore(const std::string& name, int scr, const std::string& dt, float speed, int length, int duration)
        : playerName(name), score(scr), date(dt), gameSpeed(speed), snakeLength(length), gameDuration(duration) {
    }

    bool operator<(const HighScore& other) const {
        return score > other.score;
    }
};