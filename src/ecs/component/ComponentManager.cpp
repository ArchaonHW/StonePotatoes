#include "ComponentManager.h"

namespace StonePotatoes {
namespace ECS {

ComponentManager::ComponentManager() = default;

ComponentManager::~ComponentManager() = default;

const ComponentTypeInfo*
ComponentManager::FindByName(const std::string& name) const {
    return GetComponentInfoByName(name);
}

} // namespace ECS
} // namespace StonePotatoes
