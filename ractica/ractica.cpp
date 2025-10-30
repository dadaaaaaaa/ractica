#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

// Структура для представления точки/сегмента змейки
struct Point {
    int x, y, z;
    Point(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Структура для составного препятствия
struct Obstacle {
    std::vector<Point> blocks;
    Point center;

    Obstacle(const Point& center) : center(center) {
        // Создаем 5 блоков: центральный + 4 вокруг
        blocks.push_back(center); // Центральный блок
        blocks.push_back(Point(center.x + 1, center.y, center.z)); // Правый
        blocks.push_back(Point(center.x - 1, center.y, center.z)); // Левый
        blocks.push_back(Point(center.x, center.y, center.z + 1)); // Передний
        blocks.push_back(Point(center.x, center.y, center.z - 1)); // Задний
    }

    bool contains(const Point& point) const {
        for (const auto& block : blocks) {
            if (block == point) return true;
        }
        return false;
    }
};

// Структура для вершины модели
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

// Структура для 3D модели
struct Model {
    std::vector<Vertex> vertices;
    GLuint VAO, VBO;
    bool hasTexture;

    Model() : VAO(0), VBO(0), hasTexture(false) {}

    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
            vertices.data(), GL_STATIC_DRAW);

        // Позиция
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        // Нормаль
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        // Текстурные координаты
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, texCoords));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);
    }
};

// Структура для спрайта (облака, птицы и т.д.)
struct Sprite {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;

    Sprite(glm::vec3 pos, glm::vec3 col, float s, float sp)
        : position(pos), color(col), size(s), speed(sp) {
    }
};

// Константы игры
const int GRID_WIDTH = 120;
const int GRID_HEIGHT = 10;
const int GRID_DEPTH = 120;
const float CELL_SIZE = 0.1f;
const int OBSTACLE_COUNT = 10;
const int INITIAL_FOOD_COUNT = 10;

// Направления
enum Direction {
    FORWARD, BACKWARD, RIGHT, LEFT, UP, DOWN
};

// Глобальные переменные
std::vector<Point> snake;
std::vector<Point> food;
std::vector<Obstacle> obstacles;
std::vector<Point> fenceBlocks;
Direction currentDirection = FORWARD;
int verticalDirection = 0;
int score = 0;
bool gameOver = false;
bool paused = false;

// Модели
Model snakeModel;
Model foodModel;
Model obstacleModel;
Model floorModel;
Model fenceModel;
Model cloudModel;
Model birdModel;
Model flowerModel;
Model treeModel;
Model appleModel;

// Спрайты
std::vector<Sprite> cloudSprites;   // Облака
std::vector<Sprite> birdSprites;    // Птицы
std::vector<Sprite> flowerSprites;  // Цветы на земле

// Камера
glm::vec3 cameraPos;
glm::vec3 targetCameraPos;
glm::vec3 cameraFront;
glm::vec3 targetCameraFront;
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 5.0f;
float targetCameraDistance = 5.0f;
float cameraHeight = 3.0f;
float cameraSmoothness = 0.1f;
float minCameraDistance = 2.0f;
float maxCameraDistance = 8.0f;
float cameraZoomSpeed = 0.5f;

// Шейдерные переменные
GLuint shaderProgram;
GLuint modelLoc, viewLoc, projectionLoc, colorLoc, useTextureLoc;

// Шейдеры
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec3 Normal;
    out vec3 FragPos;
    out vec2 TexCoords;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoords = aTexCoords;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    
    uniform vec3 color;
    uniform bool useTexture;
    
    void main() {
        vec3 result;
        
        if (useTexture) {
            // Процедурная генерация текстур для разных объектов
            if (color.r > 0.8 && color.g < 0.2) {
                // Забор - деревянная текстура
                float woodPattern = sin(TexCoords.x * 30.0) * 0.3 + 0.7;
                float ringPattern = sin(TexCoords.y * 15.0) * 0.2 + 0.8;
                result = color * woodPattern * ringPattern;
            }
            else if (color.g > 0.8 && color.b < 0.3) {
                // Трава - зеленая текстура с узором
                float grassPattern = sin(TexCoords.x * 50.0) * sin(TexCoords.y * 50.0) * 0.4 + 0.6;
                result = color * grassPattern;
            }
            else if (color.r > 0.8 && color.g > 0.8) {
                // Яблоко - красная с пятнышками
                float spots = step(0.8, sin(TexCoords.x * 40.0) * sin(TexCoords.y * 40.0));
                result = color * (0.8 + spots * 0.2);
            }
            else if (color.r > 0.9 && color.g > 0.9 && color.b > 0.9) {
                // Облака - белые с мягкими краями
                float cloud = sin(TexCoords.x * 25.0) * sin(TexCoords.y * 25.0) * 0.5 + 0.5;
                result = color * cloud;
            }
            else if (color.b > 0.8) {
                // Птицы - синие с узором
                float birdPattern = sin(TexCoords.x * 60.0) * 0.4 + 0.6;
                result = color * birdPattern;
            }
            else if (color.r > 0.8 || color.g > 0.8 || color.b > 0.8) {
                // Цветы - яркие цвета
                float flowerPattern = sin(TexCoords.x * 35.0) * cos(TexCoords.y * 35.0) * 0.3 + 0.7;
                result = color * flowerPattern;
            }
            else {
                // Остальные объекты - простая текстура
                float pattern = sin(TexCoords.x * 20.0) * sin(TexCoords.y * 20.0) * 0.3 + 0.7;
                result = color * pattern;
            }
            
            // Добавляем освещение
            vec3 lightDir = vec3(0.0, -1.0, 0.0);
            float diff = max(dot(normalize(Normal), -lightDir), 0.3);
            result = result * (0.7 + 0.3 * diff);
        } else {
            // Старое освещение для объектов без текстуры
            vec3 lightDir = vec3(0.0, -1.0, 0.0);
            float diff = max(dot(normalize(Normal), -lightDir), 0.2);
            vec3 ambient = 0.6 * color;
            vec3 diffuse = diff * color;
            result = ambient + diffuse * 0.4;
        }
        
        FragColor = vec4(result, 1.0);
    }
)";

// Простая функция загрузки OBJ файла (базовая совместимость)
bool loadOBJFile(const std::string& path, Model& model) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "OBJ file not found: " << path << " - using procedural model" << std::endl;
        return false;
    }

    // Базовая загрузка только вершин (без нормалей и текстурных координат)
    std::vector<glm::vec3> positions;
    std::vector<Vertex> vertices;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (prefix == "f") {
            // Простой парсинг лиц - только индексы позиций
            std::string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            // Извлекаем только первый номер (позицию)
            int idx1 = std::stoi(v1.substr(0, v1.find('/')));
            int idx2 = std::stoi(v2.substr(0, v2.find('/')));
            int idx3 = std::stoi(v3.substr(0, v3.find('/')));

            // Создаем вершины
            Vertex vertex1, vertex2, vertex3;
            vertex1.position = positions[idx1 - 1];
            vertex2.position = positions[idx2 - 1];
            vertex3.position = positions[idx3 - 1];

            // Простые нормали и текстурные координаты
            vertex1.normal = vertex2.normal = vertex3.normal = glm::vec3(0, 1, 0);
            vertex1.texCoords = glm::vec2(0, 0);
            vertex2.texCoords = glm::vec2(1, 0);
            vertex3.texCoords = glm::vec2(0.5, 1);

            vertices.push_back(vertex1);
            vertices.push_back(vertex2);
            vertices.push_back(vertex3);
        }
    }

    if (vertices.empty()) {
        std::cout << "No vertices loaded from: " << path << std::endl;
        return false;
    }

    model.vertices = vertices;
    model.hasTexture = true;
    model.setupBuffers();

    std::cout << "Loaded OBJ: " << path << " (" << vertices.size() << " vertices)" << std::endl;
    return true;
}

// Вспомогательные функции для создания геометрии
void createCircle(std::vector<Vertex>& vertices, float cx, float cy, float radius, int segments, const glm::vec3& normal = glm::vec3(0.0f, 0.0f, 1.0f)) {
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        Vertex v1, v2, v3;
        v1.position = glm::vec3(cx, cy, 0.0f);
        v2.position = glm::vec3(cx + cos(angle1) * radius, cy + sin(angle1) * radius, 0.0f);
        v3.position = glm::vec3(cx + cos(angle2) * radius, cy + sin(angle2) * radius, 0.0f);

        v1.normal = v2.normal = v3.normal = normal;

        v1.texCoords = glm::vec2(0.5f, 0.5f);
        v2.texCoords = glm::vec2(0.5f + cos(angle1) * 0.5f, 0.5f + sin(angle1) * 0.5f);
        v3.texCoords = glm::vec2(0.5f + cos(angle2) * 0.5f, 0.5f + sin(angle2) * 0.5f);

        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
    }
}

void createCylinder(std::vector<Vertex>& vertices, float x, float y, float z, float radius, float height, int segments, const glm::vec3& color) {
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        // Боковая поверхность
        glm::vec3 p1(x + cos(angle1) * radius, y, z + sin(angle1) * radius);
        glm::vec3 p2(x + cos(angle2) * radius, y, z + sin(angle2) * radius);
        glm::vec3 p3(x + cos(angle1) * radius, y + height, z + sin(angle1) * radius);
        glm::vec3 p4(x + cos(angle2) * radius, y + height, z + sin(angle2) * radius);

        glm::vec3 normal1 = glm::normalize(glm::vec3(cos(angle1), 0.0f, sin(angle1)));
        glm::vec3 normal2 = glm::normalize(glm::vec3(cos(angle2), 0.0f, sin(angle2)));

        // Первый треугольник боковой поверхности
        vertices.push_back({ p1, normal1, glm::vec2(0.0f, 0.0f) });
        vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });

        // Второй треугольник боковой поверхности
        vertices.push_back({ p2, normal2, glm::vec2(1.0f, 0.0f) });
        vertices.push_back({ p3, normal1, glm::vec2(0.0f, 1.0f) });
        vertices.push_back({ p4, normal2, glm::vec2(1.0f, 1.0f) });
    }
}

void createSpherePart(std::vector<Vertex>& vertices, float cx, float cy, float cz, float radius, int segments, int rings, const glm::vec3& color) {
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                cx + radius * sin(theta1) * cos(phi1),
                cy + radius * cos(theta1),
                cz + radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                cx + radius * sin(theta1) * cos(phi2),
                cy + radius * cos(theta1),
                cz + radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                cx + radius * sin(theta2) * cos(phi2),
                cy + radius * cos(theta2),
                cz + radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                cx + radius * sin(theta2) * cos(phi1),
                cy + radius * cos(theta2),
                cz + radius * sin(theta2) * sin(phi1)
            );

            glm::vec3 normal1 = glm::normalize(v1 - glm::vec3(cx, cy, cz));
            glm::vec3 normal2 = glm::normalize(v2 - glm::vec3(cx, cy, cz));
            glm::vec3 normal3 = glm::normalize(v3 - glm::vec3(cx, cy, cz));
            glm::vec3 normal4 = glm::normalize(v4 - glm::vec3(cx, cy, cz));

            glm::vec2 t1 = glm::vec2((float)j / segments, (float)i / rings);
            glm::vec2 t2 = glm::vec2((float)(j + 1) / segments, (float)i / rings);
            glm::vec2 t3 = glm::vec2((float)(j + 1) / segments, (float)(i + 1) / rings);
            glm::vec2 t4 = glm::vec2((float)j / segments, (float)(i + 1) / rings);

            // Первый треугольник
            vertices.push_back({ v1, normal1, t1 });
            vertices.push_back({ v2, normal2, t2 });
            vertices.push_back({ v3, normal3, t3 });

            // Второй треугольник
            vertices.push_back({ v3, normal3, t3 });
            vertices.push_back({ v4, normal4, t4 });
            vertices.push_back({ v1, normal1, t1 });
        }
    }
}

// Создание моделей для различных объектов
void createCloudPart(std::vector<Vertex>& vertices, float x, float y, float z, float radius) {
    createSpherePart(vertices, x, y, z, radius, 8, 4, glm::vec3(1.0f));
}

void createCloudModel(Model& model) {
    // Создаем облако из нескольких сфероподобных частей
    createCloudPart(model.vertices, 0.0f, 0.0f, 0.0f, 0.4f);
    createCloudPart(model.vertices, 0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, -0.3f, 0.1f, 0.0f, 0.3f);
    createCloudPart(model.vertices, 0.0f, 0.3f, 0.0f, 0.25f);
    createCloudPart(model.vertices, 0.2f, -0.1f, 0.0f, 0.25f);
    createCloudPart(model.vertices, -0.2f, -0.1f, 0.0f, 0.25f);

    model.hasTexture = true;
    model.setupBuffers();
}

void createBirdModel(Model& model) {
    // Тело птицы (основной треугольник)
    Vertex body[] = {
        // Тело (треугольник)
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{0.5f, 0.15f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.3f}},
        {{0.5f, -0.15f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.7f}},

        // Хвост
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
        {{-0.2f, 0.2f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.2f, -0.2f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

        // Крыло 1
        {{0.2f, 0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.3f, 0.3f}},
        {{0.4f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.7f, 0.0f}},
        {{0.2f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.3f, 0.0f}},

        // Крыло 2
        {{0.2f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.3f, 0.7f}},
        {{0.4f, -0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.7f, 1.0f}},
        {{0.2f, -0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.3f, 1.0f}}
    };

    for (const auto& v : body) {
        model.vertices.push_back(v);
    }
    model.hasTexture = true;
    model.setupBuffers();
}

void createFlowerModel(Model& model) {
    // Центр цветка (желтый)
    createCircle(model.vertices, 0.0f, 0.0f, 0.1f, 8);

    // Лепестки (6 лепестков по кругу)
    for (int i = 0; i < 6; i++) {
        float angle = i * 3.14159f / 3.0f;
        float x = cos(angle) * 0.25f;
        float y = sin(angle) * 0.25f;
        createCircle(model.vertices, x, y, 0.08f, 6);
    }

    // Стебель
    Vertex stem[] = {
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : stem) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void createTreeModel(Model& model) {
    // Ствол
    createCylinder(model.vertices, 0.0f, 0.0f, 0.0f, 0.08f, 0.6f, 8, glm::vec3(0.4f, 0.2f, 0.1f));

    // Крона (сфера)
    createSpherePart(model.vertices, 0.0f, 0.8f, 0.0f, 0.3f, 12, 8, glm::vec3(0.1f, 0.4f, 0.1f));

    model.hasTexture = true;
    model.setupBuffers();
}

void createDetailedAppleModel(Model& model) {
    // Основное тело яблока
    createSpherePart(model.vertices, 0.0f, 0.0f, 0.0f, 0.5f, 16, 12, glm::vec3(1.0f, 0.0f, 0.0f));

    // Углубление сверху
    createSpherePart(model.vertices, 0.0f, 0.3f, 0.0f, 0.1f, 8, 4, glm::vec3(0.3f, 0.2f, 0.1f));

    // Черенок
    Vertex stem[] = {
        {{-0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.02f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.02f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    for (const auto& v : stem) {
        model.vertices.push_back(v);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

// Существующие функции создания базовых моделей
void createTexturedCubeModel(Model& model) {
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (int i = 0; i < 288; i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex.normal = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        vertex.texCoords = glm::vec2(vertices[i + 6], vertices[i + 7]);
        model.vertices.push_back(vertex);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void createTexturedSphereModel(Model& model) {
    const int segments = 16;
    const int rings = 16;
    const float radius = 0.5f;

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            float theta1 = (float)i / rings * 3.14159f;
            float theta2 = (float)(i + 1) / rings * 3.14159f;
            float phi1 = (float)j / segments * 2.0f * 3.14159f;
            float phi2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            glm::vec3 v1 = glm::vec3(
                radius * sin(theta1) * cos(phi1),
                radius * cos(theta1),
                radius * sin(theta1) * sin(phi1)
            );
            glm::vec3 v2 = glm::vec3(
                radius * sin(theta1) * cos(phi2),
                radius * cos(theta1),
                radius * sin(theta1) * sin(phi2)
            );
            glm::vec3 v3 = glm::vec3(
                radius * sin(theta2) * cos(phi2),
                radius * cos(theta2),
                radius * sin(theta2) * sin(phi2)
            );
            glm::vec3 v4 = glm::vec3(
                radius * sin(theta2) * cos(phi1),
                radius * cos(theta2),
                radius * sin(theta2) * sin(phi1)
            );

            glm::vec2 t1 = glm::vec2((float)j / segments, (float)i / rings);
            glm::vec2 t2 = glm::vec2((float)(j + 1) / segments, (float)i / rings);
            glm::vec2 t3 = glm::vec2((float)(j + 1) / segments, (float)(i + 1) / rings);
            glm::vec2 t4 = glm::vec2((float)j / segments, (float)(i + 1) / rings);

            // Первый треугольник
            model.vertices.push_back({ v1, glm::normalize(v1), t1 });
            model.vertices.push_back({ v2, glm::normalize(v2), t2 });
            model.vertices.push_back({ v3, glm::normalize(v3), t3 });

            // Второй треугольник
            model.vertices.push_back({ v3, glm::normalize(v3), t3 });
            model.vertices.push_back({ v4, glm::normalize(v4), t4 });
            model.vertices.push_back({ v1, glm::normalize(v1), t1 });
        }
    }

    model.hasTexture = true;
    model.setupBuffers();
}

void createTexturedFloorModel(Model& model) {
    float floorSize = 40.0f;

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(-floorSize, -0.1f, -floorSize);
    v2.position = glm::vec3(-floorSize, -0.1f, floorSize);
    v3.position = glm::vec3(floorSize, -0.1f, -floorSize);
    v4.position = glm::vec3(floorSize, -0.1f, floorSize);

    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    v1.normal = v2.normal = v3.normal = v4.normal = normal;

    // Текстурные координаты для большого повторения
    v1.texCoords = glm::vec2(0.0f, 0.0f);
    v2.texCoords = glm::vec2(0.0f, 20.0f);
    v3.texCoords = glm::vec2(20.0f, 0.0f);
    v4.texCoords = glm::vec2(20.0f, 20.0f);

    model.vertices.push_back(v1);
    model.vertices.push_back(v3);
    model.vertices.push_back(v4);

    model.vertices.push_back(v4);
    model.vertices.push_back(v2);
    model.vertices.push_back(v1);

    model.hasTexture = true;
    model.setupBuffers();
}

void createFenceModel(Model& model) {
    float vertices[] = {
        // positions          // normals           // texture coords
        // Боковые стороны (высокие)
        -0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 0.0f,
         0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 3.0f,
         0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  3.0f, 3.0f,
        -0.3f,  1.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 3.0f,
        -0.3f, -0.5f, -0.1f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 0.0f,
         0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 3.0f,
         0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   3.0f, 3.0f,
        -0.3f,  1.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 3.0f,
        -0.3f, -0.5f,  0.1f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.3f,  1.5f,  0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.3f,  1.5f, -0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.3f, -0.5f, -0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.3f, -0.5f, -0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.3f, -0.5f,  0.1f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.3f,  1.5f,  0.1f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.3f,  1.5f,  0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.3f,  1.5f, -0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.3f, -0.5f, -0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.3f, -0.5f, -0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.3f, -0.5f,  0.1f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.3f,  1.5f,  0.1f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         // Верхняя часть забора
         -0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.3f, 1.5f,  0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.3f, 1.5f, -0.1f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (int i = 0; i < 240; i += 8) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex.normal = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        vertex.texCoords = glm::vec2(vertices[i + 6], vertices[i + 7]);
        model.vertices.push_back(vertex);
    }

    model.hasTexture = true;
    model.setupBuffers();
}

// Генерация забора по границам игрового поля
void generateFence() {
    fenceBlocks.clear();

    // Верхняя граница (z = 0)
    for (int x = 0; x < GRID_WIDTH; x += 2) {
        fenceBlocks.push_back(Point(x, 0, 0));
    }

    // Нижняя граница (z = GRID_DEPTH - 1)
    for (int x = 0; x < GRID_WIDTH; x += 2) {
        fenceBlocks.push_back(Point(x, 0, GRID_DEPTH - 1));
    }

    // Левая граница (x = 0)
    for (int z = 2; z < GRID_DEPTH - 2; z += 2) {
        fenceBlocks.push_back(Point(0, 0, z));
    }

    // Правая граница (x = GRID_WIDTH - 1)
    for (int z = 2; z < GRID_DEPTH - 2; z += 2) {
        fenceBlocks.push_back(Point(GRID_WIDTH - 1, 0, z));
    }

    std::cout << "Generated " << fenceBlocks.size() << " fence blocks" << std::endl;
}

// Генерация спрайтов неба (облака, птицы)
void generateSkySprites() {
    cloudSprites.clear();
    birdSprites.clear();

    // Генерация облаков
    for (int i = 0; i < 20; i++) {
        float x = (rand() % 400 - 200) * 0.1f;
        float y = 8.0f + (rand() % 60) * 0.1f;
        float z = (rand() % 400 - 200) * 0.1f;
        float size = 5.8f + (rand() % 15) * 0.1f;
        float speed = 0.05f + (rand() % 10) * 0.02f;

        // Разные типы облаков
        int cloudType = rand() % 3;
        glm::vec3 color;
        switch (cloudType) {
        case 0: color = glm::vec3(0.95f, 0.95f, 0.95f); break; // Белые
        case 1: color = glm::vec3(0.85f, 0.85f, 0.85f); break; // Серые
        case 2: color = glm::vec3(0.90f, 0.90f, 0.92f); break; // Голубоватые
        }

        cloudSprites.push_back(Sprite(glm::vec3(x, y, z), color, size, speed));
    }

    // Генерация птиц
    for (int i = 0; i < 12; i++) {
        float x = (rand() % 300 - 150) * 0.1f;
        float y = 4.0f + (rand() % 30) * 0.1f;
        float z = (rand() % 300 - 150) * 0.1f;
        float size = 0.15f + (rand() % 8) * 0.03f;
        float speed = 0.2f + (rand() % 15) * 0.03f;

        int birdType = rand() % 4;
        glm::vec3 color;
        switch (birdType) {
        case 0: color = glm::vec3(0.1f, 0.1f, 0.3f); break;  // Темно-синие
        case 1: color = glm::vec3(0.3f, 0.2f, 0.1f); break;  // Коричневые
        case 2: color = glm::vec3(0.8f, 0.8f, 0.9f); break;  // Светлые
        case 3: color = glm::vec3(0.2f, 0.2f, 0.2f); break;  // Серые
        }

        birdSprites.push_back(Sprite(glm::vec3(x, y, z), color, size, speed));
    }

    std::cout << "Generated " << cloudSprites.size() << " clouds and "
        << birdSprites.size() << " birds" << std::endl;
}

// Генерация спрайтов на земле (цветы)
void generateGroundSprites() {
    flowerSprites.clear();

    // Генерация цветов
    for (int i = 0; i < 25; i++) {
        float x = (rand() % 200 - 100) * 0.1f;
        float y = 0.01f;
        float z = (rand() % 200 - 100) * 0.1f;
        float size = 0.1f + (rand() % 5) * 0.02f;

        // Яркие цвета для цветов
        int colorType = rand() % 4;
        glm::vec3 color;
        switch (colorType) {
        case 0: color = glm::vec3(1.0f, 0.2f, 0.2f); break; // Красный
        case 1: color = glm::vec3(0.2f, 0.2f, 1.0f); break; // Синий
        case 2: color = glm::vec3(1.0f, 0.8f, 0.2f); break; // Желтый
        case 3: color = glm::vec3(0.8f, 0.2f, 0.8f); break; // Фиолетовый
        }

        flowerSprites.push_back(Sprite(glm::vec3(x, y, z), color, size, 0.0f));
    }

    std::cout << "Generated " << flowerSprites.size() << " ground sprites" << std::endl;
}

// Обновление спрайтов неба
void updateSkySprites() {
    // Двигаем облака
    for (auto& cloud : cloudSprites) {
        cloud.position.x += cloud.speed * 0.01f;
        // Если облако улетело далеко, возвращаем его
        if (cloud.position.x > 12.0f) {
            cloud.position.x = -12.0f;
            cloud.position.z = (rand() % 200 - 100) * 0.1f;
        }
    }

    // Двигаем птиц (быстрее и с более сложной траекторией)
    for (auto& bird : birdSprites) {
        bird.position.x += bird.speed * 0.02f;
        bird.position.y += sin(glfwGetTime() * 2.0f + bird.position.x) * 0.01f;

        if (bird.position.x > 15.0f) {
            bird.position.x = -15.0f;
            bird.position.y = 5.0f + (rand() % 20) * 0.1f;
            bird.position.z = (rand() % 200 - 100) * 0.1f;
        }
    }
}

// Компиляция шейдера
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }
    return shader;
}

// Создание шейдерной программы
void createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    modelLoc = glGetUniformLocation(shaderProgram, "model");
    viewLoc = glGetUniformLocation(shaderProgram, "view");
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    colorLoc = glGetUniformLocation(shaderProgram, "color");
    useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
}

// Плавная интерполяция
glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + t * (b - a);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Обновление позиции камеры
void updateCameraTarget() {
    if (snake.empty()) return;

    const Point& head = snake[0];
    glm::vec3 headPos = glm::vec3(
        (head.x - GRID_WIDTH / 2.0f) * CELL_SIZE,
        head.y * CELL_SIZE,
        (head.z - GRID_DEPTH / 2.0f) * CELL_SIZE
    );

    // Обновляем целевую позицию камеры с учетом текущей дистанции
    targetCameraPos = headPos + glm::vec3(0.0f, cameraHeight, -targetCameraDistance);
    targetCameraFront = glm::normalize(headPos - targetCameraPos);
}

// Плавное обновление камеры
void updateCameraSmoothly() {
    cameraPos = lerp(cameraPos, targetCameraPos, cameraSmoothness);
    cameraFront = lerp(cameraFront, targetCameraFront, cameraSmoothness);
    cameraDistance = lerp(cameraDistance, targetCameraDistance, cameraSmoothness);
}

// Обработка прокрутки колесика мыши
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Изменяем целевую дистанцию камеры
    targetCameraDistance -= yoffset * cameraZoomSpeed;

    // Ограничиваем дистанцию
    if (targetCameraDistance < minCameraDistance) {
        targetCameraDistance = minCameraDistance;
    }
    if (targetCameraDistance > maxCameraDistance) {
        targetCameraDistance = maxCameraDistance;
    }

    // Обновляем позицию камеры
    updateCameraTarget();

    std::cout << "Camera distance: " << targetCameraDistance << std::endl;
}

// Генерация препятствий
void generateObstacles() {
    obstacles.clear();
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        Point center;
        bool validPosition = false;
        int attempts = 0;

        do {
            center.x = 10 + rand() % (GRID_WIDTH - 20);
            center.y = 0;
            center.z = 10 + rand() % (GRID_DEPTH - 20);

            // Проверяем, что все блоки препятствия валидны
            validPosition = true;
            Obstacle tempObstacle(center);

            for (const auto& block : tempObstacle.blocks) {
                // Проверяем границы
                if (block.x < 0 || block.x >= GRID_WIDTH || block.z < 0 || block.z >= GRID_DEPTH) {
                    validPosition = false;
                    break;
                }

                // Проверяем столкновение со змейкой
                for (const auto& segment : snake) {
                    if (segment == block) {
                        validPosition = false;
                        break;
                    }
                }

                // Проверяем столкновение с едой
                for (const auto& apple : food) {
                    if (apple == block) {
                        validPosition = false;
                        break;
                    }
                }

                // Проверяем столкновение с забором
                for (const auto& fenceBlock : fenceBlocks) {
                    if (fenceBlock == block) {
                        validPosition = false;
                        break;
                    }
                }

                // Проверяем столкновение с другими препятствиями
                for (const auto& existingObstacle : obstacles) {
                    if (existingObstacle.contains(block)) {
                        validPosition = false;
                        break;
                    }
                }

                if (!validPosition) break;
            }

            attempts++;
            if (attempts > 100) {
                std::cout << "Warning: Could not find valid obstacle position after 100 attempts" << std::endl;
                break;
            }

        } while (!validPosition);

        if (validPosition) {
            obstacles.push_back(Obstacle(center));
        }
    }
    std::cout << "Generated " << obstacles.size() << " obstacles" << std::endl;
}

// Генерация одного яблока
void generateSingleFood() {
    Point newFood;
    bool validPosition = false;
    int attempts = 0;

    do {
        newFood.x = 5 + rand() % (GRID_WIDTH - 10);
        newFood.y = 0;
        newFood.z = 5 + rand() % (GRID_DEPTH - 10);

        validPosition = true;

        // Проверяем столкновение со змейкой
        for (const auto& segment : snake) {
            if (segment == newFood) {
                validPosition = false;
                break;
            }
        }

        // Проверяем столкновение с существующей едой
        for (const auto& apple : food) {
            if (apple == newFood) {
                validPosition = false;
                break;
            }
        }

        // Проверяем столкновение с препятствиями
        for (const auto& obstacle : obstacles) {
            if (obstacle.contains(newFood)) {
                validPosition = false;
                break;
            }
        }

        // Проверяем столкновение с забором
        for (const auto& fenceBlock : fenceBlocks) {
            if (fenceBlock == newFood) {
                validPosition = false;
                break;
            }
        }

        attempts++;
        if (attempts > 50) {
            std::cout << "Warning: Could not find valid food position after 50 attempts" << std::endl;
            break;
        }

    } while (!validPosition);

    if (validPosition) {
        food.push_back(newFood);
    }
}

// Генерация начальных яблок
void generateInitialFood() {
    food.clear();
    for (int i = 0; i < INITIAL_FOOD_COUNT; i++) {
        generateSingleFood();
    }
}

// Инициализация игры
void initGame() {
    snake.clear();
    snake.push_back(Point(GRID_WIDTH / 2, 0, GRID_DEPTH / 2));
    snake.push_back(Point(GRID_WIDTH / 2 - 1, 0, GRID_DEPTH / 2));
    snake.push_back(Point(GRID_WIDTH / 2 - 2, 0, GRID_DEPTH / 2));

    generateFence();
    generateInitialFood();
    generateObstacles();
    generateSkySprites();
    generateGroundSprites();

    currentDirection = FORWARD;
    verticalDirection = 0;
    score = 0;
    gameOver = false;

    // Сбрасываем дистанцию камеры
    targetCameraDistance = 5.0f;
    cameraDistance = 5.0f;

    updateCameraTarget();
    cameraPos = targetCameraPos;
    cameraFront = targetCameraFront;
}

// Отладочный вывод
void debugSnakeInfo() {
    if (snake.empty()) return;
    std::cout << "Snake head: (" << snake[0].x << ", " << snake[0].y << ", " << snake[0].z << ")" << std::endl;
}

// Обновление игры
void updateGame() {
    if (gameOver || paused) return;

    Point newHead = snake[0];

    switch (currentDirection) {
    case FORWARD: newHead.z++; break;
    case BACKWARD: newHead.z--; break;
    case RIGHT: newHead.x--; break;
    case LEFT: newHead.x++; break;
    }

    newHead.y = 0;

    debugSnakeInfo();

    // Проверка столкновений с границами
    if (newHead.x < 0 || newHead.x >= GRID_WIDTH ||
        newHead.z < 0 || newHead.z >= GRID_DEPTH) {
        gameOver = true;
        std::cout << "Game Over: Wall collision!" << std::endl;
        return;
    }

    // Проверка столкновений с собой
    for (const auto& segment : snake) {
        if (segment == newHead) {
            gameOver = true;
            std::cout << "Game Over: Self collision!" << std::endl;
            return;
        }
    }

    // Проверка столкновений с препятствиями
    for (const auto& obstacle : obstacles) {
        if (obstacle.contains(newHead)) {
            gameOver = true;
            std::cout << "Game Over: Obstacle collision at (" << newHead.x << ", " << newHead.z << ")!" << std::endl;
            return;
        }
    }

    snake.insert(snake.begin(), newHead);

    // Проверка съедания яблок
    auto foodIt = std::find(food.begin(), food.end(), newHead);
    if (foodIt != food.end()) {
        score++;
        food.erase(foodIt);
        generateSingleFood();
        std::cout << "Food eaten! Score: " << score << std::endl;
    }
    else {
        snake.pop_back();
    }

    updateCameraTarget();
}

// Отрисовка модели в позиции
void drawModel(const Model& model, float x, float y, float z, float scale, const glm::vec3& color) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniform3fv(colorLoc, 1, &color[0]);
    glUniform1i(useTextureLoc, model.hasTexture);

    model.draw();
}

// Отрисовка пола
void drawFloor() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));
    model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(glm::vec3(0.3f, 0.6f, 0.2f)));
    glUniform1i(useTextureLoc, floorModel.hasTexture);

    floorModel.draw();
}

// Отрисовка змейки
void drawSnake() {
    for (size_t i = 0; i < snake.size(); i++) {
        const Point& segment = snake[i];
        float x = (segment.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = segment.y * CELL_SIZE + 0.05f;
        float z = (segment.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        glm::vec3 color = (i == 0) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.7f, 0.0f);
        drawModel(snakeModel, x, y, z, CELL_SIZE * 0.5f, color);
    }
}

// Отрисовка яблок
void drawFood() {
    for (const auto& apple : food) {
        float x = (apple.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = apple.y * CELL_SIZE + 0.05f;
        float z = (apple.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        drawModel(appleModel, x, y, z, CELL_SIZE * 1.2f, glm::vec3(1.0f, 0.8f, 0.2f));
    }
}

// Отрисовка препятствий как деревьев
void drawObstaclesAsTrees() {
    for (const auto& obstacle : obstacles) {
        for (const auto& block : obstacle.blocks) {
            float x = (block.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
            float y = block.y * CELL_SIZE;
            float z = (block.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

            drawModel(treeModel, x, y, z, CELL_SIZE * 3.0f, glm::vec3(0.1f, 0.4f, 0.1f));
        }
    }
}

// Отрисовка забора
void drawFence() {
    for (const auto& fenceBlock : fenceBlocks) {
        float x = (fenceBlock.x - GRID_WIDTH / 2.0f) * CELL_SIZE;
        float y = fenceBlock.y * CELL_SIZE;
        float z = (fenceBlock.z - GRID_DEPTH / 2.0f) * CELL_SIZE;

        // Коричневый цвет для деревянного забора
        glm::vec3 fenceColor(0.55f, 0.27f, 0.07f);

        // Высокие столбы забора
        drawModel(fenceModel, x, y, z, CELL_SIZE * 1.2f, fenceColor);
    }
}

// Отрисовка спрайтов неба
void drawSkySprites() {
    // Отрисовка облаков
    for (const auto& cloud : cloudSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cloud.position);
        model = glm::scale(model, glm::vec3(cloud.size));
        // Поворачиваем облака к камере (billboarding)
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, &cloud.color[0]);
        glUniform1i(useTextureLoc, cloudModel.hasTexture);

        cloudModel.draw();
    }

    // Отрисовка птиц
    for (const auto& bird : birdSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird.position);
        model = glm::scale(model, glm::vec3(bird.size));

        // Птицы летят под углом
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // Анимация крыльев
        float wingFlap = sin(glfwGetTime() * 5.0f + bird.position.x) * 0.1f;
        model = glm::scale(model, glm::vec3(1.0f + wingFlap, 1.0f, 1.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, &bird.color[0]);
        glUniform1i(useTextureLoc, birdModel.hasTexture);

        birdModel.draw();
    }
}

// Отрисовка спрайтов на земле
void drawGroundSprites() {
    for (const auto& flower : flowerSprites) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, flower.position);
        model = glm::scale(model, glm::vec3(flower.size));

        // Цветы повернуты к камере (billboard)
        glm::vec3 toCamera = glm::normalize(cameraPos - flower.position);
        // Простой billboard - всегда смотрим на камеру по Y
        model = glm::rotate(model, atan2f(toCamera.x, toCamera.z), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, &flower.color[0]);
        glUniform1i(useTextureLoc, flowerModel.hasTexture);

        flowerModel.draw();
    }
}

// Основная функция отрисовки
void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    drawFloor();
    drawGroundSprites(); // Сначала рисуем цветы на земле
    drawFence();
    drawObstaclesAsTrees(); // Рисуем деревья вместо кубов
    drawSnake();
    drawFood();
    drawSkySprites(); // Потом рисуем небо поверх всего
}

// Обработка ввода с клавиатуры
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_LEFT:
            if (currentDirection == FORWARD) currentDirection = LEFT;
            else if (currentDirection == LEFT) currentDirection = BACKWARD;
            else if (currentDirection == BACKWARD) currentDirection = RIGHT;
            else if (currentDirection == RIGHT) currentDirection = FORWARD;
            break;
        case GLFW_KEY_RIGHT:
            if (currentDirection == FORWARD) currentDirection = RIGHT;
            else if (currentDirection == RIGHT) currentDirection = BACKWARD;
            else if (currentDirection == BACKWARD) currentDirection = LEFT;
            else if (currentDirection == LEFT) currentDirection = FORWARD;
            break;
        case GLFW_KEY_R:
            if (gameOver) initGame();
            break;
        case GLFW_KEY_P:
            paused = !paused;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        case GLFW_KEY_D:
            debugSnakeInfo();
            break;
        }
    }
}

// Загрузка всех моделей
bool loadAllModels() {
    std::cout << "Loading models..." << std::endl;

    // Пытаемся загрузить модели из файлов, если не получится - создаем процедурные
    bool cloudLoaded = loadOBJFile("models/cloud.obj", cloudModel);
    bool birdLoaded = loadOBJFile("models/bird.obj", birdModel);
    bool treeLoaded = loadOBJFile("models/tree.obj", treeModel);
    bool flowerLoaded = loadOBJFile("models/flower.obj", flowerModel);
    bool appleLoaded = loadOBJFile("models/apple.obj", appleModel);

    // Если не загрузились - создаем процедурные
    if (!cloudLoaded) {
        std::cout << "Creating procedural cloud model..." << std::endl;
        createCloudModel(cloudModel);
    }
    if (!birdLoaded) {
        std::cout << "Creating procedural bird model..." << std::endl;
        createBirdModel(birdModel);
    }
    if (!treeLoaded) {
        std::cout << "Creating procedural tree model..." << std::endl;
        createTreeModel(treeModel);
    }
    if (!flowerLoaded) {
        std::cout << "Creating procedural flower model..." << std::endl;
        createFlowerModel(flowerModel);
    }
    if (!appleLoaded) {
        std::cout << "Creating procedural apple model..." << std::endl;
        createDetailedAppleModel(appleModel);
    }

    // Базовые модели (всегда процедурные)
    createTexturedCubeModel(snakeModel);
    createTexturedSphereModel(foodModel);
    createTexturedCubeModel(obstacleModel);
    createTexturedFloorModel(floorModel);
    createFenceModel(fenceModel);

    std::cout << "All models loaded successfully!" << std::endl;
    return true;
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "3D Snake Game with Enhanced Sprites", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);

    srand(static_cast<unsigned int>(time(0)));
    createShaderProgram();

    // Загружаем модели со специализированными спрайтами
    loadAllModels();

    initGame();

    double lastUpdateTime = glfwGetTime();
    double lastSpriteUpdateTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();

        std::string title = "3D Snake with Enhanced Sprites - Score: " + std::to_string(score) +
            " | Camera: " + std::to_string((int)targetCameraDistance) + "m";
        if (gameOver) title += " - GAME OVER! Press R to restart";
        if (paused) title += " - PAUSED";
        glfwSetWindowTitle(window, title.c_str());

        if (currentTime - lastUpdateTime > 0.12) {
            if (!paused && !gameOver) updateGame();
            lastUpdateTime = currentTime;
        }

        // Обновляем спрайты неба каждые 0.05 секунды для плавности
        if (currentTime - lastSpriteUpdateTime > 0.05) {
            updateSkySprites();
            lastSpriteUpdateTime = currentTime;
        }

        updateCameraSmoothly();
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}