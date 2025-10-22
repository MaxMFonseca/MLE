#include "Base.h"

#include "mle/core/Assert.h"
#include "mle/lua/Utils.h"
#include "mle/ui/Entt.h"
#include "mle/ui/UI.h"

namespace mle::ui::comp {
void Background::apply(const Entt& e, const sol::object& obj) {
    MLE_ASSERT(obj.valid());
    e.patchOrEmplace<Background>([&](Background& bg) { bg.set(obj); });
}

}  // namespace mle::ui::comp
