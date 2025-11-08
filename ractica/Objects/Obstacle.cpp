#include "../pch.h"
#include "Obstacle.h"
#include <iostream>
#include <random>

Obstacle::Obstacle(const Point& center) : center(center), type(ObstacleType::CROSS) {
    generateCross();
}

Obstacle::Obstacle(const Point& center, ObstacleType obstacleType)
    : center(center), type(obstacleType) {

    switch (type) {
    case ObstacleType::CROSS: generateCross(); break;
    case ObstacleType::SQUARE: generateSquare(); break;
    case ObstacleType::LINE: generateLine(); break;
    case ObstacleType::RANDOM: generateRandom(); break;
    }
}

void Obstacle::generateCross() {
    blocks.push_back(center);
    blocks.push_back(Point(center.x + 1, center.y, center.z));
    blocks.push_back(Point(center.x - 1, center.y, center.z));
    blocks.push_back(Point(center.x, center.y, center.z + 1));
    blocks.push_back(Point(center.x, center.y, center.z - 1));
}

void Obstacle::generateSquare() {
    // Создаем квадрат 2x2
    for (int x = -1; x <= 1; x++) {
        for (int z = -1; z <= 1; z++) {
            blocks.push_back(Point(center.x + x, center.y, center.z + z));
        }
    }
}

void Obstacle::generateLine() {
    // Создаем линию длиной 3 блока
    blocks.push_back(center);
    blocks.push_back(Point(center.x + 1, center.y, center.z));
    blocks.push_back(Point(center.x + 2, center.y, center.z));
}

void Obstacle::generateRandom() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(3, 6); // От 3 до 6 блоков

    int blockCount = dis(gen);
    blocks.push_back(center);

    for (int i = 1; i < blockCount; i++) {
        int dir = rand() % 4;
        Point lastBlock = blocks.back();

        switch (dir) {
        case 0: blocks.push_back(Point(lastBlock.x + 1, lastBlock.y, lastBlock.z)); break;
        case 1: blocks.push_back(Point(lastBlock.x - 1, lastBlock.y, lastBlock.z)); break;
        case 2: blocks.push_back(Point(lastBlock.x, lastBlock.y, lastBlock.z + 1)); break;
        case 3: blocks.push_back(Point(lastBlock.x, lastBlock.y, lastBlock.z - 1)); break;
        }
    }
}

bool Obstacle::contains(const Point& point) const {
    for (const auto& block : blocks) {
        if (block == point) {
            return true;
        }
    }
    return false;
}

void Obstacle::printDebugInfo() const {
#ifdef _DEBUG
    if (ENABLE_DEBUG_INFO) {
        std::string typeStr;
        switch (type) {
        case ObstacleType::CROSS: typeStr = "CROSS"; break;
        case ObstacleType::SQUARE: typeStr = "SQUARE"; break;
        case ObstacleType::LINE: typeStr = "LINE"; break;
        case ObstacleType::RANDOM: typeStr = "RANDOM"; break;
        }

        std::cout << "🚧 Obstacle " << typeStr << " at (" << center.x << ", " << center.y << ", " << center.z
            << ") with " << blocks.size() << " blocks" << std::endl;
    }
#endif
}