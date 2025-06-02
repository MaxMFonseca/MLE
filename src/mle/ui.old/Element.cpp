#include "Element.h"

#include <algorithm>
#include <unordered_map>

#include "mle/common/Logger.h"
#include "mle/common/Utils.h"
#include "mle/common/math/Types.h"
#include "mle/common/math/Types2D.h"
#include "mle/lua/Lua.h"
#include "mle/ui/UI.h"
#include "mle/window/Types.h"
#include "mle/window/UserInputManager.h"
#include "mle/window/Utils.h"
#include "mle/window/Window.h"
#include "sol/forward.hpp"
#include "sol/raii.hpp"

namespace mle::ui {
namespace {
// clang-format off
f32 charToPos(char c) {
    switch (c) {
        case 'c': return 0.5;
        case 't': case 'l': return 0;
        case 'b': case 'r': return 1;
        default: MLE_UNREACHABLE_LOG("Unknown char: {}", c);
    }
}
// clang-format on
vec2f strToPos(const std::string& str) {
    if (str.size() == 2) {
        return {charToPos(str[0]), charToPos(str[1])};
    }
    MLE_ASSERT(str.size() == 1);
    switch (str[0]) {
        case 'c':
            return {0.5, 0.5};
        case 't':
            return {0, 0};
        case 'b':
            return {0, 1};
        case 'l':
            return {0, 0};
        case 'r':
            return {1, 0};
        default:
            MLE_UNREACHABLE_LOG("Unknown str: {}", str);
    }
}

u32 getNextId() {
    static u32 counter = max<u32>();
    return counter--;
}

Element::TargetBounds::Dependency parseDepTable(const sol::table& t, bool is_x) {
    if (auto t3 = t[3]; t3.valid()) {
        MLE_ASSERT(t3.is<std::string>());
        std::string t3s = t3;
        MLE_ASSERT(t3s.size() == 1);
        is_x = t3s[0] == 'x';
    }
    if (t[1].is<f32>()) {
        return {.val = t[1].get<f32>(), .name = t[2].get<std::string>(), .is_x = is_x};
    }
    return {.val = charToPos(t[1].get<char>()), .name = t[2].get<std::string>(), .is_x = is_x};
}

inline bool isValidName(const std::string& name) {
    return name.size() >= Element::MIN_NAME_SIZE && !isDigit(name[0]);
}

std::unordered_map<std::string, Element::RenderableCreateFnVariant> renderable_create_functions_;  // NOLINT
}  // namespace

Element::Element(const sol::table& table, const std::string& name, ElementRef parent) :
    name_(name.empty() ? "root" : name),
    parent_(parent) {
    MLE_ASSERT_LOG(isValidName(name_), "Invalid name: {}", name);

    MLE_T("Constructing element: {} {}", name_, (void*)this);

    if (auto r_layer = table["layer"]; r_layer.valid()) {
        MLE_ASSERT(r_layer.is<int>());
        int layer = r_layer;
        MLE_ASSERT_LOG(layer > -7 && layer <= 8, "Invalid layer: {}", layer);
        layer_ = r_layer;
    }

    if (auto r_visible = table["visible"]; r_visible.valid()) {
        setVisible(r_visible);
    } else {
        setVisible();
    }

    if (auto r_active = table["active"]; r_active.valid()) {
        setActive(r_active);
    } else {
        setActive();
    }

    if (auto r_opacity = table["opacity"]; r_opacity.valid()) {
        setOpacity(r_opacity);
    }

    if (auto r_always_render = table["always_render"]; r_always_render.valid()) {
        setAlwaysRender(r_always_render);
    }

    if (sol::object r_background = table["background"]; r_background.valid()) {
        setBackground(r_background);
    }

    if (auto r_table = table["table"]; r_table.valid()) {
        table_ = lua::createTable(r_table.get<sol::table>());
    }
    if (auto r_table = table["new_table"]; r_table.valid()) {
        table_ = r_table.get<sol::table>();
    }

    if (auto r_on_update = table["onUpdate"]; r_on_update.valid()) {
        setOnUpdate(r_on_update);
    }

    parseHoverFunctional(table);

    target_bounds_ = std::make_unique<TargetBounds>();
    setTargetBounds(table);

    if (auto r_listen = table["listen"]; r_listen.valid()) {
        for (const auto& [key, item] : r_listen.get<sol::table>()) {
            listenEvent(key.as<std::string>(), item);
        }
    }

    if (auto r_listen_keys = table["listen_keys"]; r_listen_keys.valid()) {
        for (const auto& [key, item] : r_listen_keys.get<sol::table>()) {
            listenKey(key.as<std::string>(), item);
        }
    }

    makeRenderable(table);
}

Element::~Element() {
    MLE_D("Destroying element: {} {}", name_, (void*)this);
    for (auto& [key, item] : event_handlers_) {
        ui::unlistenEvent(key, this);
    }
}

void Element::update() {
    if (hover_state_ == HoverState::IN || hover_state_ == HoverState::JUST_IN) {
        if (hover_state_ == HoverState::JUST_IN) {
            if (hoverable_) {
                callElementFn(hoverable_->on_hover_in);
                activateHoverKeyListeners();
            }
        }
        if (hoverable_) {
            callElementFn(hoverable_->on_hover);
        }
        hover_state_ = HoverState::JUST_OUT;
    } else if (hover_state_ == HoverState::JUST_OUT) {
        if (hoverable_) {
            callElementFn(hoverable_->on_hover_out);
            deactivateHoverKeyListeners();
        }
        hover_state_ = HoverState::OUT;
    }

    callElementFn(on_update_);

    if (renderable_) {
        renderable_->update();
    }

    if (isRequestingInternalBoundsUpdate()) {
        setRequestingInternalBoundsUpdate(false);
        MLE_T("Requesting internal bounds update for: {}", name_);
        if (parent_) {
            getParentContainer().requestChildInternalBoundsUpdate(this);
        }
    }
    if (isRequestingExternalBoundsUpdate()) {
        setRequestingExternalBoundsUpdate(false);
        MLE_T("Requesting external bounds update for: {}", name_);
        if (parent_) {
            getParentContainer().notifyChildRequestedExternalBoundsUpdate();
        }
    }
}

void Element::activateHoverKeyListeners() {
    for (auto& kl : hoverable_->key_listeners) {
        window::getUIM().signKeylistener(kl.get());
    }
}

void Element::deactivateHoverKeyListeners() {
    for (auto& kl : hoverable_->key_listeners) {
        window::getUIM().unsignKeylistener(kl.get());
    }
}

void Element::checkHover(vec2f pos) {
    if (!isVisible()) {
        return;
    }

    if (isContainer() || isHoverable()) {
        if (!contains(rect_on_parent_, pos)) {
            return;
        }

        last_cursor_internal_pos_ = normalize(rect_on_parent_, pos);

        if (isContainer()) {
            getContainer()->checkHover();
        }

        if (hoverable_) {
            if (hover_state_ == HoverState::OUT) {
                hover_state_ = HoverState::JUST_IN;
            } else if (hover_state_ == HoverState::JUST_OUT) {
                hover_state_ = HoverState::IN;
            }
        }
    }
}

bool Element::render() {
    if (!isVisible()) {
        return false;
    }
    if ((renderable_ && renderable_->render()) || isRequestingRender()) {
        setRequestingRender(false);
        return true;
    }
    return false;
}

void Element::getRenderData(int layer, RenderableData& data) {
    if (!isVisible()) {
        return;
    }

    layer += layer_;

    if (background_.a > 0) {
        data[layer].rects.emplace_back(rect_on_root_clamped_.pos, rect_on_root_clamped_.size, background_.withAMultiplied(opacity_));
    }
    if (renderable_) {
        renderable_->getRenderData(layer, data);
    }
}

Element::Container& Element::getParentContainer() {
    MLE_ASSERT(parent_);
    return *parent_->getContainer();
}

void Element::updateInternalBounds(std::optional<NewBounds> new_bounds) {
    if (new_bounds) {
        rect_on_parent_ = new_bounds->content;
        rect_on_root_ = new_bounds->on_root;
        rect_on_root_clamped_ = new_bounds->on_root_clamped;
        aspect_ratio_ = new_bounds->aspect_ratio;
        rect_internal_clamped_ = calculateRelativeTransformation(rect_on_root_, rect_on_root_clamped_);
    }
    if (renderable_) {
        renderable_->updateInternalBounds(new_bounds);
    }
}

vec2f Element::getSizeReal() const {
    if (parent_) {
        return vec2f(parent_->getSizeReal()) * rect_on_parent_.size;
    }
    return ui::getRootSize();
}

Element::ContainerRef Element::getContainer() {
    MLE_ASSERT(isContainer());
    return static_cast<Container*>(renderable_.get());
}

void Element::setBackground(const Color& color) {
    background_ = color.toLinear();
    setRequestingRender();
}

void Element::setRequestingInternalBoundsUpdate(bool v) {
    behaviour_.set(BehaviourFlagBits::REQUESTING_INTERNAL_BOUNDS_UPDATE, v);
    setRequestingRender();
}

void Element::setRequestingExternalBoundsUpdate(bool v) {
    behaviour_.set(BehaviourFlagBits::REQUESTING_EXTERNAL_BOUNDS_UPDATE, v);
    setRequestingRender();
}

void Element::setTargetBounds(const sol::table& table) {
    {
        sol::object r_pos_x = table["pos_x"];
        sol::object r_pos_y = table["pos_y"];
        if (auto r_pos = table["pos"]; r_pos.valid()) {
            if (r_pos.is<sol::table>()) {
                auto t = r_pos.get<sol::table>();
                if (t[2].is<std::string>() && (t[2].get<std::string>().size() >= MIN_NAME_SIZE)) {
                    if (t[1].is<f32>()) {
                        target_bounds_->dep_pos_x = parseDepTable(t, true);
                        target_bounds_->dep_pos_y = parseDepTable(t, false);
                    } else {
                        auto s = strToPos(t[1].get<std::string>());
                        target_bounds_->dep_pos_y.name = target_bounds_->dep_pos_x.name = t[2].get<std::string>();
                        target_bounds_->dep_pos_x.val = s.x;
                        target_bounds_->dep_pos_y.val = s.y;
                    }
                } else {
                    r_pos_x = r_pos[1];
                    r_pos_y = r_pos[2];
                }
            } else if (r_pos.is<std::string>()) {
                auto s = r_pos.get<std::string>();
                if (isValidName(s)) {
                    target_bounds_->dep_pos_y.name = target_bounds_->dep_pos_x.name = s;
                } else {
                    auto parsed_s = strToPos(s);
                    target_bounds_->pos_x.val = parsed_s.x;
                    target_bounds_->pos_y.val = parsed_s.y;
                }
            } else if (r_pos.is<f32>()) {
                f32 v = r_pos.get<f32>();
                target_bounds_->pos_x.val = v;
                target_bounds_->pos_y.val = v;
            }
        }
        if (r_pos_x.valid()) {
            auto parsed = parseBound(r_pos_x, false, true);
            target_bounds_->pos_x = parsed.v;
            if (!parsed.dependencies.empty()) {
                target_bounds_->dep_pos_x = parsed.dependencies.at(0);
            }
        }
        if (r_pos_y.valid()) {
            auto parsed = parseBound(r_pos_y, false, false);
            target_bounds_->pos_y = parsed.v;
            if (!parsed.dependencies.empty()) {
                target_bounds_->dep_pos_y = parsed.dependencies.at(0);
            }
        }
    }

    {
        bool any_x = false, any_y = false;
        sol::object r_size_x = table["size_x"];
        sol::object r_size_y = table["size_y"];
        if (auto r_size = table["size"]; r_size.valid()) {
            if (r_size.is<std::string>()) {
                std::string s = r_size;
                auto p = parseStringBound(s, true);
                target_bounds_->size_x = target_bounds_->size_y = p.v;
                target_bounds_->dep_size_x = target_bounds_->dep_size_y = p.dependencies;
                any_x = any_y = true;
            } else if (r_size.is<sol::table>()) {
                if (r_size[2].is<std::string>() && (isValidName(r_size[2]))) {
                    target_bounds_->dep_size_x.emplace_back(r_size[1].get<f32>(), r_size[2].get<std::string>());
                    target_bounds_->dep_size_y.emplace_back(r_size[1].get<f32>(), r_size[2].get<std::string>());
                    any_x = any_y = true;
                } else {
                    r_size_x = r_size[1];
                    r_size_y = r_size[2];
                }
            } else if (r_size.is<f32>()) {
                f32 v = r_size.get<f32>();
                target_bounds_->size_x.val = v;
                target_bounds_->size_y.val = v;
            }
        }
        if (r_size_x.valid()) {
            auto parsed = parseBound(r_size_x, true, true);
            target_bounds_->size_x = parsed.v;
            target_bounds_->dep_size_x = std::move(parsed.dependencies);
            any_x = true;
        }
        if (r_size_y.valid()) {
            auto parsed = parseBound(r_size_y, true, false);
            target_bounds_->size_y = parsed.v;
            target_bounds_->dep_size_y = std::move(parsed.dependencies);
            any_y = true;
        }
        if (!any_x && !any_y) {
            target_bounds_->size_x.val = target_bounds_->size_y.val = 1;
            target_bounds_->size_x.type = target_bounds_->size_y.type = Bound::Type::REMAINING_SHARE;
        }
    }

    {
        sol::object r_origin_x = table["origin_x"];
        sol::object r_origin_y = table["origin_y"];
        if (auto r_origin = table["origin"]; r_origin.valid()) {
            if (r_origin.is<sol::table>()) {
                r_origin_x = r_origin[1];
                r_origin_y = r_origin[2];
            } else if (r_origin.is<std::string>()) {
                vec2f parsed_s = strToPos(r_origin);
                target_bounds_->origin.x = parsed_s.x;
                target_bounds_->origin.y = parsed_s.y;
            } else if (r_origin.is<f32>()) {
                f32 v = r_origin.get<f32>();
                target_bounds_->origin.x = v;
                target_bounds_->origin.x = v;
            }
        }
        if (r_origin_x.valid()) {
            auto parsed = parseBound(r_origin_x, false, true);
            target_bounds_->origin.x = parsed.v.val;
        }
        if (r_origin_y.valid()) {
            auto parsed = parseBound(r_origin_y, false, false);
            target_bounds_->origin.y = parsed.v.val;
        }
    }

    {
        if (auto r_aspect_ratio = table["aspect_ratio"]; r_aspect_ratio.valid()) {
            target_bounds_->aspect_ratio = r_aspect_ratio;
        }
    }

    {
        sol::object padding_t = table["padding_t"];
        sol::object padding_r = table["padding_r"];
        sol::object padding_b = table["padding_b"];
        sol::object padding_l = table["padding_l"];
        if (auto r_padding = table["padding"]; r_padding.valid()) {
            if (r_padding.is<sol::table>()) {
                sol::table padding_table = r_padding;
                if (auto val = padding_table["t"]; val.valid()) {
                    padding_t = val;
                }
                if (auto val = padding_table["r"]; val.valid()) {
                    padding_r = val;
                }
                if (auto val = padding_table["b"]; val.valid()) {
                    padding_b = val;
                }
                if (auto val = padding_table["l"]; val.valid()) {
                    padding_l = val;
                }
                if (auto val = padding_table[1]; val.valid()) {
                    padding_t = val;
                }
                if (auto val = padding_table[2]; val.valid()) {
                    padding_r = val;
                }
                if (auto val = padding_table[3]; val.valid()) {
                    padding_b = val;
                }
                if (auto val = padding_table[4]; val.valid()) {
                    padding_l = val;
                }
            } else {
                auto parsed = parseBound(r_padding, true, true);
                target_bounds_->padding_r = target_bounds_->padding_b = target_bounds_->padding_l = target_bounds_->padding_t = parsed.v;
            }
        }
        if (padding_t.valid()) {
            auto parsed = parseBound(padding_t, true, true);
            target_bounds_->padding_t = parsed.v;
        }
        if (padding_r.valid()) {
            auto parsed = parseBound(padding_r, true, true);
            target_bounds_->padding_r = parsed.v;
        }
        if (padding_b.valid()) {
            auto parsed = parseBound(padding_b, true, true);
            target_bounds_->padding_b = parsed.v;
        }
        if (padding_l.valid()) {
            auto parsed = parseBound(padding_l, true, true);
            target_bounds_->padding_l = parsed.v;
        }
        if (auto r_padding_lr = table["padding_lr"]; r_padding_lr.valid()) {
            auto parsed = parseBound(r_padding_lr, true, true);
            target_bounds_->padding_l = target_bounds_->padding_r = parsed.v;
        }
        if (auto r_padding_tb = table["padding_tb"]; r_padding_tb.valid()) {
            auto parsed = parseBound(r_padding_tb, true, true);
            target_bounds_->padding_t = target_bounds_->padding_b = parsed.v;
        }
    }

    {
        sol::object margin_t = table["margin_t"];
        sol::object margin_r = table["margin_r"];
        sol::object margin_b = table["margin_b"];
        sol::object margin_l = table["margin_l"];
        if (auto r_margin = table["margin"]; r_margin.valid()) {
            if (r_margin.is<sol::table>()) {
                sol::table margin_table = r_margin;
                if (auto val = margin_table["t"]; val.valid()) {
                    margin_t = val;
                }
                if (auto val = margin_table["r"]; val.valid()) {
                    margin_r = val;
                }
                if (auto val = margin_table["b"]; val.valid()) {
                    margin_b = val;
                }
                if (auto val = margin_table["l"]; val.valid()) {
                    margin_l = val;
                }
                if (auto val = margin_table[1]; val.valid()) {
                    margin_t = val;
                }
                if (auto val = margin_table[2]; val.valid()) {
                    margin_r = val;
                }
                if (auto val = margin_table[3]; val.valid()) {
                    margin_b = val;
                }
                if (auto val = margin_table[4]; val.valid()) {
                    margin_l = val;
                }
            } else {
                auto parsed = parseBound(r_margin, true, true);
                target_bounds_->margin_r = target_bounds_->margin_b = target_bounds_->margin_l = target_bounds_->margin_t = parsed.v;
            }
        }
        if (margin_t.valid()) {
            auto parsed = parseBound(margin_t, true, true);
            target_bounds_->margin_t = parsed.v;
        }
        if (margin_r.valid()) {
            auto parsed = parseBound(margin_r, true, true);
            target_bounds_->margin_r = parsed.v;
        }
        if (margin_b.valid()) {
            auto parsed = parseBound(margin_b, true, true);
            target_bounds_->margin_b = parsed.v;
        }
        if (margin_l.valid()) {
            auto parsed = parseBound(margin_l, true, true);
            target_bounds_->margin_l = parsed.v;
        }
        if (auto r_margin_lr = table["margin_lr"]; r_margin_lr.valid()) {
            auto parsed = parseBound(r_margin_lr, true, true);
            target_bounds_->margin_l = target_bounds_->margin_r = parsed.v;
        }
        if (auto r_margin_tb = table["margin_tb"]; r_margin_tb.valid()) {
            auto parsed = parseBound(r_margin_tb, true, true);
            target_bounds_->margin_t = target_bounds_->margin_b = parsed.v;
        }
    }
}

Element::ParsedBound Element::parseBound(const sol::object& o, bool is_size, bool is_x) {
    ParsedBound ret;
    if (o.is<f32>()) {
        ret.v.val = o.as<f32>();
    } else if (o.is<std::string>()) {
        ret = parseStringBound(o.as<std::string>(), is_size);
    } else if (o.is<sol::table>()) {
        auto t = o.as<sol::table>();
        if (t[2].is<std::string>()) {
            MLE_ASSERT(isValidName(t[2].get<std::string>()));
            ret.dependencies.emplace_back(parseDepTable(t, is_x));
        } else {
            for (auto& [_, val] : t.as<sol::table>()) {
                if (val.is<sol::table>()) {
                    ret.dependencies.emplace_back(parseDepTable(val, is_x));
                } else if (val.is<f32>()) {
                    ret.v.val = val.as<f32>();
                } else if (val.is<std::string>()) {
                    ret = parseStringBound(val.as<std::string>(), is_size);
                }
            }
        }
    }
    return ret;
}

Element::ParsedBound Element::parseStringBound(const std::string& s, bool is_size) {
    ParsedBound ret;
    if (s.size() == 1) {
        ret.v.val = charToPos(s[0]);
    } else if (isValidName(s)) {
        ret.dependencies.emplace_back(static_cast<int>(is_size), s);
    } else {
        auto [val, suffix] = extractNumAndSuffix<f32>(s);
        ret.v.val = val;
        if (!suffix.empty()) {
            if (suffix == "s") {
                ret.v.type = Bound::Type::STATIC_SHARE;
            } else if (suffix == "r") {
                ret.v.type = Bound::Type::REMAINING_SHARE;
            } else if (suffix == "root") {
                ret.v.type = Bound::Type::ROOT;
            } else if (suffix == "ssx") {
                ret.v.type = Bound::Type::SELF_X;
            } else if (suffix == "ssy") {
                ret.v.type = Bound::Type::SELF_Y;
            } else if (suffix == "psx") {
                ret.v.type = Bound::Type::PARENT_SIZE_X;
            } else if (suffix == "psy") {
                ret.v.type = Bound::Type::PARENT_SIZE_Y;
            } else {
                MLE_UNREACHABLE_LOG("Unknown suffix: {}", suffix);
            }
        }
    }
    return ret;
}

void Element::log(const std::string& prefix) {
    MLE_D("{}-- {} --", prefix, name_);
    MLE_D("{}  parent: {}", prefix, (parent_ ? parent_->name_ : "null"));
    MLE_D("{}  behaviour: {}", prefix, behaviour_);
    if (table_) {
        MLE_D("{}  table: {}", prefix, table_);
    } else {
        MLE_D("{}  table: -", prefix);
    }
    MLE_D("{}  background: {}", prefix, background_);
    MLE_D("{}  layer: {}", prefix, layer_);
    MLE_D("{}  hover_state: {}", prefix, hover_state_);
    MLE_D("{}  target_bounds: ", prefix);
    MLE_D("{}    pos_x: {}, pos_y: {}", prefix, target_bounds_->pos_x, target_bounds_->pos_y);
    if (!target_bounds_->dep_pos_x.name.empty()) {
        MLE_D("{}    dep_pos_x: {}", prefix, target_bounds_->dep_pos_x);
    }
    if (!target_bounds_->dep_pos_y.name.empty()) {
        MLE_D("{}    dep_pos_y: {}", prefix, target_bounds_->dep_pos_y);
    }
    MLE_D("{}    size_x: {}, size_y: {}", prefix, target_bounds_->size_x, target_bounds_->size_y);
    if (!target_bounds_->dep_size_x.empty()) {
        MLE_D("{}    dep_size_x: {}", prefix, fmt::join(target_bounds_->dep_size_x, ", "));
    }
    if (!target_bounds_->dep_size_y.empty()) {
        MLE_D("{}    dep_size_y: {}", prefix, fmt::join(target_bounds_->dep_size_y, ", "));
    }
    MLE_D("{}    padding_t: {}, padding_r: {}, padding_b: {}, padding_l: {}", prefix, target_bounds_->padding_t, target_bounds_->padding_r,
          target_bounds_->padding_b, target_bounds_->padding_l);
    MLE_D("{}    margin_t: {}, margin_r: {}, margin_b: {}, margin_l: {}", prefix, target_bounds_->margin_t, target_bounds_->margin_r, target_bounds_->margin_b,
          target_bounds_->margin_l);
    MLE_D("{}    origin: {}", prefix, target_bounds_->origin);
    MLE_D("{}    aspect_ratio: {}", prefix, target_bounds_->aspect_ratio);

    if (hasRenderable()) {
        renderable_->log(prefix + "  ");
    }
}

f32 Element::getTargetAspectRatio() const {
    if (target_bounds_->aspect_ratio != 0.0F) {
        return target_bounds_->aspect_ratio;
    }
    if (hasRenderable()) {
        return renderable_->getAspectRatio();
    }
    return 0.0F;
}

void Element::registerRenderableType(const std::string& trigger, RenderableCreateFnVariant&& fn) {
    MLE_D("Registering renderable create function: {}", trigger);
    renderable_create_functions_.emplace(trigger, std::move(fn));
}

std::expected<Element::RenderableCreateFnVariant, Result> Element::findRenderableCreateFn(const std::string& trigger) {
    auto it = renderable_create_functions_.find(trigger);
    if (it == renderable_create_functions_.end()) {
        return std::unexpected(Result::NOT_FOUND);
    }
    return it->second;
}

void Element::makeRenderable(const sol::table& table) {
    bool has_renderable = false;
    Element::RenderableCreateFnVariant fn_v;
    if (auto r_renderable = table["renderable"]; r_renderable.valid()) {
        if (r_renderable.is<std::string>()) {
            auto fn = findRenderableCreateFn(r_renderable.get<std::string>());
            if (!fn) {
                MLE_W("Failed to find renderable create function: {}", r_renderable.get<std::string>());
                return;
            }
            fn_v = *fn;
        } else {
            fn_v = r_renderable.get<sol::function>();
        }
        has_renderable = true;
    } else {
        for (auto& [key, _] : table) {
            if (key.is<std::string>()) {
                auto fn = findRenderableCreateFn(key.as<std::string>());
                if (fn) {
                    fn_v = *fn;
                    has_renderable = true;
                    break;
                }
            }
        }
    }

    if (!has_renderable) {
        return;
    }

    if (std::holds_alternative<sol::function>(fn_v)) {
        auto fn = std::get<sol::function>(fn_v);
        auto fn_result = fn(this, table);

        auto tuple = fn_result.get<std::tuple<RenderableRef, sol::table>>();

        renderable_ = RenderableHnd{std::get<0>(tuple)};
        renderable_->set(std::get<1>(tuple));
    } else {
        renderable_ = RenderableHnd{std::get<RenderableCreateFn>(fn_v)(this)};
        renderable_->set(table);
    }
}

void Element::parseHoverFunctional(const sol::table& table) {
    if (sol::object r_hover = table["on_hover"]; r_hover.valid()) {
        setOnHover(r_hover);
    }
    if (sol::object r_hover_in = table["on_hover_in"]; r_hover_in.valid()) {
        setOnHoverIn(r_hover_in);
    }
    if (sol::object r_hover_out = table["on_hover_out"]; r_hover_out.valid()) {
        setOnHoverOut(r_hover_out);
    }
    if (sol::object r_listen_keys = table["listen_keys_on_hover"]; r_listen_keys.valid()) {
        for (auto& [name, key] : r_listen_keys.as<sol::table>()) {
            listenKeyOnHover(name.as<std::string>(), key);
        }
    }
}

void Element::setOnHover(const sol::object& obj) {
    if (obj.is<sol::function>()) {
        addHover();
        hoverable_->on_hover = obj.as<sol::function>();
    } else {
        setOnHover(obj.as<ElementFn>());
    }
}

void Element::setOnHover(ElementFnVariant&& fn) {
    addHover();
    hoverable_->on_hover = std::move(fn);
}

void Element::setOnHoverIn(const sol::object& obj) {
    if (obj.is<sol::function>()) {
        addHover();
        hoverable_->on_hover_in = obj.as<sol::function>();
    } else {
        setOnHoverIn(obj.as<ElementFn>());
    }
}

void Element::setOnHoverIn(ElementFnVariant&& fn) {
    addHover();
    hoverable_->on_hover_in = std::move(fn);
}

void Element::setOnHoverOut(const sol::object& obj) {
    if (obj.is<sol::function>()) {
        addHover();
        hoverable_->on_hover_out = obj.as<sol::function>();
    } else {
        setOnHoverOut(obj.as<ElementFn>());
    }
}

void Element::setOnHoverOut(ElementFnVariant&& fn) {
    addHover();
    hoverable_->on_hover_out = std::move(fn);
}

void Element::setOnUpdate(ElementFnVariant&& fn) {
    on_update_ = std::move(fn);
}

void Element::setOpacity(f32 opacity) {
    opacity_ = opacity;
    setRequestingRender();
}

void Element::setAlwaysRender(bool v) {
    behaviour_.set(BehaviourFlagBits::ALWAYS_RENDER, v);
    setRequestingRender();
    MLE_T("Setting always render for: {} to: {}", name_, v);
}

void Element::listenKeyOnHover(const std::string& name, const sol::object& obj) {
    if (obj.is<sol::function>()) {
        addHover();
        auto [key, state, mods] = window::toKeyTuple(name);
        auto hnd = std::make_unique<window::KeyListener>();
        auto fn = obj.as<sol::function>();
        hnd->setCallback([fn, this](UserPtr) { fn.call(this); });
        hnd->setKey(key);
        hnd->setState(state);
        hnd->setMods(mods);
        hoverable_->key_listeners.push_back(std::move(hnd));
    } else {
        MLE_UNREACHABLE_TODO;
    }
}

void Element::listenKeyOnHover(const std::string& name, const ElementFnVariant& fn) {
    addHover();
    auto [key, state, mods] = window::toKeyTuple(name);
    auto hnd = std::make_unique<window::KeyListener>();
    if (std::holds_alternative<ElementFn>(fn)) {
        hnd->setCallback([ifn = std::get<ElementFn>(fn), this](UserPtr) { ifn(this); });
    } else {
        hnd->setCallback([ifn = std::get<ElementFn2>(fn)](UserPtr) { ifn(); });
    }
    hnd->setKey(key);
    hnd->setState(state);
    hnd->setMods(mods);
    hoverable_->key_listeners.push_back(std::move(hnd));
}

void Element::listenKey(const std::string& name, const sol::object& obj) {
    if (obj.is<sol::function>()) {
        auto [key, state, mods] = window::toKeyTuple(name);
        auto fn = obj.as<sol::function>();
        key_listeners_.emplace_back(window::UserInputManager::makeKeyListener(nullptr, [fn, this](UserPtr) { fn.call(this); }, key, state, mods));
    } else {
        listenKey(name, obj.as<ElementFn>());
    }
}

void Element::listenKey(const std::string& name, const ElementFnVariant& fn) {
    auto [key, state, mods] = window::toKeyTuple(name);
    MLE_ASSERT_LOG(std::ranges::find_if(key_listeners_,
                                        [key, state, mods](const auto& kl) {
                                            return kl->getKey() == key && kl->getState() == state && kl->getMods() == mods;
                                        }) == key_listeners_.end(),
                   "Key listener already exists for: {}, element: {}", name, name_);
    if (std::holds_alternative<ElementFn>(fn)) {
        key_listeners_.emplace_back(
            window::UserInputManager::makeKeyListener(nullptr, [ifn = std::get<ElementFn>(fn), this](UserPtr) { ifn(this); }, key, state, mods));
    } else if (std::holds_alternative<ElementFn2>(fn)) {
        key_listeners_.emplace_back(window::UserInputManager::makeKeyListener(nullptr, [ifn = std::get<ElementFn2>(fn)](UserPtr) { ifn(); }, key, state, mods));
    } else {
        MLE_UNREACHABLE;
    }
}

void Element::unlistenKey(const std::string& name) {
    MLE_I("{} Unlistening key: {}", name_, name);

    auto [key, state, mods] = window::toKeyTuple(name);
    auto it = std::ranges::remove_if(key_listeners_,
                                     [key, state, mods](const auto& kl) { return kl->getKey() == key && kl->getState() == state && kl->getMods() == mods; });
    auto match_count = std::distance(it.begin(), it.end());
    MLE_ASSERT(match_count <= 1);

    ui::addToDeletionQueue([p = key_listeners_.back().release()]() { delete p; });
    key_listeners_.pop_back();
}

void Element::addHover() {
    if (!hoverable_) {
        hoverable_ = std::make_unique<Hoverable>();
        setHoverable();
    }
}

void Element::callElementFn(const ElementFnVariant& fn) {
    if (std::holds_alternative<std::monostate>(fn)) {
        return;
    }

    if (std::holds_alternative<sol::function>(fn)) {
        std::get<sol::function>(fn).call(this);
    } else if (std::holds_alternative<ElementFn>(fn)) {
        std::get<ElementFn>(fn)(this);
    } else if (std::holds_alternative<ElementFn2>(fn)) {
        std::get<ElementFn2>(fn)();
    }
}

void Element::callEvent(const std::string& event_name, const sol::object& event) {
    auto fn_it = std::ranges::find_if(event_handlers_, [&event_name](const auto& p) { return p.first == event_name; });
    MLE_ASSERT(fn_it != event_handlers_.end());
    if (std::holds_alternative<sol::function>(fn_it->second)) {
        std::get<sol::function>(fn_it->second).call(this, event);
    } else if (std::holds_alternative<EventFn>(fn_it->second)) {
        std::get<EventFn>(fn_it->second)(this, event);
    }
}

void Element::listenEvent(const std::string& event_name, EventFnVariant&& fn) {
    ui::listenEvent(event_name, this);
    event_handlers_.emplace_back(event_name, std::move(fn));
}

void Element::unlistenEvent(const std::string& event_name) {
    ui::unlistenEvent(event_name, this);
    std::erase_if(event_handlers_, [&event_name](const auto& p) { return p.first == event_name; });
}

sol::table Element::getTable() {
    if (!table_) {
        table_ = lua::createTable();
    }
    return table_;
}

void Element::registerLuaTypes() {
    void (Element::*listen_key_o)(const std::string&, const sol::object&) = &Element::listenKey;
    void (Element::*set_bg_o)(const sol::object& o) = &Element::setBackground;

    auto ui_element_ut = lua::newUsertype<Element>("ui_Element", sol::no_constructor);

    ui_element_ut["opacity"] = sol::property(&Element::getOpacity, &Element::setOpacity);
    ui_element_ut["background"] = sol::property(&Element::getBackground, set_bg_o);

    ui_element_ut["getName"] = &Element::getName;
    ui_element_ut["getRenderable"] = &Element::getRenderable;
    ui_element_ut["getTable"] = &Element::getTable;
    ui_element_ut["getAspectRatio"] = &Element::getAspectRatio;

    ui_element_ut["setVisible"] = &Element::setVisible;
    ui_element_ut["setOnUpdate"] = [](ElementRef self, const sol::function& fn) { self->setOnUpdate(fn); };

    ui_element_ut["listen"] = [](ElementRef self, const std::string& event, const sol::function& fn) { self->listenEvent(event, fn); };
    ui_element_ut["unlisten"] = &Element::unlistenEvent;

    ui_element_ut["listenKey"] = listen_key_o;
    ui_element_ut["unlistenKey"] = &Element::unlistenKey;

    auto ui_element_renderable_ut = lua::newUsertype<Element::Renderable>("ui_Element_Renderable", sol::no_constructor);
    auto ut_container = lua::newUsertype<Element::Container>("ui_Element_Container", sol::no_constructor, sol::base_classes, sol::bases<Element::Renderable>());
    ut_container["addChild"] = &Element::Container::addChild;
    ut_container["removeChild"] = &Element::Container::removeChild;

    lua::getTable("ui")["Element_registerRenderableType"] = [](const std::string& trigger, const sol::function& fn) { registerRenderableType(trigger, fn); };
}

///////////////////////////////////////////////////
// ----------------- Container ----------------- //
///////////////////////////////////////////////////

void Element::Container::addChild(const sol::table& table) {
    std::string name;
    if (auto r_child_name = table[1]; r_child_name.valid()) {
        name = r_child_name;
    } else if (auto r_child_name = table["name"]; r_child_name.valid()) {
        name = r_child_name;
    } else {
        name = "__" + std::to_string(getNextId());
    }

    MLE_D("Adding child {} to {}", name, element_->name_);

    auto child = std::make_unique<Element>(table, name, element_);
    children_.emplace_back(child->getName(), std::move(child));

    children_bounds_dirty_ = true;
    layers_dirty_ = true;
    element_->setRequestingInternalBoundsUpdate();
}

void Element::Container::addChildren(const sol::table& table) {
    for (auto& [k, v] : table) {
        if (k.is<std::string>()) {
            MLE_UNREACHABLE_LOG("Cannot name children with keys cuz it messes up order, use the name field instead or [1]. Offender: {}", k.as<std::string>());
        }

        addChild(v);
    }
}

void Element::Container::removeChild(const std::string& name) {
    MLE_D("Removing child {} from container: {}", name, element_->getName());

    auto it = std::ranges::remove_if(children_, [&name](const auto& p) { return p.first == name; });
    MLE_ASSERT(std::distance(it.begin(), it.end()) == 1);
    children_.erase(it.begin());

    children_bounds_dirty_ = true;
    layers_dirty_ = true;
}

void Element::Container::removeChildren() {
    if (children_.empty()) {
        return;
    }

    MLE_D("Removing all {} children from container: {}", children_.size(), element_->getName());

    children_.clear();
    children_to_update_internals_.clear();
    children_sorted_by_layer_.clear();
    children_bounds_dirty_ = false;
    layers_dirty_ = false;
}

bool Element::Container::render() {
    auto needs_render = false;
    for (auto& [_, child] : children_) {
        needs_render |= child->render();
    }
    return needs_render;
}

void Element::Container::getRenderData(int layer, RenderableData& data) const {
    for (const auto& [_, child] : children_) {
        child->getRenderData(layer, data);
    }
}

void Element::Container::update() {
    for (auto& [_, child] : children_) {
        child->update();
    }
}

void Element::Container::notifyChildRequestedExternalBoundsUpdate() {
    children_bounds_dirty_ = true;
    element_->setRequestingInternalBoundsUpdate();
}

void Element::Container::requestChildInternalBoundsUpdate(ElementRef child) {
    children_to_update_internals_.emplace_back(child);
    element_->setRequestingInternalBoundsUpdate();
}

void Element::Container::log(const std::string& prefix) const {
    MLE_D("{}Children: {}", prefix, children_.size());
    for (const auto& [k, v] : children_) {
        v->log(prefix);
    }
}

usize Element::Container::getChildPos(const std::string& name) {
    for (usize i = 0; i < children_.size(); i++) {
        if (children_[i].first == name) {
            return i;
        }
    }
    MLE_THROW(ELEMENT_NOT_FOUND, "Child {} not found in container {}", name, element_->getName());
}

ElementRef Element::Container::getChild(usize pos) {
    MLE_ASSERT(pos < children_.size());
    return children_[pos].second.get();
}

ElementRef Element::Container::getChild(const std::string& name) {
    return getChild(getChildPos(name));
}

void Element::Container::checkHover() {
    for (ElementRef child : getChildrenSortedByLayer()) {
        child->checkHover(element_->last_cursor_internal_pos_);
    }
}

const std::vector<ElementRef>& Element::Container::getChildrenSortedByLayer() {
    if (layers_dirty_) {
        children_sorted_by_layer_.clear();
        children_sorted_by_layer_.reserve(children_.size());
        for (auto& [_, child] : children_) {
            children_sorted_by_layer_.emplace_back(child.get());
        }
        std::ranges::sort(children_sorted_by_layer_, [](auto& a, auto& b) { return a->getLayer() < b->getLayer(); });
        layers_dirty_ = false;
    }
    return children_sorted_by_layer_;
}

void Element::Container::updateInternalBounds(std::optional<NewBounds> new_bounds) {
    if (new_bounds) {
        updateChildrenBounds();
    }
    if (children_bounds_dirty_) {
        updateChildrenBounds();
    }

    if (!children_to_update_internals_.empty()) {
        for (auto* child : children_to_update_internals_) {
            child->updateInternalBounds();
        }
        children_to_update_internals_.clear();
    }
}
}  // namespace mle::ui
