#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct AnimationFrame {
    std::shared_ptr<sf::Texture> texture;
    sf::IntRect rect;
};

struct AnimationClip {
    std::vector<AnimationFrame> frames;
    float frameTime{0.1f};
    bool loop{true};
};

struct AnimationSet {
    std::unordered_map<std::string, AnimationClip> clips;
    sf::Vector2f scale{1.f, 1.f};
    sf::Vector2f anchor{0.5f, 0.5f};
};

class Animator {
public:
    Animator() = default;
    explicit Animator(const AnimationSet* animationSet);

    void setAnimationSet(const AnimationSet* animationSet);
    void play(const std::string& clipName, bool restart = false);
    void update(float dt);
    void setPosition(const sf::Vector2f& position);
    void move(const sf::Vector2f& delta);
    void setFacing(float facing);

    [[nodiscard]] const sf::Sprite& sprite() const;
    [[nodiscard]] sf::Sprite& sprite();
    [[nodiscard]] sf::FloatRect bounds() const;
    [[nodiscard]] sf::Vector2f position() const;
    [[nodiscard]] bool isFinished() const;
    [[nodiscard]] const std::string& currentClip() const;

private:
    void applyFrame();

    const AnimationSet* animationSet_{nullptr};
    sf::Sprite sprite_;
    std::string currentClip_;
    float elapsed_{0.f};
    std::size_t frameIndex_{0};
    bool finished_{false};
    float facing_{1.f};
};
