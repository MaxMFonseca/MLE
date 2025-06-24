#include "Collidable.h"

namespace mle::ui::element::comp {
void Collidable::hovered() {
    if (state == State::IN) {
        state = State::KEEP;
    } else {
        state = State::ENTER;
    }
}

void Collidable::update(entt::entity self) {
    auto& reg = getRegistry();

    switch (state) {
        case State::ENTER: {
            if (const auto* fnc = reg.try_get<OnHoverEnter>(self); fnc) {
                fnc->fn(self);
            }
        }
            [[fallthrough]];
        case State::KEEP: {
            if (const auto* fnc = reg.try_get<OnHover>(self); fnc) {
                fnc->fn(self);
            }
            state = State::IN;
        } break;
        case State::IN:
        case State::LEAVE: {
            if (const auto* fnc = reg.try_get<OnHoverLeave>(self); fnc) {
                fnc->fn(self);
            }
            state = State::OUT;
        } break;
        case State::OUT: {
            void(0);
        } break;
    }
}

void Collidable::add(entt::entity self) {
    auto& reg = getRegistry();
    if (!reg.try_get<Collidable>(self)) {
        reg.emplace<Collidable>(self);
    }
}
}  // namespace mle::ui::element::comp
