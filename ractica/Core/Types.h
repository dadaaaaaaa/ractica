#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "../pch.h"
#include "Constants.h"

// ============================================================================
// ОСНОВНЫЕ ГЕОМЕТРИЧЕСКИЕ ТИПЫ
// ============================================================================

// Структура для представления точки/сегмента змейки
struct Point {
    int x, y, z;

    Point(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}

    // Оператор сравнения
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    // Оператор неравенства
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};

// Структура для вершины модели
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;

    Vertex() : position(0.0f), normal(0.0f), texCoords(0.0f) {}
    Vertex(glm::vec3 pos, glm::vec3 norm, glm::vec2 tex)
        : position(pos), normal(norm), texCoords(tex) {
    }
};

// Структура для трансформаций объекта
struct Transform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform() : position(0.0f), rotation(0.0f), scale(1.0f) {}
    Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : position(pos), rotation(rot), scale(scl) {
    }
};

// ============================================================================
// ИГРОВЫЕ СУЩНОСТИ
// ============================================================================

// Структура для рекордов (только данные)
struct HighScore {
    std::string playerName;
    int score;
    std::string date;
    float gameSpeed;
    int snakeLength;
    int gameDuration;

    HighScore()
        : playerName(""), score(0), date(""), gameSpeed(1.0f), snakeLength(0), gameDuration(0) {
    }

    HighScore(const std::string& name, int scr, const std::string& dt, float speed, int length, int duration)
        : playerName(name), score(scr), date(dt), gameSpeed(speed), snakeLength(length), gameDuration(duration) {
    }
};

// Структура для еды
struct Food {
    Point position;
    bool eaten;
    float spawnTime;

    Food() : position(0, 0, 0), eaten(false), spawnTime(0.0f) {}
    Food(Point pos) : position(pos), eaten(false), spawnTime(0.0f) {}
};

// Структура для препятствия


// ============================================================================
// ГРАФИЧЕСКИЕ ТИПЫ
// ============================================================================

// Структура для материала объекта
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    Material()
        : ambient(0.1f), diffuse(0.5f), specular(1.0f), shininess(32.0f) {
    }

    Material(glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, float shine)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shine) {
    }
};

// Структура для текстуры
struct Texture {
    GLuint id;
    std::string type;
    std::string path;

    // Конструктор по умолчанию
    Texture() : id(0), type(""), path("") {}

    // Конструктор с параметрами
    Texture(GLuint texId, const std::string& texType, const std::string& texPath)
        : id(texId), type(texType), path(texPath) {
    }
};

// ============================================================================
// ДЕКОРАТИВНЫЕ ОБЪЕКТЫ
// ============================================================================

// Структура для облака
struct Cloud {
    glm::vec3 position;
    glm::vec3 velocity;
    float scale;

    Cloud() : position(0.0f), velocity(0.0f), scale(1.0f) {}
    Cloud(glm::vec3 pos, glm::vec3 vel, float scl)
        : position(pos), velocity(vel), scale(scl) {
    }
};

// Структура для цветка
struct Flower {
    glm::vec3 position;
    glm::vec3 color;
    float scale;

    Flower() : position(0.0f), color(1.0f), scale(1.0f) {}
    Flower(glm::vec3 pos, glm::vec3 col, float scl)
        : position(pos), color(col), scale(scl) {
    }
};

// ============================================================================
// СИСТЕМНЫЕ ТИПЫ
// ============================================================================

// Структура для границ игрового поля
struct Bounds {
    int minX, maxX;
    int minY, maxY;
    int minZ, maxZ;

    Bounds()
        : minX(0), maxX(GRID_WIDTH), minY(0), maxY(GRID_HEIGHT), minZ(0), maxZ(GRID_DEPTH) {
    }

    Bounds(int minX, int maxX, int minY, int maxY, int minZ, int maxZ)
        : minX(minX), maxX(maxX), minY(minY), maxY(maxY), minZ(minZ), maxZ(maxZ) {
    }
};

// Структура для результатов проверки коллизий
struct CollisionResult {
    bool hasCollision;
    Point collisionPoint;
    std::string collisionType; // "wall", "obstacle", "self"

    CollisionResult() : hasCollision(false), collisionPoint(0, 0, 0), collisionType("") {}
    CollisionResult(bool collision, Point point, std::string type)
        : hasCollision(collision), collisionPoint(point), collisionType(type) {
    }
};

// ============================================================================
// СТРУКТУРЫ ДЛЯ СИСТЕМЫ СОХРАНЕНИЯ
// ============================================================================

#pragma pack(push, 1)
// Структура для сохранения облака
struct CloudSave {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;
};

// Структура для сохранения птицы
struct BirdSave {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;
    glm::vec3 direction;
};

// Структура для сохранения цветка
struct FlowerSave {
    glm::vec3 position;
    glm::vec3 color;
    float size;
};

// Основная структура сохранения игры
struct SaveData {
    int version = 1;
    int score;
    int gameDuration;
    float gameSpeed;
    int currentDirection;

    // Змейка
    int snakeLength;
    Point snake[1000];

    // Еда
    int foodCount;
    Point food[50];

    // Препятствия
    int obstaclesCount;
    Point obstacles[100];

    // Облака
    int cloudSpritesCount;
    CloudSave cloudSprites[100];

    // Птицы
    int birdsCount;
    BirdSave birds[50];

    // Цветы
    int flowerSpritesCount;
    FlowerSave flowerSprites[100];
};
#pragma pack(pop)

// Структура для сохранения настроек
struct SettingsData {
    int version = 1;
    float gameSpeed;
    int currentSpeedIndex;
    char playerName[32];
};

#endif // GAME_TYPES_H