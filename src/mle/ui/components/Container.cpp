#include "Container.h"

namespace mle::ui {
EntityStorage::~EntityStorage() {
    if (!isArray()) {
        children_.vec.~vector();
    }
}

std::span<const EntityStorage::Entry> EntityStorage::get() const {
    if (isArray()) {
        return {children_.array.data(), count_};
    }
    return {children_.vec.data(), children_.vec.size()};
}

void EntityStorage::add(std::string name, entt::entity e, usize pos) {
    if (isArray()) {
        if (count_ < 6) {
            if (pos == max<usize>()) {
                children_.array.at(count_) = {.name = std::move(name), .e = e};
            } else {
                for (usize i = count_; i > pos; --i) {
                    children_.array.at(i) = children_.array.at(i - 1);
                }
                children_.array.at(pos) = {.name = std::move(name), .e = e};
            }
            count_++;
        } else {
            auto copy = children_.array;
            new (&children_.vec) std::vector<Entry>(copy.begin(), copy.end());

            count_++;
            insertInVec({.name = std::move(name), .e = e}, pos);
        }
    } else {
        insertInVec({.name = std::move(name), .e = e}, pos);
    }
}

void EntityStorage::insertInVec(Entry child, usize pos) {
    if (pos == max<usize>()) {
        children_.vec.push_back(std::move(child));
    } else {
        children_.vec.insert(children_.vec.begin() + as<uint>(pos), std::move(child));
    }
}

void EntityStorage::remove(usize idx) {
    if (idx != max<usize>()) {
        if (isArray()) {
            for (usize i = idx; i < count_ - 1; ++i) {
                children_.array.at(i) = children_.array.at(i + 1);
            }
            count_--;
        } else {
            children_.vec.erase(children_.vec.begin() + as<uint>(idx));
        }
    }
}

void EntityStorage::remove(entt::entity e) {
    auto idx = getIdx(e);
    if (idx) {
        remove(*idx);
    }
}

void EntityStorage::remove(const std::string& name) {
    auto idx = getIdx(name);
    if (idx) {
        remove(*idx);
    }
}

std::optional<usize> EntityStorage::getIdx(entt::entity e) const {
    auto children = get();
    for (usize i = 0; i < children.size(); ++i) {
        if (children[i].e == e) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<usize> EntityStorage::getIdx(const std::string& name) const {
    auto children = get();
    for (usize i = 0; i < children.size(); ++i) {
        if (children[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

entt::entity EntityStorage::getEFromName(const std::string& name) const {
    auto idx = getIdx(name);
    if (idx) {
        return get()[*idx].e;
    }
    return entt::null;
}

std::string EntityStorage::getNameFromE(entt::entity e) const {
    auto idx = getIdx(e);
    if (idx) {
        return get()[*idx].name;
    }
    return {};
}

namespace comp {}
}  // namespace mle::ui
