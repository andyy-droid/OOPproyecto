#include "../include/Duck.h"

#include <random>
#include <cmath>

static float randRange(float a, float b) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(a, b);
    return dist(gen);
}

Duck::Duck(const sf::Vector2f& startPos, const sf::Vector2u& windowSize, const std::string& texturePath)
    : vx_(0.f), vy_(0.f), baseY_(startPos.y), amplitude_(20.f), frequency_(2.f), windowSize_(windowSize)
{
    ensureTextureLoaded(texturePath);

    if (hasTexture_ && sprite_) sprite_->setPosition(startPos);
    else placeholder_.setPosition(startPos);

    // Randomize horizontal speed
    vx_ = randRange(80.f, 160.f);
    if (randRange(0.f, 1.f) < 0.5f) vx_ = -vx_;

    amplitude_ = randRange(10.f, 40.f);
    frequency_ = randRange(1.0f, 3.0f);

    // Initial flip if going left (apply to sprite scale or placeholder)
    if (vx_ < 0.f) {
        if (hasTexture_ && sprite_) {
            auto sc = sprite_->getScale();
            sprite_->setScale({-std::abs(sc.x), sc.y});
        } else {
            auto sc = placeholder_.getScale();
            placeholder_.setScale({-std::abs(sc.x), sc.y});
        }
    }
}

Duck::~Duck() {}

void Duck::ensureTextureLoaded(const std::string& path) {
    sf::Image image;
    bool loaded = image.loadFromFile(path);

    if (!loaded) {
        // Create a simple transparent placeholder image using RenderTexture
        const unsigned int W = 64, H = 48;
        sf::RenderTexture rt;
        rt.create(sf::Vector2u(W, H));
        rt.clear(sf::Color::Transparent);

        sf::CircleShape body(14.f);
        body.setFillColor(sf::Color(80, 160, 40));
        body.setPosition({10.f, 12.f});

        sf::CircleShape head(8.f);
        head.setFillColor(sf::Color(80, 160, 40));
        head.setPosition({34.f, 8.f});

        sf::ConvexShape beak;
        beak.setPointCount(3);
        beak.setPoint(0, sf::Vector2f(52.f, 16.f));
        beak.setPoint(1, sf::Vector2f(62.f, 12.f));
        beak.setPoint(2, sf::Vector2f(62.f, 20.f));
        beak.setFillColor(sf::Color(230, 180, 40));

        rt.draw(body);
        rt.draw(head);
        rt.draw(beak);
        rt.display();

        image = rt.getTexture().copyToImage();
        // try to save placeholder for future runs
        image.saveToFile(path);
        loaded = true;
    }

    if (loaded) {
        // Improved color-key: remove white halos
        unsigned int w = image.getSize().x;
        unsigned int h = image.getSize().y;
        const float KEY_HIGH = 250.f;
        const float KEY_LOW  = 200.f;

        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                sf::Vector2u coords(x, y);
                auto c = image.getPixel(coords);
                float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
                if (lum >= KEY_HIGH) {
                    c.a = 0;
                    image.setPixel(coords, c);
                } else if (lum > KEY_LOW) {
                    float t = (KEY_HIGH - lum) / (KEY_HIGH - KEY_LOW);
                    if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
                    unsigned int newA = static_cast<unsigned int>(c.a * t + 0.5f);
                    c.a = static_cast<uint8_t>(newA);
                    image.setPixel(coords, c);
                }
            }
        }

        // Flip horizontally (mirror X)
        sf::Image flipped;
        flipped.create(sf::Vector2u(w, h), sf::Color::Transparent);
        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                sf::Vector2u src(x, y);
                sf::Color px = image.getPixel(src);
                sf::Vector2u dst(w - 1 - x, y);
                flipped.setPixel(dst, px);
            }
        }

        texture_.loadFromImage(flipped);
        texture_.setSmooth(true);
        sprite_.reset(new sf::Sprite(texture_));
        auto b = sprite_->getLocalBounds();
        sprite_->setOrigin({b.size.x / 2.f, b.size.y / 2.f});

        const float desiredHeight = 32.f;
        if (b.size.y > 0.f) {
            float scale = desiredHeight / b.size.y;
            sprite_->setScale({scale, scale});
        }

        hasTexture_ = true;
    } else {
        hasTexture_ = false;
        const float phW = 42.f, phH = 32.f;
        placeholder_.setSize({phW, phH});
        placeholder_.setFillColor(sf::Color(200, 180, 0));
        placeholder_.setOrigin({placeholder_.getSize().x / 2.f, placeholder_.getSize().y / 2.f});
    }
}

void Duck::update(float dt) {
    if (!isAlive_) return;

    if (isFalling_) {
        const float gravity = 800.f;
        vy_ += gravity * dt;
        if (hasTexture_ && sprite_) {
            sprite_->move({vx_ * dt, vy_ * dt});
            sprite_->rotate(sf::degrees(200.f * dt));
        } else {
            placeholder_.move({vx_ * dt, vy_ * dt});
        }

        if ((hasTexture_ && sprite_ && sprite_->getPosition().y > static_cast<float>(windowSize_.y) + 64.f)
            || (!hasTexture_ && placeholder_.getPosition().y > static_cast<float>(windowSize_.y) + 64.f)) {
            isAlive_ = false;
        }
        return;
    }

    time_ += dt;
    float newX = (hasTexture_ && sprite_) ? sprite_->getPosition().x + vx_ * dt : placeholder_.getPosition().x + vx_ * dt;
    float newY = baseY_ + amplitude_ * std::sin(frequency_ * time_);

    if (hasTexture_) { if (sprite_) sprite_->setPosition({newX, newY}); }
    else placeholder_.setPosition({newX, newY});

    auto bounds = hasTexture_ ? (sprite_ ? sprite_->getGlobalBounds() : sf::FloatRect()) : placeholder_.getGlobalBounds();
    if (bounds.position.x + bounds.size.x < 0.f) {
        if (hasTexture_ && sprite_) sprite_->setPosition({bounds.size.x / 2.f, newY});
        else placeholder_.setPosition({bounds.size.x / 2.f, newY});
        vx_ = std::abs(vx_);
        if (hasTexture_ && sprite_) { auto sc = sprite_->getScale(); sprite_->setScale({std::abs(sc.x), sc.y}); }
    } else if (bounds.position.x > static_cast<float>(windowSize_.x)) {
        if (hasTexture_ && sprite_) sprite_->setPosition({static_cast<float>(windowSize_.x) - bounds.size.x / 2.f, newY});
        else placeholder_.setPosition({static_cast<float>(windowSize_.x) - bounds.size.x / 2.f, newY});
        vx_ = -std::abs(vx_);
        if (hasTexture_ && sprite_) { auto sc = sprite_->getScale(); sprite_->setScale({-std::abs(sc.x), sc.y}); }
    }
}

void Duck::draw(sf::RenderWindow& window) const {
    if (!isAlive_) return;
    if (hasTexture_) { if (sprite_) window.draw(*sprite_); }
    else window.draw(placeholder_);
}

sf::FloatRect Duck::getBounds() const {
    return hasTexture_ ? (sprite_ ? sprite_->getGlobalBounds() : sf::FloatRect()) : placeholder_.getGlobalBounds();
}

void Duck::onShot() {
    if (!isAlive_ || isFalling_) return;
    isFalling_ = true;
    vy_ = -200.f;
    vx_ *= 0.25f;
}
#include "../include/Duck.h"

#include <random>
#include <cmath>
#include <iostream>

static float randRange(float a, float b) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(a, b);
    return dist(gen);
}

Duck::Duck(const sf::Vector2f& startPos, const sf::Vector2u& windowSize, const std::string& texturePath)
    : vx_(0.f), vy_(0.f), baseY_(startPos.y), amplitude_(20.f), frequency_(2.f), windowSize_(windowSize)
{
    ensureTextureLoaded(texturePath);

    if (hasTexture_) sprite_->setPosition(startPos);
    else placeholder_.setPosition(startPos);

    // Randomize horizontal speed (avoid very small values)
    vx_ = randRange(80.f, 160.f);
    if (randRange(0.f, 1.f) < 0.5f) vx_ = -vx_; // may go left or right

    // Randomize sine amplitude/frequency a bit
    amplitude_ = randRange(10.f, 40.f);
    frequency_ = randRange(1.0f, 3.0f);

    // Flip horizontally if going left (apply to sprite or placeholder)
    sf::Image image;
    bool loaded = image.loadFromFile(path);

    if (!loaded) {
        // create assets/images directory if missing by attempting save later
        // Generate a simple transparent placeholder using RenderTexture
        const unsigned int W = 64, H = 48;
        sf::RenderTexture rt;
        rt.create(W, H);
        rt.clear(sf::Color::Transparent);

        // Draw a simple duck: body + head + beak
        sf::CircleShape body(14.f);
        body.setFillColor(sf::Color(80, 160, 40));
        body.setPosition(10.f, 12.f);

        sf::CircleShape head(8.f);
        head.setFillColor(sf::Color(80, 160, 40));
        head.setPosition(34.f, 8.f);

        sf::ConvexShape beak;
        beak.setPointCount(3);
        beak.setPoint(0, sf::Vector2f(52.f, 16.f));
        beak.setPoint(1, sf::Vector2f(62.f, 12.f));
        beak.setPoint(2, sf::Vector2f(62.f, 20.f));
        beak.setFillColor(sf::Color(230, 180, 40));

        rt.draw(body);
        rt.draw(head);
        rt.draw(beak);
        rt.display();

        image = rt.getTexture().copyToImage();

        // Save generated placeholder so future runs load the PNG directly
        image.saveToFile(path);
        loaded = true;
    }

    if (loaded) {
        // Apply improved color-key and then flip image horizontally
        unsigned int w = image.getSize().x;
        unsigned int h = image.getSize().y;

        const float KEY_HIGH = 250.f; // above this -> fully transparent
        const float KEY_LOW  = 200.f; // below this -> keep original

        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                sf::Vector2u coords(x, y);
                auto c = image.getPixel(coords);
                float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
                if (lum >= KEY_HIGH) {
                    c.a = 0;
                    image.setPixel(coords, c);
                } else if (lum > KEY_LOW) {
                    float t = (KEY_HIGH - lum) / (KEY_HIGH - KEY_LOW);
                    if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
                    unsigned int newA = static_cast<unsigned int>(c.a * t + 0.5f);
                    c.a = static_cast<uint8_t>(newA);
                    image.setPixel(coords, c);
                }
            }
        }

        // Flip horizontally (mirror X)
        sf::Image flipped;
        flipped.create(w, h, sf::Color::Transparent);
        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                sf::Vector2u src(x, y);
                sf::Color px = image.getPixel(src);
                sf::Vector2u dst(w - 1 - x, y);
                flipped.setPixel(dst, px);
            }
        }

        // load texture from flipped image
        texture_.loadFromImage(flipped);
        texture_.setSmooth(true);

        // Construct sprite now that texture exists
        sprite_.reset(new sf::Sprite(texture_));
        // center origin for easier rotation/positioning
        auto b = sprite_->getLocalBounds();
        sprite_->setOrigin({b.size.x / 2.f, b.size.y / 2.f});

        // Scale down so ducks appear smaller on screen
        const float desiredHeight = 32.f; // pixels
        if (b.size.y > 0.f) {
            float scale = desiredHeight / b.size.y;
            sprite_->setScale({scale, scale});
        }

        hasTexture_ = true;
    } else {
        // fallback placeholder (shouldn't reach here because we generate it above)
        hasTexture_ = false;
        const float phW = 42.f, phH = 32.f; // smaller placeholder
        placeholder_.setSize({phW, phH});
        placeholder_.setFillColor(sf::Color(200, 180, 0));
        placeholder_.setOrigin({placeholder_.getSize().x / 2.f, placeholder_.getSize().y / 2.f});
    }
        }

        // load texture from modified image
        texture_.loadFromImage(image);
        texture_.setSmooth(true);
        // Construct sprite now that texture exists
        sprite_.reset(new sf::Sprite(texture_));
        // center origin for easier rotation/positioning
        auto b = sprite_->getLocalBounds();
        sprite_->setOrigin({b.size.x / 2.f, b.size.y / 2.f});

        // Scale down so ducks appear smaller on screen
        const float desiredHeight = 32.f; // pixels
        if (b.size.y > 0.f) {
            float scale = desiredHeight / b.size.y;
            sprite_->setScale({scale, scale});
        }

        hasTexture_ = true;
    } else {
        // No texture available: use a placeholder rectangle (smaller)
        hasTexture_ = false;
        const float phW = 42.f, phH = 32.f; // smaller placeholder
        placeholder_.setSize({phW, phH});
        placeholder_.setFillColor(sf::Color(200, 180, 0));
        placeholder_.setOrigin({placeholder_.getSize().x / 2.f, placeholder_.getSize().y / 2.f});
    }
}

void Duck::update(float dt) {
    if (!isAlive_) return;

    if (isFalling_) {
        // When falling, apply gravity and limit horizontal velocity
        const float gravity = 800.f; // px/s^2

        vy_ += gravity * dt;
            if (hasTexture_) {
                if (sprite_) sprite_->move({vx_ * dt, vy_ * dt});
                if (sprite_) sprite_->rotate(sf::degrees(200.f * dt));
            } else {
                placeholder_.move({vx_ * dt, vy_ * dt});
            }

        // If passed bottom, mark dead (remove later by game)
            if ((hasTexture_ && sprite_ && sprite_->getPosition().y > static_cast<float>(windowSize_.y) + 64.f)
                || (!hasTexture_ && placeholder_.getPosition().y > static_cast<float>(windowSize_.y) + 64.f)) {
            isAlive_ = false;
        }
        return;
    }

    // Flying: horizontal linear movement + vertical sinusoidal motion
    time_ += dt;

    float newX = (sprite_ ? sprite_->getPosition().x : 0.f) + vx_ * dt;
    float newY = baseY_ + amplitude_ * std::sin(frequency_ * time_);

    if (hasTexture_) { if (sprite_) sprite_->setPosition({newX, newY}); }
    else placeholder_.setPosition({newX, newY});

    // Bounce on horizontal edges
    auto bounds = hasTexture_ ? (sprite_ ? sprite_->getGlobalBounds() : sf::FloatRect()) : placeholder_.getGlobalBounds();
    if (bounds.position.x + bounds.size.x < 0.f) {
        // jumped off left, flip to right
        if (hasTexture_ && sprite_) sprite_->setPosition({bounds.size.x / 2.f, newY});
        else placeholder_.setPosition({bounds.size.x / 2.f, newY});
        vx_ = std::abs(vx_);
        if (hasTexture_ && sprite_) { auto sc = sprite_->getScale(); sprite_->setScale({std::abs(sc.x), sc.y}); }
    } else if (bounds.position.x > static_cast<float>(windowSize_.x)) {
        // jumped off right
        if (hasTexture_ && sprite_) sprite_->setPosition({static_cast<float>(windowSize_.x) - bounds.size.x / 2.f, newY});
        else placeholder_.setPosition({static_cast<float>(windowSize_.x) - bounds.size.x / 2.f, newY});
        vx_ = -std::abs(vx_);
        if (hasTexture_ && sprite_) { auto sc = sprite_->getScale(); sprite_->setScale({-std::abs(sc.x), sc.y}); }
    }
}

void Duck::draw(sf::RenderWindow& window) const {
    if (!isAlive_) return;
    if (hasTexture_) {
        if (sprite_) window.draw(*sprite_);
    } else {
        window.draw(placeholder_);
    }
}

sf::FloatRect Duck::getBounds() const {
    return hasTexture_ ? (sprite_ ? sprite_->getGlobalBounds() : sf::FloatRect()) : placeholder_.getGlobalBounds();
}

void Duck::onShot() {
    if (!isAlive_ || isFalling_) return;
    isFalling_ = true;
    // give an initial small upward impulse and reduce horizontal speed
    vy_ = -200.f;
    vx_ *= 0.25f; // reduce horizontal motion while falling
}
