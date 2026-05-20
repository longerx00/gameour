#include "Enemy.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kGravity = 1800.f;
}

Enemy::Enemy(
    const AnimationSet* animationSet,
    EnemyType type,
    const sf::Vector2f& position,
    float moveSpeed,
    float maxHealth,
    float contactDamage,
    float worldWidth
) : Entity(animationSet, position, maxHealth),
    type_(type),
    moveSpeed_(moveSpeed),
    contactDamage_(contactDamage),
    contactTimer_(0.35f),
    worldWidth_(worldWidth) {
    if (type_ == EnemyType::BossGolem) {
        rangedAttackCooldown_ = 1.75f;
        rangedAttackTimer_ = 1.1f;
        attackPower_ = contactDamage_ * 0.9f;
    } else if (type_ == EnemyType::Golem) {
        rangedAttackCooldown_ = 2.6f;
        rangedAttackTimer_ = 1.8f;
        attackPower_ = contactDamage_ * 0.7f;
    }
    animator_.play("idle", true);
}

void Enemy::update(float dt, const std::vector<Platform>& platforms) {
    contactTimer_ = std::max(0.f, contactTimer_ - dt);
    rangedAttackTimer_ = std::max(0.f, rangedAttackTimer_ - dt);

    const float deltaX = targetPosition_.x - position().x;
    const float direction = deltaX < 0.f ? -1.f : 1.f;
    setFacing(-direction);

    if (type_ == EnemyType::Bat) {
        const float deltaY = targetPosition_.y - position().y;
        velocity_ = {
            std::abs(deltaX) > 12.f ? direction * moveSpeed_ : 0.f,
            std::clamp(deltaY * 1.6f, -moveSpeed_, moveSpeed_)
        };
        animator_.move(velocity_ * dt);
        clampToWorld(0.f, worldWidth_);
    } else {
        const float preferredDistance = isBoss() ? 260.f : 10.f;
        if (std::abs(deltaX) > preferredDistance + 24.f) {
            velocity_.x = direction * moveSpeed_;
        } else if (isBoss() && std::abs(deltaX) < preferredDistance - 36.f) {
            velocity_.x = -direction * moveSpeed_ * 0.75f;
        } else {
            velocity_.x = 0.f;
        }
        applyGravity(dt, kGravity);
        moveAndCollide(dt, platforms, worldWidth_);
    }

    if ((type_ == EnemyType::Golem || type_ == EnemyType::BossGolem) && rangedAttackTimer_ <= 0.f && std::abs(deltaX) < 520.f) {
        attackQueued_ = true;
        if (type_ == EnemyType::BossGolem) {
            attackPattern_ = (attackPattern_ + 1) % 3;
        } else {
            attackPattern_ = 0;
        }
        rangedAttackTimer_ = rangedAttackCooldown_;
    }

    updateAnimationState();
    updateAnimation(dt);
}

void Enemy::setTarget(const sf::Vector2f& targetPosition) {
    targetPosition_ = targetPosition;
}

bool Enemy::canHitPlayer() const {
    return contactTimer_ <= 0.f;
}

void Enemy::resetHitCooldown() {
    contactTimer_ = contactCooldown_;
}

float Enemy::contactDamage() const {
    return contactDamage_;
}

bool Enemy::consumeAttackRequest() {
    if (!attackQueued_) {
        return false;
    }

    attackQueued_ = false;
    return true;
}

EnemyType Enemy::type() const {
    return type_;
}

bool Enemy::isBoss() const {
    return type_ == EnemyType::BossGolem;
}

float Enemy::attackPower() const {
    return attackPower_;
}

sf::Vector2f Enemy::attackOrigin() const {
    const sf::FloatRect body = bounds();
    return {body.left + body.width * 0.5f, body.top + body.height * 0.32f};
}

int Enemy::attackPattern() const {
    return attackPattern_;
}

void Enemy::updateAnimationState() {
    if (std::abs(velocity_.x) > 4.f || type_ == EnemyType::Bat) {
        animator_.play("run");
    } else {
        animator_.play("idle");
    }
}
