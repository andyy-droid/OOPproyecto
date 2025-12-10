#ifndef DUCK_H
#define DUCK_H

#include <SFML/Graphics.hpp>
#include <string>

class Duck {
public:
    // Constructor: position (start), windowSize for bounds check, optional texture path
    Duck(const sf::Vector2f& startPos, const sf::Vector2u& windowSize, const std::string& texturePath = "assets/images/duck.png");
    ~Duck();

    // Update duck logic (dt seconds elapsed)
    void update(float dt);

    // Draw the duck to the provided window
    void draw(sf::RenderWindow& window) const;

    // Return global bounding box (for hit tests)
    sf::FloatRect getBounds() const;

    // Mark as hit / shot (starts falling)
    void onShot();

    // Accessors
    bool isAlive() const { return isAlive_; }
    bool isFalling() const { return isFalling_; }
    sf::Vector2f getPosition() const { return sprite_ ? sprite_->getPosition() : sf::Vector2f(0.f,0.f); }

private:
    // Visual
    sf::Texture texture_;
    std::unique_ptr<sf::Sprite> sprite_;
    sf::RectangleShape placeholder_;
    bool hasTexture_ = false;

    // Movement
    float vx_; // horizontal velocity (px/s)
    float vy_; // vertical velocity (px/s), used when falling
    float baseY_; // baseline Y used for sinusoidal flight
    float time_ = 0.f; // elapsed time for sine motion
    float amplitude_; // vertical oscillation amplitude
    float frequency_; // vertical oscillation frequency

    // State
    bool isAlive_ = true;
    bool isFalling_ = false;

    // Bounds
    sf::Vector2u windowSize_;

    // Helpers
    void ensureTextureLoaded(const std::string& path);
};

#endif // DUCK_H
