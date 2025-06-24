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

struct Collidable {
    enum class State : u8 { OUT, ENTER, IN, LEAVE, KEEP };
    State state = State::OUT;

    void update(entt::entity self);
    void hovered();

    static void add(entt::entity self);

    // TODO: rotations
};
}  // namespace mle::ui::element::comp
