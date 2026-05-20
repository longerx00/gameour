#include "Game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <tuple>
#include <windows.h>
#include <mmsystem.h>

namespace {
constexpr unsigned kWindowWidth = 1280;
constexpr unsigned kWindowHeight = 720;
constexpr float kGroundTop = 670.f;
constexpr float kPreviewPedestalY = 612.f;

float clampf(float value, float low, float high) {
    return std::max(low, std::min(value, high));
}

sf::Color multiplyColor(const sf::Color& color, const sf::Color& tint) {
    return {
        static_cast<sf::Uint8>((static_cast<unsigned>(color.r) * tint.r) / 255),
        static_cast<sf::Uint8>((static_cast<unsigned>(color.g) * tint.g) / 255),
        static_cast<sf::Uint8>((static_cast<unsigned>(color.b) * tint.b) / 255),
        static_cast<sf::Uint8>((static_cast<unsigned>(color.a) * tint.a) / 255)
    };
}
}

Game::Game()
    : window_(sf::VideoMode(kWindowWidth, kWindowHeight), "The Brink"),
      worldView_(sf::FloatRect(0.f, 0.f, static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight))),
      assets_(),
      player_(),
      rng_(std::random_device{}()) {
    window_.setFramerateLimit(144);
    decorations_.reserve(64);
    backgroundLayers_.reserve(24);
    enemies_.reserve(32);
    bullets_.reserve(64);
    enemyProjectiles_.reserve(64);
    rewardOptions_.reserve(3);
    assets_.load();
    player_ = Player(&assets_.animationSet("player"), {220.f, 580.f});
    initializeLevels();
    selectLevel(0);
    player_.reset({220.f, 580.f});
    playMusic("audio/Intro Theme.mp3");
    updateTitle();
}

void Game::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        const float dt = std::min(clock.restart().asSeconds(), 0.033f);
        processEvents();
        update(dt);
        render();
    }
}

void Game::processEvents() {
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window_.close();
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            if (state_ == AppState::Menu) {
                window_.close();
            } else {
                state_ = AppState::Menu;
                enemies_.clear();
                bullets_.clear();
                enemyProjectiles_.clear();
                playMusic("audio/Intro Theme.mp3");
            }
        }

        if (state_ == AppState::Menu && event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                levelSelection_ = std::max(0, levelSelection_ - 1);
                selectLevel(static_cast<std::size_t>(levelSelection_));
            } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                levelSelection_ = std::min(static_cast<int>(levels_.size()) - 1, levelSelection_ + 1);
                selectLevel(static_cast<std::size_t>(levelSelection_));
            } else if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
                menuSelection_ = std::max(0, menuSelection_ - 1);
            } else if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) {
                menuSelection_ = std::min(1, menuSelection_ + 1);
            } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                if (menuSelection_ == 0) {
                    startRun();
                } else {
                    window_.close();
                }
            }
        }

        if (state_ == AppState::Reward && event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                rewardSelection_ = std::max(0, rewardSelection_ - 1);
            } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                rewardSelection_ = std::min(static_cast<int>(rewardOptions_.size()) - 1, rewardSelection_ + 1);
            } else if (event.key.code == sf::Keyboard::Num1) {
                applyReward(0);
            } else if (event.key.code == sf::Keyboard::Num2 && rewardOptions_.size() > 1) {
                applyReward(1);
            } else if (event.key.code == sf::Keyboard::Num3 && rewardOptions_.size() > 2) {
                applyReward(2);
            } else if (event.key.code == sf::Keyboard::Enter) {
                applyReward(static_cast<std::size_t>(rewardSelection_));
            }
        }

        if (state_ == AppState::GameOver && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            state_ = AppState::Menu;
            playMusic("audio/Intro Theme.mp3");
        }

        if (state_ == AppState::Victory && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            state_ = AppState::Menu;
            playMusic("audio/Intro Theme.mp3");
        }
    }
}

void Game::update(float dt) {
    if (state_ == AppState::Menu) {
        player_.update(dt, platforms_);
        if (player_.position().y > 620.f) {
            player_.reset({220.f, 580.f});
        }
    } else if (state_ == AppState::Intermission) {
        updateIntermission(dt);
    } else if (state_ == AppState::Wave) {
        updateWave(dt);
    }

    updateWorldView();
    updateTitle();
}

void Game::render() {
    window_.clear(clearColor_);
    renderWorld();

    window_.setView(window_.getDefaultView());
    if (state_ == AppState::Menu) {
        renderMenu();
    } else {
        if (state_ == AppState::Intermission || state_ == AppState::Wave) {
            renderHud();
        }

        if (state_ == AppState::Intermission) {
            renderIntermissionOverlay();
        } else if (state_ == AppState::Reward) {
            renderRewardOverlay();
        } else if (state_ == AppState::Victory) {
            renderVictoryOverlay();
        } else if (state_ == AppState::GameOver) {
            renderGameOverOverlay();
        }
    }

    window_.display();
}

void Game::updateIntermission(float dt) {
    player_.update(dt, platforms_);
    intermissionTimer_ -= dt;
    if (intermissionTimer_ <= 0.f) {
        startNextWave();
    }
}

void Game::updateWave(float dt) {
    player_.update(dt, platforms_);

    if (player_.tryShoot()) {
        bullets_.emplace_back(
            assets_.texturePtr("bullet"),
            player_.center() + sf::Vector2f(34.f * player_.facing(), -8.f),
            player_.facing(),
            player_.bulletDamage(),
            player_.bulletPierce()
        );
    }

    spawnTimer_ -= dt;
    if (enemiesToSpawn_ > 0 && spawnTimer_ <= 0.f) {
        spawnEnemy();
    }

    for (Bullet& bullet : bullets_) {
        bullet.update(dt, worldWidth_);
        for (Enemy& enemy : enemies_) {
            if (!bullet.isAlive() || !enemy.isAlive() || !bullet.bounds().intersects(enemy.bounds())) {
                continue;
            }

            enemy.takeDamage(bullet.damage());
            bullet.registerHit();
            if (!enemy.isAlive()) {
                ++defeatedEnemies_;
                player_.onEnemyDefeated();
            }
        }

        if (!bullet.isAlive()) {
            continue;
        }

        for (const Platform& platform : platforms_) {
            if (bullet.bounds().intersects(platform.bounds)) {
                bullet.destroy();
                break;
            }
        }
    }

    bullets_.erase(
        std::remove_if(bullets_.begin(), bullets_.end(), [](const Bullet& bullet) { return !bullet.isAlive(); }),
        bullets_.end()
    );

    for (Enemy& enemy : enemies_) {
        enemy.setTarget(player_.position());
        enemy.update(dt, platforms_);
        if (enemy.consumeAttackRequest()) {
            const sf::Vector2f origin = enemy.attackOrigin();
            sf::Vector2f toPlayer = player_.center() - origin;
            const float length = std::max(1.f, std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y));
            toPlayer.x /= length;
            toPlayer.y /= length;

            auto emitProjectile = [&](float speed, float damage, float radius, const sf::Color& color, const sf::Vector2f& direction) {
                EnemyProjectile projectile;
                projectile.shape.setRadius(radius);
                projectile.shape.setPointCount(radius >= 16.f ? 8U : 6U);
                projectile.shape.setOrigin(radius, radius);
                projectile.shape.setPosition(origin);
                projectile.shape.setFillColor(color);
                projectile.velocity = direction * speed;
                projectile.damage = damage;
                enemyProjectiles_.push_back(projectile);
            };

            if (enemy.isBoss()) {
                if (enemy.attackPattern() == 0) {
                    emitProjectile(410.f, enemy.attackPower(), 18.f, sf::Color(212, 82, 72), toPlayer);
                } else if (enemy.attackPattern() == 1) {
                    const sf::Vector2f leftSpread{toPlayer.x - 0.28f, toPlayer.y - 0.12f};
                    const sf::Vector2f rightSpread{toPlayer.x + 0.28f, toPlayer.y - 0.12f};
                    emitProjectile(360.f, enemy.attackPower() * 0.72f, 12.f, sf::Color(230, 128, 88), leftSpread);
                    emitProjectile(390.f, enemy.attackPower() * 0.82f, 14.f, sf::Color(212, 82, 72), toPlayer);
                    emitProjectile(360.f, enemy.attackPower() * 0.72f, 12.f, sf::Color(230, 128, 88), rightSpread);
                } else {
                    const std::array<float, 3> offsets{-120.f, 0.f, 120.f};
                    for (float offset : offsets) {
                        EnemyProjectile projectile;
                        projectile.shape.setRadius(16.f);
                        projectile.shape.setPointCount(8U);
                        projectile.shape.setOrigin(16.f, 16.f);
                        projectile.shape.setPosition(player_.center().x + offset, player_.center().y - 220.f);
                        projectile.shape.setFillColor(sf::Color(245, 154, 82));
                        projectile.velocity = {0.f, 210.f};
                        projectile.damage = enemy.attackPower() * 0.58f;
                        enemyProjectiles_.push_back(projectile);
                    }
                }
            } else {
                emitProjectile(280.f, enemy.attackPower(), 10.f, sf::Color(160, 96, 188), toPlayer);
            }
        }

        if (enemy.isAlive() && enemy.bounds().intersects(player_.bounds()) && enemy.canHitPlayer() && !player_.isInvulnerable()) {
            player_.receiveDamage(enemy.contactDamage());
            enemy.resetHitCooldown();
        }
    }

    updateEnemyProjectiles(dt);

    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& enemy) { return !enemy.isAlive(); }),
        enemies_.end()
    );

    if (!player_.isAlive()) {
        state_ = AppState::GameOver;
        return;
    }

    if (enemiesToSpawn_ <= 0 && enemies_.empty()) {
        if (levelCompleted_) {
            state_ = AppState::Victory;
            playMusic("audio/Worldmap Theme.mp3");
        } else {
            finishWave();
        }
    }
}

void Game::updateWorldView() {
    const float centerX = clampf(player_.position().x, kWindowWidth * 0.5f, worldWidth_ - kWindowWidth * 0.5f);
    worldView_.setCenter(centerX, kWindowHeight * 0.5f);
}

void Game::renderWorld() {
    window_.setView(worldView_);
    const sf::FloatRect visibleArea(
        worldView_.getCenter().x - worldView_.getSize().x * 0.5f - 96.f,
        worldView_.getCenter().y - worldView_.getSize().y * 0.5f - 96.f,
        worldView_.getSize().x + 192.f,
        worldView_.getSize().y + 192.f
    );

    const float viewLeft = worldView_.getCenter().x - worldView_.getSize().x * 0.5f;
    for (BackgroundLayer& layer : backgroundLayers_) {
        layer.sprite.setPosition(
            layer.basePosition.x + viewLeft * (1.f - layer.parallax),
            layer.basePosition.y
        );
        window_.draw(layer.sprite);
    }

    for (const Decoration& decoration : decorations_) {
        if (decoration.frontLayer) {
            continue;
        }
        if (!decoration.sprite.getGlobalBounds().intersects(visibleArea)) {
            continue;
        }
        window_.draw(decoration.sprite);
    }

    for (const Platform& platform : platforms_) {
        if (!platform.bounds.intersects(visibleArea)) {
            continue;
        }
        platform.draw(window_);
    }

    for (const Bullet& bullet : bullets_) {
        if (!bullet.bounds().intersects(visibleArea)) {
            continue;
        }
        bullet.draw(window_);
    }

    for (const EnemyProjectile& projectile : enemyProjectiles_) {
        if (!projectile.shape.getGlobalBounds().intersects(visibleArea)) {
            continue;
        }
        window_.draw(projectile.shape);
    }

    for (const Enemy& enemy : enemies_) {
        if (!enemy.bounds().intersects(visibleArea)) {
            continue;
        }
        enemy.draw(window_);
        enemy.drawHealthBar(window_);
    }

    if (state_ == AppState::Menu) {
        player_.draw(window_);
    } else {
        player_.draw(window_);
        player_.drawHealthBar(window_);
    }

    for (const Decoration& decoration : decorations_) {
        if (!decoration.frontLayer) {
            continue;
        }
        if (!decoration.sprite.getGlobalBounds().intersects(visibleArea)) {
            continue;
        }
        window_.draw(decoration.sprite);
    }
}

void Game::renderMenu() {
    sf::RectangleShape panel({1044.f, 520.f});
    panel.setPosition(118.f, 114.f);
    panel.setFillColor(sf::Color(9, 14, 22, 222));
    panel.setOutlineThickness(3.f);
    panel.setOutlineColor(sf::Color(228, 206, 116, 220));
    window_.draw(panel);

    sf::RectangleShape headerBand({1044.f, 84.f});
    headerBand.setPosition(118.f, 114.f);
    headerBand.setFillColor(sf::Color(14, 24, 36, 228));
    window_.draw(headerBand);

    sf::RectangleShape footerBand({1044.f, 82.f});
    footerBand.setPosition(118.f, 552.f);
    footerBand.setFillColor(sf::Color(12, 20, 30, 226));
    window_.draw(footerBand);

    drawText("THE BRINK", 42, {640.f, 154.f}, sf::Color(255, 246, 186), true);
    drawText("Choose a biome, soundtrack, and difficulty", 20, {640.f, 186.f}, sf::Color(208, 227, 255), true);

    renderLevelSelect();

    const sf::Color selected(255, 209, 82);
    const sf::Color normal(236, 239, 244);

    auto drawMenuButton = [&](const sf::Vector2f& pos, const std::string& label, bool active) {
        sf::RectangleShape button({180.f, 44.f});
        button.setPosition(pos);
        button.setFillColor(active ? sf::Color(33, 50, 66, 236) : sf::Color(18, 28, 40, 224));
        button.setOutlineThickness(2.f);
        button.setOutlineColor(active ? sf::Color(255, 209, 82) : sf::Color(86, 110, 132));
        window_.draw(button);
        drawText(label, 26, {pos.x + 90.f, pos.y + 22.f}, active ? selected : normal, true);
    };

    // drawMenuButton({360.f, 566.f}, "START", menuSelection_ == 0);
    // drawMenuButton({740.f, 566.f}, "EXIT", menuSelection_ == 1);
    // drawText("Left/Right: level   Up/Down: menu   Enter/Space: confirm", 17, {640.f, 616.f}, sf::Color(182, 205, 230), true);
    // drawText("Authors: Sergei Lebedev  Alexander Kopeykin  Alexander Butorin", 15, {640.f, 594.f}, sf::Color(172, 193, 216), true);
    // Было 566.f -> Сдвигаем кнопки чуть ниже, например на 545.f, чтобы они сели на нижнюю полосу (footerBand)
    // Кнопки START и EXIT встают идеально по центру нижней полосы (footerBand)
    drawMenuButton({360.f, 485.f}, "START", menuSelection_ == 0);
    drawMenuButton({740.f, 485.f}, "EXIT", menuSelection_ == 1);
    drawText("Authors: Sergei Lebedev, Alexander Kopeykin, Alexander Butorin", 15, {640.f, 570.f}, sf::Color(172, 193, 216), true);
    drawText("Left/Right: level   Up/Down: menu   Enter/Space: confirm", 17, {640.f, 600.f}, sf::Color(182, 205, 230), true);
}

void Game::renderLevelSelect() {
    const float cardWidth = 136.f;
    const float cardHeight = 214.f;
    const float gap = 12.f;
    const float totalWidth = cardWidth * static_cast<float>(levels_.size()) + gap * static_cast<float>(levels_.size() - 1);
    const float startX = (static_cast<float>(kWindowWidth) - totalWidth) * 0.5f;

    for (std::size_t i = 0; i < levels_.size(); ++i) {
        const bool selected = static_cast<int>(i) == levelSelection_;
        sf::RectangleShape card({cardWidth, cardHeight});
        card.setPosition(startX + static_cast<float>(i) * (cardWidth + gap), 236.f);
        card.setFillColor(multiplyColor(levels_[i].skyColor, selected ? sf::Color(225, 225, 225) : sf::Color(165, 165, 165)));
        card.setOutlineThickness(selected ? 3.f : 2.f);
        card.setOutlineColor(selected ? sf::Color(255, 226, 128) : sf::Color(220, 230, 244, 130));
        window_.draw(card);

        const sf::Vector2f cardPos = card.getPosition();
        const float previewInset = 8.f;
        const float previewWidth = cardWidth - previewInset * 2.f;
        const float previewHeight = 132.f;
        const sf::Color previewSky = levels_[i].skyColor;
        const sf::Color previewHill = multiplyColor(levels_[i].layerTint, sf::Color(92, 134, 92));
        const sf::Color previewFront = multiplyColor(levels_[i].propTint, sf::Color(44, 79, 57));

        sf::RectangleShape previewBg({previewWidth, previewHeight});
        previewBg.setPosition(cardPos.x + previewInset, cardPos.y + previewInset);
        previewBg.setFillColor(previewSky);
        window_.draw(previewBg);

        sf::CircleShape cloudA(10.f);
        cloudA.setScale(1.8f, 1.0f);
        cloudA.setPosition(cardPos.x + 14.f, cardPos.y + 18.f);
        cloudA.setFillColor(sf::Color(246, 243, 236, 230));
        window_.draw(cloudA);

        sf::CircleShape cloudB(8.f);
        cloudB.setScale(1.7f, 1.0f);
        cloudB.setPosition(cardPos.x + 76.f, cardPos.y + 28.f);
        cloudB.setFillColor(sf::Color(246, 243, 236, 210));
        window_.draw(cloudB);

        sf::CircleShape hillLeft(34.f);
        hillLeft.setScale(1.35f, 1.0f);
        hillLeft.setPosition(cardPos.x + 4.f, cardPos.y + 54.f);
        hillLeft.setFillColor(previewHill);
        window_.draw(hillLeft);

        sf::CircleShape hillRight(28.f);
        hillRight.setScale(1.6f, 1.0f);
        hillRight.setPosition(cardPos.x + 54.f, cardPos.y + 62.f);
        hillRight.setFillColor(previewHill);
        window_.draw(hillRight);

        sf::RectangleShape ground({previewWidth, 34.f});
        ground.setPosition(cardPos.x + previewInset, cardPos.y + 102.f);
        ground.setFillColor(previewFront);
        window_.draw(ground);

        sf::RectangleShape platformA({34.f, 6.f});
        platformA.setPosition(cardPos.x + 16.f, cardPos.y + 86.f);
        platformA.setFillColor(sf::Color(82, 58, 72));
        window_.draw(platformA);

        sf::RectangleShape platformB({28.f, 6.f});
        platformB.setPosition(cardPos.x + 72.f, cardPos.y + 72.f);
        platformB.setFillColor(sf::Color(82, 58, 72));
        window_.draw(platformB);

        switch (levels_[i].theme) {
            case LevelTheme::Grasslands: {
                sf::CircleShape bush(12.f);
                bush.setScale(1.4f, 1.1f);
                bush.setPosition(cardPos.x + 74.f, cardPos.y + 102.f);
                bush.setFillColor(sf::Color(26, 204, 91));
                window_.draw(bush);
                break;
            }
            case LevelTheme::OakWoods: {
                sf::RectangleShape trunk({6.f, 32.f});
                trunk.setPosition(cardPos.x + 58.f, cardPos.y + 72.f);
                trunk.setFillColor(sf::Color(84, 58, 48));
                window_.draw(trunk);
                sf::CircleShape crown(15.f);
                crown.setScale(1.4f, 1.25f);
                crown.setPosition(cardPos.x + 45.f, cardPos.y + 52.f);
                crown.setFillColor(sf::Color(38, 104, 82));
                window_.draw(crown);
                break;
            }
            case LevelTheme::MushroomGrove: {
                sf::CircleShape cap(10.f);
                cap.setScale(1.3f, 0.9f);
                cap.setPosition(cardPos.x + 52.f, cardPos.y + 104.f);
                cap.setFillColor(sf::Color(220, 110, 200));
                window_.draw(cap);
                sf::RectangleShape stem({5.f, 10.f});
                stem.setPosition(cardPos.x + 60.f, cardPos.y + 114.f);
                stem.setFillColor(sf::Color(232, 220, 186));
                window_.draw(stem);
                break;
            }
            case LevelTheme::DesertRuins: {
                sf::RectangleShape dune({previewWidth, 16.f});
                dune.setPosition(cardPos.x + previewInset, cardPos.y + 94.f);
                dune.setFillColor(sf::Color(214, 171, 96));
                window_.draw(dune);
                sf::RectangleShape cactus({5.f, 20.f});
                cactus.setPosition(cardPos.x + 78.f, cardPos.y + 90.f);
                cactus.setFillColor(sf::Color(56, 135, 86));
                window_.draw(cactus);
                break;
            }
            case LevelTheme::Iceland: {
                sf::RectangleShape snow({previewWidth, 14.f});
                snow.setPosition(cardPos.x + previewInset, cardPos.y + 94.f);
                snow.setFillColor(sf::Color(229, 244, 255));
                window_.draw(snow);
                sf::CircleShape ice(11.f);
                ice.setScale(1.3f, 1.0f);
                ice.setPosition(cardPos.x + 70.f, cardPos.y + 102.f);
                ice.setFillColor(sf::Color(198, 236, 255));
                window_.draw(ice);
                break;
            }
            case LevelTheme::Dungeon: {
                sf::RectangleShape pillar({8.f, 26.f});
                pillar.setPosition(cardPos.x + 60.f, cardPos.y + 86.f);
                pillar.setFillColor(sf::Color(88, 75, 112));
                window_.draw(pillar);
                break;
            }
            case LevelTheme::BossArena: {
                sf::CircleShape moon(10.f);
                moon.setPosition(cardPos.x + 78.f, cardPos.y + 18.f);
                moon.setFillColor(sf::Color(238, 214, 180));
                window_.draw(moon);
                sf::RectangleShape lava({previewWidth, 8.f});
                lava.setPosition(cardPos.x + previewInset, cardPos.y + 118.f);
                lava.setFillColor(sf::Color(180, 62, 56));
                window_.draw(lava);
                break;
            }
        }

        sf::RectangleShape strip({cardWidth, 44.f});
        strip.setPosition(card.getPosition().x, card.getPosition().y + cardHeight - 42.f);
        strip.setFillColor(sf::Color(8, 12, 18, 196));
        window_.draw(strip);

        drawText(levels_[i].title, 16, {card.getPosition().x + cardWidth * 0.5f, card.getPosition().y + 176.f}, sf::Color::White, true);
        if (selected) {
            drawText(levels_[i].subtitle, 18, {640.f, 216.f}, sf::Color(223, 238, 255), true);
        }
    }
}

void Game::renderHud() {
    sf::RectangleShape panel({648.f, 128.f});
    panel.setPosition(18.f, 18.f);
    panel.setFillColor(sf::Color(13, 20, 29, 204));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(82, 115, 142, 168));
    window_.draw(panel);

    sf::RectangleShape titleBand({648.f, 34.f});
    titleBand.setPosition(18.f, 18.f);
    titleBand.setFillColor(sf::Color(18, 31, 45, 220));
    window_.draw(titleBand);

    const LevelDefinition& level = levels_[activeLevelIndex_];
    drawText(level.title, 20, {38.f, 24.f}, sf::Color(220, 236, 252));
    drawText("Wave " + std::to_string(waveNumber_), 19, {208.f, 24.f}, sf::Color(255, 229, 144));
    drawText("Kills " + std::to_string(defeatedEnemies_), 19, {306.f, 24.f}, sf::Color(198, 224, 255));
    if (state_ == AppState::Wave) {
        drawText("Enemies " + std::to_string(enemiesToSpawn_ + static_cast<int>(enemies_.size())), 19, {426.f, 24.f}, sf::Color(255, 205, 156));
    } else {
        drawText("Next " + std::to_string(static_cast<int>(std::ceil(intermissionTimer_))), 19, {426.f, 24.f}, sf::Color(255, 205, 156));
    }

    sf::RectangleShape barBack({246.f, 22.f});
    barBack.setPosition(38.f, 64.f);
    barBack.setFillColor(sf::Color(61, 30, 34, 220));
    window_.draw(barBack);

    sf::RectangleShape barFill({246.f * player_.healthRatio(), 22.f});
    barFill.setPosition(38.f, 64.f);
    barFill.setFillColor(sf::Color(90, 214, 116, 244));
    window_.draw(barFill);

    drawText("HP " + std::to_string(static_cast<int>(std::ceil(player_.health()))) + "/" + std::to_string(static_cast<int>(player_.maxHealth())), 17, {48.f, 66.f}, sf::Color(248, 248, 248));

    const std::array<std::pair<std::string, sf::Color>, 5> stats{{
        {"ATK " + std::to_string(static_cast<int>(std::round(player_.bulletDamage()))), sf::Color(255, 186, 128)},
        {"SPD " + std::to_string(static_cast<int>(std::round(player_.moveSpeed()))), sf::Color(182, 244, 197)},
        {"JMP " + std::to_string(static_cast<int>(std::round(player_.jumpStrength()))), sf::Color(210, 205, 255)},
        {"ARM " + std::to_string(static_cast<int>(std::round((1.f - player_.armorMultiplier()) * 100.f))) + "%", sf::Color(189, 218, 255)},
        {player_.doubleJumpUnlocked() ? "AIR READY" : "AIR LOCK", player_.doubleJumpUnlocked() ? sf::Color(252, 214, 140) : sf::Color(144, 158, 176)}
    }};

    float chipX = 298.f;
    for (const auto& [label, color] : stats) {
        sf::RectangleShape chip({68.f, 28.f});
        chip.setPosition(chipX, 60.f);
        chip.setFillColor(sf::Color(22, 34, 47, 228));
        chip.setOutlineThickness(1.f);
        chip.setOutlineColor(sf::Color(70, 92, 112, 170));
        window_.draw(chip);
        drawText(label, 12, {chipX + 8.f, 68.f}, color);
        chipX += 72.f;
    }

    drawText("Move A/D  Jump Space  Shoot F/Ctrl  Dash Shift", 15, {38.f, 104.f}, sf::Color(164, 186, 208));

    const Enemy* boss = nullptr;
    for (const Enemy& enemy : enemies_) {
        if (enemy.isBoss()) {
            boss = &enemy;
            break;
        }
    }

    if (boss) {
        sf::RectangleShape bossPanel({420.f, 44.f});
        bossPanel.setPosition(430.f, 18.f);
        bossPanel.setFillColor(sf::Color(22, 18, 24, 216));
        bossPanel.setOutlineThickness(2.f);
        bossPanel.setOutlineColor(sf::Color(156, 86, 76, 196));
        window_.draw(bossPanel);

        sf::RectangleShape bossBarBack({332.f, 14.f});
        bossBarBack.setPosition(502.f, 36.f);
        bossBarBack.setFillColor(sf::Color(72, 30, 34, 234));
        window_.draw(bossBarBack);

        sf::RectangleShape bossBarFill({332.f * boss->healthRatio(), 14.f});
        bossBarFill.setPosition(502.f, 36.f);
        bossBarFill.setFillColor(sf::Color(214, 82, 68, 244));
        window_.draw(bossBarFill);

        drawText("BOSS", 17, {448.f, 28.f}, sf::Color(255, 221, 182));
        drawText(std::to_string(static_cast<int>(std::ceil(boss->health()))) + "/" + std::to_string(static_cast<int>(boss->maxHealth())), 14, {726.f, 28.f}, sf::Color(248, 230, 222));
    }
}

void Game::renderIntermissionOverlay() {
    sf::RectangleShape box({520.f, 110.f});
    box.setPosition(380.f, 68.f);
    box.setFillColor(sf::Color(23, 33, 42, 214));
    window_.draw(box);

    const bool bossNext = !bossSpawned_ && (waveNumber_ + 1) >= levels_[activeLevelIndex_].bossWave;
    drawText(bossNext ? "Boss wave incoming" : "Prepare for wave " + std::to_string(waveNumber_ + 1), 28, {640.f, 105.f}, sf::Color(255, 244, 181), true);
    drawText(bossNext ? "A large enemy with ranged attacks is waiting ahead." : levels_[activeLevelIndex_].subtitle, 21, {640.f, 142.f}, sf::Color(202, 225, 246), true);
}

void Game::renderRewardOverlay() {
    sf::RectangleShape overlay({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    overlay.setFillColor(sf::Color(8, 13, 20, 180));
    window_.draw(overlay);

    drawText("Wave cleared. Choose an upgrade", 34, {640.f, 72.f}, sf::Color(255, 242, 164), true);

    const float startX = 180.f;
    for (std::size_t i = 0; i < rewardOptions_.size(); ++i) {
        const bool selected = static_cast<int>(i) == rewardSelection_;
        sf::RectangleShape card({280.f, 220.f});
        card.setPosition(startX + static_cast<float>(i) * 320.f, 180.f);
        card.setFillColor(selected ? sf::Color(47, 72, 92, 236) : sf::Color(22, 31, 42, 224));
        card.setOutlineThickness(4.f);
        card.setOutlineColor(selected ? sf::Color(255, 221, 111) : sf::Color(92, 123, 147));
        window_.draw(card);

        drawText(std::to_string(static_cast<int>(i) + 1), 22, {startX + static_cast<float>(i) * 320.f + 22.f, 198.f}, sf::Color(255, 225, 123));
        drawText(rewardOptions_[i].title, 27, {startX + static_cast<float>(i) * 320.f + 140.f, 248.f}, sf::Color(246, 249, 254), true);
        drawText(rewardOptions_[i].description, 19, {startX + static_cast<float>(i) * 320.f + 28.f, 310.f}, sf::Color(205, 221, 236));
    }
}

void Game::renderVictoryOverlay() {
    sf::RectangleShape overlay({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    overlay.setFillColor(sf::Color(8, 28, 16, 176));
    window_.draw(overlay);

    sf::RectangleShape panel({620.f, 210.f});
    panel.setPosition(330.f, 200.f);
    panel.setFillColor(sf::Color(18, 52, 34, 228));
    panel.setOutlineThickness(4.f);
    panel.setOutlineColor(sf::Color(232, 214, 122));
    window_.draw(panel);

    drawText("Level Cleared", 40, {640.f, 252.f}, sf::Color(255, 244, 188), true);
    drawText(levels_[activeLevelIndex_].title + " boss defeated", 26, {640.f, 308.f}, sf::Color(232, 245, 236), true);
    drawText("Press Enter to return to menu", 22, {640.f, 360.f}, sf::Color(255, 214, 144), true);
}

void Game::renderGameOverOverlay() {
    sf::RectangleShape overlay({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    overlay.setFillColor(sf::Color(34, 8, 8, 170));
    window_.draw(overlay);

    sf::RectangleShape panel({560.f, 190.f});
    panel.setPosition(360.f, 210.f);
    panel.setFillColor(sf::Color(53, 21, 21, 230));
    panel.setOutlineThickness(4.f);
    panel.setOutlineColor(sf::Color(197, 85, 69));
    window_.draw(panel);

    drawText("Run over", 40, {640.f, 255.f}, sf::Color(255, 227, 200), true);
    drawText(levels_[activeLevelIndex_].title + " | Wave " + std::to_string(waveNumber_), 24, {640.f, 308.f}, sf::Color(245, 245, 245), true);
    drawText("Press Enter to return to menu", 22, {640.f, 350.f}, sf::Color(255, 208, 136), true);
}

void Game::startRun() {
    restartCampaign();
    state_ = AppState::Intermission;
    intermissionTimer_ = 2.6f;
    playMusic(levels_[activeLevelIndex_].musicFile);
}

void Game::restartCampaign() {
    player_.resetProgression({220.f, 580.f});
    bullets_.clear();
    enemyProjectiles_.clear();
    enemies_.clear();
    rewardOptions_.clear();
    waveNumber_ = 0;
    defeatedEnemies_ = 0;
    enemiesToSpawn_ = 0;
    spawnTimer_ = 0.f;
    rewardSelection_ = 0;
    bossSpawned_ = false;
    levelCompleted_ = false;
    buildLevel();
    buildBackground();
    buildDecorations();
}

void Game::startNextWave() {
    ++waveNumber_;
    state_ = AppState::Wave;
    playMusic(levels_[activeLevelIndex_].musicFile);

    if (!bossSpawned_ && waveNumber_ >= levels_[activeLevelIndex_].bossWave) {
        enemiesToSpawn_ = 0;
        spawnBoss();
        bossSpawned_ = true;
        levelCompleted_ = true;
        playMusic("audio/Boss Theme.mp3");
        return;
    }

    enemiesToSpawn_ = 4 + waveNumber_ * 2 + levels_[activeLevelIndex_].waveBonus;
    spawnTimer_ = 0.4f;
}

void Game::finishWave() {
    generateRewardOptions();
    state_ = AppState::Reward;
    playMusic("audio/Worldmap Theme.mp3");
}

void Game::buildLevel() {
    platforms_.clear();
    platforms_.emplace_back(assets_.texture("platform_long"), sf::Vector2f(0.f, kGroundTop), sf::Vector2f(worldWidth_, 84.f));

    std::vector<sf::Vector2f> smallPlatforms{
        {360.f, 560.f}, {760.f, 500.f}, {1180.f, 430.f}, {1580.f, 540.f}, {1940.f, 470.f},
        {2320.f, 400.f}, {2700.f, 520.f}, {3100.f, 450.f}, {3480.f, 360.f}, {3920.f, 540.f},
        {4300.f, 480.f}, {4680.f, 390.f}, {5100.f, 500.f}, {5520.f, 430.f}, {5900.f, 350.f}
    };
    std::vector<sf::Vector2f> longPlatforms{
        {900.f, 620.f}, {2100.f, 610.f}, {3300.f, 610.f}, {4540.f, 610.f}
    };

    switch (levels_[activeLevelIndex_].theme) {
        case LevelTheme::OakWoods:
            for (auto& pos : smallPlatforms) {
                pos.y -= (static_cast<int>(pos.x) / 400 % 2 == 0) ? 14.f : -6.f;
            }
            break;
        case LevelTheme::MushroomGrove:
            for (auto& pos : smallPlatforms) {
                pos.y -= 18.f;
            }
            break;
        case LevelTheme::DesertRuins:
            for (auto& pos : longPlatforms) {
                pos.y -= 12.f;
            }
            break;
        case LevelTheme::Iceland:
            for (auto& pos : smallPlatforms) {
                pos.y += (static_cast<int>(pos.x) / 500 % 2 == 0) ? 8.f : -22.f;
            }
            break;
        case LevelTheme::Dungeon:
            for (auto& pos : smallPlatforms) {
                pos.y -= (static_cast<int>(pos.x) / 300 % 3) * 10.f;
            }
            break;
        case LevelTheme::BossArena:
            smallPlatforms = {
                {700.f, 520.f}, {1260.f, 420.f}, {1820.f, 520.f}, {3000.f, 520.f}, {3560.f, 420.f}, {4120.f, 520.f}
            };
            longPlatforms = {
                {960.f, 610.f}, {2440.f, 610.f}, {3920.f, 610.f}
            };
            break;
        case LevelTheme::Grasslands:
            break;
    }

    for (const sf::Vector2f& position : smallPlatforms) {
        platforms_.emplace_back(assets_.texture("platform_small"), position, sf::Vector2f(220.f, 30.f));
    }

    for (const sf::Vector2f& position : longPlatforms) {
        platforms_.emplace_back(assets_.texture("platform_long"), position, sf::Vector2f(340.f, 36.f));
    }

    platforms_.emplace_back(assets_.texture("platform_small"), sf::Vector2f(96.f, kPreviewPedestalY), sf::Vector2f(188.f, 28.f));
}

void Game::buildBackground() {
    backgroundLayers_.clear();
    const LevelDefinition& level = levels_[activeLevelIndex_];

    auto addTiledLayer = [&](const char* id, float parallax, float scale, float bottom, const sf::Color& tint, float widthPadding = 256.f) {
        const sf::Texture& texture = assets_.texture(id);
        const float y = bottom - static_cast<float>(texture.getSize().y) * scale;
        BackgroundLayer layer;
        layer.parallax = parallax;
        layer.tint = tint;
        layer.sprite.setTexture(texture);
        layer.sprite.setTextureRect(sf::IntRect(
            0,
            0,
            static_cast<int>((worldWidth_ + widthPadding * 2.f) / scale),
            static_cast<int>(texture.getSize().y)
        ));
        layer.sprite.setScale(scale, scale);
        layer.basePosition = {-widthPadding, y};
        layer.sprite.setPosition(layer.basePosition);
        layer.sprite.setColor(tint);
        backgroundLayers_.push_back(layer);
    };

    auto addCloud = [&](const char* id, float parallax, float scale, const sf::Vector2f& position, const sf::Color& tint) {
        BackgroundLayer layer;
        layer.parallax = parallax;
        layer.tint = tint;
        layer.sprite.setTexture(assets_.texture(id));
        layer.sprite.setScale(scale, scale);
        layer.basePosition = position;
        layer.sprite.setPosition(position);
        layer.sprite.setColor(tint);
        backgroundLayers_.push_back(layer);
    };

    switch (level.theme) {
        case LevelTheme::Grasslands:
            addTiledLayer("bg_back_1", 0.04f, 3.5f, kGroundTop + 20.f, sf::Color(255, 255, 255));
            addTiledLayer("bg_back_4", 0.11f, 3.18f, kGroundTop + 8.f, sf::Color(244, 250, 244));
            addTiledLayer("oak_bg_3", 0.20f, 3.85f, kGroundTop + 2.f, sf::Color(98, 146, 106));
            break;
        case LevelTheme::OakWoods:
            addTiledLayer("oak_bg_1", 0.04f, 4.0f, kGroundTop + 12.f, level.layerTint);
            addTiledLayer("oak_bg_2", 0.12f, 4.0f, kGroundTop + 6.f, level.layerTint);
            addTiledLayer("oak_bg_3", 0.24f, 4.0f, kGroundTop, level.layerTint);
            break;
        case LevelTheme::MushroomGrove:
            addTiledLayer("bg_back_1", 0.04f, 3.5f, kGroundTop + 20.f, sf::Color(255, 236, 255));
            addTiledLayer("oak_bg_2", 0.11f, 3.85f, kGroundTop + 10.f, sf::Color(154, 104, 174));
            addTiledLayer("oak_bg_3", 0.20f, 3.85f, kGroundTop + 2.f, sf::Color(102, 62, 128));
            break;
        case LevelTheme::DesertRuins:
            addTiledLayer("bg_back_1", 0.04f, 3.5f, kGroundTop + 20.f, sf::Color(255, 246, 220));
            addTiledLayer("oak_bg_2", 0.11f, 3.85f, kGroundTop + 10.f, sf::Color(176, 138, 92));
            addTiledLayer("oak_bg_3", 0.20f, 3.85f, kGroundTop + 2.f, sf::Color(116, 82, 56));
            break;
        case LevelTheme::Iceland:
            addTiledLayer("bg_back_1", 0.04f, 3.5f, kGroundTop + 20.f, sf::Color(248, 252, 255));
            addTiledLayer("oak_bg_2", 0.11f, 3.85f, kGroundTop + 10.f, sf::Color(148, 184, 206));
            addTiledLayer("oak_bg_3", 0.20f, 3.85f, kGroundTop + 2.f, sf::Color(92, 126, 156));
            break;
        case LevelTheme::Dungeon:
            addTiledLayer("oak_bg_1", 0.04f, 4.0f, kGroundTop + 12.f, level.layerTint);
            addTiledLayer("oak_bg_2", 0.12f, 4.0f, kGroundTop + 6.f, level.layerTint);
            addTiledLayer("oak_bg_3", 0.24f, 4.0f, kGroundTop, level.layerTint);
            break;
        case LevelTheme::BossArena:
            addTiledLayer("oak_bg_1", 0.04f, 4.0f, kGroundTop + 12.f, level.layerTint);
            addTiledLayer("oak_bg_2", 0.12f, 4.0f, kGroundTop + 6.f, level.layerTint);
            addTiledLayer("oak_bg_3", 0.24f, 4.0f, kGroundTop, level.layerTint);
            break;
    }

    if (level.theme != LevelTheme::Dungeon && level.theme != LevelTheme::BossArena) {
        addCloud("bg_cloud_1", 0.10f, 1.8f, {120.f, 96.f}, level.propTint);
        addCloud("bg_cloud_2", 0.12f, 1.6f, {840.f, 72.f}, level.propTint);
        addCloud("bg_cloud_1", 0.11f, 2.0f, {1880.f, 108.f}, level.propTint);
        addCloud("bg_cloud_2", 0.14f, 1.5f, {3160.f, 86.f}, level.propTint);
        addCloud("bg_cloud_1", 0.13f, 1.9f, {4540.f, 66.f}, level.propTint);
        addCloud("bg_cloud_2", 0.15f, 1.7f, {5860.f, 102.f}, level.propTint);
    }
}

void Game::buildDecorations() {
    decorations_.clear();
    const LevelDefinition& level = levels_[activeLevelIndex_];

    auto addDecoration = [&](const std::string& id, const sf::Vector2f& footPosition, float scale, const sf::Color& tint, bool frontLayer) {
        Decoration decoration;
        decoration.sprite.setTexture(assets_.texture(id));
        const sf::Vector2u size = decoration.sprite.getTexture()->getSize();
        float bottomTrim = 0.f;
        if (id == "tree" || id == "pine" || id == "palm") {
            bottomTrim = 2.f;
        } else if (id == "bush" || id == "shrooms") {
            bottomTrim = 4.f;
        } else if (id == "grass1" || id == "grass2" || id == "grass3" || id == "grass4"
                   || id == "oak_grass_1" || id == "oak_grass_2" || id == "oak_grass_3"
                   || id == "flower") {
            bottomTrim = 3.f;
        } else if (id == "rock" || id == "rock2" || id == "rock3"
                   || id == "oak_rock_1" || id == "oak_rock_2" || id == "oak_rock_3"
                   || id == "crate" || id == "sign" || id == "oak_sign" || id == "oak_lamp") {
            bottomTrim = 1.f;
        }

        decoration.sprite.setOrigin(static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) - bottomTrim);
        decoration.sprite.setScale(scale, scale);
        decoration.sprite.setPosition(footPosition);
        decoration.sprite.setColor(tint);
        decoration.frontLayer = frontLayer;
        decorations_.push_back(decoration);
    };

    switch (level.theme) {
        case LevelTheme::Grasslands:
            addDecoration("tree", {150.f, 678.f}, 2.35f, level.propTint, false);
            addDecoration("bush", {430.f, 670.f}, 3.0f, level.propTint, false);
            addDecoration("grass1", {620.f, 670.f}, 2.8f, level.propTint, true);
            addDecoration("flower", {744.f, 672.f}, 3.0f, level.propTint, true);
            addDecoration("tree", {940.f, 678.f}, 2.35f, level.propTint, false);
            addDecoration("rock", {1240.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("grass2", {1500.f, 670.f}, 2.8f, level.propTint, true);
            addDecoration("crate", {1760.f, 672.f}, 1.6f, level.propTint, false);
            addDecoration("tree", {2050.f, 678.f}, 2.35f, level.propTint, false);
            addDecoration("sign", {2860.f, 672.f}, 1.7f, level.propTint, false);
            addDecoration("tree", {4250.f, 678.f}, 2.35f, level.propTint, false);
            addDecoration("rock3", {4820.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("tree", {5950.f, 678.f}, 2.35f, level.propTint, false);
            break;
        case LevelTheme::OakWoods:
            addDecoration("pine", {170.f, 676.f}, 3.0f, level.propTint, false);
            addDecoration("oak_grass_1", {470.f, 668.f}, 2.6f, level.propTint, true);
            addDecoration("oak_lamp", {740.f, 673.f}, 2.1f, level.propTint, false);
            addDecoration("oak_rock_2", {1110.f, 672.f}, 2.5f, level.propTint, false);
            addDecoration("pine", {1540.f, 676.f}, 3.0f, level.propTint, false);
            addDecoration("oak_sign", {2050.f, 673.f}, 2.1f, level.propTint, false);
            addDecoration("oak_grass_3", {2460.f, 668.f}, 2.6f, level.propTint, true);
            addDecoration("oak_rock_1", {2960.f, 672.f}, 2.5f, level.propTint, false);
            addDecoration("pine", {3560.f, 676.f}, 3.0f, level.propTint, false);
            addDecoration("oak_grass_2", {4280.f, 668.f}, 2.6f, level.propTint, true);
            addDecoration("oak_rock_3", {5120.f, 672.f}, 2.5f, level.propTint, false);
            addDecoration("pine", {5980.f, 676.f}, 3.0f, level.propTint, false);
            break;
        case LevelTheme::MushroomGrove:
            addDecoration("shrooms", {180.f, 670.f}, 2.5f, level.propTint, true);
            addDecoration("bush", {420.f, 666.f}, 2.5f, level.propTint, false);
            addDecoration("flower", {760.f, 668.f}, 2.6f, level.propTint, true);
            addDecoration("tree", {1260.f, 676.f}, 1.95f, level.propTint, false);
            addDecoration("shrooms", {1680.f, 670.f}, 2.5f, level.propTint, true);
            addDecoration("bush", {2320.f, 666.f}, 2.5f, level.propTint, false);
            addDecoration("flower", {2940.f, 668.f}, 2.6f, level.propTint, true);
            addDecoration("tree", {3600.f, 676.f}, 1.95f, level.propTint, false);
            addDecoration("shrooms", {4300.f, 670.f}, 2.5f, level.propTint, true);
            addDecoration("flower", {5000.f, 668.f}, 2.6f, level.propTint, true);
            addDecoration("tree", {5900.f, 676.f}, 1.95f, level.propTint, false);
            break;
        case LevelTheme::DesertRuins:
            addDecoration("palm", {200.f, 676.f}, 3.0f, level.propTint, false);
            addDecoration("rock2", {520.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("sign", {860.f, 674.f}, 1.8f, level.propTint, false);
            addDecoration("palm", {1480.f, 676.f}, 3.0f, level.propTint, false);
            addDecoration("crate", {2080.f, 672.f}, 1.8f, level.propTint, false);
            addDecoration("rock3", {2860.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("palm", {3540.f, 676.f}, 3.0f, level.propTint, false);
            addDecoration("rock", {4240.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("crate", {5120.f, 672.f}, 1.8f, level.propTint, false);
            addDecoration("palm", {5960.f, 676.f}, 3.0f, level.propTint, false);
            break;
        case LevelTheme::Iceland:
            addDecoration("pine", {180.f, 676.f}, 2.85f, level.propTint, false);
            addDecoration("rock3", {480.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("grass4", {820.f, 668.f}, 2.5f, level.propTint, true);
            addDecoration("pine", {1460.f, 676.f}, 2.85f, level.propTint, false);
            addDecoration("spikes", {2040.f, 674.f}, 2.0f, level.propTint, true);
            addDecoration("rock2", {2920.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("pine", {3740.f, 676.f}, 2.85f, level.propTint, false);
            addDecoration("grass2", {4660.f, 668.f}, 2.5f, level.propTint, true);
            addDecoration("rock", {5420.f, 672.f}, 3.0f, level.propTint, false);
            addDecoration("pine", {6060.f, 676.f}, 2.85f, level.propTint, false);
            break;
        case LevelTheme::Dungeon:
            addDecoration("skulls", {220.f, 674.f}, 2.4f, level.propTint, true);
            addDecoration("spikes", {540.f, 674.f}, 2.1f, level.propTint, true);
            addDecoration("crate", {980.f, 672.f}, 1.8f, level.propTint, false);
            addDecoration("skulls", {1680.f, 674.f}, 2.4f, level.propTint, true);
            addDecoration("rock2", {2480.f, 672.f}, 2.9f, level.propTint, false);
            addDecoration("spikes", {3240.f, 674.f}, 2.1f, level.propTint, true);
            addDecoration("crate", {4080.f, 672.f}, 1.8f, level.propTint, false);
            addDecoration("skulls", {4860.f, 674.f}, 2.4f, level.propTint, true);
            addDecoration("rock3", {5640.f, 672.f}, 2.9f, level.propTint, false);
            addDecoration("spikes", {6180.f, 674.f}, 2.1f, level.propTint, true);
            break;
        case LevelTheme::BossArena:
            addDecoration("skulls", {260.f, 674.f}, 2.7f, level.propTint, true);
            addDecoration("spikes", {860.f, 674.f}, 2.2f, level.propTint, true);
            addDecoration("rock3", {1740.f, 672.f}, 3.2f, level.propTint, false);
            addDecoration("skulls", {2820.f, 674.f}, 2.7f, level.propTint, true);
            addDecoration("spikes", {3940.f, 674.f}, 2.2f, level.propTint, true);
            addDecoration("rock2", {5120.f, 672.f}, 3.2f, level.propTint, false);
            addDecoration("skulls", {6020.f, 674.f}, 2.7f, level.propTint, true);
            break;
    }
}

void Game::spawnEnemy() {
    const LevelDefinition& level = levels_[activeLevelIndex_];
    std::uniform_real_distribution<float> sideRoll(0.f, 1.f);
    std::uniform_real_distribution<float> offsetRoll(620.f, 980.f);
    std::uniform_real_distribution<float> jitterRoll(-40.f, 40.f);
    std::uniform_real_distribution<float> speedRoll(95.f, 145.f);
    std::uniform_real_distribution<float> typeRoll(0.f, 1.f);

    const bool spawnRight = sideRoll(rng_) > 0.5f;
    float x = player_.position().x + (spawnRight ? offsetRoll(rng_) : -offsetRoll(rng_));
    x = clampf(x, 70.f, worldWidth_ - 70.f);

    const float healthScale = level.enemyHealthMultiplier;
    const float damageScale = level.enemyDamageMultiplier;
    const float speedBonus = level.enemySpeedBonus;
    const float roll = typeRoll(rng_);

    if ((waveNumber_ >= 4 || level.theme == LevelTheme::BossArena) && roll > 0.78f) {
        enemies_.emplace_back(
            &assets_.animationSet("golem"),
            EnemyType::Golem,
            sf::Vector2f(x, 610.f),
            74.f + speedBonus * 0.6f + waveNumber_ * 3.f,
            (140.f + waveNumber_ * 20.f) * healthScale,
            (20.f + waveNumber_ * 2.8f) * damageScale,
            worldWidth_
        );
    } else if (waveNumber_ >= 2 && roll > 0.48f) {
        enemies_.emplace_back(
            &assets_.animationSet("bat"),
            EnemyType::Bat,
            sf::Vector2f(x, 260.f + jitterRoll(rng_)),
            120.f + speedBonus + waveNumber_ * 10.f,
            (34.f + waveNumber_ * 8.f) * healthScale,
            (10.f + waveNumber_ * 1.5f) * damageScale,
            worldWidth_
        );
    } else {
        enemies_.emplace_back(
            &assets_.animationSet("mushroom"),
            EnemyType::Mushroom,
            sf::Vector2f(x, 610.f),
            speedRoll(rng_) + speedBonus + waveNumber_ * 5.f,
            (42.f + waveNumber_ * 9.f) * healthScale,
            (12.f + waveNumber_ * 1.8f) * damageScale,
            worldWidth_
        );
    }

    --enemiesToSpawn_;
    spawnTimer_ = std::max(0.34f, 1.0f - waveNumber_ * 0.04f);
}

void Game::spawnBoss() {
    const LevelDefinition& level = levels_[activeLevelIndex_];
    enemies_.emplace_back(
        &assets_.animationSet("golem"),
        EnemyType::BossGolem,
        sf::Vector2f(4520.f, 606.f),
        112.f + level.enemySpeedBonus,
        980.f * level.enemyHealthMultiplier,
        42.f * level.enemyDamageMultiplier,
        worldWidth_
    );
    if (level.theme == LevelTheme::BossArena || level.waveBonus >= 2) {
        enemies_.emplace_back(
            &assets_.animationSet("bat"),
            EnemyType::Bat,
            sf::Vector2f(5080.f, 240.f),
            180.f + level.enemySpeedBonus,
            90.f * level.enemyHealthMultiplier,
            18.f * level.enemyDamageMultiplier,
            worldWidth_
        );
    }
}

void Game::updateEnemyProjectiles(float dt) {
    for (EnemyProjectile& projectile : enemyProjectiles_) {
        if (!projectile.alive) {
            continue;
        }

        if (projectile.velocity.x != 0.f) {
            projectile.velocity.y += 420.f * dt;
        }
        projectile.shape.move(projectile.velocity * dt);

        const sf::Vector2f pos = projectile.shape.getPosition();
        if (pos.x < -64.f || pos.x > worldWidth_ + 64.f || pos.y > kWindowHeight + 240.f) {
            projectile.alive = false;
            continue;
        }

        if (projectile.shape.getGlobalBounds().intersects(player_.bounds()) && !player_.isInvulnerable()) {
            player_.receiveDamage(projectile.damage);
            projectile.alive = false;
            continue;
        }

        for (const Platform& platform : platforms_) {
            if (projectile.shape.getGlobalBounds().intersects(platform.bounds)) {
                projectile.alive = false;
                break;
            }
        }
    }

    enemyProjectiles_.erase(
        std::remove_if(
            enemyProjectiles_.begin(),
            enemyProjectiles_.end(),
            [](const EnemyProjectile& projectile) { return !projectile.alive; }
        ),
        enemyProjectiles_.end()
    );
}

void Game::generateRewardOptions() {
    std::vector<RewardOption> pool{
        {UpgradeType::Damage, "Sharper shots", "Bullets hit harder.\n+9 damage."},
        {UpgradeType::FireRate, "Rapid fire", "Weapon cooldown drops.\nAbout 16% faster fire rate."},
        {UpgradeType::Vitality, "Field medicine", "Increase max HP and heal part of it.\n+30 max HP, +35 heal."},
        {UpgradeType::DoubleJump, "Air step", "Gain an extra jump in the air.\nTake higher routes and dodge bosses."},
        {UpgradeType::MoveSpeed, "Light boots", "Better movement and chase control.\n+26 speed, +32 dash speed."},
        {UpgradeType::Jump, "Spring step", "Reach higher routes.\n+55 jump power."},
        {UpgradeType::Armor, "Guard plating", "Incoming damage is reduced.\nStronger armor layer."},
        {UpgradeType::Pierce, "Piercing ammo", "Shots pass through targets.\n+1 extra enemy per bullet."},
        {UpgradeType::Leech, "Blood siphon", "Kills restore health.\nLife steal on takedown."}
    };

    pool.push_back(
        player_.dashUnlocked()
            ? RewardOption{UpgradeType::Dash, "Dash boost", "Shorter dash cooldown and a longer burst."}
            : RewardOption{UpgradeType::Dash, "Unlock dash", "Gain a fast dodge on Shift."}
    );

    std::shuffle(pool.begin(), pool.end(), rng_);
    rewardOptions_.assign(pool.begin(), pool.begin() + 3);
    rewardSelection_ = 0;
}

void Game::applyReward(std::size_t index) {
    if (index >= rewardOptions_.size()) {
        return;
    }

    player_.applyUpgrade(rewardOptions_[index].type);
    player_.heal(20.f);
    rewardOptions_.clear();
    state_ = AppState::Intermission;
    intermissionTimer_ = 3.2f;
    playMusic(levels_[activeLevelIndex_].musicFile);
}

void Game::updateTitle() {
    std::ostringstream title;
    title << "The Brink | " << levels_[activeLevelIndex_].title
          << " | Wave " << waveNumber_
          << " | HP " << static_cast<int>(std::ceil(player_.health()))
          << "/" << static_cast<int>(player_.maxHealth());

    if (state_ == AppState::Menu) {
        title << " | Main Menu";
    } else if (state_ == AppState::Reward) {
        title << " | Choose Reward";
    } else if (state_ == AppState::Victory) {
        title << " | Level Cleared";
    } else if (state_ == AppState::GameOver) {
        title << " | Run Over";
    }

    window_.setTitle(title.str());
}

void Game::initializeLevels() {
    levels_ = {
        {LevelTheme::Grasslands, "Grasslands", "Balanced start with the cleanest sight lines.", "audio/Grasslands Theme.mp3", sf::Color(93, 182, 246), sf::Color(255, 255, 255), sf::Color(255, 255, 255), 0, 1.0f, 1.0f, 0.f, false, 4},
        {LevelTheme::OakWoods, "Oak Woods", "Dense forest with a little more vertical pressure.", "audio/Mushroom Theme.mp3", sf::Color(88, 164, 168), sf::Color(215, 235, 215), sf::Color(235, 255, 225), 1, 1.08f, 1.05f, 10.f, false, 4},
        {LevelTheme::MushroomGrove, "Mushroom Grove", "Bright biome with faster pacing and riskier waves.", "audio/Mushroom Theme.mp3", sf::Color(138, 106, 214), sf::Color(238, 210, 255), sf::Color(255, 224, 255), 1, 1.08f, 1.0f, 16.f, false, 4},
        {LevelTheme::DesertRuins, "Desert Ruins", "Hot palette, sharper damage, steady pressure.", "audio/Desert Theme.mp3", sf::Color(243, 193, 112), sf::Color(255, 226, 176), sf::Color(255, 236, 194), 2, 1.14f, 1.14f, 8.f, false, 5},
        {LevelTheme::Iceland, "Iceland", "Cold look with higher contrast and stiffer jumps.", "audio/Iceland Theme.mp3", sf::Color(166, 212, 255), sf::Color(216, 240, 255), sf::Color(234, 248, 255), 2, 1.16f, 1.1f, 14.f, false, 5},
        {LevelTheme::Dungeon, "Dungeon", "Dark arena with heavier waves and less visual comfort.", "audio/Dungeon Theme.mp3", sf::Color(42, 43, 74), sf::Color(150, 130, 190), sf::Color(190, 170, 220), 3, 1.22f, 1.16f, 18.f, false, 5},
        {LevelTheme::BossArena, "Boss Arena", "Early boss pressure with the hardest wave mix.", "audio/Boss Theme.mp3", sf::Color(84, 38, 52), sf::Color(190, 120, 120), sf::Color(225, 170, 170), 3, 1.35f, 1.22f, 22.f, true, 3}
    };
}

void Game::selectLevel(std::size_t index) {
    if (index >= levels_.size()) {
        return;
    }

    activeLevelIndex_ = index;
    clearColor_ = levels_[activeLevelIndex_].skyColor;
    buildLevel();
    buildBackground();
    buildDecorations();
    player_.reset({220.f, 580.f});
}

void Game::playMusic(const std::string& relativePath, bool loop) {
    const std::filesystem::path path = assets_.assetPath(relativePath);
    mciSendStringW(L"close bgm", nullptr, 0, nullptr);

    const std::wstring openCommand = L"open \"" + path.wstring() + L"\" type mpegvideo alias bgm";
    if (mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr) != 0) {
        return;
    }

    const std::wstring playCommand = loop ? L"play bgm repeat" : L"play bgm";
    mciSendStringW(playCommand.c_str(), nullptr, 0, nullptr);
    mciSendStringW(L"setaudio bgm volume to 580", nullptr, 0, nullptr);
}

void Game::drawText(
    const std::string& text,
    unsigned size,
    const sf::Vector2f& position,
    const sf::Color& color,
    bool centered
) {
    sf::Text drawable;
    drawable.setFont(assets_.font());
    drawable.setString(text);
    drawable.setCharacterSize(size);
    drawable.setFillColor(color);
    drawable.setPosition(position);

    if (centered) {
        const sf::FloatRect bounds = drawable.getLocalBounds();
        drawable.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    }

    window_.draw(drawable);
}
