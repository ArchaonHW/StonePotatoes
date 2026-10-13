#pragma once

#include <cstddef>
#include <cstdint>

namespace Genesis {
namespace Memory {

class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    ~MemoryPool();

    void* Allocate();
    void Free(void* ptr);

    size_t GetBlockSize() const;
    size_t GetBlockCount() const;
    size_t GetUsedBlocks() const;
    size_t GetFreeBlocks() const;

private:
    size_t m_BlockSize;
    size_t m_BlockCount;
    size_t m_UsedBlocks;
    void* m_PoolMemory;
    void* m_FreeList;
};

} // namespace Memory
} // namespace Genesis
