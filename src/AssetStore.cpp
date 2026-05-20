#include "AssetStore.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

AssetStore::AssetStore() = default;

void AssetStore::load() {
    assetRoot_ = findAssetRoot();
    loadEnvironmentAssets();
    loadPlayerAssets();
    loadEnemyAssets();
    loadUiAssets();
    loadFontFallback();
}

const sf::Texture& AssetStore::texture(const std::string& id) const {
    return *textures_.at(id);
}

std::shared_ptr<sf::Texture> AssetStore::texturePtr(const std::string& id) const {
    return textures_.at(id);
}

const AnimationSet& AssetStore::animationSet(const std::string& id) const {
    return animationSets_.at(id);
}

const sf::Font& AssetStore::font() const {
    return font_;
}

fs::path AssetStore::assetPath(const std::string& relativePath) const {
    return resolve(relativePath);
}

fs::path AssetStore::findAssetRoot() const {
    std::vector<fs::path> candidates;
    fs::path current = fs::current_path();
    for (int i = 0; i < 5; ++i) {
        candidates.push_back(current / "ProjectGame");
        if (!current.has_parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    for (const fs::path& candidate : candidates) {
        if (fs::exists(candidate)) {
            return candidate;
        }
    }

    throw std::runtime_error("ProjectGame asset folder was not found.");
}

fs::path AssetStore::resolve(const std::string& relativePath) const {
    const fs::path result = assetRoot_ / fs::path(relativePath);
    if (!fs::exists(result)) {
        throw std::runtime_error("Missing asset: " + result.string());
    }
    return result;
}

std::shared_ptr<sf::Texture> AssetStore::loadTexture(const std::string& id, const std::string& relativePath, bool repeated) {
    auto texture = std::make_shared<sf::Texture>();
    if (!texture->loadFromFile(resolve(relativePath).string())) {
        throw std::runtime_error("Unable to load texture: " + relativePath);
    }

    texture->setSmooth(false);
    texture->setRepeated(repeated);
    textures_[id] = texture;
    return texture;
}

void AssetStore::loadFontFallback() {
    const std::array<fs::path, 3> fontCandidates{
        fs::path("C:/Windows/Fonts/arial.ttf"),
        fs::path("C:/Windows/Fonts/segoeui.ttf"),
        fs::path("C:/Windows/Fonts/tahoma.ttf")
    };

    for (const fs::path& candidate : fontCandidates) {
        if (fs::exists(candidate) && font_.loadFromFile(candidate.string())) {
            return;
        }
    }

    throw std::runtime_error("Unable to load a UI font from Windows fonts.");
}

AnimationClip AssetStore::loadClipFromFiles(const std::string& relativeDirectory, float frameTime, bool loop) {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(resolve(relativeDirectory))) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    AnimationClip clip;
    clip.frameTime = frameTime;
    clip.loop = loop;

    for (const fs::path& file : files) {
        auto texture = std::make_shared<sf::Texture>();
        if (!texture->loadFromFile(file.string())) {
            throw std::runtime_error("Unable to load animation frame: " + file.string());
        }
        texture->setSmooth(false);

        clip.frames.push_back({
            texture,
            sf::IntRect({0, 0}, sf::Vector2i(static_cast<int>(texture->getSize().x), static_cast<int>(texture->getSize().y)))
        });
    }

    return clip;
}

AnimationClip AssetStore::loadClipFromSheet(
    const std::string& relativePath,
    const sf::Vector2i& frameSize,
    float frameTime,
    bool loop,
    int frameCount
) {
    auto texture = std::make_shared<sf::Texture>();
    if (!texture->loadFromFile(resolve(relativePath).string())) {
        throw std::runtime_error("Unable to load spritesheet: " + relativePath);
    }
    texture->setSmooth(false);

    const int totalFrames = frameCount > 0 ? frameCount : static_cast<int>(texture->getSize().x) / frameSize.x;

    AnimationClip clip;
    clip.frameTime = frameTime;
    clip.loop = loop;

    for (int i = 0; i < totalFrames; ++i) {
        clip.frames.push_back({texture, sf::IntRect({i * frameSize.x, 0}, frameSize)});
    }

    return clip;
}

void AssetStore::loadEnvironmentAssets() {
    loadTexture("bg_back_1", "Multi_Platformer_Tileset_Original/GrassLand/Background/GrassLand_Background_1.png", true);
    loadTexture("bg_back_2", "Multi_Platformer_Tileset_Original/GrassLand/Background/GrassLand_Background_2.png", true);
    loadTexture("bg_back_3", "Multi_Platformer_Tileset_Original/GrassLand/Background/GrassLand_Background_3.png", true);
    loadTexture("bg_back_4", "Multi_Platformer_Tileset_Original/GrassLand/Background/GrassLand_Background_4.png", true);
    loadTexture("bg_cloud_1", "Multi_Platformer_Tileset_Original/GrassLand/Background/GrassLand_Cloud_1.png");
    loadTexture("bg_cloud_2", "Multi_Platformer_Tileset_Original/GrassLand/Background/GrassLand_Cloud_2.png");
    loadTexture("oak_bg_1", "oak_woods_v1.0/background/background_layer_1.png", true);
    loadTexture("oak_bg_2", "oak_woods_v1.0/background/background_layer_2.png", true);
    loadTexture("oak_bg_3", "oak_woods_v1.0/background/background_layer_3.png", true);
    loadTexture("sunny_bg_back", "Sunny-land-files/Assets/environment/Background/back.png", true);
    loadTexture("sunny_bg_middle", "Sunny-land-files/Assets/environment/Background/middle.png", true);
    loadTexture("sunny_bg_props", "Sunny-land-files/Assets/environment/Background/props.png", true);
    loadTexture("platform_long", "Sunny-land-files/Assets/environment/Props/platform-long.png");
    loadTexture("platform_small", "Sunny-land-files/Assets/environment/Props/small-platform.png");
    loadTexture("tree", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Tree.png");
    loadTexture("bush", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Bush.png");
    loadTexture("rock", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Stone_1.png");
    loadTexture("rock2", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Stone_2.png");
    loadTexture("rock3", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Stone_3.png");
    loadTexture("grass1", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Grass_1.png");
    loadTexture("grass2", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Grass_2.png");
    loadTexture("grass3", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Grass_3.png");
    loadTexture("grass4", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Grass_4.png");
    loadTexture("flower", "Multi_Platformer_Tileset_Original/GrassLand/Details/GrassLand_Flower.png");
    loadTexture("crate", "Sunny-land-files/Assets/environment/Props/crate.png");
    loadTexture("sign", "Sunny-land-files/Assets/environment/Props/sign.png");
    loadTexture("oak_grass_1", "oak_woods_v1.0/decorations/grass_1.png");
    loadTexture("oak_grass_2", "oak_woods_v1.0/decorations/grass_2.png");
    loadTexture("oak_grass_3", "oak_woods_v1.0/decorations/grass_3.png");
    loadTexture("oak_lamp", "oak_woods_v1.0/decorations/lamp.png");
    loadTexture("oak_rock_1", "oak_woods_v1.0/decorations/rock_1.png");
    loadTexture("oak_rock_2", "oak_woods_v1.0/decorations/rock_2.png");
    loadTexture("oak_rock_3", "oak_woods_v1.0/decorations/rock_3.png");
    loadTexture("oak_sign", "oak_woods_v1.0/decorations/sign.png");
    loadTexture("pine", "Sunny-land-files/Assets/environment/Props/pine.png");
    loadTexture("palm", "Sunny-land-files/Assets/environment/Props/palm.png");
    loadTexture("shrooms", "Sunny-land-files/Assets/environment/Props/shrooms.png");
    loadTexture("skulls", "Sunny-land-files/Assets/environment/Props/skulls.png");
    loadTexture("spikes", "Sunny-land-files/Assets/environment/Props/spikes.png");
}

void AssetStore::loadPlayerAssets() {
    AnimationSet player;
    player.scale = {2.6f, 2.6f};
    player.anchor = {0.5f, 0.6f};
    player.clips["idle"] = loadClipFromFiles("Sunny-land-files/Assets/Characters/Foxy/Sprites/idle", 0.14f, true);
    player.clips["run"] = loadClipFromFiles("Sunny-land-files/Assets/Characters/Foxy/Sprites/run", 0.09f, true);
    player.clips["jump"] = loadClipFromFiles("Sunny-land-files/Assets/Characters/Foxy/Sprites/jump", 0.12f, true);
    player.clips["dash"] = loadClipFromFiles("Sunny-land-files/Assets/Characters/Foxy/Sprites/Roll", 0.05f, true);
    player.clips["hurt"] = loadClipFromFiles("Sunny-land-files/Assets/Characters/Foxy/Sprites/hurt", 0.09f, false);
    animationSets_["player"] = player;
}

void AssetStore::loadEnemyAssets() {
    AnimationSet mushroom;
    mushroom.scale = {1.9f, 1.9f};
    mushroom.anchor = {0.5f, 0.7f};
    mushroom.clips["idle"] = loadClipFromSheet(
        "Forest_Monsters_FREE/Mushroom/Mushroom without VFX/Mushroom-Idle.png",
        {80, 64},
        0.11f,
        true
    );
    mushroom.clips["run"] = loadClipFromSheet(
        "Forest_Monsters_FREE/Mushroom/Mushroom without VFX/Mushroom-Run.png",
        {80, 64},
        0.08f,
        true
    );
    mushroom.clips["hurt"] = loadClipFromSheet(
        "Forest_Monsters_FREE/Mushroom/Mushroom without VFX/Mushroom-Hit.png",
        {80, 64},
        0.08f,
        false
    );
    animationSets_["mushroom"] = mushroom;

    AnimationSet bat;
    bat.scale = {1.8f, 1.8f};
    bat.anchor = {0.5f, 0.5f};
    bat.clips["idle"] = loadClipFromSheet(
        "DarkFantasyEnemies_FREE/Bat/Bat without VFX/Bat-IdleFly.png",
        {64, 64},
        0.09f,
        true
    );
    bat.clips["run"] = loadClipFromSheet(
        "DarkFantasyEnemies_FREE/Bat/Bat without VFX/Bat-Run.png",
        {64, 64},
        0.08f,
        true
    );
    bat.clips["hurt"] = loadClipFromSheet(
        "DarkFantasyEnemies_FREE/Bat/Bat without VFX/Bat-Hurt.png",
        {64, 64},
        0.08f,
        false
    );
    animationSets_["bat"] = bat;

    AnimationSet golem;
    golem.scale = {2.2f, 2.2f};
    golem.anchor = {0.5f, 0.72f};
    golem.clips["idle"] = loadClipFromSheet(
        "Golems_Free_Version/Golem_1/Blue/No_Swoosh_VFX/Golem_1_idle.png",
        {72, 64},
        0.12f,
        true
    );
    golem.clips["run"] = loadClipFromSheet(
        "Golems_Free_Version/Golem_1/Blue/No_Swoosh_VFX/Golem_1_walk.png",
        {90, 64},
        0.09f,
        true
    );
    golem.clips["hurt"] = loadClipFromSheet(
        "Golems_Free_Version/Golem_1/Blue/No_Swoosh_VFX/Golem_1_hurt.png",
        {72, 64},
        0.10f,
        false
    );
    animationSets_["golem"] = golem;
}

void AssetStore::loadUiAssets() {
    loadTexture("bullet", "Sunny-land-files/Assets/Misc/Sunnyland items/Sprites/gem/gem-1.png");
}
