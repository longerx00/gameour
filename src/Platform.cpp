#include "Platform.hpp"

Platform::Platform(const sf::Texture& texture, const sf::Vector2f& position, const sf::Vector2f& size) {
    bounds = {position.x, position.y, size.x, size.y};
    sprite.setTexture(texture);
    const auto textureSize = texture.getSize();
    sprite.setScale(
        size.x / static_cast<float>(textureSize.x),
        size.y / static_cast<float>(textureSize.y)
    );
    sprite.setPosition(position);
}

void Platform::draw(sf::RenderTarget& target) const {
    target.draw(sprite);
}
