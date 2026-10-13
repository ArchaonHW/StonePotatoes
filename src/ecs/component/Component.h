#pragma once

#include <cstdint>

namespace Genesis {
namespace ECS {

using ComponentID = uint32_t;

class ComponentBase {
public:
    virtual ~ComponentBase() = default;
};

// TODO: Implement full component registration system
// For now, this is a stub

} // namespace ECS
} // namespace Genesis
