#pragma once

#include <map>
#include <memory>

#include "mle/common/Color.h"
#include "mle/common/Flags.h"
#include "mle/common/Types.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/ui/Types.h"
#include "mle/ui/UI.h"
#include "mle/window/Types.h"
#include "sol/forward.hpp"
#include "sol/sol.hpp"
#include "spdlog/fmt/bundled/core.h"

namespace mle::ui {
class Element {
  public:
    class Renderable;
    using RenderableHnd = std::unique_ptr<Renderable>;
    using RenderableRef = Renderable*;

    class Container;
    using ContainerRef = Container*;

    using ElementFn = std::function<void(ElementRef)>;
    using ElementFn2 = std::function<void()>;
    using ElementFnVariant = std::variant<std::monostate, sol::function, ElementFn, ElementFn2>;

    using EventFn = std::function<void(ElementRef, const sol::object&)>;
    using EventFnVariant = std::variant<std::monostate, sol::function, EventFn>;

    using RenderableCreateFn = std::function<Renderable*(ElementRef)>;
    using RenderableCreateFnVariant = std::variant<sol::function, RenderableCreateFn>;

    struct Bound {
        enum class Type : u8 { PARENT, ROOT, STATIC_SHARE, REMAINING_SHARE, SELF_X, SELF_Y, PARENT_SIZE_X, PARENT_SIZE_Y };
        f32 val = 0.0F;
        Type type = Type::PARENT;
    };
    struct TargetBounds {
        Bound pos_x, pos_y, size_x, size_y;
        Bound padding_t, padding_r, padding_b, padding_l;
        Bound margin_t, margin_r, margin_b, margin_l;
        vec2f origin;
        f32 aspect_ratio = 0.0F;

        struct Dependency {
            f32 val;
            std::string name;
            bool is_x;
        };
        using DepVec = std::vector<Dependency>;

        Dependency dep_pos_x;
        Dependency dep_pos_y;
        DepVec dep_size_x;
        DepVec dep_size_y;
    };

    struct NewBounds {
        Rectf on_root;
        Rectf on_root_clamped{};
        f32 aspect_ratio{};

        Rectf content{};
        Rectf padding{};
        Rectf margin{};
    };

    MLE_FLAGS_BEGIN(Behaviour, u16)
    MLE_FLAG(Behaviour, ACTIVE)
    MLE_FLAG(Behaviour, VISIBLE)
    MLE_FLAG(Behaviour, REQUESTING_INTERNAL_BOUNDS_UPDATE)
    MLE_FLAG(Behaviour, REQUESTING_EXTERNAL_BOUNDS_UPDATE)
    MLE_FLAG(Behaviour, REQUESTING_RENDER)
    MLE_FLAG(Behaviour, ALWAYS_RENDER)
    MLE_FLAG(Behaviour, CONTAINER)
    MLE_FLAG(Behaviour, ROOT)
    MLE_FLAG(Behaviour, HOVERABLE)
    MLE_FLAGS_END(Behaviour);

    enum class HoverState : u8 { OUT, JUST_IN, IN, JUST_OUT };

    static constexpr usize MIN_NAME_SIZE = 3;

  public:
    Element(const Element&) = delete;
    Element(Element&&) = delete;
    Element& operator=(const Element&) = delete;
    Element& operator=(Element&&) = delete;

    explicit Element(const sol::table& table, const std::string& name = "", ElementRef parent = nullptr);
    ~Element();

    bool render();
    void getRenderData(int layer, RenderableData& data);

    void update();

    void checkHover(vec2f pos);

    void addHover();

    void updateInternalBounds(std::optional<NewBounds> new_bounds = {});

    void callEvent(const std::string& name, const sol::object& event);
    void listenEvent(const std::string& name, EventFnVariant&& fn);
    void unlistenEvent(const std::string& name);

    [[nodiscard]] const auto& getName() const { return name_; }
    [[nodiscard]] sol::table getTable();
    [[nodiscard]] const auto& getBackground() const { return background_; }
    [[nodiscard]] Container& getParentContainer();
    [[nodiscard]] auto getLayer() const { return layer_; }
    [[nodiscard]] const auto& getTargetBounds() const { return *target_bounds_; }
    [[nodiscard]] f32 getTargetAspectRatio() const;
    [[nodiscard]] ContainerRef getContainer();
    [[nodiscard]] RenderableRef getRenderable() { return renderable_.get(); }
    [[nodiscard]] vec2f getSizeReal() const;
    [[nodiscard]] vec2f getCursorInternalPos() { return last_cursor_internal_pos_; }
    [[nodiscard]] const Rectf& getRectOnParent() const { return rect_on_parent_; }
    [[nodiscard]] const Rectf& getRectOnRoot() const { return rect_on_root_; }
    [[nodiscard]] const Rectf& getRectOnRootClamped() const { return rect_on_root_clamped_; }
    [[nodiscard]] const Rectf& getRectInternalClamped() const { return rect_internal_clamped_; }
    [[nodiscard]] f32 getAspectRatio() const { return aspect_ratio_; }
    [[nodiscard]] f32 getOpacity() const { return opacity_; }

    [[nodiscard]] bool isVisible() const { return behaviour_.have(BehaviourFlagBits::VISIBLE); }
    [[nodiscard]] bool isActive() const { return behaviour_.have(BehaviourFlagBits::ACTIVE); }
    [[nodiscard]] bool isRequestingInternalBoundsUpdate() const { return behaviour_.have(BehaviourFlagBits::REQUESTING_INTERNAL_BOUNDS_UPDATE); }
    [[nodiscard]] bool isRequestingExternalBoundsUpdate() const { return behaviour_.have(BehaviourFlagBits::REQUESTING_EXTERNAL_BOUNDS_UPDATE); }
    [[nodiscard]] bool isRequestingRender() { return behaviour_.have(BehaviourFlagBits::REQUESTING_RENDER); }
    [[nodiscard]] bool isAlwaysRender() const { return behaviour_.have(BehaviourFlagBits::ALWAYS_RENDER); }
    [[nodiscard]] bool isContainer() const { return behaviour_.have(BehaviourFlagBits::CONTAINER); }
    [[nodiscard]] bool isRoot() const { return behaviour_.have(BehaviourFlagBits::ROOT); }
    [[nodiscard]] bool isHoverable() const { return behaviour_.have(BehaviourFlagBits::HOVERABLE); }

    [[nodiscard]] bool hasRenderable() const { return renderable_ != nullptr; }

    void setBackground(const Color& color);
    void setBackground(const sol::object& obj) { setBackground(Color::fromLua(obj)); }
    void setVisible(bool v = true) { behaviour_.set(BehaviourFlagBits::VISIBLE, v); }
    void setActive(bool v = true) { behaviour_.set(BehaviourFlagBits::ACTIVE, v); }
    void setRequestingInternalBoundsUpdate(bool v = true);
    void setRequestingExternalBoundsUpdate(bool v = true);
    void setRequestingRender(bool v = true) { behaviour_.set(BehaviourFlagBits::REQUESTING_RENDER, v || isAlwaysRender()); }
    void setContainer() { behaviour_.set(BehaviourFlagBits::CONTAINER); }
    void setRoot() { behaviour_.set(BehaviourFlagBits::ROOT); }
    void setHoverable(bool v = true) { behaviour_.set(BehaviourFlagBits::HOVERABLE, v); }
    void setTargetBounds(const sol::table& table);
    void setOnHover(const sol::object& obj);
    void setOnHover(ElementFnVariant&& fn);
    void setOnHoverIn(const sol::object& obj);
    void setOnHoverIn(ElementFnVariant&& fn);
    void setOnHoverOut(const sol::object& obj);
    void setOnHoverOut(ElementFnVariant&& fn);
    void setOnUpdate(ElementFnVariant&& fn);
    void setOpacity(f32 opacity);
    void setAlwaysRender(bool v = true);

    void listenKeyOnHover(const std::string& name, const sol::object& obj);
    void listenKeyOnHover(const std::string& name, const ElementFnVariant& fn);

    void listenKey(const std::string& name, const sol::object& obj);
    void listenKey(const std::string& name, const ElementFnVariant& fn);
    void unlistenKey(const std::string& name);

    void removeBackground() { background_.a = 0; }

    void log(const std::string& prefix = "");

    static void registerRenderableType(const std::string& trigger, RenderableCreateFnVariant&& fn);
    static std::expected<Element::RenderableCreateFnVariant, Result> findRenderableCreateFn(const std::string& trigger);

    static void registerLuaTypes();

  private:
    void parseHoverFunctional(const sol::table& table);
    void callElementFn(const ElementFnVariant& fn);

    void activateHoverKeyListeners();
    void deactivateHoverKeyListeners();

    struct ParsedBound {
        Bound v;
        TargetBounds::DepVec dependencies;
    };
    static ParsedBound parseBound(const sol::object& o, bool is_size, bool is_x);
    static ParsedBound parseStringBound(const std::string& s, bool is_size);
    void makeRenderable(const sol::table& table);

  private:
    std::string name_;
    ElementRef parent_;

    BehaviourFlags behaviour_;

    RenderableHnd renderable_;
    f32 opacity_ = 1.0F;

    sol::table table_;

    Color background_{0.0F, 0.0F, 0.0F, 0.0F};

    Rectf rect_on_parent_{};
    Rectf rect_on_root_{};
    Rectf rect_on_root_clamped_{};
    Rectf rect_internal_clamped_{};
    f32 aspect_ratio_ = 0;
    int layer_ = 0;

    HoverState hover_state_ = HoverState::OUT;
    vec2f last_cursor_internal_pos_{};

    ElementFnVariant on_update_;

    struct Hoverable {
        ElementFnVariant on_hover;
        ElementFnVariant on_hover_in;
        ElementFnVariant on_hover_out;

        std::vector<window::KeyListenerHnd> key_listeners;
    };
    std::unique_ptr<Hoverable> hoverable_{};

    std::vector<window::KeyListenerHnd> key_listeners_;
    std::vector<std::pair<std::string, EventFnVariant>> event_handlers_;

    // TODO: linear allocate all the things
    std::unique_ptr<TargetBounds> target_bounds_{};
};

class Element::Renderable {
  public:
    Renderable(const Renderable&) = delete;
    Renderable(Renderable&&) = delete;
    Renderable& operator=(const Renderable&) = delete;
    Renderable& operator=(Renderable&&) = delete;

    explicit Renderable(ElementRef element) :
        element_(element) {}
    virtual ~Renderable() { MLE_VD((void*)element_); }

    virtual void set(const sol::table& /*obj*/) {}
    virtual void update() {}
    virtual bool render() { return false; }
    virtual void getRenderData(int /*unused*/, RenderableData& /*unused*/) const {}
    virtual void updateInternalBounds(std::optional<NewBounds> /*new_bounds*/) {}
    virtual f32 getAspectRatio() { return 0.0F; }

    virtual void log(const std::string&) const = 0;

    template <typename T>
    static RenderableRef makeNew(ElementRef element) {
        return new T(element);
    }

    template <typename T>
    T* as() {
        return static_cast<T*>(this);
    }

  protected:
    ElementRef element_;
};

class Element::Container : public Element::Renderable {
  public:
    Container(const Container&) = delete;
    Container(Container&&) = delete;
    Container& operator=(const Container&) = delete;
    Container& operator=(Container&&) = delete;

    explicit Container(ElementRef element) :
        Renderable(element) {
        element->setContainer();
    }

    ~Container() override = default;

    bool render() override;
    void getRenderData(int layer, RenderableData& data) const override;
    void update() override;

    void checkHover();
    void updateInternalBounds(std::optional<NewBounds> new_bounds) override;

    virtual void addChild(const sol::table& table);
    virtual void addChildren(const sol::table& table);
    void removeChild(const std::string& name);
    void removeChildren();
    usize getChildPos(const std::string& name);
    ElementRef getChild(usize pos);
    ElementRef getChild(const std::string& name);

    void requestChildInternalBoundsUpdate(ElementRef child);
    void notifyChildRequestedExternalBoundsUpdate();

    virtual void updateChildrenBounds() = 0;

    void log(const std::string& prefix) const override;

  protected:
    const std::vector<ElementRef>& getChildrenSortedByLayer();

  protected:
    std::vector<std::pair<std::string, ElementHnd>> children_;
    std::vector<ElementRef> children_to_update_internals_;
    bool layers_dirty_ = false;
    bool children_bounds_dirty_ = false;

  private:
    std::vector<ElementRef> children_sorted_by_layer_;
};
}  // namespace mle::ui

namespace fmt {
template <>
struct formatter<mle::ui::Element::Bound::Type> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::Element::Bound::Type v, FormatContext& ctx) const {
        switch (v) {
            case mle::ui::Element::Bound::Type::PARENT:
                return format_to(ctx.out(), "PARENT");
            case mle::ui::Element::Bound::Type::ROOT:
                return format_to(ctx.out(), "ROOT");
            case mle::ui::Element::Bound::Type::STATIC_SHARE:
                return format_to(ctx.out(), "STATIC_SHARE");
            case mle::ui::Element::Bound::Type::REMAINING_SHARE:
                return format_to(ctx.out(), "REMAINING_SHARE");
            case mle::ui::Element::Bound::Type::SELF_X:
                return format_to(ctx.out(), "SELF_X");
            case mle::ui::Element::Bound::Type::SELF_Y:
                return format_to(ctx.out(), "SELF_Y");
            case mle::ui::Element::Bound::Type::PARENT_SIZE_X:
                return format_to(ctx.out(), "PARENT_SIZE_X");
            case mle::ui::Element::Bound::Type::PARENT_SIZE_Y:
                return format_to(ctx.out(), "PARENT_SIZE_Y");
            default:
                MLE_UNREACHABLE;
        }
    }
};

template <>
struct formatter<mle::ui::Element::HoverState> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(mle::ui::Element::HoverState state, FormatContext& ctx) const {
        switch (state) {
            case mle::ui::Element::HoverState::OUT:
                return format_to(ctx.out(), "OUT");
            case mle::ui::Element::HoverState::JUST_IN:
                return format_to(ctx.out(), "JUST_IN");
            case mle::ui::Element::HoverState::IN:
                return format_to(ctx.out(), "IN");
            case mle::ui::Element::HoverState::JUST_OUT:
                return format_to(ctx.out(), "JUST_OUT");
            default:
                MLE_UNREACHABLE;
        }
    }
};

template <>
struct formatter<mle::ui::Element::Bound> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Element::Bound& bound, FormatContext& ctx) const {
        return format_to(ctx.out(), "[val: {}, type: {}]", bound.val, bound.type);
    }
};

template <>
struct formatter<mle::ui::Element::TargetBounds::Dependency> : formatter<std::string> {
    template <typename FormatContext>
    constexpr auto format(const mle::ui::Element::TargetBounds::Dependency& dep, FormatContext& ctx) const {
        return format_to(ctx.out(), "[val: {}, name: {}]", dep.val, dep.name);
    }
};
}  // namespace fmt

MLE_FLAGS_FMT_BEGIN(mle::ui::Element::Behaviour)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, ACTIVE)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, VISIBLE)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, REQUESTING_INTERNAL_BOUNDS_UPDATE)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, REQUESTING_EXTERNAL_BOUNDS_UPDATE)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, REQUESTING_RENDER)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, ALWAYS_RENDER)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, CONTAINER)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, ROOT)
MLE_FLAGS_FMT_CASE(mle::ui::Element::Behaviour, HOVERABLE)
MLE_FLAGS_FMT_END(mle::ui::Element::Behaviour)
