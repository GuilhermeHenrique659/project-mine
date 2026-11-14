#pragma once

#include <algorithm>
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
class Registry {
public:
    // Ids are sequential and never reused.
    EntityId create(std::string label) {
        const EntityId id = next_id_++;
        entities_.push_back({id, std::move(label)});
        return id;
    }

    // Removes the entity and all its components. Unknown ids are ignored.
    void destroy(EntityId id) {
        const auto found = find_iterator(id);
        if (found == entities_.end()) {
            return;
        }
        entities_.erase(found);
        for (auto& [type, pool] : pools_) {
            pool->erase(id);
        }
    }

    bool is_alive(EntityId id) const { return find(id) != nullptr; }

    const Entity* find(EntityId id) const {
        const auto found = find_iterator(id);
        return found == entities_.end() ? nullptr : &*found;
    }

    const std::vector<Entity>& entities() const { return entities_; }

    // Attaches a component, replacing the one of the same type if present.
    template <typename C>
    C& add(EntityId id, C component) {
        return pool<C>().items.insert_or_assign(id, std::move(component)).first->second;
    }

    // The entity's component, or null when it has none of that type.
    template <typename C>
    C* get(EntityId id) {
        const auto* found = find_pool<C>();
        if (found == nullptr) {
            return nullptr;
        }
        auto item = found->items.find(id);
        return item == found->items.end() ? nullptr : const_cast<C*>(&item->second);
    }

    template <typename C>
    const C* get(EntityId id) const {
        const auto* found = find_pool<C>();
        if (found == nullptr) {
            return nullptr;
        }
        const auto item = found->items.find(id);
        return item == found->items.end() ? nullptr : &item->second;
    }

    template <typename C>
    bool has(EntityId id) const {
        return get<C>(id) != nullptr;
    }

    template <typename C>
    void remove(EntityId id) {
        const auto* found = find_pool<C>();
        if (found == nullptr) {
            return;
        }
        const_cast<ComponentPool<C>*>(found)->items.erase(id);
    }

    // Calls fn(EntityId, Cs&...) for every entity that has all of Cs. Do not create or
    // destroy entities from inside fn.
    template <typename... Cs, typename Fn>
    void each(Fn&& fn) {
        for (const Entity& entity : entities_) {
            if (!(has<Cs>(entity.id) && ...)) {
                continue;
            }
            fn(entity.id, *get<Cs>(entity.id)...);
        }
    }

private:
    struct PoolBase {
        virtual ~PoolBase() = default;
        virtual void erase(EntityId id) = 0;
    };

    template <typename C>
    struct ComponentPool : PoolBase {
        void erase(EntityId id) override { items.erase(id); }
        std::unordered_map<EntityId, C> items;
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
    const ComponentPool<C>* find_pool() const {
        const auto found = pools_.find(std::type_index(typeid(C)));
        return found == pools_.end() ? nullptr
                                     : static_cast<const ComponentPool<C>*>(found->second.get());
    }

    std::vector<Entity>::iterator find_iterator(EntityId id) {
        return std::find_if(entities_.begin(), entities_.end(),
                            [id](const Entity& entity) { return entity.id == id; });
    }
    std::vector<Entity>::const_iterator find_iterator(EntityId id) const {
        return std::find_if(entities_.begin(), entities_.end(),
                            [id](const Entity& entity) { return entity.id == id; });
    }

    EntityId next_id_ = 1;
    std::vector<Entity> entities_;
    std::unordered_map<std::type_index, std::unique_ptr<PoolBase>> pools_;
};

} // namespace core
