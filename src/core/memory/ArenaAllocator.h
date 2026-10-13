#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace StonePotatoes {
namespace Memory {

// Growable arena (region) allocator.
// Bump-allocates within chunks; allocates a new, larger chunk when the current
// one is full. Individual frees are unsupported - Reset() releases everything
// back for reuse without deallocating the chunks.
// 可成長的區域（arena）分配器。
// 在區塊內推移指標分配；目前區塊用盡時配置更大的新區塊。
// 不支援個別釋放 — Reset() 將所有記憶體回收重用但不釋放區塊。
class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t initialChunkSize = 64 * 1024);
    ~ArenaAllocator();

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    // Returns nullptr only if a chunk allocation itself fails.
    // 只有在區塊配置失敗時才回傳 nullptr。
    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    // Rewind all chunks for reuse. Pointers handed out become invalid.
    // 回捲所有區塊以供重用。先前發出的指標全部失效。
    void Reset();

    size_t GetTotalAllocated() const { return m_TotalAllocated; }
    size_t GetTotalUsed() const { return m_TotalUsed; }
    size_t GetChunkCount() const { return m_Chunks.size(); }

private:
    struct Chunk {
        uint8_t* data;
        size_t   capacity;
        size_t   offset;
    };

    Chunk* NewChunk(size_t minSize);

    std::vector<Chunk> m_Chunks;
    size_t m_CurrentChunk;
    size_t m_InitialChunkSize;
    size_t m_TotalAllocated;
    size_t m_TotalUsed;
};

} // namespace Memory
} // namespace StonePotatoes
