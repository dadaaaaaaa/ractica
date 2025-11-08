#pragma once
#include <string>
#include <glm/glm.hpp>

// ============================================================================
// ОСНОВНЫЕ КОНСТАНТЫ ИГРЫ
// ============================================================================

// Размеры игрового поля
const int GRID_WIDTH = 120;
const int GRID_HEIGHT = 10;
const int GRID_DEPTH = 120;
const float CELL_SIZE = 0.1f;

// Количество объектов
const int OBSTACLE_COUNT = 10;
const int INITIAL_FOOD_COUNT = 10;
const int INITIAL_SNAKE_LENGTH = 3;
const int MAX_SNAKE_LENGTH = 1000;
const float SNAKE_GROWTH_PER_FOOD = 1.0f;

// ============================================================================
// ТИПЫ ПРЕПЯТСТВИЙ
// ============================================================================

enum class ObstacleType {
    CROSS,      // Крестообразное
    SQUARE,     // Квадратное
    LINE,       // Линейное
    RANDOM      // Случайное
};

// ============================================================================
// НАСТРОЙКИ КАМЕРЫ
// ============================================================================

const float CAMERA_FOV = 60.0f;
const float CAMERA_NEAR_PLANE = 0.1f;
const float CAMERA_FAR_PLANE = 100.0f;
const float CAMERA_DEFAULT_HEIGHT = 3.0f;
const float CAMERA_MIN_DISTANCE = 2.0f;
const float CAMERA_MAX_DISTANCE = 8.0f;
const float CAMERA_ZOOM_SPEED = 0.5f;
const float CAMERA_SMOOTHNESS = 0.1f;
const float CAMERA_ROTATION_SPEED = 0.05f;
const float CAMERA_ANGLE_X = -45.0f;
const float CAMERA_ANGLE_Y = 45.0f;

// ============================================================================
// НАСТРОЙКИ ОКНА И ГРАФИКИ
// ============================================================================

const int DEFAULT_WINDOW_WIDTH = 1200;
const int DEFAULT_WINDOW_HEIGHT = 800;
const float BACKGROUND_COLOR_R = 0.1f;
const float BACKGROUND_COLOR_G = 0.2f;
const float BACKGROUND_COLOR_B = 0.3f;
const float BACKGROUND_COLOR_A = 1.0f;
const float UI_BACKGROUND_ALPHA = 0.7f;
const float UI_ELEMENT_ALPHA = 0.9f;

// ============================================================================
// СИСТЕМА ГЕНЕРАЦИИ МИРА
// ============================================================================

// Количество декораций
const int CLOUD_COUNT = 20;
const int BIRD_COUNT = 15;
const int FLOWER_COUNT = 25;

// Лимиты попыток генерации
const int MAX_GENERATION_ATTEMPTS = 30;
const int MAX_OBSTACLE_GENERATION_ATTEMPTS = 100;
const int MAX_FOOD_GENERATION_ATTEMPTS = 50;

// Облака
const float CLOUD_MIN_DISTANCE = 10.0f;
const float CLOUD_MAX_DISTANCE = 16.0f;
const float CLOUD_MIN_HEIGHT = 2.3f;
const float CLOUD_MAX_HEIGHT = 3.7f;
const float CLOUD_SAFE_DISTANCE = 2.0f;
const float CLOUD_TARGET_DISTANCE = 13.0f;
const float CLOUD_MIN_DISTANCE_THRESHOLD = 9.0f;
const float CLOUD_MAX_DISTANCE_THRESHOLD = 17.0f;
const float CLOUD_HEIGHT_VARIATION = 0.0001f;
const float CLOUD_SPEED = 0.2f;

// Птицы
const float BIRD_MIN_DISTANCE = 8.0f;
const float BIRD_MAX_DISTANCE = 12.0f;
const float BIRD_MIN_HEIGHT = 2.8f;
const float BIRD_MAX_HEIGHT = 4.0f;
const float BIRD_SAFE_DISTANCE = 1.5f;
const float BIRD_TARGET_DISTANCE = 10.0f;
const float BIRD_MIN_DISTANCE_THRESHOLD = 6.0f;
const float BIRD_MAX_DISTANCE_THRESHOLD = 14.0f;
const float BIRD_DIRECTION_CHANGE_CHANCE = 1.0f / 300.0f;
const float BIRD_MAX_DIRECTION_CHANGE = 10.0f * 3.14159f / 180.0f;
const float BIRD_SPEED = 0.3f;

// Цветы
const float FIELD_EXTENT = 10.0f;
const float MIN_FLOWER_SIZE = 0.1f;
const float MAX_FLOWER_SIZE = 0.2f;
const float FLOWER_HEIGHT = 0.01f;

// Еда и препятствия
const float FOOD_GENERATION_MARGIN = 5.0f;
const float OBSTACLE_GENERATION_MARGIN = 10.0f;
const float MIN_OBJECT_DISTANCE = 2.0f;

// ============================================================================
// СИСТЕМА РЕКОРДОВ И СОХРАНЕНИЙ
// ============================================================================

const int MIN_SCORE_FOR_HIGHSCORE = 1;
const int HIGH_SCORES_DISPLAY_LIMIT = 10;
const int MAX_PLAYER_NAME_LENGTH = 20;
const std::string DEFAULT_PLAYER_NAME = "Player";

// Пути файлов
const std::string SAVE_FILE_PATH = "savegame.dat";
const std::string SETTINGS_FILE_PATH = "settings.dat";
const std::string HIGH_SCORES_FILE_PATH = "highscores.dat";

// ============================================================================
// СИСТЕМА СКОРОСТИ ИГРЫ И СЛОЖНОСТИ
// ============================================================================

const float BASE_GAME_SPEED = 0.12f;
const float SPEED_INCREMENT_PER_FOOD = 0.002f;
const float MAX_GAME_SPEED_MULTIPLIER = 3.0f;
const float MIN_GAME_SPEED_MULTIPLIER = 0.5f;
const float GAME_SPEED_STEP = 0.1f;

// ============================================================================
// НАСТРОЙКИ СЕТИ
// ============================================================================

const std::string DEFAULT_SERVER_URL = "https://snake-game.loca.lt";
const int NETWORK_TIMEOUT_SECONDS = 10;
const int NETWORK_CONNECT_TIMEOUT_SECONDS = 5;
const std::string NETWORK_USER_AGENT = "3D-Snake-Game/1.0";

// ============================================================================
// НАСТРОЙКИ UI И ШРИФТОВ
// ============================================================================

const int DEFAULT_FONT_SIZE = 24;
const std::string DEFAULT_FONT_PATH = "C:/Windows/Fonts/arial.ttf";
const int UI_FONT_SIZE_SMALL = 18;
const int UI_FONT_SIZE_LARGE = 32;
const float TEXT_SCALE_FACTOR = 1.0f;

// Кнопки и интерфейс
const int BUTTON_WIDTH = 200;
const int BUTTON_HEIGHT = 50;
const int BUTTON_SPACING = 80;
const int MENU_START_Y = 400;
const int BACK_BUTTON_X_OFFSET = 220;
const int BACK_BUTTON_Y = 50;

// Позиции текста в UI
const int TITLE_Y = 600;
const int SCORE_DISPLAY_Y = 50;
const int SPEED_DISPLAY_Y = 80;

// High Scores таблица
const int HIGH_SCORES_RANK_X = 200;
const int HIGH_SCORES_PLAYER_X = 300;
const int HIGH_SCORES_SCORE_X = 500;
const int HIGH_SCORES_DATE_X = 600;
const int HIGH_SCORES_SPEED_X = 750;
const int HIGH_SCORES_LENGTH_X = 850;
const int HIGH_SCORES_TIME_X = 950;
const int HIGH_SCORES_HEADER_Y = 500;
const int HIGH_SCORES_ROW_SPACING = 35;
const int HIGH_SCORES_MAX_DISPLAY = 10;
const int HIGH_SCORES_MAX_NAME_LENGTH = 10;

// Controls меню
const int CONTROLS_START_Y = 450;
const int CONTROLS_SPACING = 40;

// Settings меню
const int SETTINGS_SPEED_Y = 350;
const int SETTINGS_SPEED_BUTTON_OFFSET = 100;
const int SETTINGS_SPEED_BUTTON_SIZE = 20;

// Масштабирование UI
const float SCALE_BASE_WIDTH = 1200.0f;
const float SCALE_BASE_HEIGHT = 800.0f;

// ============================================================================
// ЦВЕТА
// ============================================================================

// Основные цвета UI
const glm::vec3 COLOR_BUTTON_NORMAL(0.1f, 0.3f, 0.1f);
const glm::vec3 COLOR_BUTTON_HOVER(0.2f, 0.6f, 0.2f);
const glm::vec3 COLOR_TEXT_WHITE(1.0f, 1.0f, 1.0f);
const glm::vec3 COLOR_TEXT_RED(1.0f, 0.0f, 0.0f);
const glm::vec3 COLOR_TEXT_GREEN(0.0f, 1.0f, 0.0f);
const glm::vec3 COLOR_TEXT_BLUE(0.0f, 0.0f, 1.0f);
const glm::vec3 COLOR_TEXT_YELLOW(1.0f, 1.0f, 0.0f);

// Цвета рекордов
const glm::vec3 COLOR_GOLD(1.0f, 0.8f, 0.0f);
const glm::vec3 COLOR_SILVER(0.7f, 0.7f, 0.7f);
const glm::vec3 COLOR_BRONZE(0.8f, 0.5f, 0.2f);

// Цвета фонов меню
const glm::vec3 COLOR_MAIN_MENU_BG(0.1f, 0.2f, 0.3f);
const glm::vec3 COLOR_SETTINGS_BG(0.1f, 0.3f, 0.2f);
const glm::vec3 COLOR_HIGHSCORES_BG(0.15f, 0.05f, 0.25f);
const glm::vec3 COLOR_CONTROLS_BG(0.1f, 0.2f, 0.1f);
const glm::vec3 COLOR_PAUSE_OVERLAY(0.0f, 0.0f, 0.0f);
const glm::vec3 COLOR_GAME_OVER_OVERLAY(0.0f, 0.0f, 0.0f);

// Прозрачности
const float UI_BACKGROUND_ALPHA = 0.7f;
const float UI_ELEMENT_ALPHA = 0.9f;
const float OVERLAY_ALPHA = 0.7f;

// ============================================================================
// ФИЗИКА И КОЛЛИЗИИ
// ============================================================================

const float COLLISION_DETECTION_MARGIN = 0.1f;
const float MIN_MOVEMENT_THRESHOLD = 0.001f;
const float GRAVITY = -9.8f;
const float PHYSICS_TIMESTEP = 0.016f;

// ============================================================================
// ТАЙМАУТЫ И ИНТЕРВАЛЫ
// ============================================================================

const double MIN_FRAME_TIME = 1.0 / 60.0; // 60 FPS
const double AUTO_SAVE_INTERVAL = 30.0; // секунд
const double GAME_OVER_DISPLAY_TIME = 3.0; // секунд
const double ANIMATION_UPDATE_INTERVAL = 0.05; // секунд

// ============================================================================
// КОДЫ КЛАВИШ
// ============================================================================

const int KEY_LEFT = 263;
const int KEY_RIGHT = 262;
const int KEY_UP = 265;
const int KEY_DOWN = 264;
const int KEY_SPACE = 32;
const int KEY_ESC = 256;
const int KEY_ENTER = 257;
const int KEY_P = 80;
const int KEY_R = 82;
const int KEY_Q = 81;
const int KEY_E = 69;
const int KEY_D = 68;
const int KEY_PLUS = 334;
const int KEY_MINUS = 333;
const int KEY_EQUAL = 61;

// ============================================================================
// ПЕРЕЧИСЛЕНИЯ
// ============================================================================

// Направления движения
enum Direction {
    FORWARD,
    BACKWARD,
    RIGHT,
    LEFT,
    UP,
    DOWN
};

// Состояния игры
enum GameState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    SETTINGS,
    HIGH_SCORES,
    CONTROLS,
    GAME_OVER
};

// Типы объектов
enum ObjectType {
    OBJECT_SNAKE,
    OBJECT_FOOD,
    OBJECT_OBSTACLE,
    OBJECT_CLOUD,
    OBJECT_BIRD,
    OBJECT_FLOWER,
    OBJECT_GROUND
};

// Типы шейдеров
enum ShaderType {
    SHADER_MAIN,
    SHADER_UI,
    SHADER_TEXT,
    SHADER_SKYBOX
};

// ============================================================================
// НАСТРОЙКИ ЗВУКА (если будешь добавлять)
// ============================================================================

const float SOUND_VOLUME_DEFAULT = 0.8f;
const float SOUND_VOLUME_MIN = 0.0f;
const float SOUND_VOLUME_MAX = 1.0f;

// ============================================================================
// ДЕБАГ НАСТРОЙКИ
// ============================================================================

#ifdef _DEBUG
const bool ENABLE_DEBUG_INFO = true;
const bool ENABLE_COLLISION_VISUALIZATION = true;
const bool ENABLE_WIREFRAME_MODE = false;
const bool LOG_NETWORK_REQUESTS = true;
#else
const bool ENABLE_DEBUG_INFO = false;
const bool ENABLE_COLLISION_VISUALIZATION = false;
const bool ENABLE_WIREFRAME_MODE = false;
const bool LOG_NETWORK_REQUESTS = false;
#endif