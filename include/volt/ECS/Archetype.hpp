#pragma once

#include <volt/types/EngineTypes.hpp>
#include <volt/ECS/Entity.hpp>
#include <volt/ECS/Component.hpp>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>

namespace volt {

class Archetype; // Forward declaration

struct EntityRecord {
    Archetype* archetype = nullptr;
    u32 row = 0;
    u32 generation = 0;
    bool alive = false;
};

class IComponentColumn {
public:
    virtual ~IComponentColumn() = default;
    virtual void move_row_to(usize src_row, IComponentColumn* dest_col) = 0;
    virtual void swap_pop(usize row) = 0;
    virtual usize size() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<IComponentColumn> create_empty() const = 0;
};

template<typename T>
class ComponentColumn : public IComponentColumn {
public:
    std::vector<T> data;

    void move_row_to(usize src_row, IComponentColumn* dest_col) override {
        assert(src_row < data.size());
        auto* dest = static_cast<ComponentColumn<T>*>(dest_col);
        dest->data.push_back(std::move(data[src_row]));
    }

    void swap_pop(usize row) override {
        assert(row < data.size());
        if (row + 1 < data.size()) {
            data[row] = std::move(data.back());
        }
        data.pop_back();
    }

    usize size() const noexcept override {
        return data.size();
    }

    [[nodiscard]] std::unique_ptr<IComponentColumn> create_empty() const override {
        return std::make_unique<ComponentColumn<T>>();
    }
};

class Archetype {
public:
    ComponentMask mask = 0;
    std::vector<Entity> entities;
    std::vector<std::unique_ptr<IComponentColumn>> columns;
    std::unordered_map<ComponentType, usize> type_to_column;

    explicit Archetype(ComponentMask m = 0) : mask(m) {}

    [[nodiscard]] usize size() const noexcept { 
        return entities.size(); 
    }

    [[nodiscard]] bool empty() const noexcept { 
        return entities.empty(); 
    }

    [[nodiscard]] ComponentMask get_mask() const noexcept { 
        return mask; 
    }

    template<typename T>
    void register_column() {
        ComponentType type = GetComponentType<T>();
        if (!type_to_column.contains(type)) {
            type_to_column[type] = columns.size();
            columns.push_back(std::make_unique<ComponentColumn<T>>());
        }
    }

    template<typename T>
    ComponentColumn<T>* get_column() {
        ComponentType type = GetComponentType<T>();
        auto it = type_to_column.find(type);
        if (it == type_to_column.end()) return nullptr;
        return static_cast<ComponentColumn<T>*>(columns[it->second].get());
    }

    IComponentColumn* get_column_by_type(ComponentType type) {
        auto it = type_to_column.find(type);
        if (it == type_to_column.end()) return nullptr;
        return columns[it->second].get();
    }

    usize add_entity(Entity e) {
        entities.push_back(e);
        return entities.size() - 1;
    }

    // O(1) swap-and-pop removal. Returns the swapped entity so its record can be updated.
    Entity remove_entity(usize row) {
        assert(row < entities.size());
        for (auto& col : columns) {
            col->swap_pop(row);
        }

        Entity swapped = NullEntity;
        if (row + 1 < entities.size()) {
            entities[row] = entities.back();
            swapped = entities[row];
        }
        entities.pop_back();
        return swapped;
    }
};

} // namespace volt
