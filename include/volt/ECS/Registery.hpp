#pragma once

#include<volt/types/EngineTypes.hpp>
#include<volt/ECS/Entity.hpp>
#include<volt/ECS/Component.hpp>
#include<volt/ECS/Archetype.hpp>

#include<memory>
namespace volt{
    
   class Registry{
    public:
   
        Entity CreateEntity() {
			
        }

		bool IsAlive(Entity entity)const {
			
		}
		void DestroyEntity(Entity entity) {
		
        }

		template<typename T, typename... Args>
		T& emplaceComponent(Entity entity, Args&&... args) {
			
		}

		template<typename T>
		void removeComponent(Entity entity, ComponentType componentType) {

		}

		template<typename T>
		void removeAllComponents(Entity entity) {
		}

		template<typename T>
		void removeEntity(Entity entity) {
		}

		template<typename T>
		T& getComponent(Entity entity) {

		}

		template<typename T>
		bool hasComponent(Entity entity) {

		 }
		template<typename T>
		const T& getComponent(Entity entity) const {

		}

    private:

    std::vector<EntityRecord> records;

    std::vector<std::unique_ptr<Archetype>> archetypes;

    std::unordered_map<ComponentMask, Archetype*> archetypeMap;

    std::vector<u32> freeEntityIDs;

	
   };
}



