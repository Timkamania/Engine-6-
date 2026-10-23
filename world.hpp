#pragma once
#include "entity.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <typeindex>
#include <memory>
#include <cassert>

namespace ecs {

    class World {
    public:
        Entity createEntity() {
            Entity e = nextId_++;
            alive_.insert(e);
            return e;
        }

        void destroyEntity(Entity e) {
            alive_.erase(e);
            for (auto& [type, storage] : storages_) {
                // storage — это std::shared_ptr<IStorage>
                storage->erase(e);
            }
        }

        template<typename T>
        void addComponent(Entity e, T component) {
            getStorage<T>().emplace(e, std::move(component));
        }

        template<typename T>
        T& getComponent(Entity e) {
            auto& storage = getStorage<T>();
            auto it = storage.find(e);
            assert(it != storage.end() && "Component not found");
            return it->second;
        }

        template<typename T>
        bool hasComponent(Entity e) const {
            auto& storage = getStorageConst<T>();
            return storage.find(e) != storage.end();
        }

        template<typename T>
        void removeComponent(Entity e) {
            getStorage<T>().erase(e);
        }

        // Возвращает все сущности, у которых есть ВСЕ перечисленные компоненты
        template<typename... Ts>
        std::vector<Entity> getEntitiesWith() {
            std::vector<Entity> result;
            for (Entity e : alive_) {
                if ((hasComponent<Ts>(e) && ...)) {
                    result.push_back(e);
                }
            }
            return result;
        }

        size_t entityCount() const { return alive_.size(); }

    private:
        // Базовый интерфейс для стирания типов (чтобы destroyEntity мог чистить все storage)
        struct IStorage {
            virtual ~IStorage() = default;
            virtual void erase(Entity e) = 0;
        };

        template<typename T>
        struct Storage : IStorage {
            std::unordered_map<Entity, T> data;
            void erase(Entity e) override { 
                data.erase(e);
            }
        };

        template<typename T>
        std::unordered_map<Entity, T>& getStorage() {
            auto type = std::type_index(typeid(T));
            auto it = storages_.find(type);
            if (it == storages_.end()) {
                auto storage = std::make_shared<Storage<T>>();
                storages_[type] = storage;
                return storage->data;
            }
            return std::static_pointer_cast<Storage<T>>(it->second)->data;
        }

        // const-версия для hasComponent (не создаёт storage, если его нет)
        template<typename T>
        const std::unordered_map<Entity, T>& getStorageConst() const {
            static const std::unordered_map<Entity, T> empty;
            auto type = std::type_index(typeid(T));
            auto it = storages_.find(type);
            if (it == storages_.end()) return empty;
            return std::static_pointer_cast<Storage<T>>(it->second)->data;
        }

        Entity nextId_ = 1;
        std::unordered_set<Entity> alive_;
        std::unordered_map<
          std::type_index,
          std::shared_ptr<IStorage>
        >storages_;
            
    };
}