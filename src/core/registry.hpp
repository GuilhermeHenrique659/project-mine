#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/entity.hpp"

namespace core {

// Entities and their components. An entity is an id and a label; everything else is a
// component attached to it (at most one per type). Systems read and write components
// through each<>(). Knows nothing about rendering or input.
//
// Storage: ids are sequential and never reused, so lookups index vectors by id (O(1)).
// Each component type keeps its owners sorted by id (= creation order), and each<>() walks
// the smallest of the requested pools. Component addresses stay valid until that component
// is removed or replaced.
class Registry {
public:
    template <typename C>
    using Hook = std::function<void(EntityId, const C&)>;

    Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    EntityId create(std::string label) {
        const EntityId id = next_id_++;
        slot_of_.resize(next_id_, 0);
        entities_.push_back({id, std::move(label)});
        slot_of_[id] = entities_.size();
        return id;
    }

    // Removes the entity and all its components (remove hooks run). Unknown ids are ignored.
    void destroy(EntityId id) {
        if (!is_alive(id)) {
            return;
        }
        for (auto& [type, pool] : pools_) {
            pool->erase(id);
        }
        const std::size_t index = slot_of_[id] - 1;
        entities_.erase(entities_.begin() + static_cast<std::ptrdiff_t>(index));
        slot_of_[id] = 0;
        for (std::size_t i = index; i < entities_.size(); ++i) {
            slot_of_[entities_[i].id] = i + 1;
        }
    }

    bool is_alive(EntityId id) const { return id < slot_of_.size() && slot_of_[id] != 0; }

    const Entity* find(EntityId id) const {
        return is_alive(id) ? &entities_[slot_of_[id] - 1] : nullptr;
    }

    // In creation order.
    const std::vector<Entity>& entities() const { return entities_; }

    // Attaches a component, replacing the one of the same type if present (the replaced one
    // goes through the remove hooks first).
    template <typename C>
    C& add(EntityId id, C component) {
        return pool<C>().put(id, std::move(component));
    }

    // The entity's component, or null when it has none of that type.
    template <typename C>
    C* get(EntityId id) {
        auto* found = find_pool<C>();
        return found == nullptr ? nullptr : found->find(id);
    }

    template <typename C>
    const C* get(EntityId id) const {
        const auto* found = find_pool<C>();
        return found == nullptr ? nullptr : found->find(id);
    }

    template <typename C>
    bool has(EntityId id) const {
        return get<C>(id) != nullptr;
    }

    template <typename C>
    void remove(EntityId id) {
        if (auto* found = find_pool<C>()) {
            found->erase(id);
        }
    }

    // Calls fn(EntityId, Cs&...) for every entity that has all of Cs, in creation order.
    // Walks a snapshot of the smallest pool, so fn may add or remove components and
    // entities; an entity that loses one of Cs before its turn is skipped.
    template <typename C, typename... Cs, typename Fn>
    void each(Fn&& fn) {
        const std::vector<EntityId>* smallest = nullptr;
        if (!smallest_ids<C, Cs...>(smallest)) {
            return;
        }
        const std::vector<EntityId> ids = *smallest;
        for (const EntityId id : ids) {
            C* first = get<C>(id);
            if (first == nullptr || !(has<Cs>(id) && ...)) {
                continue;
            }
            fn(id, *first, *get<Cs>(id)...);
        }
    }

    // Called after a component of type C is attached (also when it replaces another).
    template <typename C>
    void on_add(Hook<C> hook) {
        pool<C>().added.push_back(std::move(hook));
    }

    // Called before a component of type C goes away: removed, replaced or destroyed with
    // its entity. Not called when the registry itself is destroyed.
    template <typename C>
    void on_remove(Hook<C> hook) {
        pool<C>().removed.push_back(std::move(hook));
    }

    // One object of type T stored with the registry (an index, a cache), created on first
    // use and destroyed with the registry.
    template <typename T>
    T& context() {
        auto& slot = contexts_[std::type_index(typeid(T))];
        if (!slot) {
            slot = std::make_unique<ContextHolder<T>>();
        }
        return static_cast<ContextHolder<T>&>(*slot).value;
    }

    // The context object of type T, null when it was never created.
    template <typename T>
    const T* find_context() const {
        const auto found = contexts_.find(std::type_index(typeid(T)));
        return found == contexts_.end() ? nullptr
                                        : &static_cast<const ContextHolder<T>&>(*found->second).value;
    }

private:
    struct PoolBase {
        virtual ~PoolBase() = default;
        virtual void erase(EntityId id) = 0;
    };

    template <typename C>
    struct ComponentPool : PoolBase {
        C* find(EntityId id) const {
            return id < by_id.size() ? by_id[id].get() : nullptr;
        }

        C& put(EntityId id, C component) {
            if (id >= by_id.size()) {
                by_id.resize(id + 1);
            }
            if (by_id[id]) {
                notify(removed, id, *by_id[id]);
                *by_id[id] = std::move(component);
            } else {
                by_id[id] = std::make_unique<C>(std::move(component));
                // New entities have the highest ids: appending is the usual case.
                ids.insert(std::lower_bound(ids.begin(), ids.end(), id), id);
            }
            notify(added, id, *by_id[id]);
            return *by_id[id];
        }

        void erase(EntityId id) override {
            if (find(id) == nullptr) {
                return;
            }
            notify(removed, id, *by_id[id]);
            by_id[id].reset();
            ids.erase(std::lower_bound(ids.begin(), ids.end(), id));
        }

        static void notify(const std::vector<Hook<C>>& hooks, EntityId id, const C& component) {
            for (const auto& hook : hooks) {
                hook(id, component);
            }
        }

        std::vector<std::unique_ptr<C>> by_id;
        std::vector<EntityId> ids; // sorted
        std::vector<Hook<C>> added;
        std::vector<Hook<C>> removed;
    };

    struct ContextBase {
        virtual ~ContextBase() = default;
    };

    template <typename T>
    struct ContextHolder : ContextBase {
        T value{};
    };

    template <typename C>
    ComponentPool<C>& pool() {
        auto& slot = pools_[std::type_index(typeid(C))];
        if (!slot) {
            slot = std::make_unique<ComponentPool<C>>();
        }
        return static_cast<ComponentPool<C>&>(*slot);
    }

    template <typename C>
    ComponentPool<C>* find_pool() const {
        const auto found = pools_.find(std::type_index(typeid(C)));
        return found == pools_.end() ? nullptr : static_cast<ComponentPool<C>*>(found->second.get());
    }

    // The ids of the smallest pool among Cs; false when one of them has no pool at all.
    template <typename... Cs>
    bool smallest_ids(const std::vector<EntityId>*& smallest) const {
        const auto consider = [&smallest](const auto* found) {
            if (found == nullptr) {
                return false;
            }
            if (smallest == nullptr || found->ids.size() < smallest->size()) {
                smallest = &found->ids;
            }
            return true;
        };
        return (consider(find_pool<Cs>()) && ...);
    }

    EntityId next_id_ = 1;
    std::vector<Entity> entities_;
    // Entity id -> index in entities_ + 1; 0 = not alive.
    std::vector<std::size_t> slot_of_ = std::vector<std::size_t>(1, 0);
    std::unordered_map<std::type_index, std::unique_ptr<PoolBase>> pools_;
    std::unordered_map<std::type_index, std::unique_ptr<ContextBase>> contexts_;
};

} // namespace core
