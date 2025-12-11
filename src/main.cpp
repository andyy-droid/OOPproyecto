#include "Game.h"

int main() {
    Game game(800, 600, "SHOOTING DUCKS - Prototype");
    if (!game.init()) return -1;
    game.run();
    return 0;
}
