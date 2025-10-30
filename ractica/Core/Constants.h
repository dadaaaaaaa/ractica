#pragma once

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