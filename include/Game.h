#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include "Duck.h"

class Game {
public:
    Game(unsigned int width = 800, unsigned int height = 600, const std::string& title = "Duck Hunt");
    ~Game();

    // Initialize resources. Returns false if initialization fails.
    bool init();

    // Run the main loop
    void run();

private:
    // Input, update, render
    void handleInput();
    void update(float dt);
    void render();

    // Spawn helper
    void spawnDuck();

    sf::RenderWindow window_;
    unsigned int width_;
    unsigned int height_;
    std::string title_;

    // Game state
    int score_ = 0;
    int ammo_ = 3;
    std::vector<std::unique_ptr<Duck>> ducks_;

    // Resources
    sf::Font font_;
    std::unique_ptr<sf::Text> scoreText_;
    std::unique_ptr<sf::Text> ammoText_;

    // Timing
    sf::Clock clock_;
    float spawnTimer_ = 0.f;
    float spawnInterval_ = 2.5f; // seconds

    bool running_ = false;
};

#endif // GAME_H
