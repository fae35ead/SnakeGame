#include "SnakeGame.h"
#include <fstream>
#include <mmsystem.h>
#include <ctime>

using namespace std;

// 构造函数
SnakeGame::SnakeGame() : state(GameState::MAIN_MENU), menuCursor(0), currentMap(1) {
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    loadConfig();
    BeginBatchDraw();
    srand((unsigned int)time(NULL));

    mciSendString(L"open \"bgm.mp3\" alias bgm", NULL, 0, NULL);
    mciSendString(L"play bgm repeat", NULL, 0, NULL);
    updateVolume();

    mciSendString(L"open \"eatFood.mp3\" alias eat", NULL, 0, NULL);
    mciSendString(L"open \"victory.mp3\" alias victory", NULL, 0, NULL);
    mciSendString(L"open \"click.mp3\" alias click", NULL, 0, NULL);
    updateVolume();

    loadHighScore();

    loadimage(&imgSlow, L"icon_slow.png", UNIT, UNIT);
    loadimage(&imgCut, L"icon_cut.png", UNIT, UNIT);
    loadimage(&imgDouble, L"icon_double.png", UNIT, UNIT);
    loadimage(&imgGhost, L"icon_ghost.png", UNIT, UNIT);
}

// 析构函数
SnakeGame::~SnakeGame() {
    mciSendString(L"close bgm", NULL, 0, NULL);
    mciSendString(L"close eat", NULL, 0, NULL);
    mciSendString(L"close victory", NULL, 0, NULL);
    mciSendString(L"close click", NULL, 0, NULL);
    EndBatchDraw();
    closegraph();
}

void SnakeGame::updateVolume() {
    wstring volCmd = L"setaudio bgm volume to " + to_wstring(config.musicVolume * 10);
    mciSendString(volCmd.c_str(), NULL, 0, NULL);
    wstring sfxVolCmd = L"setaudio eat volume to " + to_wstring(config.sfxVolume * 10);
    mciSendString(sfxVolCmd.c_str(), NULL, 0, NULL);
    wstring vicVolCmd = L"setaudio victory volume to " + to_wstring(config.sfxVolume * 10);
    mciSendString(vicVolCmd.c_str(), NULL, 0, NULL);
    wstring clickVolCmd = L"setaudio click volume to " + to_wstring(config.sfxVolume * 10);
    mciSendString(clickVolCmd.c_str(), NULL, 0, NULL);
}

void SnakeGame::run() {
    while (state != GameState::EXIT) {
        switch (state) {
        case GameState::MAIN_MENU:         handleMainMenu(); break;
        case GameState::MAP_SELECT:        handleMapSelect(); break;
        case GameState::SETTINGS:          handleSettings(); break;
        case GameState::AUDIO_SETTINGS:     handleAudioSettings(); break;
        case GameState::DIFFICULTY_SETTINGS:handleDifficultySettings(); break;
        case GameState::PLAYING:           playGame(); break;
        case GameState::GAMEOVER:          showGameOver(); break;
        }
        Sleep(20);
    }
}

// 1、存档与加载系统
void SnakeGame::saveGame() {
    ofstream f(SAVE_FILE, ios::binary);
    if (!f) return;

    // 写入固定数据
    f.write((char*)&score, sizeof(int));
    f.write((char*)&currentMap, sizeof(int));
    f.write((char*)&dir, sizeof(Direction));
    f.write((char*)&food, sizeof(Point));
    f.write((char*)&smallFoodCount, sizeof(int));
    f.write((char*)&bigFoodTime, sizeof(double));
    f.write((char*)&isBigFood, sizeof(bool));

    // 写入动态长度蛇身
    int size = (int)snake.size();
    f.write((char*)&size, sizeof(int));
    f.write((char*)snake.data(), sizeof(Point) * size);

    // 写入buff状态
    f.write((char*)&buffSlowTime, sizeof(double));
    f.write((char*)&buffDoubleTime, sizeof(double));
    f.write((char*)&buffGhostTime, sizeof(double));
    f.write((char*)&permanentSlow, sizeof(int));

    // 写入屏幕上的道具
    int itemCount = (int)items.size();
    f.write((char*)&itemCount, sizeof(int));
    if (itemCount > 0) {
        f.write((char*)items.data(), sizeof(Item) * itemCount);
    }

    f.close();
}

bool SnakeGame::loadGame() {
    ifstream f(SAVE_FILE, ios::binary);
    if (!f) return false;

    // 读取基础数据
    f.read((char*)&score, sizeof(int));
    f.read((char*)&currentMap, sizeof(int));
    f.read((char*)&dir, sizeof(Direction));
    f.read((char*)&food, sizeof(Point));
    f.read((char*)&smallFoodCount, sizeof(int));
    f.read((char*)&bigFoodTime, sizeof(double));
    f.read((char*)&isBigFood, sizeof(bool));

    // 读取蛇身
    int size;
    f.read((char*)&size, sizeof(int));
    snake.resize(size);
    f.read((char*)snake.data(), sizeof(Point) * size);

    // 读取buff
    f.read((char*)&buffSlowTime, sizeof(double));
    f.read((char*)&buffDoubleTime, sizeof(double));
    f.read((char*)&buffGhostTime, sizeof(double));
    f.read((char*)&permanentSlow, sizeof(double));

    // 读取道具
    int itemCount;
    f.read((char*)&itemCount, sizeof(int));
    items.resize(itemCount);
    if (itemCount > 0) {
        f.read((char*)items.data(), sizeof(Item) * itemCount);
    }

    f.close();

    nextDir = dir;
    initMap(currentMap);
    return true;
}

// 2. 菜单与 UI 处理
void SnakeGame::drawUIFrame(const wstring& title) {
    setbkcolor(RGB(25, 25, 35));
    cleardevice();
    settextcolor(WHITE);
    settextstyle(40, 0, L"微软雅黑", 0, 0, FW_BOLD, false, false, false);
    outtextxy(WIN_WIDTH / 2 - 75, 100, title.c_str());
}

void SnakeGame::handleMainMenu() {
    const vector<wstring> options = { L"新 游 戏", L"继 续 游 戏", L"设 置", L"退 出 游 戏" };
    drawUIFrame(L"贪吃蛇");

    // 绘制排行榜
    settextcolor(LIGHTGRAY);
    settextstyle(20, 0, L"微软雅黑");
    outtextxy(WIN_WIDTH - 180, 150, L"🏆 排行榜 🏆");

    settextstyle(18, 0, L"Consolas");
    for (int i = 0; i < 5; i++) {
        wstring rankStr = to_wstring(i + 1) + L". " + to_wstring(highScores[i]);
        // 前三名用不同颜色高亮
        if (i == 0) settextcolor(YELLOW);
        else if (i == 1) settextcolor(RGB(192, 192, 192)); // 银色
        else if (i == 2) settextcolor(RGB(205, 127, 50));  // 铜色
        else settextcolor(WHITE);

        outtextxy(WIN_WIDTH - 160, 190 + i * 30, rankStr.c_str());
    }

    for (int i = 0; i < (int)options.size(); i++) {
        settextcolor(i == menuCursor ? YELLOW : WHITE);
        settextstyle(25, 0, L"微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 60, 180 + i * 45, options[i].c_str());
    }
    FlushBatchDraw();
    ExMessage msg;
    if (peekmessage(&msg, EX_KEY) && msg.message == WM_KEYDOWN) {
        // 播放按键音效
        if (msg.vkcode == VK_UP || msg.vkcode == 'W' ||
            msg.vkcode == VK_DOWN || msg.vkcode == 'S' ||
            msg.vkcode == VK_RETURN) {
            mciSendString(L"play click from 0", NULL, 0, NULL);
        }
        // 处理菜单导航
        if (msg.vkcode == VK_UP || msg.vkcode == 'W') menuCursor = (menuCursor - 1 + (int)options.size()) % (int)options.size();
        else if (msg.vkcode == VK_DOWN || msg.vkcode == 'S') menuCursor = (menuCursor + 1) % (int)options.size();
        else if (msg.vkcode == VK_RETURN) {
            if (menuCursor == 0) state = GameState::MAP_SELECT;
            else if (menuCursor == 1) { if (loadGame()) state = GameState::PLAYING; }
            else if (menuCursor == 2) { state = GameState::SETTINGS; menuCursor = 0; }
            else if (menuCursor == 3) state = GameState::EXIT;
        }
    }
}

void SnakeGame::handleSettings() {
    const vector<wstring> options = { L"音频设置", L"难度设置", L"返回主菜单" };
    drawUIFrame(L"设 置");
    for (int i = 0; i < (int)options.size(); i++) {
        settextcolor(i == menuCursor ? YELLOW : WHITE);
        outtextxy(WIN_WIDTH / 2 - 60, 180 + i * 45, options[i].c_str());
    }
    FlushBatchDraw();
    ExMessage msg;
    if (peekmessage(&msg, EX_KEY) && msg.message == WM_KEYDOWN) {
        // 播放按键音效
        if (msg.vkcode == VK_UP || msg.vkcode == 'W' ||
            msg.vkcode == VK_DOWN || msg.vkcode == 'S' ||
            msg.vkcode == VK_RETURN) {
            mciSendString(L"play click from 0", NULL, 0, NULL);
        }
        // 处理菜单导航
        if (msg.vkcode == VK_UP || msg.vkcode == 'W') menuCursor = (menuCursor - 1 + (int)options.size()) % (int)options.size();
        else if (msg.vkcode == VK_DOWN || msg.vkcode == 'S') menuCursor = (menuCursor + 1) % (int)options.size();
        else if (msg.vkcode == VK_RETURN) {
            if (menuCursor == 0) { state = GameState::AUDIO_SETTINGS; menuCursor = 0; }
            else if (menuCursor == 1) { state = GameState::DIFFICULTY_SETTINGS; menuCursor = 0; }
            else if (menuCursor == 2) { state = GameState::MAIN_MENU; menuCursor = 0; }
        }
    }
}

void SnakeGame::handleAudioSettings() {
    drawUIFrame(L"音频设置");
    wstring sfx = L"音效大小: " + to_wstring(config.sfxVolume);
    wstring mus = L"音乐大小: " + to_wstring(config.musicVolume);
    wstring mas = L"整体音量: " + to_wstring(config.masterVolume);
    const vector<wstring> options = { sfx, mus, mas, L"保存并返回" };
    for (int i = 0; i < (int)options.size(); i++) {
        settextcolor(i == menuCursor ? YELLOW : WHITE);
        outtextxy(WIN_WIDTH / 2 - 100, 180 + i * 45, options[i].c_str());
    }
    FlushBatchDraw();
    ExMessage msg;
    if (peekmessage(&msg, EX_KEY) && msg.message == WM_KEYDOWN) {
        // 播放按键音效
        if (msg.vkcode == VK_UP || msg.vkcode == 'W' ||
            msg.vkcode == VK_DOWN || msg.vkcode == 'S' ||
            msg.vkcode == VK_LEFT || msg.vkcode == 'A' ||
            msg.vkcode == VK_RIGHT || msg.vkcode == 'D' ||
            msg.vkcode == VK_RETURN) {
            mciSendString(L"play click from 0", NULL, 0, NULL);
        }
        // 处理菜单导航
        if (msg.vkcode == VK_UP || msg.vkcode == 'W') menuCursor = (menuCursor - 1 + (int)options.size()) % (int)options.size();
        else if (msg.vkcode == VK_DOWN || msg.vkcode == 'S') menuCursor = (menuCursor + 1) % (int)options.size();
        else if (msg.vkcode == VK_LEFT || msg.vkcode == 'A' || msg.vkcode == VK_RIGHT || msg.vkcode == 'D') {
            int delta = (msg.vkcode == VK_RIGHT || msg.vkcode == 'D' ? 5 : -5);
            if (menuCursor == 0) {
                config.sfxVolume = max(0, min(100, config.sfxVolume + delta));
                updateVolume();
                mciSendString(L"play eat from 0", NULL, 0, NULL);
                saveConfig();
            }
            if (menuCursor == 1) {
                config.musicVolume = max(0, min(100, config.musicVolume + delta));
                updateVolume();
                saveConfig();
            }
            if (menuCursor == 2) {
                config.masterVolume = max(0, min(100, config.masterVolume + delta));
                saveConfig();
            }
        }
        else if (msg.vkcode == VK_RETURN && menuCursor == 3) { state = GameState::SETTINGS; menuCursor = 0; }
    }
}

void SnakeGame::handleDifficultySettings() {
    drawUIFrame(L"难度设置");
    wstring mode = L"当前模式: " + wstring(config.diff == Difficulty::SIMPLE ? L"简单 (穿墙)" : L"标准 (撞墙)");
    const vector<wstring> options = { mode, L"保存并返回" };
    for (int i = 0; i < (int)options.size(); i++) {
        settextcolor(i == menuCursor ? YELLOW : WHITE);
        outtextxy(WIN_WIDTH / 2 - 120, 180 + i * 45, options[i].c_str());
    }
    FlushBatchDraw();
    ExMessage msg;
    if (peekmessage(&msg, EX_KEY) && msg.message == WM_KEYDOWN) {
        // 播放按键音效
        if (msg.vkcode == VK_UP || msg.vkcode == 'W' ||
            msg.vkcode == VK_DOWN || msg.vkcode == 'S' ||
            msg.vkcode == VK_RETURN) {
            mciSendString(L"play click from 0", NULL, 0, NULL);
        }
        // 处理菜单导航
        if (msg.vkcode == VK_UP || msg.vkcode == 'W' || msg.vkcode == VK_DOWN || msg.vkcode == 'S') menuCursor = (menuCursor == 0 ? 1 : 0);
        if (msg.vkcode == VK_RETURN) {
            if (menuCursor == 0) {
                config.diff = (config.diff == Difficulty::SIMPLE ? Difficulty::STANDARD : Difficulty::SIMPLE);
                saveConfig();
            }
            else { state = GameState::SETTINGS; menuCursor = 1; }
        }
    }
}

void SnakeGame::handleMapSelect() {
    drawUIFrame(L"选择地图");
    const vector<wstring> options = { L"经典空地", L"十字路口", L"封闭环绕", L"返回" };
    for (int i = 0; i < (int)options.size(); i++) {
        settextcolor(i == menuCursor ? YELLOW : WHITE);
        outtextxy(WIN_WIDTH / 2 - 60, 180 + i * 45, options[i].c_str());
    }
    FlushBatchDraw();
    ExMessage msg;
    if (peekmessage(&msg, EX_KEY) && msg.message == WM_KEYDOWN) {
        // 播放按键音效
        if (msg.vkcode == VK_UP || msg.vkcode == 'W' ||
            msg.vkcode == VK_DOWN || msg.vkcode == 'S' ||
            msg.vkcode == VK_RETURN) {
            mciSendString(L"play click from 0", NULL, 0, NULL);
        }
        // 处理菜单导航
        if (msg.vkcode == VK_UP || msg.vkcode == 'W') menuCursor = (menuCursor - 1 + (int)options.size()) % (int)options.size();
        else if (msg.vkcode == VK_DOWN || msg.vkcode == 'S') menuCursor = (menuCursor + 1) % (int)options.size();
        else if (msg.vkcode == VK_RETURN) {
            if (menuCursor < 3) {
                currentMap = menuCursor + 1;
                resetGame();
                state = GameState::PLAYING;
            }
            else { state = GameState::MAIN_MENU; menuCursor = 0; }
        }
    }
}

//道具系统辅助函数，用于带透明通道的图像绘制
void SnakeGame::putimage_alpha(int x, int y, IMAGE* img) {
    int w = img->getwidth();
    int h = img->getheight();
    DWORD* dst = GetImageBuffer();
    DWORD* src = GetImageBuffer(img);
    int src_width = w;
    int dst_width = getwidth();
    int i_start = 0, i_end = w;
    int j_start = 0, j_end = h;
    if (x < 0) i_start = -x;
    if (y < 0) j_start = -y;
    if (x + w > dst_width) i_end = dst_width - x;
    if (y + h > getheight()) j_end = getheight() - y;
    for (int j = j_start; j < j_end; j++) {
        for (int i = i_start; i < i_end; i++) {
            DWORD src_color = src[j * src_width + i];
            unsigned char alpha = (src_color >> 24);
            if (alpha == 0) continue;
            int dst_idx = (y + j) * dst_width + (x + i);
            if (alpha == 255) dst[dst_idx] = src_color;
            else {
                DWORD dst_color = dst[dst_idx];
                unsigned char r_src = (src_color >> 16) & 0xFF;
                unsigned char g_src = (src_color >> 8) & 0xFF;
                unsigned char b_src = src_color & 0xFF;
                unsigned char r_dst = (dst_color >> 16) & 0xFF;
                unsigned char g_dst = (dst_color >> 8) & 0xFF;
                unsigned char b_dst = dst_color & 0xFF;
                dst[dst_idx] = ((alpha * (r_src - r_dst) >> 8) + r_dst) << 16
                    | ((alpha * (g_src - g_dst) >> 8) + g_dst) << 8
                    | ((alpha * (b_src - b_dst) >> 8) + b_dst);
            }
        }
    }
}

void SnakeGame::spawnItem() {
    if (rand() % 500 != 0) return; // 约每 500 帧随机生成一个

    Item it;
    bool overlap;
    do {
        it.pos.x = rand() % GRID_W;
        it.pos.y = rand() % GRID_H;

        overlap = false;
        // 检查是否与现有的物品重叠
        for (auto& existingItem : items) {
            if (it.pos == existingItem.pos) {
                overlap = true;
                break;
            }
        }
    } while (isOccupied(it.pos) || it.pos == food || overlap);

    int r = rand() % 4;
    if (r == 0) it.type = ItemType::SLOW;
    else if (r == 1) it.type = ItemType::CUT;
    else if (r == 2) it.type = ItemType::DOUBLE;
    else it.type = ItemType::GHOST;

    it.life = 300; // 场上保留 300 帧
    items.push_back(it);
}

void SnakeGame::updateItems() {
    for (auto it = items.begin(); it != items.end(); ) {
        it->life--;
        if (it->life <= 0) it = items.erase(it);
        else ++it;
    }
    // 更新 Buff 时间 (每帧约 10ms，故减去 0.01s)
    if (buffSlowTime > 0) buffSlowTime -= 0.01;
    if (buffDoubleTime > 0) buffDoubleTime -= 0.01;

    if (buffGhostTime > 0) {
        buffGhostTime -= 0.01;

        if (buffGhostTime <= 0 && !isHeadSafe()) {
            buffGhostTime = 0.1;
        }
    }
}

void SnakeGame::applyItemEffect(ItemType type) {
    mciSendString(L"play click from 0", NULL, 0, NULL);
    switch (type) {
    case ItemType::SLOW:
        buffSlowTime = 10.0;
        {
            int currentBase = 150 - (score / 10) * 2;
            if (currentBase + permanentSlow <= 40) {
                permanentSlow += 30;
            }
        }
        break;

    case ItemType::DOUBLE: buffDoubleTime = 10.0; break;
    case ItemType::GHOST: buffGhostTime = 5.0; break;
    case ItemType::CUT: {
        if (snake.size() > 3) {
            int cutCount = (int)snake.size() / 2;
            Point cutPoint = snake.back();
            spawnParticles(cutPoint.x, cutPoint.y, GREEN, true);
            snake.resize(max(3, (int)snake.size() - cutCount));
        }
        break;
    }
    }
}

void SnakeGame::renderItems() {
    for (auto& it : items) {
        IMAGE* img = nullptr;
        switch (it.type) {
        case ItemType::SLOW: img = &imgSlow; break;
        case ItemType::CUT: img = &imgCut; break;
        case ItemType::DOUBLE: img = &imgDouble; break;
        case ItemType::GHOST: img = &imgGhost; break;
        }
        if (img) putimage_alpha(it.pos.x * UNIT, it.pos.y * UNIT, img);
    }
}

// 3. 游戏核心逻辑
void SnakeGame::initMap(int mapIdx) {
    walls.clear();
    if (mapIdx == 2) {
        for (int i = GRID_W / 4; i < GRID_W * 3 / 4; i++) walls.push_back({ i, GRID_H / 2 });
        for (int j = GRID_H / 4; j < GRID_H * 3 / 4; j++) walls.push_back({ GRID_W / 2, j });
    }
    else if (mapIdx == 3) {
        for (int i = 5; i < GRID_W - 5; i++) {
            walls.push_back({ i, 5 });
            walls.push_back({ i, GRID_H - 6 });
        }
        for (int k = 1; k <= 3; k++) {
            walls.push_back({ 5, 5 + k });
            walls.push_back({ GRID_W - 6, 5 + k });
            walls.push_back({ 5, GRID_H - 6 - k });
            walls.push_back({ GRID_W - 6, GRID_H - 6 - k });
        }
    }
}

void SnakeGame::resetGame() {
    score = 0;
    smallFoodCount = 0;
    permanentSlow = 0;
    isBigFood = false;
    newRecordAchieved = false;
    buffSlowTime = 0.0;
    buffDoubleTime = 0.0;
    buffGhostTime = 0.0;
    items.clear();
    initMap(currentMap);
    int startX = GRID_W / 2, startY = GRID_H / 2;
    snake.clear();
    snake.push_back({ startX, startY });
    snake.push_back({ startX - 1, startY });
    dir = nextDir = Direction::RIGHT;
    generateFood();
}

void SnakeGame::playGame() {
    DWORD lastUpdateTime = GetTickCount();
    nextDir = dir;
    while (state == GameState::PLAYING) {
        handleGameInput();
        if (isPaused) {
            Sleep(50);
            continue;
        }

        if (GetTickCount() - lastUpdateTime > getSpeed()) {
            updateGameLogic();
            lastUpdateTime = GetTickCount();
            dirLocked = false; // 更新逻辑后释放转向锁
        }
        spawnItem();
        updateItems();
        renderGame();
        updateParticles();
        Sleep(10);
    }
}

void SnakeGame::renderGame() {
    setbkcolor(RGB(20, 20, 25));
    cleardevice();

    // 绘制背景网格线
    setlinecolor(RGB(30, 30, 35));
    for (int i = 0; i <= WIN_WIDTH; i += UNIT) line(i, 0, i, WIN_HEIGHT);
    for (int i = 0; i <= WIN_HEIGHT; i += UNIT) line(0, i, WIN_WIDTH, i);

    setfillcolor(RGB(80, 80, 90));
    for (auto& w : walls) solidrectangle(w.x * UNIT, w.y * UNIT, (w.x + 1) * UNIT, (w.y + 1) * UNIT);

    if (isBigFood) {
        setfillcolor(LIGHTRED);
        solidcircle(food.x * UNIT + UNIT, food.y * UNIT + UNIT, UNIT);
        settextcolor(YELLOW);
        settextstyle(20, 0, L"Consolas");
        outtextxy(food.x * UNIT, food.y * UNIT - 20, (to_wstring(max(0, (int)ceil(bigFoodTime))) + L"s").c_str());
    }
    else {
        setfillcolor(LIGHTRED);
        solidcircle(food.x * UNIT + UNIT / 2, food.y * UNIT + UNIT / 2, UNIT / 2 - 2);
    }

    renderItems(); // 渲染特殊道具

    for (size_t i = 0; i < snake.size(); i++) {
        COLORREF bodyColor = (i == 0 ? LIGHTGREEN : GREEN);
        if (buffGhostTime > 0) {
            bodyColor = RGB(150, 255, 255);

            if (buffGhostTime < 2.0) {
                if ((int)(buffGhostTime * 8) % 2 == 0) { // *8 决定闪烁频率
                    bodyColor = WHITE; // 闪烁白色
                }
            }
        }// 默认幽灵色

        setfillcolor(bodyColor);
        fillroundrect(snake[i].x * UNIT + 1, snake[i].y * UNIT + 1, (snake[i].x + 1) * UNIT - 1, (snake[i].y + 1) * UNIT - 1, 5, 5);

        // 绘制蛇头眼睛
        if (i == 0) {
            setfillcolor(BLACK);
            int hx = snake[0].x * UNIT, hy = snake[0].y * UNIT;
            if (dir == Direction::UP) { solidcircle(hx + 6, hy + 6, 2); solidcircle(hx + 14, hy + 6, 2); }
            else if (dir == Direction::DOWN) { solidcircle(hx + 6, hy + 14, 2); solidcircle(hx + 14, hy + 14, 2); }
            else if (dir == Direction::LEFT) { solidcircle(hx + 6, hy + 6, 2); solidcircle(hx + 6, hy + 14, 2); }
            else if (dir == Direction::RIGHT) { solidcircle(hx + 14, hy + 6, 2); solidcircle(hx + 14, hy + 14, 2); }
        }
    }
    settextcolor(WHITE);
    settextstyle(18, 0, L"Consolas");
    wstring status = L"得分: " + to_wstring(score) + L" | 距奖励出现还差: " + to_wstring(max(0, 5 - smallFoodCount)) + L"个食物";
    outtextxy(20, 10, status.c_str());

    wstring hsText = L"最高分:" + to_wstring(highScores.empty() ? 0 : highScores[0]);
    outtextxy(WIN_WIDTH - 160, 10, hsText.c_str());

    // 绘制 Buff 状态提示
    int infoY = WIN_HEIGHT - 30;
    settextstyle(16, 0, L"微软雅黑");
    if (buffSlowTime > 0) { settextcolor(CYAN); outtextxy(20, infoY, (L"减速中: " + to_wstring((int)ceil(buffSlowTime)) + L"s").c_str()); infoY -= 20; }
    if (buffDoubleTime > 0) { settextcolor(YELLOW); outtextxy(20, infoY, (L"双倍得分: " + to_wstring((int)ceil(buffDoubleTime)) + L"s").c_str()); infoY -= 20; }
    if (buffGhostTime > 0) { settextcolor(RGB(150, 255, 255)); outtextxy(20, infoY, (L"幽灵模式: " + to_wstring((int)ceil(buffGhostTime)) + L"s").c_str()); }

    renderParticles();
    FlushBatchDraw();
}

void SnakeGame::renderParticles() {
    for (auto& p : particles) {
        setfillcolor(p.color);
        // 粒子随寿命变小
        int r = p.life / 5 + 1;
        solidcircle((int)p.x, (int)p.y, 2);
    }
}

void SnakeGame::updateGameLogic() {
    dir = nextDir; // 应用锁定的方向
    Point head = snake.front();
    if (dir == Direction::UP) head.y--;
    else if (dir == Direction::DOWN) head.y++;
    else if (dir == Direction::LEFT) head.x--;
    else if (dir == Direction::RIGHT) head.x++;

    // 穿墙逻辑 (幽灵模式强制支持穿墙)
    if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) {
        if (config.diff == Difficulty::SIMPLE || buffGhostTime > 0) {
            head.x = (head.x + GRID_W) % GRID_W;
            head.y = (head.y + GRID_H) % GRID_H;
        }
        else { onGameOver(); return; }
    }

    // 碰撞检查 (幽灵模式跳过自身与墙壁碰撞)
    if (buffGhostTime <= 0) {
        for (auto& w : walls) if (head == w) { onGameOver(); return; }
        for (auto& s : snake) if (head == s) { onGameOver(); return; }
    }

    // 检查道具碰撞
    for (auto it = items.begin(); it != items.end(); ) {
        if (head == it->pos) {
            applyItemEffect(it->type);
            it = items.erase(it);
        }
        else ++it;
    }

    snake.insert(snake.begin(), head);
    bool ateFood = false;
    if (isBigFood) {
        if (head.x >= food.x && head.x <= food.x + 1 && head.y >= food.y && head.y <= food.y + 1) {
            ateFood = true;
            spawnParticles(head.x, head.y, LIGHTRED, true);
        }
        bigFoodTime -= 0.1;
        if (bigFoodTime <= 0) generateFood();
    }
    else {
        if (head == food) {
            ateFood = true;
            spawnParticles(head.x, head.y, LIGHTRED, false);
        }
    }

    if (ateFood) {
        int gain = 10;
        if (isBigFood) {
            int t = (int)ceil(bigFoodTime);
            gain = (t >= 6) ? 100 : t * 10;
        }
        else { smallFoodCount++; }

        if (buffDoubleTime > 0) gain *= 2; // 双倍得分 Buff
        score += gain;
        generateFood();
        mciSendString(L"play eat from 0", NULL, 0, NULL);
    }
    else {
        snake.pop_back();
    }
}

void SnakeGame::updateParticles() {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->x += it->vx;
        it->y += it->vy;
        it->life--;
        if (it->life <= 0) it = particles.erase(it);
        else ++it;
    }
}

void SnakeGame::spawnParticles(int x, int y, COLORREF color, bool isBig) {
    int count = isBig ? 50 : 10;

    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = x * UNIT + UNIT / 2;
        p.y = y * UNIT + UNIT / 2;

        // 随机速度和方向
        double angle = (rand() % 360) * 3.14159 / 180.0;

        double baseSpeed = isBig ? 2.0 : 1.0;
        double speed = (rand() % 50) / 10.0 + baseSpeed;
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;

        p.life = rand() % 20 + (isBig ? 40 : 20);

        if (isBig && rand() % 3 == 0) {
            p.color = YELLOW;
        }
        else p.color = color;

        particles.push_back(p);
    }
}

void SnakeGame::loadConfig() {
    ifstream f(CONFIG_FILE, ios::binary);
    if (f) {
        f.read((char*)&config, sizeof(GameConfig));
        f.close();
    }
}

void SnakeGame::saveConfig() {
    ofstream f(CONFIG_FILE, ios::binary);
    if (f) {
        f.write((char*)&config, sizeof(GameConfig));
        f.close();
    }
}

void SnakeGame::loadHighScore() {
    highScores.assign(5, 0);
    ifstream f(HIGHSCORE_FILE, ios::binary);
    if (f) {
        for (int i = 0; i < 5; i++) {
            if (!f.read((char*)&highScores[i], sizeof(int))) {
                break;
            }
        }
        f.close();
        sort(highScores.rbegin(), highScores.rend());
    }
}

void SnakeGame::saveHighScore() {
    ofstream f(HIGHSCORE_FILE, ios::binary);
    for (int s : highScores) {
        f.write((char*)&s, sizeof(int));
    }
    f.close();
}

void SnakeGame::generateFood() {
    if (smallFoodCount >= 5) {
        isBigFood = true;
        smallFoodCount = 0;
        bigFoodTime = 6.0;
        do {
            food.x = rand() % (GRID_W - 1);
            food.y = rand() % (GRID_H - 1);
        } while (isOccupied(food) || isOccupied({ food.x + 1, food.y }) || isOccupied({ food.x, food.y + 1 }) || isOccupied({ food.x + 1, food.y + 1 }));
    }
    else {
        isBigFood = false;
        do {
            food.x = rand() % GRID_W;
            food.y = rand() % GRID_H;
        } while (isOccupied(food));
    }
}

bool SnakeGame::isHeadSafe() {
    Point head = snake.front();

    // 检查是否在墙
    for (auto& w : walls) if (head == w) return false;
    // 检查是否撞到自己
    for (size_t i = 1; i < snake.size(); i++) {
        if (head == snake[i]) return false;
    }

    return true;
}

bool SnakeGame::isOccupied(Point p) {
    for (auto& s : snake) if (s == p) return true;
    for (auto& w : walls) if (w == p) return true;
    return false;
}

int SnakeGame::getSpeed() {
    // 基础速度计算
    int rawSpeed = 150 - (score / 10) * 2;

    // 加上永久减速效果
    int finalSpeed = rawSpeed + permanentSlow;

    // 确保不超过初始速度 150 (越慢数值越大)
    if (finalSpeed > 150) finalSpeed = 150;

    // 加上临时减速 Buff
    if (buffSlowTime > 0) finalSpeed += 100;

    // 确保不低于最高速度限制 40 (越快数值越小)
    return max(40, finalSpeed);
}

void SnakeGame::handleGameInput() {
    ExMessage msg;
    while (peekmessage(&msg, EX_KEY)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.vkcode == VK_SPACE) {
                isPaused = !isPaused;
                // 绘制暂停文字
                if (isPaused) {
                    settextcolor(WHITE);
                    settextstyle(40, 0, L"微软雅黑", 0, 0, FW_BOLD, false, false, false);
                    outtextxy(WIN_WIDTH / 2 - 80, WIN_HEIGHT / 2 - 30, L"游 戏 暂 停");
                    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT / 2 + 10, L"按 [SPACE] 恢 复");
                    FlushBatchDraw();
                }
            }
            if (msg.vkcode == VK_ESCAPE) { saveGame(); state = GameState::MAIN_MENU; return; }

            if (!isPaused) {
                if (dirLocked) continue; // 本帧已转向则锁死输入
                if ((msg.vkcode == 'W' || msg.vkcode == VK_UP) && dir != Direction::DOWN) { nextDir = Direction::UP; dirLocked = true; }
                else if ((msg.vkcode == 'S' || msg.vkcode == VK_DOWN) && dir != Direction::UP) { nextDir = Direction::DOWN; dirLocked = true; }
                else if ((msg.vkcode == 'A' || msg.vkcode == VK_LEFT) && dir != Direction::RIGHT) { nextDir = Direction::LEFT; dirLocked = true; }
                else if ((msg.vkcode == 'D' || msg.vkcode == VK_RIGHT) && dir != Direction::LEFT) { nextDir = Direction::RIGHT; dirLocked = true; }
            }
        }
    }
}

void SnakeGame::onGameOver() {
    state = GameState::GAMEOVER;
    loadHighScore();

    // 检查是否打破第一名
    if (score > highScores[0]) {
        newRecordAchieved = true;
        mciSendString(L"play victory from 0", NULL, 0, NULL);
    }
    // 插入当前分数并重新排序
    highScores.push_back(score);
    sort(highScores.rbegin(), highScores.rend());

    // 保留前五名
    if (highScores.size() > (size_t)5) highScores.resize(5);
    saveHighScore();
}

void SnakeGame::showGameOver() {
    drawUIFrame(L"游 戏 结 束");
    settextcolor(WHITE);
    settextstyle(20, 0, L"微软雅黑");
    outtextxy(WIN_WIDTH / 2 - 60, 150, (L"最终得分: " + to_wstring(score)).c_str());

    if (newRecordAchieved) {
        settextcolor(YELLOW);
        settextstyle(25, 0, L"微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 110, 200, L"恭喜！打破最高分纪录！");
    }
    else {
        settextcolor(LIGHTGRAY);
        settextstyle(20, 0, L"微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 80, 200, (L"历史最高: " + to_wstring(highScores.empty() ? 0 : highScores[0])).c_str());
    }

    outtextxy(WIN_WIDTH / 2 - 110, 250, L"按 [Enter] 返回主菜单");
    FlushBatchDraw();
    ExMessage msg;
    while (peekmessage(&msg, EX_KEY)) {
        if (msg.message == WM_KEYDOWN && msg.vkcode == VK_RETURN) { state = GameState::MAIN_MENU; menuCursor = 0; }
    }
}