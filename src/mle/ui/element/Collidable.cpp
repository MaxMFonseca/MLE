#include "Collidable.h"

#include "mle/ui/UI.h"
#include "mle/window/Types.h"
#include "mle/window/Window.h"

namespace mle::ui::element::comp {
bool Collidable::hovered(entt::entity self) {
    if (state == State::IN) {
        state = State::KEEP;
    } else {
        state = State::ENTER;
    }

    if (!clicable) {
        return false;
    }

    auto& reg = getRegistry();
    switch (window::getUIM().getState(window::Key::MOUSE_LEFT)) {
        case window::KeyState::PRESSED: {
            auto* comp = reg.try_get<OnLeftPressed>(self);
            if (comp) {
                comp->fn(self);
            }
            return true;
        } break;
        case window::KeyState::DOWN: {  // NOLINT
            auto* comp = reg.try_get<OnLeftDown>(self);
            if (comp) {
                comp->fn(self);
            }
            return true;
        } break;
        case window::KeyState::RELEASED: {
            auto* comp = reg.try_get<OnLeftReleased>(self);
            if (comp) {
                comp->fn(self);
            }
            return true;
        } break;
        case window::KeyState::UP:
            MLE_NOOP;
            break;
    }

    return false;
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
