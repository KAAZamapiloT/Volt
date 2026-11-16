#pragma once

#include <volt/types/EngineTypes.hpp>
#include <volt/ECS/Entity.hpp>
#include <volt/ECS/Component.hpp>
#include <volt/ECS/Archetype.hpp>
#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace volt {

/// <summary>
/// Acts as  Database mangagement system for entities 
/// records-> Array of entity records stores a entity
/// Archtypes -> stores avalible archtypes 
///     archetypeMap -> stores a map of Componenet mask -> Archtype  gives which Archtypes contatins a particular component mask
///     free entity id -> not in use entity ID's
/// </summary>
class Registry {
public:
    Registry() {
        
        get_or_create_archetype(0);
    }

    ~Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;


	// Creates a new entity and returns it. The entity will be in the "empty" archetype (no components).
    Entity CreateEntity() {
        u32 id = 0;
        if (!freeEntityIDs.empty()) {
            id = freeEntityIDs.back();
            freeEntityIDs.pop_back();
        } else {
            id = static_cast<u32>(records.size());
            records.push_back(EntityRecord{ nullptr, 0, 0});
        }

        Entity e{ id, records[id].generation };
        Archetype* empty_arch = get_or_create_archetype(0);
        records[id].archetype = empty_arch;
        records[id].row = static_cast<u32>(empty_arch->add_entity(e));
        return e;
    }

    // return if a particular entity is alive
    [[nodiscard]] bool IsAlive(Entity entity) const noexcept {
        return entity.id < records.size() &&
               records[entity.id].archetype != nullptr &&
               records[entity.id].generation == entity.generation;
    }


    // destorying a particular entity 
    void DestroyEntity(Entity entity) {
        if (!IsAlive(entity)) return;

        EntityRecord& rec = records[entity.id];
        if (rec.archetype) {
            Entity swapped = rec.archetype->remove_entity(rec.row);
            if (swapped != NullEntity ) {
                records[swapped.id].row = rec.row;
            }
        }

        rec.generation++;
        rec.archetype = nullptr;
        rec.row = 0;
        freeEntityIDs.push_back(entity.id);
    }

   
	// Add a component of type T to a Alread Exsisting entity and returning a refrence to it it also takes arguments
    // for archtypes a new archtype will be created , enetity will be added to new artype and removed form old arhtype
    template<typename T, typename... Args>
    T& emplaceComponent(Entity entity, Args&&... args) {
        assert(IsAlive(entity) && "Cannot emplace component on dead entity");
        EntityRecord& rec = records[entity.id];
        Archetype* old_arch = rec.archetype;

        if (hasComponent<T>(entity)) {
            auto* col = old_arch->get_column<T>();
            col->data[rec.row] = T(std::forward<Args>(args)...);
            return col->data[rec.row];
        }

        ComponentMask new_mask = old_arch->mask | GetComponentMask<T>();
        Archetype* new_arch = get_or_create_archetype(new_mask);

       
        for (const auto& [type, col_idx] : old_arch->type_to_column) {
            if (!new_arch->type_to_column.contains(type)) {
                new_arch->type_to_column[type] = new_arch->columns.size();
                new_arch->columns.push_back(old_arch->columns[col_idx]->create_empty());
            }
        }
        new_arch->register_column<T>();

      
        for (const auto& [type, col_idx] : old_arch->type_to_column) {
            auto* old_col = old_arch->columns[col_idx].get();
            auto* new_col = new_arch->get_column_by_type(type);
            old_col->move_row_to(rec.row, new_col);
        }


        auto* comp_col = new_arch->get_column<T>();
        comp_col->data.emplace_back(std::forward<Args>(args)...);

        Entity swapped = old_arch->remove_entity(rec.row);
        if (swapped != NullEntity) {
            records[swapped.id].row = rec.row;
        }

        usize new_row = new_arch->add_entity(entity);
        rec.archetype = new_arch;
        rec.row = static_cast<u32>(new_row);

        return comp_col->data[new_row];
    }


    /// <summary>
    /// removing a compeonent from an entity 
    /// new archtype will add a updated enity and old arhtype will remove this entity
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="entity"></param>
    template<typename T>
    void removeComponent(Entity entity) {
        if (!IsAlive(entity) || !hasComponent<T>(entity)) return;

        EntityRecord& rec = records[entity.id];
        Archetype* old_arch = rec.archetype;
        ComponentMask new_mask = old_arch->mask & ~GetComponentMask<T>();
        Archetype* new_arch = get_or_create_archetype(new_mask);

        ComponentType removed_type = GetComponentType<T>();

        for (const auto& [type, col_idx] : old_arch->type_to_column) {
            if (type != removed_type) {
                if (!new_arch->type_to_column.contains(type)) {
                    new_arch->type_to_column[type] = new_arch->columns.size();
                    new_arch->columns.push_back(old_arch->columns[col_idx]->create_empty());
                }
                old_arch->columns[col_idx]->move_row_to(rec.row, new_arch->get_column_by_type(type));
            }
        }

        Entity swapped = old_arch->remove_entity(rec.row);
        if (swapped != NullEntity) {
            records[swapped.id].row = rec.row;
        }

        usize new_row = new_arch->add_entity(entity);
        rec.archetype = new_arch;
        rec.row = static_cast<u32>(new_row);
    }

    void removeAllComponents(Entity entity) {
        if (!IsAlive(entity)) return;
        EntityRecord& rec = records[entity.id];
        if (rec.archetype && rec.archetype->mask != 0) {
            Archetype* empty_arch = get_or_create_archetype(0);

            Entity swapped = rec.archetype->remove_entity(rec.row);
            if (swapped != NullEntity) {
                records[swapped.id].row = rec.row;
            }

            usize new_row = empty_arch->add_entity(entity);
            rec.archetype = empty_arch;
            rec.row = static_cast<u32>(new_row);
        }
    }

    template<typename T>
    [[nodiscard]] T& getComponent(Entity entity) {
        assert(IsAlive(entity) && "Cannot get component from dead entity");
        assert(hasComponent<T>(entity) && "Entity does not possess requested component");
        EntityRecord& rec = records[entity.id];
        return rec.archetype->get_column<T>()->data[rec.row];
    }

    template<typename T>
    [[nodiscard]] const T& getComponent(Entity entity) const {
        assert(IsAlive(entity) && "Cannot get component from dead entity");
        assert(hasComponent<T>(entity) && "Entity does not possess requested component");
        const EntityRecord& rec = records[entity.id];
        return rec.archetype->get_column<T>()->data[rec.row];
    }

    template<typename T>
    [[nodiscard]] bool hasComponent(Entity entity) const noexcept {
        if (!IsAlive(entity)) return false;
        const EntityRecord& rec = records[entity.id];
        return rec.archetype && (rec.archetype->mask & GetComponentMask<T>()) != 0;
    }

   
    // apply functions to each Comps -> compent mask
    template<typename... Comps, typename Func>
    void Each(Func&& func) {
        ComponentMask query_mask = (GetComponentMask<Comps>() | ...);

        for (auto& [mask, arch] : archetypeMap) {
            if ((mask & query_mask) == query_mask) {
                auto cols = std::make_tuple(arch->get_column<Comps>()...);
                const usize count = arch->size();
                for (usize i = 0; i < count; ++i) {
                    if constexpr (std::is_invocable_v<Func, Entity, Comps&...>) {
                        func(arch->entities[i], (std::get<ComponentColumn<Comps>*>(cols)->data[i])...);
                    } else {
                        func((std::get<ComponentColumn<Comps>*>(cols)->data[i])...);
                    }
                }
            }
        }
    }

    // get no of alive entites
    [[nodiscard]] usize entity_count() const noexcept {
        usize total = 0;
        for (const auto& rec : records) {
            if (rec.archetype != nullptr) ++total;
        }
        return total;
    }

    // get total archypes currently exsisting

    [[nodiscard]] usize archetype_count() const noexcept {
        return archetypeMap.size();
    }

private:
    Archetype* get_or_create_archetype(ComponentMask mask) {
        auto it = archetypeMap.find(mask);
        if (it != archetypeMap.end()) return it->second;

        auto arch = std::make_unique<Archetype>(mask);
        Archetype* ptr = arch.get();
        archetypeMap[mask] = ptr;
        archetypes.push_back(std::move(arch));
        return ptr;
    }

    std::vector<EntityRecord> records;
    std::vector<std::unique_ptr<Archetype>> archetypes;
    std::unordered_map<ComponentMask, Archetype*> archetypeMap;
    std::vector<u32> freeEntityIDs;
};

} // namespace volt
