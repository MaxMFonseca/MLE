#include "Types.h"

#include "mle/core/Assert.h"

namespace mle::lua {
std::string toString(const sol::object& obj, const std::string& prefix) {
    if (!obj.valid()) {
        return "<invalid>";
    }

    constexpr int MAX_DEPTH = 32;  // TODO: make this configurable outside of this function

    struct StackEntry {
        std::string name;
        sol::object value;
        std::string prefix;
        int depth;
    };

    std::string result;
    std::vector<StackEntry> stack;
    stack.push_back({"", obj, prefix + " > ", 0});

    while (!stack.empty()) {
        auto [name, value, prefix, depth] = stack.back();
        stack.pop_back();

        MLE_ASSERT_LOG(depth <= MAX_DEPTH, "Lua table exceeds maximum toString depth of {}", MAX_DEPTH);

        sol::type t = value.get_type();
        std::string label = name.empty() ? "" : fmt::format("{} = ", name);

        switch (t) {
            case sol::type::boolean:
                result += fmt::format("{}{}{}\n", prefix, label, value.as<bool>() ? "true" : "false");
                break;
            case sol::type::number:
                result += fmt::format("{}{}{}\n", prefix, label, value.as<double>());
                break;
            case sol::type::string:
                result += fmt::format("{}{}\"{}\"\n", prefix, label, value.as<std::string>());
                break;
            case sol::type::nil:
                result += fmt::format("{}{}nil\n", prefix, label);
                break;
            case sol::type::function:
                result += fmt::format("{}{}<function>\n", prefix, label);
                break;
            case sol::type::userdata:
                result += fmt::format("{}{}<userdata>\n", prefix, label);
                break;
            case sol::type::lightuserdata:
                result += fmt::format("{}{}<lightuserdata>\n", prefix, label);
                break;
            case sol::type::thread:
                result += fmt::format("{}{}<thread>\n", prefix, label);
                break;
            case sol::type::none:
                result += fmt::format("{}{}<none>\n", prefix, label);
                break;
            case sol::type::table: {
                sol::table table = value.as<sol::table>();
                result += fmt::format("{}{}{{}}\n", prefix, label);

                std::string next_prefix = prefix;
                next_prefix += "> ";
                std::vector<StackEntry> entries;

                for (auto& [k, v] : table) {
                    std::string key;
                    if (k.get_type() == sol::type::string) {
                        key = k.as<std::string>();
                    } else if (k.get_type() == sol::type::number) {
                        key = fmt::format("[{}]", k.as<double>());
                    } else {
                        key = "<key>";
                    }
                    entries.push_back({key, v, next_prefix, depth + 1});
                }

                std::ranges::reverse(entries);
                for (auto& entry : entries) {
                    stack.push_back(std::move(entry));
                }

                break;
            }
            default:
                result += fmt::format("{}{}<unknown>\n", prefix, label);
                break;
        }
    }

    return result;
}
}  // namespace mle::lua
