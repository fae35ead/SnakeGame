#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#pragma comment(lib, "winmm.lib")

#include "SnakeGame.h"

int main() {
    SnakeGame game;
    game.run();
    return 0;
}