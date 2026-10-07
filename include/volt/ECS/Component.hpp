#pragma once

#include <volt/types/EngineTypes.hpp>

namespace volt {

    using ComponentType = u32;

    inline ComponentType NextComponentType = 0;

    template<typename T>
    ComponentType GetComponentType()
    {
        static const ComponentType type = NextComponentType++;
        return type;
    }

}