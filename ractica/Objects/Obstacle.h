#pragma once
#include "../Core/Types.h"
#include <vector>

// Структура для составного препятствия
struct Obstacle {
    std::vector<Point> blocks;
    Point center;

    Obstacle(const Point& center) : center(center) {
        blocks.push_back(center);
        blocks.push_back(Point(center.x + 1, center.y, center.z));
        blocks.push_back(Point(center.x - 1, center.y, center.z));
        blocks.push_back(Point(center.x, center.y, center.z + 1));
        blocks.push_back(Point(center.x, center.y, center.z - 1));
    }

    bool contains(const Point& point) const {
        for (const auto& block : blocks) {
            if (block == point) return true;
        }
        return false;
    }
};