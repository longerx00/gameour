#pragma once

#include "Animation.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class AssetStore {
public:
    AssetStore();

    void load();

    [[nodiscard]] const sf::Texture& texture(const std::string& id) const;
    [[nodiscard]] std::shared_ptr<sf::Texture> texturePtr(const std::string& id) const;
    [[nodiscard]] const AnimationSet& animationSet(const std::string& id) const;
    [[nodiscard]] const sf::Font& font() const;
    [[nodiscard]] std::filesystem::path assetPath(const std::string& relativePath) const;

private:
    std::filesystem::path findAssetRoot() const;
    std::filesystem::path resolve(const std::string& relativePath) const;

    std::shared_ptr<sf::Texture> loadTexture(const std::string& id, const std::string& relativePath, bool repeated = false);
    void loadFontFallback();

    AnimationClip loadClipFromFiles(const std::string& relativeDirectory, float frameTime, bool loop);
    AnimationClip loadClipFromSheet(
        const std::string& relativePath,
        const sf::Vector2i& frameSize,
        float frameTime,
        bool loop,
        int frameCount = 0
    );

    void loadEnvironmentAssets();
    void loadPlayerAssets();
    void loadEnemyAssets();
    void loadUiAssets();

    std::filesystem::path assetRoot_;
    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, AnimationSet> animationSets_;
    sf::Font font_;
};
