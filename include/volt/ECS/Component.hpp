#pragma once

#include <volt/types/EngineTypes.hpp>
#include <cassert>

namespace volt {

    using ComponentType = u32;

    inline ComponentType NextComponentType = 0;

    template<typename T>
    ComponentType GetComponentType() {
        static const ComponentType type = NextComponentType++;
        assert(type < 64 && "Volt minimal ECS supports up to 64 component types");
        return type;
    }

    template<typename T>
    inline ComponentMask GetComponentMask() {
        return (ComponentMask{ 1 } << GetComponentType<T>());
    }

} // namespace volt
