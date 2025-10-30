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

// Структура для рекордов
struct HighScore {
    std::string playerName;
    int score;
    std::string date;

    HighScore(const std::string& name = "", int sc = 0, const std::string& dt = "")
        : playerName(name), score(sc), date(dt) {
    }
};