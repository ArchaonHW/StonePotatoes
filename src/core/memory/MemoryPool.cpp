#include "MemoryPool.h"
#include "../logging/Logger.h"

#include <cstdlib>
#include <new>

namespace StonePotatoes {
namespace Memory {

namespace {

void* AlignedAlloc(size_t size, size_t alignment) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    // aligned_alloc requires size to be a multiple of alignment.
    const size_t rounded = (size + alignment - 1) / alignment * alignment;
    return std::aligned_alloc(alignment, rounded);
#endif
}

void AlignedFree(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

size_t AlignUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

MemoryPool::MemoryPool(size_t blockSize, size_t blockCount)
    : m_BlockSize(blockSize)
    , m_BlockStride(0)
    , m_BlockCount(blockCount)
    , m_UsedBlocks(0)
    , m_PoolMemory(nullptr)
    , m_FreeList(nullptr) {
    constexpr size_t kAlignment = alignof(std::max_align_t);
    m_BlockStride = AlignUp(
        blockSize < sizeof(void*) ? sizeof(void*) : blockSize, kAlignment);

    if (blockCount == 0) {
        STONEPOTATOES_LOG_ERROR("MemoryPool: blockCount must be > 0");
        return;
    }

    m_PoolMemory = AlignedAlloc(m_BlockStride * m_BlockCount, kAlignment);
    if (!m_PoolMemory) {
        STONEPOTATOES_LOG_ERROR(
            "MemoryPool: failed to allocate %zu bytes",
            m_BlockStride * m_BlockCount);
        return;
    }

    Reset();
}

MemoryPool::~MemoryPool() {
    AlignedFree(m_PoolMemory);
    m_PoolMemory = nullptr;
    m_FreeList = nullptr;
}

void* MemoryPool::Allocate() {
    if (!m_FreeList) {
        STONEPOTATOES_LOG_ERROR("MemoryPool: exhausted (blockSize=%zu)",
                                m_BlockSize);
        return nullptr;
    }
    void* block = m_FreeList;
    m_FreeList = *static_cast<void**>(m_FreeList);
    ++m_UsedBlocks;
    return block;
}

void MemoryPool::Free(void* ptr) {
    if (!ptr) {
        return;
    }
    if (!Owns(ptr)) {
        STONEPOTATOES_LOG_ERROR("MemoryPool: Free() with foreign pointer %p",
                                ptr);
        return;
    }
    *static_cast<void**>(ptr) = m_FreeList;
    m_FreeList = ptr;
    --m_UsedBlocks;
}

void MemoryPool::Reset() {
    // Thread all blocks into the free list.
    m_FreeList = m_PoolMemory;
    auto* byte = static_cast<uint8_t*>(m_PoolMemory);
    for (size_t i = 0; i + 1 < m_BlockCount; ++i) {
        void* current = byte + i * m_BlockStride;
        void* next = byte + (i + 1) * m_BlockStride;
        *static_cast<void**>(current) = next;
    }
    if (m_BlockCount > 0) {
        void* last = byte + (m_BlockCount - 1) * m_BlockStride;
        *static_cast<void**>(last) = nullptr;
    }
    m_UsedBlocks = 0;
}

bool MemoryPool::Owns(const void* ptr) const {
    const auto* p = static_cast<const uint8_t*>(ptr);
    const auto* begin = static_cast<const uint8_t*>(m_PoolMemory);
    const auto* end = begin + m_BlockStride * m_BlockCount;
    return p >= begin && p < end &&
           ((p - begin) % m_BlockStride) == 0;
}

size_t MemoryPool::GetBlockSize() const {
    return m_BlockSize;
}

size_t MemoryPool::GetBlockCount() const {
    return m_BlockCount;
}

size_t MemoryPool::GetUsedBlocks() const {
    return m_UsedBlocks;
}

size_t MemoryPool::GetFreeBlocks() const {
    return m_BlockCount - m_UsedBlocks;
}

} // namespace Memory
} // namespace StonePotatoes
