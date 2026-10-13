#pragma once

#include <cstdint>

namespace StonePotatoes {
namespace ECS {

// Entity ID: 64-bit
// Bits 0-31: Index
// Bits 32-63: Generation
using EntityID = uint64_t;

struct Entity {
    EntityID id;

    constexpr Entity() : id(0) {}
    constexpr explicit Entity(EntityID id) : id(id) {}

    constexpr bool IsValid() const {
        return id != 0;
    }

    constexpr uint32_t GetIndex() const {
        return static_cast<uint32_t>(id & 0xFFFFFFFFULL);
    }

    constexpr uint32_t GetGeneration() const {
        return static_cast<uint32_t>((id >> 32) & 0xFFFFFFFFULL);
    }

    static constexpr Entity FromIndexAndGeneration(uint32_t index, uint32_t generation) {
        return Entity((static_cast<uint64_t>(generation) << 32) | static_cast<uint64_t>(index));
    }

    bool operator==(const Entity& other) const {
        return id == other.id;
    }

    bool operator!=(const Entity& other) const {
        return id != other.id;
    }
};

// Null entity
constexpr Entity INVALID_ENTITY(0);

} // namespace ECS
} // namespace StonePotatoes
