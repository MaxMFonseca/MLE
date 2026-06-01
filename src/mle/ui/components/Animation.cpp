#include "Animation.h"

#include "mle/lua/Utils.h"
#include "mle/renderer/Renderer.h"
#include "mle/ui/Entt.h"
#include "mle/ui/components/Base.h"
#include "mle/ui/components/Renderable.h"
#include "mle/ui/renderable/Sprite.h"
#include "mle/ui/renderable/Text.h"
#include "mle/utils/String.h"
#include "sol/forward.hpp"

namespace mle::ui::comp {
namespace {
[[nodiscard]] std::optional<AnimationEase> parseEase(std::string_view str) {
    if (matchAny(str, "linear")) {
        return AnimationEase::LINEAR;
    }
    if (matchAny(str, "in_quad")) {
        return AnimationEase::IN_QUAD;
    }
    if (matchAny(str, "out_quad")) {
        return AnimationEase::OUT_QUAD;
    }
    if (matchAny(str, "in_out_quad")) {
        return AnimationEase::IN_OUT_QUAD;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<AnimationTarget> parseTarget(std::string_view str) {
    if (str == "pos_x") {
        return AnimationTarget::POS_X;
    }
    if (str == "pos_y") {
        return AnimationTarget::POS_Y;
    }
    if (str == "size_x") {
        return AnimationTarget::SIZE_X;
    }
    if (str == "size_y") {
        return AnimationTarget::SIZE_Y;
    }
    if (str == "render_scale") {
        return AnimationTarget::RENDER_SCALE;
    }
    if (str == "background") {
        return AnimationTarget::BACKGROUND;
    }
    if (str == "border_color") {
        return AnimationTarget::BORDER_COLOR;
    }
    return std::nullopt;
}

[[nodiscard]] AnimationValue parseValue(AnimationTarget target, const sol::object& obj) {
    AnimationValue value{};
    if (target == AnimationTarget::RENDER_SCALE) {
        value.kind = AnimationValueKind::NUMBER;
        if (lua::valid<f32>(obj)) {
            value.number = lua::as<f32>(obj);
        }
        return value;
    }
    if (target == AnimationTarget::POS_X || target == AnimationTarget::POS_Y || target == AnimationTarget::SIZE_X ||
        target == AnimationTarget::SIZE_Y) {
        value.kind = AnimationValueKind::TARGET_BOUND;
        value.target_bound.set(obj);
        return value;
    }

    value.kind = AnimationValueKind::COLOR;
    value.color = Color::fromLua(obj);
    return value;
}

[[nodiscard]] std::optional<vec2u> parseVec2u(const sol::object& obj) {
    vec2f as_f{};
    if (lua::tryAs<vec2f>(obj, as_f) && as_f.x > 0.0F && as_f.y > 0.0F) {
        return vec2u{as<u32>(std::round(as_f.x)), as<u32>(std::round(as_f.y))};
    }

    vec2i as_i{};
    if (lua::tryAs<vec2i>(obj, as_i) && as_i.x > 0 && as_i.y > 0) {
        return vec2u{as<u32>(as_i.x), as<u32>(as_i.y)};
    }

    return std::nullopt;
}

[[nodiscard]] AnimationValue lerpValue(const AnimationValue& from, const AnimationValue& to, f32 t) {
    AnimationValue value = from;
    if (from.kind == AnimationValueKind::NUMBER) {
        value.number = glm::mix(from.number, to.number, t);
        return value;
    }
    if (from.kind == AnimationValueKind::TARGET_BOUND) {
        value.target_bound = from.target_bound;
        value.target_bound.val = glm::mix(from.target_bound.val, to.target_bound.val, t);
        return value;
    }
    value.color = Color::mix(from.color, to.color, t);
    return value;
}

[[nodiscard]] f32 trackProgress(const AnimationTrack& track, f32 elapsed) {
    if (track.duration <= 0.0F) {
        return 1.0F;
    }
    f32 local = elapsed - track.delay;
    if (local <= 0.0F) {
        return 0.0F;
    }
    if (!track.loop) {
        return glm::clamp(local / track.duration, 0.0F, 1.0F);
    }

    const auto cycle = as<u32>(std::floor(local / track.duration));
    f32 t = std::fmod(local, track.duration) / track.duration;
    if (track.yoyo && cycle % 2 == 1) {
        t = 1.0F - t;
    }
    return glm::clamp(t, 0.0F, 1.0F);
}

void applyTrackValue(const Entt& ew, AnimationTarget target, const AnimationValue& value) {
    switch (target) {
        case AnimationTarget::POS_X:
            ew.patchOrEmplace<TargetPosition>([&](TargetPosition& pos) { pos.x = value.target_bound; });
            ew.requestExternalBoundsUpdate();
            break;
        case AnimationTarget::POS_Y:
            ew.patchOrEmplace<TargetPosition>([&](TargetPosition& pos) { pos.y = value.target_bound; });
            ew.requestExternalBoundsUpdate();
            break;
        case AnimationTarget::SIZE_X:
            ew.patchOrEmplace<TargetSize>([&](TargetSize& size) { size.x = value.target_bound; });
            ew.requestExternalBoundsUpdate();
            break;
        case AnimationTarget::SIZE_Y:
            ew.patchOrEmplace<TargetSize>([&](TargetSize& size) { size.y = value.target_bound; });
            ew.requestExternalBoundsUpdate();
            break;
        case AnimationTarget::RENDER_SCALE:
            ew.patchOrEmplace<RenderScale>([&](RenderScale& scale) { scale.scale = glm::max(value.number, 0.01F); });
            break;
        case AnimationTarget::BACKGROUND:
            ew.patchOrEmplace<Background>([&](Background& bg) {
                bg.lt = value.color.toLinear();
                bg.rt = value.color.toLinear();
                bg.lb = value.color.toLinear();
                bg.rb = value.color.toLinear();
            });
            break;
        case AnimationTarget::BORDER_COLOR:
            ew.patchOrEmplace<Border>([&](Border& border) { border.color = value.color.toLinear(); });
            break;
    }
}

[[nodiscard]] Expected<vec2u> spriteSourceExtent(const renderable::Sprite& sprite) {
    if (sprite.source == renderable::SpriteSource::IMAGE) {
        if (sprite.image != nullptr) {
            return sprite.image->getExtent();
        }
        return Renderer::i().textureCache().getDefaultTexture()->getExtent();
    }

    return Renderer::i().textureCache().getExtent(sprite.texture_id);
}

void tickSpriteAnimation(const Entt& ew, const SpriteAnimation& animation, f32 elapsed) {
    if (animation.frames == 0 || animation.fps <= 0.0F || animation.frame_size.x == 0 || animation.frame_size.y == 0) {
        return;
    }

    auto* renderable = ew.tryGet<Renderable>();
    if (!renderable || !renderable->impl || renderable->impl->getType() != renderable::Sprite::type()) {
        return;
    }

    auto* sprite = as<renderable::Sprite*>(renderable->impl.get());
    auto extent_r = spriteSourceExtent(*sprite);
    if (!extent_r.has_value() || extent_r.value().x == 0 || extent_r.value().y == 0) {
        return;
    }

    u32 frame = as<u32>(std::floor(glm::max(elapsed, 0.0F) * animation.fps));
    if (animation.loop) {
        frame %= animation.frames;
    } else {
        frame = glm::min(frame, animation.frames - 1);
    }

    const u32 cols = glm::max(extent_r.value().x / animation.frame_size.x, 1U);
    const vec2f frame_px{as<f32>((frame % cols) * animation.frame_size.x), as<f32>((frame / cols) * animation.frame_size.y)};
    const vec2f extent = vec2f(extent_r.value());

    sprite->uv = frame_px / extent;
    sprite->uv_size = vec2f(animation.frame_size) / extent;
    sprite->versionUp();
}

void tickTypewriterAnimation(const Entt& ew, const TypewriterAnimation& animation, f32 elapsed) {
    auto text_r = renderable::Text::getFromEntt(ew);
    if (!text_r.has_value() || animation.cps <= 0.0F) {
        return;
    }

    auto& text = text_r->get();
    const f32 local = glm::max(elapsed - animation.start_delay, 0.0F);
    const usize visible = glm::min(as<usize>(std::floor(local * animation.cps)), text.text.size());
    text.setVisibleChars(ew, visible);
}
}  // namespace

f32 Animation::ease(AnimationEase ease_, f32 t) {
    t = glm::clamp(t, 0.0F, 1.0F);
    switch (ease_) {
        case AnimationEase::LINEAR:
            return t;
        case AnimationEase::IN_QUAD:
            return t * t;
        case AnimationEase::OUT_QUAD:
            return 1.0F - ((1.0F - t) * (1.0F - t));
        case AnimationEase::IN_OUT_QUAD:
            if (t < 0.5F) {
                return 2.0F * t * t;
            }
            return 1.0F - (std::pow(-2.0F * t + 2.0F, 2.0F) / 2.0F);
    }
}

void Animation::set(const Entt& ew, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    if (!lua::valid<sol::table>(obj)) {
        MLE_E("Invalid animation value for entity {}. Expected table.", ew.fullName());
        return;
    }

    auto table = lua::as<sol::table>(obj);
    tracks.clear();
    sprite.reset();
    typewriter.reset();
    lua::tryGetKeyAs(table, "loop", loop);

    if (const auto tracks_r = table["tracks"]; lua::valid<sol::table>(tracks_r)) {
        auto tracks_table = lua::as<sol::table>(tracks_r);
        for (const auto& [_, track_r] : tracks_table) {
            if (!lua::valid<sol::table>(track_r)) {
                MLE_E("Invalid animation track on entity {}. Expected table.", ew.fullName());
                continue;
            }
            auto track_table = lua::as<sol::table>(track_r);
            const auto target_r = track_table["target"];
            if (!lua::valid<std::string>(target_r)) {
                MLE_E("Invalid animation track on entity {}. Missing string target.", ew.fullName());
                continue;
            }
            auto target = parseTarget(lua::as<std::string>(target_r));
            if (!target.has_value()) {
                MLE_E("Unknown animation target '{}' on entity {}.", lua::as<std::string>(target_r), ew.fullName());
                continue;
            }

            AnimationTrack track{};
            track.target = *target;
            if (const auto from_r = track_table["from"]; from_r.valid()) {
                track.from = parseValue(track.target, from_r);
            }
            if (const auto to_r = track_table["to"]; to_r.valid()) {
                track.to = parseValue(track.target, to_r);
            }
            lua::tryGetKeyAs(track_table, "duration", track.duration);
            lua::tryGetKeyAs(track_table, "delay", track.delay);
            lua::tryGetKeyAs(track_table, "loop", track.loop);
            lua::tryGetKeyAs(track_table, "yoyo", track.yoyo);
            if (const auto ease_r = track_table["ease"]; lua::valid<std::string>(ease_r)) {
                if (auto parsed = parseEase(lua::as<std::string>(ease_r)); parsed.has_value()) {
                    track.ease = *parsed;
                } else {
                    MLE_W("Unknown animation ease '{}', using linear.", lua::as<std::string>(ease_r));
                }
            }
            tracks.push_back(track);
        }
    }

    if (const auto sprite_r = table["sprite"]; lua::valid<sol::table>(sprite_r)) {
        auto sprite_table = lua::as<sol::table>(sprite_r);
        SpriteAnimation parsed{};
        if (const auto frame_size_r = sprite_table["frame_size"]; auto frame_size = parseVec2u(frame_size_r)) {
            parsed.frame_size = *frame_size;
        } else {
            MLE_E("Invalid animation.sprite.frame_size on entity {}.", ew.fullName());
        }
        lua::tryGetKeyAs(sprite_table, "frames", parsed.frames);
        lua::tryGetKeyAs(sprite_table, "fps", parsed.fps);
        lua::tryGetKeyAs(sprite_table, "loop", parsed.loop);
        sprite = parsed;
    }

    if (const auto typewriter_r = table["typewriter"]; lua::valid<sol::table>(typewriter_r)) {
        auto typewriter_table = lua::as<sol::table>(typewriter_r);
        TypewriterAnimation parsed{};
        lua::tryGetKeyAs(typewriter_table, "cps", parsed.cps);
        lua::tryGetKeyAs(typewriter_table, "start_delay", parsed.start_delay);
        typewriter = parsed;
    }
}

void Animation::tick(const Entt& ew, f32 dt) {
    elapsed += glm::max(dt, 0.0F);
    if (ew.has<DisabledFlag>()) {
        return;
    }

    for (const auto& track : tracks) {
        const f32 t = ease(track.ease, trackProgress(track, elapsed));
        applyTrackValue(ew, track.target, lerpValue(track.from, track.to, t));
    }

    if (sprite.has_value()) {
        tickSpriteAnimation(ew, *sprite, elapsed);
    }

    if (typewriter.has_value()) {
        tickTypewriterAnimation(ew, *typewriter, elapsed);
    }
}

void Animation::apply(const Entt& ew, const sol::object& obj) {
    ew.patchOrEmplace<Animation>([&](Animation& animation) { animation.set(ew, obj); });
}
}  // namespace mle::ui::comp
