#include "Game.h"

#include <SFML/Window.hpp>
#include <algorithm>
#include <random>
#include <iostream>
#include <SFML/Audio.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

static float randRange(float a, float b) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(a, b);
    return dist(gen);
}

Game::Game(unsigned int width, unsigned int height, const std::string& title)
    : width_(width), height_(height), title_(title) {
    window_.create(sf::VideoMode(width_, height_), title_);
    window_.setFramerateLimit(60);
}

Game::~Game() {
    if (window_.isOpen()) window_.close();
}

bool Game::init() {
    // Load font (fallback to built-in if missing)
    if (!font_.loadFromFile("./assets/fonts/Minecraft.ttf")) {
        std::cerr << "Warning: failed to open font './assets/fonts/Minecraft.ttf'\n";
        fontLoaded_ = false;
    } else {
        fontLoaded_ = true;
    }

    scoreText_.reset(new sf::Text(std::string("Score: 0"), font_, 24));
    scoreText_->setFillColor(sf::Color::White);
    scoreText_->setPosition({10.f, 10.f});

    livesText_.reset(new sf::Text(std::string("Lives: 3"), font_, 24));
    livesText_->setFillColor(sf::Color::White);
    livesText_->setPosition({10.f, 40.f});
    // prepare instructions text (shown before ducks spawn)
    if (fontLoaded_) {
        std::string instr = "INSTRUCCIONES:\n"
            "1. Posicionar el cursor sobre un pato y dar clic izquierdo para disparar.\n"
            "2. Se cuentan con 3 vidas en total.\n"
            "3. Se pierde una vida cuando se dispara al aire.\n";
        instructionsText_.reset(new sf::Text(instr, font_, 20));
        instructionsText_->setFillColor(sf::Color::White);
        sf::FloatRect b = instructionsText_->getLocalBounds();
        instructionsText_->setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        instructionsText_->setPosition(window_.getSize().x / 2.f, window_.getSize().y / 2.f);
    }

    // try to load pond background for instruction screen
    if (pondTexture_.loadFromFile("./assets/images/duck_pond.png")) {
        pondSprite_.setTexture(pondTexture_);
        // scale to window size
        auto tsize = pondTexture_.getSize();
        if (tsize.x > 0 && tsize.y > 0) {
            float sx = static_cast<float>(width_) / static_cast<float>(tsize.x);
            float sy = static_cast<float>(height_) / static_cast<float>(tsize.y);
            pondSprite_.setScale(sx, sy);
        }
        pondLoaded_ = true;
    } else {
        pondLoaded_ = false;
        std::cerr << "Warning: could not load './assets/images/duck_pond.png'\n";
    }

    // Show instructions first (blocks input except window close)
    ShowInstructions(10.f);

    // Spawn a couple of ducks to start (after instructions)
    for (int i = 0; i < 2; ++i) spawnDuck();

    // Load and play duck background music (best-effort). File: assets/music/duck.mp3
    if (duckMusic.openFromFile("./assets/music/duck.mp3")) {
        duckMusic.setLoop(true);
        duckMusic.setVolume(60.f);
        duckMusic.play();
    } else {
        std::cerr << "Warning: could not open music './assets/music/duck.mp3'\n";
    }

    running_ = true;
    clock_.restart();
    return true;
}

void Game::run() {
    if (!running_) init();
    while (window_.isOpen() && !gameOver_) {
        float dt = clock_.restart().asSeconds();
        handleInput();
        update(dt);
        render();
    }

    // If the game ended because lives reached 0, show GAME OVER screen
    if (gameOver_) {
        ShowGameOver();
    }
}

void Game::handleInput() {
    // Consume window events to keep OS/windowing system responsive
    // and handle the Close event so the user can click the X button.
    // On Windows, pump native messages (thread message queue) to detect WM_CLOSE
    #ifdef _WIN32
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_CLOSE) {
                window_.close();
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    #endif

    // Poll SFML events and handle input
    sf::Event event;
    // if game over, ignore additional input
    if (gameOver_) return;

    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window_.close();
            return;
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                window_.close();
                return;
            }
        }
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                sf::Vector2f worldPos = window_.mapPixelToCoords(pixelPos);

                // Check ducks for hit
                bool anyHit = false;
                for (auto& dptr : ducks_) {
                    if (!dptr) continue;
                    if (!dptr->isAlive()) continue;
                    if (dptr->isFalling()) continue;

                    if (dptr->getBounds().contains(worldPos)) {
                        dptr->onShot();
                        score_ += 100; // simple score rule
                        anyHit = true;
                        break; // only one duck per click
                    }
                }

                if (!anyHit) {
                    playerLives_ -= 1;
                    if (playerLives_ <= 0) {
                        playerLives_ = 0;
                        gameOver_ = true;
                        // stop music optionally
                        if (duckMusic.getStatus() == sf::Music::Playing) duckMusic.stop();
                    }
                }
            }
        }
    }
}

void Game::update(float dt) {
    // Spawn control
    spawnTimer_ += dt;
    if (spawnTimer_ >= spawnInterval_) {
        spawnTimer_ = 0.f;
        spawnDuck();
    }

    // Update ducks
    for (auto& d : ducks_) {
        if (d && d->isAlive()) d->update(dt);
    }

    // Remove not-alive ducks
    ducks_.erase(std::remove_if(ducks_.begin(), ducks_.end(), [](const std::unique_ptr<Duck>& d) {
        return !d || !d->isAlive();
    }), ducks_.end());

    // Update HUD texts
    if (scoreText_) scoreText_->setString(std::string("Score: ") + std::to_string(score_));
    //if (ammoText_) ammoText_->setString(std::string("Ammo: ") + std::to_string(ammo_));
    if (livesText_) livesText_->setString(std::string("Lives: ") + std::to_string(playerLives_));
}

void Game::render() {
    // Simple background (sky + grass)
    window_.clear(sf::Color(135, 206, 235)); // sky blue

    // grass rectangle
    sf::RectangleShape grass(sf::Vector2f(static_cast<float>(width_), 120.f));
    grass.setFillColor(sf::Color(80, 180, 70));
    grass.setPosition({0.f, static_cast<float>(height_) - 120.f});
    window_.draw(grass);

    // Draw ducks
    for (auto& d : ducks_) if (d) d->draw(window_);

    // Draw HUD
    if (scoreText_) window_.draw(*scoreText_);
    //if (ammoText_) window_.draw(*ammoText_);
    if (livesText_) window_.draw(*livesText_);

    window_.display();
}

void Game::spawnDuck() {
    // spawn at left or right edge, random Y
    float y = randRange(80.f, static_cast<float>(height_) - 200.f);
    float x;
    if (randRange(0.f, 1.f) < 0.5f) {
        x = -60.f; // start left
    } else {
        x = static_cast<float>(width_) + 60.f; // start right
    }

    ducks_.push_back(std::make_unique<Duck>(sf::Vector2f(x, y), window_.getSize()));
}

// After the main loop, if the player lost all lives show GAME OVER
// This renders a full-screen message for 2 seconds.
void Game::ShowGameOver() {
    if (!fontLoaded_) {
        window_.clear(sf::Color::Black);
        window_.display();
        sf::sleep(sf::seconds(2.f));
        return;
    }

    sf::Text goText("GAME OVER", font_, 72);
    goText.setFillColor(sf::Color::Red);
    auto bounds = goText.getLocalBounds();
    goText.setOrigin(bounds.width/2.f, bounds.height/2.f);
    goText.setPosition(static_cast<float>(width_)/2.f, static_cast<float>(height_)/2.f - 20.f);

    window_.clear(sf::Color::Black);
    window_.draw(goText);
    window_.display();
    sf::sleep(sf::seconds(3.f));

}

void Game::ShowInstructions(float seconds) {
    if (!fontLoaded_) {
        // keep window responsive to close events, but otherwise wait
        sf::Clock c;
        while (c.getElapsedTime().asSeconds() < seconds) {
            sf::Event e;
            while (window_.pollEvent(e)) {
                if (e.type == sf::Event::Closed) {
                    window_.close();
                    return;
                }
            }
            sf::sleep(sf::milliseconds(50));
        }
        return;
    }

    sf::Clock timer;
    while (timer.getElapsedTime().asSeconds() < seconds) {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
                return;
            }
            // ignore other inputs while instructions are shown
        }

        window_.clear(sf::Color::Black);
        if (pondLoaded_) window_.draw(pondSprite_);
        if (instructionsText_) window_.draw(*instructionsText_);
        window_.display();

        sf::sleep(sf::milliseconds(16));
    }
}

