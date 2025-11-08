#pragma once
#include "../Core/Types.h"
#include "../Core/Constants.h"
#include <vector>

class Obstacle {
private:
    Point center;
    std::vector<Point> blocks;
    ObstacleType type;

    void generateCross();
    void generateSquare();
    void generateLine();
    void generateRandom();

public:
    // Конструкторы
    Obstacle(const Point& center);
    Obstacle(const Point& center, ObstacleType type);

    // Основные методы
    bool contains(const Point& point) const;

    // Геттеры
    const Point& getCenter() const { return center; }
    const std::vector<Point>& getBlocks() const { return blocks; }
    int getBlockCount() const { return static_cast<int>(blocks.size()); }
    ObstacleType getType() const { return type; }

    // Отладочная информация
    void printDebugInfo() const;
};