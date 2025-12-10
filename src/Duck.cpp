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
    : vx_(0.f), vy_(0.f), baseY_(startPos.y), amplitude_(20.f), frequency_(2.f), time_(0.f), windowSize_(windowSize)
{
    ensureTextureLoaded(texturePath);

    if (hasTexture_ && sprite_) sprite_->setPosition(startPos);
    else placeholder_.setPosition(startPos);

    // Randomize horizontal speed and direction
    vx_ = randRange(80.f, 160.f);
    if (randRange(0.f, 1.f) < 0.5f) vx_ = -vx_;

    amplitude_ = randRange(10.f, 40.f);
    frequency_ = randRange(1.0f, 3.0f);

    // If moving left, flip visual horizontally
    if (vx_ < 0.f) {
        if (hasTexture_ && sprite_) {
            auto sc = sprite_->getScale();
            sprite_->setScale({-std::abs(sc.x), sc.y});
        } else {
            placeholder_.setScale({-std::abs(placeholder_.getScale().x), placeholder_.getScale().y});
        }
    }
}

Duck::~Duck() {}

void Duck::ensureTextureLoaded(const std::string& path) {
    sf::Image image;
    bool loaded = image.loadFromFile(path);

    if (!loaded) {
        // Generate a simple placeholder image and save it to the requested path (best-effort)
        const unsigned int W = 64, H = 48;
        sf::RenderTexture rt;
        if (rt.create(W, H)) {
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
            // try to save placeholder (ignore failure)
            image.saveToFile(path);
            loaded = true;
        }
    }

    if (loaded) {
        // apply simple color-keying by luminance to remove bright background
        unsigned int w = image.getSize().x;
        unsigned int h = image.getSize().y;
        const float KEY_HIGH = 250.f;
        const float KEY_LOW  = 200.f;

        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                sf::Color c = image.getPixel(x, y);
                float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
                if (lum >= KEY_HIGH) {
                    c.a = 0;
                    image.setPixel(x, y, c);
                } else if (lum > KEY_LOW) {
                    float t = (KEY_HIGH - lum) / (KEY_HIGH - KEY_LOW);
                    if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
                    c.a = static_cast<uint8_t>(c.a * t + 0.5f);
                    image.setPixel(x, y, c);
                }
            }
        }

        // flip horizontally so sprite faces right by default
        sf::Image flipped;
        flipped.create(w, h, sf::Color::Transparent);
        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                sf::Color px = image.getPixel(x, y);
                flipped.setPixel(w - 1 - x, y, px);
            }
        }

        texture_.loadFromImage(flipped);
        texture_.setSmooth(true);
        sprite_.reset(new sf::Sprite(texture_));
        auto b = sprite_->getLocalBounds();
        sprite_->setOrigin({b.width / 2.f, b.height / 2.f});

        const float desiredHeight = 32.f;
        if (b.height > 0.f) {
            float scale = desiredHeight / b.height;
            sprite_->setScale({scale, scale});
        }

        hasTexture_ = true;
    } else {
        // fallback placeholder rectangle
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
            sprite_->rotate(200.f * dt);
        } else {
            placeholder_.move({vx_ * dt, vy_ * dt});
        }

        float yPos = hasTexture_ && sprite_ ? sprite_->getPosition().y : placeholder_.getPosition().y;
        if (yPos > static_cast<float>(windowSize_.y) + 64.f) {
            isAlive_ = false;
        }
        return;
    }

    time_ += dt;
    float curX = hasTexture_ && sprite_ ? sprite_->getPosition().x : placeholder_.getPosition().x;
    float newX = curX + vx_ * dt;
    float newY = baseY_ + amplitude_ * std::sin(frequency_ * time_);

    if (hasTexture_) {
        if (sprite_) sprite_->setPosition({newX, newY});
    } else {
        placeholder_.setPosition({newX, newY});
    }

    auto bounds = hasTexture_ && sprite_ ? sprite_->getGlobalBounds() : placeholder_.getGlobalBounds();
    if (bounds.left + bounds.width < 0.f) {
        // re-enter from left
        if (hasTexture_ && sprite_) sprite_->setPosition({bounds.width / 2.f, newY});
        else placeholder_.setPosition({bounds.width / 2.f, newY});
        vx_ = std::abs(vx_);
        if (hasTexture_ && sprite_) { auto sc = sprite_->getScale(); sprite_->setScale({std::abs(sc.x), sc.y}); }
    } else if (bounds.left > static_cast<float>(windowSize_.x)) {
        // re-enter from right
        if (hasTexture_ && sprite_) sprite_->setPosition({static_cast<float>(windowSize_.x) - bounds.width / 2.f, newY});
        else placeholder_.setPosition({static_cast<float>(windowSize_.x) - bounds.width / 2.f, newY});
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
    if (hasTexture_) return sprite_ ? sprite_->getGlobalBounds() : sf::FloatRect();
    return placeholder_.getGlobalBounds();
}

void Duck::onShot() {
    if (!isAlive_ || isFalling_) return;
    isFalling_ = true;
    vy_ = -200.f;
    vx_ *= 0.25f;
}

