#include "StackAllocator.h"
#include "../logging/Logger.h"

#include <cstdlib>

namespace StonePotatoes {
namespace Memory {

StackAllocator::StackAllocator(size_t capacity)
    : m_Buffer(nullptr)
    , m_Capacity(capacity)
    , m_Offset(0) {
    if (capacity == 0) {
        STONEPOTATOES_LOG_ERROR("StackAllocator: capacity must be > 0");
        return;
    }
#ifdef _WIN32
    m_Buffer = static_cast<uint8_t*>(
        _aligned_malloc(capacity, alignof(std::max_align_t)));
#else
    m_Buffer = static_cast<uint8_t*>(
        std::aligned_alloc(alignof(std::max_align_t), capacity));
#endif
    if (!m_Buffer) {
        STONEPOTATOES_LOG_ERROR(
            "StackAllocator: failed to allocate %zu bytes", capacity);
        m_Capacity = 0;
    }
}

StackAllocator::~StackAllocator() {
#ifdef _WIN32
    _aligned_free(m_Buffer);
#else
    std::free(m_Buffer);
#endif
    m_Buffer = nullptr;
    m_Capacity = 0;
    m_Offset = 0;
}

void* StackAllocator::Allocate(size_t size, size_t alignment) {
    if (!m_Buffer || size == 0) {
        return nullptr;
    }
    const uintptr_t current =
        reinterpret_cast<uintptr_t>(m_Buffer) + m_Offset;
    const uintptr_t aligned =
        (current + alignment - 1) & ~static_cast<uintptr_t>(alignment - 1);
    const size_t newOffset =
        static_cast<size_t>(aligned - reinterpret_cast<uintptr_t>(m_Buffer)) +
        size;
    if (newOffset > m_Capacity) {
        STONEPOTATOES_LOG_ERROR(
            "StackAllocator: out of memory (need %zu, have %zu)",
            newOffset, m_Capacity);
        return nullptr;
    }
    m_Offset = newOffset;
    return reinterpret_cast<void*>(aligned);
}

void StackAllocator::FreeToMarker(Marker marker) {
    if (marker > m_Offset) {
        STONEPOTATOES_LOG_ERROR(
            "StackAllocator: invalid marker %zu (offset %zu)",
            marker, m_Offset);
        return;
    }
    m_Offset = marker;
}

void StackAllocator::FreeLast(void* ptr, size_t size) {
    if (!ptr) {
        return;
    }
    const size_t ptrOffset =
        static_cast<size_t>(static_cast<uint8_t*>(ptr) - m_Buffer);
    if (ptrOffset + size != m_Offset) {
        STONEPOTATOES_LOG_ERROR(
            "StackAllocator: FreeLast out of order for %p", ptr);
        return;
    }
    m_Offset = ptrOffset;
}

void StackAllocator::Reset() {
    m_Offset = 0;
}

} // namespace Memory
} // namespace StonePotatoes
