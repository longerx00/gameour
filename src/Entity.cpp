#include "Entity.hpp"

#include <algorithm>

Entity::Entity(const AnimationSet* animationSet, const sf::Vector2f& position, float maxHealth)
    : animator_(animationSet), maxHealth_(maxHealth), health_(maxHealth) {
    animator_.setPosition(position);
}

void Entity::draw(sf::RenderTarget& target) const {
    target.draw(animator_.sprite());
}

void Entity::drawHealthBar(sf::RenderTarget& target) const {
    const sf::FloatRect body = bounds();
    const float width = std::clamp(body.width * 0.62f, 24.f, 44.f);
    const sf::Vector2f position(body.left + (body.width - width) * 0.5f, body.top - 18.f);

    sf::RectangleShape frame({width + 4.f, 8.f});
    frame.setPosition(position.x - 2.f, position.y - 1.f);
    frame.setFillColor(sf::Color(12, 18, 24, 214));
    frame.setOutlineThickness(1.f);
    frame.setOutlineColor(sf::Color(72, 92, 112, 190));

    sf::RectangleShape back({width, 4.f});
    back.setPosition(position);
    back.setFillColor(sf::Color(52, 28, 30, 228));

    sf::RectangleShape fill({width * healthRatio(), 4.f});
    fill.setPosition(position);
    fill.setFillColor(sf::Color(102, 226, 126, 240));

    target.draw(frame);
    target.draw(back);
    target.draw(fill);
}

void Entity::takeDamage(float amount) {
    health_ = std::max(0.f, health_ - amount);
}

void Entity::heal(float amount) {
    health_ = std::min(maxHealth_, health_ + amount);
}

void Entity::healToFull() {
    health_ = maxHealth_;
}

bool Entity::isAlive() const {
    return health_ > 0.f;
}

sf::FloatRect Entity::bounds() const {
    return animator_.bounds();
}

sf::Vector2f Entity::position() const {
    return animator_.position();
}

sf::Vector2f Entity::center() const {
    const sf::FloatRect body = bounds();
    return {body.left + body.width * 0.5f, body.top + body.height * 0.5f};
}

float Entity::healthRatio() const {
    return maxHealth_ <= 0.f ? 0.f : health_ / maxHealth_;
}

float Entity::health() const {
    return health_;
}

float Entity::maxHealth() const {
    return maxHealth_;
}

void Entity::setPosition(const sf::Vector2f& position) {
    animator_.setPosition(position);
}

void Entity::setFacing(float facing) {
    animator_.setFacing(facing);
}

void Entity::applyGravity(float dt, float gravity) {
    velocity_.y += gravity * dt;
}

void Entity::moveAndCollide(float dt, const std::vector<Platform>& platforms, float worldWidth) {
    animator_.move({velocity_.x * dt, 0.f});

    sf::FloatRect horizontal = animator_.bounds();
    if (horizontal.left < 0.f) {
        animator_.move({-horizontal.left, 0.f});
    } else if (horizontal.left + horizontal.width > worldWidth) {
        animator_.move({worldWidth - (horizontal.left + horizontal.width), 0.f});
    }

    animator_.move({0.f, velocity_.y * dt});
    onGround_ = false;

    sf::FloatRect body = animator_.bounds();
    for (const Platform& platform : platforms) {
        if (!body.intersects(platform.bounds)) {
            continue;
        }

        if (velocity_.y >= 0.f && body.top + body.height - velocity_.y * dt <= platform.bounds.top + 8.f) {
            animator_.move({0.f, platform.bounds.top - (body.top + body.height)});
            velocity_.y = 0.f;
            onGround_ = true;
            body = animator_.bounds();
        } else if (velocity_.y < 0.f && body.top >= platform.bounds.top + platform.bounds.height - 8.f) {
            animator_.move({0.f, platform.bounds.top + platform.bounds.height - body.top});
            velocity_.y = 0.f;
            body = animator_.bounds();
        }
    }
}

void Entity::clampToWorld(float minX, float maxX) {
    sf::FloatRect body = animator_.bounds();
    if (body.left < minX) {
        animator_.move({minX - body.left, 0.f});
    } else if (body.left + body.width > maxX) {
        animator_.move({maxX - (body.left + body.width), 0.f});
    }
}

void Entity::updateAnimation(float dt) {
    animator_.update(dt);
}
