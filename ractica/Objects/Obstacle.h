#pragma once
#include "../Core/Types.h"
#include <vector>

// Структура для составного препятствия
struct Obstacle {
    std::vector<Point> blocks;
    Point center;

    Obstacle(const Point& center) : center(center) {
        blocks.push_back(center);
    }

    // В определении класса Obstacle (в заголовочном файле)
    bool contains(const Point& point) const {
        for (const auto& block : blocks) {
            if (block.x == point.x && block.z == point.z) {
                return true;
            }
        }
        return false;
    }
};