#include "Bullet.hpp"

Bullet::Bullet(std::shared_ptr<sf::Texture> texture, const sf::Vector2f& position, float direction, float damage, int pierce)
    : texture_(std::move(texture)), damage_(damage), remainingPierce_(pierce) {
    sprite_.setTexture(*texture_);
    sprite_.setOrigin(
        static_cast<float>(texture_->getSize().x) * 0.5f,
        static_cast<float>(texture_->getSize().y) * 0.5f
    );
    sprite_.setScale(1.25f * direction, 1.25f);
    sprite_.setPosition(position);
    velocity_ = {880.f * direction, 0.f};
}

void Bullet::update(float dt, float worldWidth) {
    sprite_.move(velocity_ * dt);
    const float x = sprite_.getPosition().x;
    alive_ = alive_ && x >= -48.f && x <= worldWidth + 48.f;
}

void Bullet::draw(sf::RenderTarget& target) const {
    target.draw(sprite_);
}

sf::FloatRect Bullet::bounds() const {
    return sprite_.getGlobalBounds();
}

bool Bullet::isAlive() const {
    return alive_;
}

float Bullet::damage() const {
    return damage_;
}

void Bullet::destroy() {
    alive_ = false;
}

void Bullet::registerHit() {
    if (remainingPierce_ > 0) {
        --remainingPierce_;
        return;
    }

    destroy();
}
