#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <memory>
#include <string>
#include "Duck.h"

class Game {
public:
    Game(unsigned int width = 800, unsigned int height = 600, const std::string& title = "SHOOTING DUCKS");
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

    // Show game over screen
    void ShowGameOver();
    // Show instructions before starting the round
    void ShowInstructions(float seconds = 7.f);

    // Spawn helper
    void spawnDuck();

    sf::RenderWindow window_;
    unsigned int width_;
    unsigned int height_;
    std::string title_;

    // Game state
    int score_ = 0;
    int playerLives_ = 3;
    bool gameOver_ = false;
    std::vector<std::unique_ptr<Duck>> ducks_;

    // Resources
    sf::Font font_;
    bool fontLoaded_ = false;
    std::unique_ptr<sf::Text> scoreText_;
    std::unique_ptr<sf::Text> livesText_;
    std::unique_ptr<sf::Text> instructionsText_;
    std::unique_ptr<sf::Text> titleText_;
    std::unique_ptr<sf::Text> loadingText_;
    sf::Music duckMusic;
    // Background pond image for instruction screen
    sf::Texture pondTexture_;
    sf::Sprite pondSprite_;
    bool pondLoaded_ = false;

    // Timing
    sf::Clock clock_;
    float spawnTimer_ = 0.f;
    float spawnInterval_ = 2.5f; // seconds

    bool running_ = false;
};

#endif // GAME_H
