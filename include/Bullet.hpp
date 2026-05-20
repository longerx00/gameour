#pragma once

#include <SFML/Graphics.hpp>

#include <memory>

class Bullet {
public:
    Bullet(std::shared_ptr<sf::Texture> texture, const sf::Vector2f& position, float direction, float damage, int pierce = 0);

    void update(float dt, float worldWidth);
    void draw(sf::RenderTarget& target) const;

    [[nodiscard]] sf::FloatRect bounds() const;
    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] float damage() const;

    void destroy();
    void registerHit();

private:
    std::shared_ptr<sf::Texture> texture_;
    sf::Sprite sprite_;
    sf::Vector2f velocity_{0.f, 0.f};
    float damage_{0.f};
    int remainingPierce_{0};
    bool alive_{true};
};
