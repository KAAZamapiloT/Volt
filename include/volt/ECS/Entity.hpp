#pragma once

#include <volt/types/EngineTypes.hpp>
#include <functional>
#include <ostream>

namespace volt {
    
    struct Entity {
        u32 id = 0;
        u32 generation = 0;

        constexpr bool operator==(const Entity& other) const noexcept = default;
    };

    inline constexpr Entity NullEntity{ 0xFFFFFFFF, 0 };

    inline std::ostream& operator<<(std::ostream& os, const Entity& e) {
        return os << "Entity(id=" << e.id << ", gen=" << e.generation << ")";
    }

} // namespace volt

namespace std {
    template <>
    struct hash<volt::Entity> {
        std::size_t operator()(const volt::Entity& e) const noexcept {
            return (static_cast<std::size_t>(e.id) << 32) | static_cast<std::size_t>(e.generation);
        }
    };
}
