#pragma once

#include "Entity.hpp"

enum class UpgradeType {
    Damage,
    FireRate,
    Vitality,
    Dash,
    DoubleJump,
    MoveSpeed,
    Jump,
    Armor,
    Pierce,
    Leech
};

class Player final : public Entity {
public:
    Player();
    Player(const AnimationSet* animationSet, const sf::Vector2f& position);

    void update(float dt, const std::vector<Platform>& platforms) override;
    void reset(const sf::Vector2f& position);
    void resetProgression(const sf::Vector2f& position);

    [[nodiscard]] float facing() const;
    [[nodiscard]] bool isInvulnerable() const;
    [[nodiscard]] bool dashUnlocked() const;
    [[nodiscard]] float bulletDamage() const;
    [[nodiscard]] float moveSpeed() const;
    [[nodiscard]] float jumpStrength() const;
    [[nodiscard]] float armorMultiplier() const;
    [[nodiscard]] int bulletPierce() const;
    [[nodiscard]] float leechAmount() const;
    [[nodiscard]] bool doubleJumpUnlocked() const;

    bool tryShoot();
    void applyUpgrade(UpgradeType type);
    void receiveDamage(float amount);
    void onEnemyDefeated();

private:
    void updateAnimationState();

    float moveSpeed_{320.f};
    float jumpStrength_{760.f};
    float shootCooldown_{0.24f};
    float shootTimer_{0.f};
    float facing_{1.f};
    bool wantsToShoot_{false};
    bool dashUnlocked_{false};
    float dashCooldown_{1.7f};
    float dashCooldownTimer_{0.f};
    float dashDuration_{0.16f};
    float dashTimer_{0.f};
    float dashSpeed_{920.f};
    float bulletDamage_{22.f};
    float hurtTimer_{0.f};
    float armorMultiplier_{1.f};
    int bulletPierce_{0};
    float leechAmount_{0.f};
    bool doubleJumpUnlocked_{false};
    int airJumpsAvailable_{0};
    int airJumpsUsed_{0};
    bool jumpHeldLastFrame_{false};
};
