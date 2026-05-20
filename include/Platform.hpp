#pragma once

#include <SFML/Graphics.hpp>

struct Platform {
    sf::FloatRect bounds;
    sf::Sprite sprite;

    Platform() = default;
    Platform(const sf::Texture& texture, const sf::Vector2f& position, const sf::Vector2f& size);

    void draw(sf::RenderTarget& target) const;
};
