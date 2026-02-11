#pragma once
#include "../Core/Types.h"
#include <vector>

// Структура для составного препятствия
struct Obstacle {
    std::vector<Point> blocks;
    Point center;

    Obstacle(const Point& center);
    bool contains(const Point& point) const;
};