#pragma once

#include "Entity.hpp"

enum class EnemyType {
    Mushroom,
    Bat,
    Golem,
    BossGolem
};

class Enemy final : public Entity {
public:
    Enemy(
        const AnimationSet* animationSet,
        EnemyType type,
        const sf::Vector2f& position,
        float moveSpeed,
        float maxHealth,
        float contactDamage,
        float worldWidth
    );

    void update(float dt, const std::vector<Platform>& platforms) override;
    void setTarget(const sf::Vector2f& targetPosition);
    bool canHitPlayer() const;
    void resetHitCooldown();
    bool consumeAttackRequest();

    [[nodiscard]] float contactDamage() const;
    [[nodiscard]] EnemyType type() const;
    [[nodiscard]] bool isBoss() const;
    [[nodiscard]] float attackPower() const;
    [[nodiscard]] sf::Vector2f attackOrigin() const;
    [[nodiscard]] int attackPattern() const;

private:
    void updateAnimationState();

    EnemyType type_{EnemyType::Mushroom};
    sf::Vector2f targetPosition_{0.f, 0.f};
    float moveSpeed_{120.f};
    float contactDamage_{10.f};
    float contactCooldown_{0.9f};
    float contactTimer_{0.f};
    float worldWidth_{3200.f};
    float rangedAttackCooldown_{2.6f};
    float rangedAttackTimer_{1.2f};
    float attackPower_{18.f};
    bool attackQueued_{false};
    int attackPattern_{0};
};
