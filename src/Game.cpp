#include "Game.h"

#include <SFML/Window.hpp>
#include <algorithm>
#include <random>
#include <iostream>
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
    window_.create(sf::VideoMode(sf::Vector2u(width_, height_)), title_);
    window_.setFramerateLimit(60);
}

Game::~Game() {
    if (window_.isOpen()) window_.close();
}

bool Game::init() {
    // Load font (fallback to built-in if missing)
    if (!font_.openFromFile("./assets/fonts/Minecraft.ttf")) {
        std::cerr << "Warning: failed to open font './assets/fonts/Minecraft.ttf'\n";
        // We continue but text might not render as intended.
    }

    scoreText_.reset(new sf::Text(font_, std::string("Score: 0"), 24));
    scoreText_->setFillColor(sf::Color::White);
    scoreText_->setPosition({10.f, 10.f});

    ammoText_.reset(new sf::Text(font_, std::string("Ammo: 3"), 24));
    ammoText_->setFillColor(sf::Color::White);
    ammoText_->setPosition({10.f, 40.f});

    // Spawn a couple of ducks to start
    for (int i = 0; i < 2; ++i) spawnDuck();

    running_ = true;
    clock_.restart();
    return true;
}

void Game::run() {
    if (!running_) init();

    while (window_.isOpen()) {
        float dt = clock_.restart().asSeconds();
        handleInput();
        update(dt);
        render();
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

    // Consume any SFML events to keep the window responsive
    while (true) {
        auto evOpt = window_.pollEvent();
        if (!evOpt) break;
        // we don't inspect the variant here; native pump handles WM_CLOSE
    }

    // Close on Escape
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
        window_.close();
        return;
    }

    static bool lastMousePressed = false;
    bool mousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    if (mousePressed && !lastMousePressed) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window_);
        sf::Vector2f worldPos = window_.mapPixelToCoords(pixelPos);

        // Reduce ammo per click
        if (ammo_ > 0) ammo_--; else ammo_ = 0;

        // Check ducks for hit
        for (auto& dptr : ducks_) {
            if (!dptr) continue;
            if (!dptr->isAlive()) continue;
            if (dptr->isFalling()) continue;

            if (dptr->getBounds().contains(worldPos)) {
                dptr->onShot();
                score_ += 100; // simple score rule
                break; // only one duck per click
            }
        }
    }
    lastMousePressed = mousePressed;
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
    if (ammoText_) ammoText_->setString(std::string("Ammo: ") + std::to_string(ammo_));
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
    if (ammoText_) window_.draw(*ammoText_);

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
