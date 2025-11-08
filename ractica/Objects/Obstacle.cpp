#include "../pch.h"
#include "Obstacle.h"

Obstacle::Obstacle(const Point& center) : center(center) {
    blocks.push_back(center);
    blocks.push_back(Point(center.x + 1, center.y, center.z));
    blocks.push_back(Point(center.x - 1, center.y, center.z));
    blocks.push_back(Point(center.x, center.y, center.z + 1));
    blocks.push_back(Point(center.x, center.y, center.z - 1));
}

bool Obstacle::contains(const Point& point) const {
    for (const auto& block : blocks) {
        if (block == point) return true;
    }
    return false;
}