#pragma once

#include<volt/types/EngineTypes.hpp>
#include<volt/ECS/Entity.hpp>
#include<volt/ECS/Component.hpp>
#include<volt/ECS/Archetype.hpp>


namespace volt{
    
   class Registry{


    public:

    
    private:

    std::vector<EntityRecord> entities;

    std::vector<std::unique_ptr<Archetype>> archetypes;

    std::unordered_map<ComponentMask, Archetype*> archetypeMap;

    std::vector<u32> freeEntityIDs;
   };
}