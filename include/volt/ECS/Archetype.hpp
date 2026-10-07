#pragma once

#include<volt/types/EngineTypes.hpp>
#include<memory>
#include<volt/ECS/Entity.hpp>
#include<volt/ECS/Component.hpp>

namespace volt{


    class IComponentColumn {
public:
    virtual ~IComponentColumn() = default;
};

template<typename T>
class ComponentColumn : public IComponentColumn {
public:
    std::vector<T> data;
};

struct ComponentColumnEntry {
    ComponentType type;
    std::unique_ptr<IComponentColumn> column;
};

struct EntityRecord{
Archetype* archetype;
u32 row;
u32 generation;
};
    class Archetype {


        private:
        std::vector<Entity> entities;
        ComponentMask componentMask;
         
        std::vector<ComponentColumnEntry> componentColumns;

    };
}
