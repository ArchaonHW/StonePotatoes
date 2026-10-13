#pragma once

#include <cstddef>
#include <cstdint>

namespace StonePotatoes {
namespace Memory {

// Fixed-size block pool backed by a single contiguous allocation.
// Freed blocks form an intrusive singly-linked free list, so Allocate/Free
// are O(1) and never touch the heap after construction.
// 固定大小區塊池，底層為單一連續配置。
// 釋放的區塊組成侵入式單向空閒鏈表，Allocate/Free 皆為 O(1)，
// 建構後不再觸及堆積。
class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Returns nullptr when the pool is exhausted.
    // 池耗盡時回傳 nullptr。
    void* Allocate();
    void Free(void* ptr);

    // Returns all blocks to the free list without releasing pool memory.
    // 將所有區塊歸還空閒鏈表，不釋放池記憶體。
    void Reset();

    // True if ptr lies inside this pool's memory region.
    // 若 ptr 位於本池記憶體範圍內則為 true。
    bool Owns(const void* ptr) const;

    size_t GetBlockSize() const;
    size_t GetBlockCount() const;
    size_t GetUsedBlocks() const;
    size_t GetFreeBlocks() const;

private:
    size_t m_BlockSize;    // requested block size
    size_t m_BlockStride;  // actual stride (>= sizeof(void*), aligned)
    size_t m_BlockCount;
    size_t m_UsedBlocks;
    void*  m_PoolMemory;
    void*  m_FreeList;     // head of intrusive free list
};

} // namespace Memory
} // namespace StonePotatoes
