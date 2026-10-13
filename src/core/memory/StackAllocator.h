#pragma once

#include <cstddef>
#include <cstdint>

namespace StonePotatoes {
namespace Memory {

// LIFO stack allocator over a fixed buffer.
// Allocations bump a pointer; freeing must happen in reverse order or via
// markers. Ideal for per-frame scratch memory and scoped allocations.
// 固定緩衝區上的後進先出堆疊分配器。
// 分配僅推移指標；釋放必須以相反順序進行或透過標記回捲。
// 適合每幀暫存記憶體與作用域分配。
class StackAllocator {
public:
    using Marker = size_t;

    explicit StackAllocator(size_t capacity);
    ~StackAllocator();

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    // Bump-allocate `size` bytes aligned to `alignment`.
    // Returns nullptr when out of space.
    // 推移指標分配 `size` 位元組並對齊 `alignment`。空間不足時回傳 nullptr。
    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    // Current position; pass to FreeToMarker() to roll back.
    // 目前位置；傳給 FreeToMarker() 以回捲。
    Marker GetMarker() const { return m_Offset; }

    // Roll back to a previously obtained marker.
    // 回捲至先前取得的標記。
    void FreeToMarker(Marker marker);

    // LIFO free: only valid for the most recent allocation.
    // 後進先出釋放：僅對最近一次分配有效。
    void FreeLast(void* ptr, size_t size);

    void Reset();

    size_t GetCapacity() const { return m_Capacity; }
    size_t GetUsed() const { return m_Offset; }
    size_t GetRemaining() const { return m_Capacity - m_Offset; }

private:
    uint8_t* m_Buffer;
    size_t   m_Capacity;
    size_t   m_Offset;
};

} // namespace Memory
} // namespace StonePotatoes
