#include "Animation.hpp"

#include <algorithm>

Animator::Animator(const AnimationSet* animationSet) {
    setAnimationSet(animationSet);
}

void Animator::setAnimationSet(const AnimationSet* animationSet) {
    animationSet_ = animationSet;
    currentClip_.clear();
    elapsed_ = 0.f;
    frameIndex_ = 0;
    finished_ = false;

    if (animationSet_ && !animationSet_->clips.empty()) {
        play(animationSet_->clips.begin()->first, true);
    }
}

void Animator::play(const std::string& clipName, bool restart) {
    if (!animationSet_) {
        return;
    }

    if (!restart && clipName == currentClip_) {
        return;
    }

    const auto it = animationSet_->clips.find(clipName);
    if (it == animationSet_->clips.end() || it->second.frames.empty()) {
        return;
    }

    currentClip_ = clipName;
    elapsed_ = 0.f;
    frameIndex_ = 0;
    finished_ = false;
    applyFrame();
}

void Animator::update(float dt) {
    if (!animationSet_ || currentClip_.empty()) {
        return;
    }

    const auto it = animationSet_->clips.find(currentClip_);
    if (it == animationSet_->clips.end() || it->second.frames.size() <= 1) {
        return;
    }

    const AnimationClip& clip = it->second;
    elapsed_ += dt;

    while (elapsed_ >= clip.frameTime) {
        elapsed_ -= clip.frameTime;
        if (frameIndex_ + 1 < clip.frames.size()) {
            ++frameIndex_;
        } else if (clip.loop) {
            frameIndex_ = 0;
        } else {
            finished_ = true;
            break;
        }
        applyFrame();
    }
}

void Animator::setPosition(const sf::Vector2f& position) {
    sprite_.setPosition(position);
}

void Animator::move(const sf::Vector2f& delta) {
    sprite_.move(delta);
}

void Animator::setFacing(float facing) {
    facing_ = facing < 0.f ? -1.f : 1.f;
    applyFrame();
}

const sf::Sprite& Animator::sprite() const {
    return sprite_;
}

sf::Sprite& Animator::sprite() {
    return sprite_;
}

sf::FloatRect Animator::bounds() const {
    return sprite_.getGlobalBounds();
}

sf::Vector2f Animator::position() const {
    return sprite_.getPosition();
}

bool Animator::isFinished() const {
    return finished_;
}

const std::string& Animator::currentClip() const {
    return currentClip_;
}

void Animator::applyFrame() {
    if (!animationSet_ || currentClip_.empty()) {
        return;
    }

    const auto it = animationSet_->clips.find(currentClip_);
    if (it == animationSet_->clips.end() || it->second.frames.empty()) {
        return;
    }

    const AnimationFrame& frame = it->second.frames[std::min(frameIndex_, it->second.frames.size() - 1)];
    const sf::Vector2f position = sprite_.getPosition();

    sprite_.setTexture(*frame.texture, true);
    sprite_.setTextureRect(frame.rect);
    sprite_.setOrigin(
        static_cast<float>(frame.rect.width) * animationSet_->anchor.x,
        static_cast<float>(frame.rect.height) * animationSet_->anchor.y
    );
    sprite_.setScale(facing_ * animationSet_->scale.x, animationSet_->scale.y);
    sprite_.setPosition(position);
}
