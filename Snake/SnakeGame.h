#pragma once
#include "GameTypes.h" 


class SnakeGame {
private:
    static constexpr int WIN_WIDTH = 640;
    static constexpr int WIN_HEIGHT = 480;
    static constexpr int GRID_W = WIN_WIDTH / UNIT;
    static constexpr int GRID_H = WIN_HEIGHT / UNIT;

    std::vector<Point> snake;
    std::vector<Point> walls;
    std::vector<Particle> particles;
    std::vector<int> highScores;
    Point food;
    GameState state;
    GameConfig config;
    Direction dir;
    Direction nextDir;
    int score;
    int currentMap;
    int menuCursor;
    int smallFoodCount = 0;
    int permanentSlow = 0;
    double bigFoodTime = 0.0;
    bool isBigFood = false;
    bool dirLocked = false;
    bool newRecordAchieved = false;
    bool isPaused = false;

    // 道具相关
    std::vector<Item> items;
    IMAGE imgSlow, imgCut, imgDouble, imgGhost;
    double buffSlowTime = 0.0;
    double buffDoubleTime = 0.0;
    double buffGhostTime = 0.0;

public:
    SnakeGame();
    ~SnakeGame();
    void run();

private:
    // 辅助功能
    void updateVolume();
    void putimage_alpha(int x, int y, IMAGE* img);
    int getSpeed();
    bool isHeadSafe();
    bool isOccupied(Point p);

    // 存档与加载
    void saveGame();
    bool loadGame();
    void loadConfig();
    void saveConfig();
    void loadHighScore();
    void saveHighScore();

    // 菜单与 UI
    void drawUIFrame(const std::wstring& title);
    void handleMainMenu();
    void handleSettings();
    void handleAudioSettings();
    void handleDifficultySettings();
    void handleMapSelect();
    void showGameOver();

    // 游戏逻辑
    void initMap(int mapIdx);
    void resetGame();
    void playGame();
    void handleGameInput();
    void updateGameLogic();
    void renderGame();
    void onGameOver();
    void generateFood();

    // 粒子与道具
    void spawnParticles(int x, int y, COLORREF color, bool isBig = false);
    void updateParticles();
    void renderParticles();
    void spawnItem();
    void updateItems();
    void applyItemEffect(ItemType type);
    void renderItems();
};
