#include "ArenaAllocator.h"
#include "../logging/Logger.h"

#include <cstdlib>

namespace StonePotatoes {
namespace Memory {

namespace {

uint8_t* AlignedAlloc(size_t size) {
#ifdef _WIN32
    return static_cast<uint8_t*>(
        _aligned_malloc(size, alignof(std::max_align_t)));
#else
    return static_cast<uint8_t*>(
        std::aligned_alloc(alignof(std::max_align_t), size));
#endif
}

void AlignedFree(uint8_t* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

} // namespace

ArenaAllocator::ArenaAllocator(size_t initialChunkSize)
    : m_CurrentChunk(0)
    , m_InitialChunkSize(initialChunkSize ? initialChunkSize : 4096)
    , m_TotalAllocated(0)
    , m_TotalUsed(0) {
    NewChunk(m_InitialChunkSize);
}

ArenaAllocator::~ArenaAllocator() {
    for (auto& c : m_Chunks) {
        AlignedFree(c.data);
    }
}

void* ArenaAllocator::Allocate(size_t size, size_t alignment) {
    if (size == 0 || m_Chunks.empty()) {
        return nullptr;
    }

    auto tryAlloc = [](Chunk& c, size_t sz, size_t align) -> void* {
        const uintptr_t cur =
            reinterpret_cast<uintptr_t>(c.data) + c.offset;
        const uintptr_t aligned =
            (cur + align - 1) & ~static_cast<uintptr_t>(align - 1);
        const size_t newOffset =
            static_cast<size_t>(aligned - reinterpret_cast<uintptr_t>(c.data)) +
            sz;
        if (newOffset > c.capacity) {
            return nullptr;
        }
        c.offset = newOffset;
        return reinterpret_cast<void*>(aligned);
    };

    if (void* p = tryAlloc(m_Chunks[m_CurrentChunk], size, alignment)) {
        m_TotalUsed += size;
        return p;
    }

    // Current chunk full: reuse next chunk or grow.
    if (m_CurrentChunk + 1 < m_Chunks.size()) {
        ++m_CurrentChunk;
    } else {
        // Each new chunk doubles the previous capacity (and fits the request).
        const size_t nextSize =
            m_Chunks.back().capacity * 2 > size
                ? m_Chunks.back().capacity * 2
                : size * 2;
        if (!NewChunk(nextSize)) {
            return nullptr;
        }
        m_CurrentChunk = m_Chunks.size() - 1;
    }
    void* p = tryAlloc(m_Chunks[m_CurrentChunk], size, alignment);
    if (p) {
        m_TotalUsed += size;
    }
    return p;
}

void ArenaAllocator::Reset() {
    for (auto& c : m_Chunks) {
        c.offset = 0;
    }
    m_CurrentChunk = 0;
    m_TotalUsed = 0;
}

ArenaAllocator::Chunk* ArenaAllocator::NewChunk(size_t minSize) {
    Chunk c{};
    c.capacity = minSize;
    c.offset = 0;
    c.data = AlignedAlloc(c.capacity);
    if (!c.data) {
        STONEPOTATOES_LOG_ERROR(
            "ArenaAllocator: failed to allocate %zu-byte chunk", minSize);
        return nullptr;
    }
    m_TotalAllocated += c.capacity;
    m_Chunks.push_back(c);
    return &m_Chunks.back();
}

} // namespace Memory
} // namespace StonePotatoes
