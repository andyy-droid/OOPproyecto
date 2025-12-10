#include "Game.h"

int main() {
    Game game(800, 600, "Duck Hunt - Prototype");
    if (!game.init()) return -1;
    game.run();
    return 0;
}
