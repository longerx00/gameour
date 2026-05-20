#pragma once

#include "AssetStore.hpp"
#include "Bullet.hpp"
#include "Enemy.hpp"
#include "Platform.hpp"
#include "Player.hpp"

#include <SFML/Graphics.hpp>

#include <random>
#include <string>
#include <vector>

struct Decoration {
    sf::Sprite sprite;
    bool frontLayer{false};
};

struct BackgroundLayer {
    sf::Sprite sprite;
    float parallax{1.f};
    sf::Vector2f basePosition{0.f, 0.f};
    sf::Color tint{sf::Color::White};
};

struct RewardOption {
    UpgradeType type{UpgradeType::Damage};
    std::string title;
    std::string description;
};

struct EnemyProjectile {
    sf::CircleShape shape;
    sf::Vector2f velocity{0.f, 0.f};
    float damage{0.f};
    bool alive{true};
};

enum class LevelTheme {
    Grasslands,
    OakWoods,
    MushroomGrove,
    DesertRuins,
    Iceland,
    Dungeon,
    BossArena
};

struct LevelDefinition {
    LevelTheme theme{LevelTheme::Grasslands};
    std::string title;
    std::string subtitle;
    std::string musicFile;
    sf::Color skyColor{61, 168, 232};
    sf::Color layerTint{255, 255, 255};
    sf::Color propTint{255, 255, 255};
    int waveBonus{0};
    float enemyHealthMultiplier{1.f};
    float enemyDamageMultiplier{1.f};
    float enemySpeedBonus{0.f};
    bool bossLevel{false};
    int bossWave{4};
};

enum class AppState {
    Menu,
    Intermission,
    Wave,
    Reward,
    Victory,
    GameOver
};

class Game {
public:
    Game();
    void run();

private:
    void processEvents();
    void update(float dt);
    void render();

    void updateIntermission(float dt);
    void updateWave(float dt);
    void updateWorldView();

    void renderWorld();
    void renderMenu();
    void renderHud();
    void renderIntermissionOverlay();
    void renderRewardOverlay();
    void renderVictoryOverlay();
    void renderGameOverOverlay();
    void renderLevelSelect();

    void startRun();
    void restartCampaign();
    void startNextWave();
    void finishWave();
    void buildLevel();
    void buildBackground();
    void buildDecorations();
    void spawnEnemy();
    void spawnBoss();
    void updateEnemyProjectiles(float dt);
    void generateRewardOptions();
    void applyReward(std::size_t index);
    void updateTitle();
    void initializeLevels();
    void selectLevel(std::size_t index);
    void playMusic(const std::string& relativePath, bool loop = true);
    void drawText(
        const std::string& text,
        unsigned size,
        const sf::Vector2f& position,
        const sf::Color& color,
        bool centered = false
    );

    sf::RenderWindow window_;
    sf::View worldView_;
    AssetStore assets_;
    Player player_;

    std::vector<Platform> platforms_;
    std::vector<Decoration> decorations_;
    std::vector<BackgroundLayer> backgroundLayers_;
    std::vector<Enemy> enemies_;
    std::vector<Bullet> bullets_;
    std::vector<EnemyProjectile> enemyProjectiles_;
    std::vector<RewardOption> rewardOptions_;

    std::mt19937 rng_;
    AppState state_{AppState::Menu};
    float worldWidth_{6400.f};
    float intermissionTimer_{0.f};
    float spawnTimer_{0.f};
    int waveNumber_{0};
    int enemiesToSpawn_{0};
    int menuSelection_{0};
    int levelSelection_{0};
    int rewardSelection_{0};
    int defeatedEnemies_{0};
    bool bossSpawned_{false};
    bool levelCompleted_{false};
    std::vector<LevelDefinition> levels_;
    std::size_t activeLevelIndex_{0};
    sf::Color clearColor_{61, 168, 232};
};
