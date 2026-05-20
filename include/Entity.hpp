#pragma once

#include "Animation.hpp"
#include "Platform.hpp"

#include <SFML/Graphics.hpp>

#include <vector>

class Entity {
public:
    Entity(const AnimationSet* animationSet, const sf::Vector2f& position, float maxHealth);
    virtual ~Entity() = default;

    virtual void update(float dt, const std::vector<Platform>& platforms) = 0;

    void draw(sf::RenderTarget& target) const;
    void drawHealthBar(sf::RenderTarget& target) const;
    void takeDamage(float amount);
    void heal(float amount);
    void healToFull();

    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] sf::FloatRect bounds() const;
    [[nodiscard]] sf::Vector2f position() const;
    [[nodiscard]] sf::Vector2f center() const;
    [[nodiscard]] float healthRatio() const;
    [[nodiscard]] float health() const;
    [[nodiscard]] float maxHealth() const;

protected:
    void setPosition(const sf::Vector2f& position);
    void setFacing(float facing);
    void applyGravity(float dt, float gravity);
    void moveAndCollide(float dt, const std::vector<Platform>& platforms, float worldWidth);
    void clampToWorld(float minX, float maxX);
    void updateAnimation(float dt);

    Animator animator_;
    sf::Vector2f velocity_{0.f, 0.f};
    float maxHealth_{100.f};
    float health_{100.f};
    bool onGround_{false};
};
