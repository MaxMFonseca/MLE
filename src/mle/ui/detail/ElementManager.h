#pragma once

#include <entt/entt.hpp>

#include "mle/common/Utils.h"
#include "mle/lua/Types.h"
#include "mle/renderer/Types.h"

namespace mle::ui::detail {
class ElementManager {
  public:
    using LuaKeyFunction = std::function<void(entt::registry& reg, entt::entity e, const sol::object& obj)>;

  public:
    MLE_NO_COPY_MOVE(ElementManager)

    ElementManager() = default;
    ~ElementManager() = default;

    void init();
    void shutdown();

    void resetRoot(const sol::table& table = {});

    void update();
    renderer::ImageRef render();

    void addElement(const sol::table& table, entt::entity parent, const std::string& name);

    auto& getRegistry() { return registry_; }

    Expected<LuaKeyFunction> findLuaKey(const std::string& key);

  private:
    void addLuaKeysEngine();

  private:
    std::unordered_map<std::string, LuaKeyFunction> lua_keys_;

    renderer::PipelineHnd pipeline_;
    entt::registry registry_;
    entt::entity root_{};
};
}  // namespace mle::ui::detail
