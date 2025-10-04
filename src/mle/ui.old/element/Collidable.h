#pragma once

#include "mle/ui/Types.h"
#include "mle/ui/element/Base.h"

namespace mle::ui::element::comp {
struct OnHover {
    CallbackFn fn;
};
struct OnHoverEnter {
    CallbackFn fn;
};
struct OnHoverLeave {
    CallbackFn fn;
};
// struct OnHoverOut {
//     CallbackFn fn;
// };
struct OnLeftPressed {
    CallbackFn fn;
};
struct OnLeftReleased {
    CallbackFn fn;
};
struct OnLeftDown {
    CallbackFn fn;
};

struct Collidable {
    enum class State : u8 { OUT, ENTER, IN, LEAVE, KEEP };
    State state = State::OUT;
    bool clicable = false;

    void update(entt::entity self);
    bool hovered(entt::entity self);

    [[nodiscard]] bool isHovered() const { return state != State::OUT; };

    static void add(entt::entity self);

    // TODO: rotations
};
}  // namespace mle::ui::element::comp
