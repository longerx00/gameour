#include "Player.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kGravity = 1800.f;
constexpr float kDefaultWorldWidth = 6400.f;
}

Player::Player() : Entity(nullptr, {0.f, 0.f}, 110.f) {}

Player::Player(const AnimationSet* animationSet, const sf::Vector2f& position)
    : Entity(animationSet, position, 110.f) {
    animator_.play("idle", true);
}

void Player::update(float dt, const std::vector<Platform>& platforms) {
    const bool moveLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
    const bool moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    const bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::W)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
    const bool jumpTriggered = jumpPressed && !jumpHeldLastFrame_;
    jumpHeldLastFrame_ = jumpPressed;
    wantsToShoot_ = sf::Keyboard::isKeyPressed(sf::Keyboard::F) || sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);

    shootTimer_ = std::max(0.f, shootTimer_ - dt);
    dashCooldownTimer_ = std::max(0.f, dashCooldownTimer_ - dt);
    dashTimer_ = std::max(0.f, dashTimer_ - dt);
    hurtTimer_ = std::max(0.f, hurtTimer_ - dt);

    if (dashUnlocked_ && dashTimer_ <= 0.f && dashCooldownTimer_ <= 0.f && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        dashTimer_ = dashDuration_;
        dashCooldownTimer_ = dashCooldown_;
        velocity_.y = 0.f;
    }

    if (dashTimer_ > 0.f) {
        velocity_.x = facing_ * dashSpeed_;
    } else {
        velocity_.x = 0.f;
        if (moveLeft) {
            velocity_.x -= moveSpeed_;
            facing_ = -1.f;
        }
        if (moveRight) {
            velocity_.x += moveSpeed_;
            facing_ = 1.f;
        }

        if (jumpTriggered && onGround_) {
            velocity_.y = -jumpStrength_;
            onGround_ = false;
            airJumpsUsed_ = 0;
        } else if (jumpTriggered && !onGround_ && doubleJumpUnlocked_ && airJumpsUsed_ < airJumpsAvailable_) {
            velocity_.y = -jumpStrength_ * 0.92f;
            ++airJumpsUsed_;
        }
    }

    applyGravity(dt, kGravity);
    moveAndCollide(dt, platforms, kDefaultWorldWidth);
    updateAnimationState();
    setFacing(facing_);
    updateAnimation(dt);
}

void Player::reset(const sf::Vector2f& position) {
    setPosition(position);
    velocity_ = {0.f, 0.f};
    health_ = maxHealth_;
    onGround_ = false;
    facing_ = 1.f;
    airJumpsUsed_ = 0;
    jumpHeldLastFrame_ = false;
    shootTimer_ = 0.f;
    hurtTimer_ = 0.f;
    dashCooldownTimer_ = 0.f;
    dashTimer_ = 0.f;
    animator_.play("idle", true);
    setFacing(facing_);
}

void Player::resetProgression(const sf::Vector2f& position) {
    moveSpeed_ = 320.f;
    jumpStrength_ = 760.f;
    shootCooldown_ = 0.24f;
    dashUnlocked_ = false;
    dashCooldown_ = 1.7f;
    dashDuration_ = 0.16f;
    dashSpeed_ = 920.f;
    bulletDamage_ = 22.f;
    armorMultiplier_ = 1.f;
    bulletPierce_ = 0;
    leechAmount_ = 0.f;
    doubleJumpUnlocked_ = false;
    airJumpsAvailable_ = 0;
    airJumpsUsed_ = 0;
    maxHealth_ = 110.f;
    reset(position);
}

float Player::facing() const {
    return facing_;
}

bool Player::isInvulnerable() const {
    return dashTimer_ > 0.f || hurtTimer_ > 0.f;
}

bool Player::dashUnlocked() const {
    return dashUnlocked_;
}

float Player::bulletDamage() const {
    return bulletDamage_;
}

float Player::moveSpeed() const {
    return moveSpeed_;
}

float Player::jumpStrength() const {
    return jumpStrength_;
}

float Player::armorMultiplier() const {
    return armorMultiplier_;
}

int Player::bulletPierce() const {
    return bulletPierce_;
}

float Player::leechAmount() const {
    return leechAmount_;
}

bool Player::doubleJumpUnlocked() const {
    return doubleJumpUnlocked_;
}

bool Player::tryShoot() {
    if (!wantsToShoot_ || shootTimer_ > 0.f) {
        return false;
    }

    shootTimer_ = shootCooldown_;
    return true;
}

void Player::applyUpgrade(UpgradeType type) {
    switch (type) {
        case UpgradeType::Damage:
            bulletDamage_ += 9.f;
            break;
        case UpgradeType::FireRate:
            shootCooldown_ = std::max(0.08f, shootCooldown_ * 0.84f);
            break;
        case UpgradeType::Vitality:
            maxHealth_ += 30.f;
            health_ = std::min(maxHealth_, health_ + 35.f);
            break;
        case UpgradeType::Dash:
            if (!dashUnlocked_) {
                dashUnlocked_ = true;
            } else {
                dashCooldown_ = std::max(0.75f, dashCooldown_ - 0.2f);
                dashDuration_ = std::min(0.26f, dashDuration_ + 0.02f);
            }
            break;
        case UpgradeType::DoubleJump:
            doubleJumpUnlocked_ = true;
            airJumpsAvailable_ = std::min(2, airJumpsAvailable_ + 1);
            break;
        case UpgradeType::MoveSpeed:
            moveSpeed_ += 26.f;
            dashSpeed_ += 32.f;
            break;
        case UpgradeType::Jump:
            jumpStrength_ += 55.f;
            break;
        case UpgradeType::Armor:
            armorMultiplier_ = std::max(0.58f, armorMultiplier_ - 0.12f);
            break;
        case UpgradeType::Pierce:
            bulletPierce_ = std::min(3, bulletPierce_ + 1);
            break;
        case UpgradeType::Leech:
            leechAmount_ = std::min(10.f, leechAmount_ + 2.5f);
            break;
    }
}

void Player::receiveDamage(float amount) {
    takeDamage(amount * armorMultiplier_);
    hurtTimer_ = 0.28f;
}

void Player::onEnemyDefeated() {
    if (leechAmount_ > 0.f) {
        heal(leechAmount_);
    }
}

void Player::updateAnimationState() {
    if (hurtTimer_ > 0.f) {
        animator_.play("hurt");
    } else if (dashTimer_ > 0.f) {
        animator_.play("dash");
    } else if (!onGround_) {
        animator_.play("jump");
    } else if (std::abs(velocity_.x) > 5.f) {
        animator_.play("run");
    } else {
        animator_.play("idle");
    }
}
