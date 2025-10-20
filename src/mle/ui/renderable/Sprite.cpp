#include "Sprite.h"

#include "../Entt.h"
#include "mle/ui/components/Renderable.h"

namespace mle::ui::renderable {
void Sprite::apply(const Entt& e, const sol::object& obj) {
    if (!obj.valid()) {
        MLE_E("Invalid object provided to Sprite::apply for entt {}", e.e());
        return;
    }
    auto* renderable = e.tryGet<comp::Renderable>();
    Sprite* self_p = nullptr;
    if (!renderable) {
        renderable = &e.emplace<comp::Renderable>(std::make_unique<Sprite>());
        self_p = as<Sprite*>(renderable->impl.get());
    } else {
        MLE_ASSERT(renderable->impl);
        if (renderable->impl->getType() != Sprite::type()) {
            MLE_E("Renderable::apply called on entt {} with incompatible Renderable type. {}x{}", e.fullName(), renderable->impl->getType().data(),
                  Sprite::type().data());
            return;
        }

        self_p = as<Sprite*>(renderable->impl.get());
    }
    self_p->set(obj);
};
}  // namespace mle::ui::renderable
