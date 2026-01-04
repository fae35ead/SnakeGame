#pragma once
#include <graphics.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// 定义常量
#define UNIT 20
#define SAVE_FILE "snake_save.dat"
#define HIGHSCORE_FILE "highscore.dat"
#define CONFIG_FILE "config.dat"

// --- 基础结构体 ---
struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

struct Particle {
    double x, y;
    double vx, vy;
    int life;
    COLORREF color;
};

// --- 枚举定义 ---
enum class ItemType { SLOW, CUT, DOUBLE, GHOST };
enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class GameState { MAIN_MENU, MAP_SELECT, PLAYING, SETTINGS, AUDIO_SETTINGS, DIFFICULTY_SETTINGS, GAMEOVER, EXIT };
enum class Difficulty { SIMPLE, STANDARD };

// --- 道具结构 ---
struct Item {
    Point pos;
    ItemType type;
    int life;
};

// --- 配置结构 ---
struct GameConfig {
    int sfxVolume = 50;
    int musicVolume = 40;
    int masterVolume = 80;
    Difficulty diff = Difficulty::STANDARD;
};
