#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <type_traits>

namespace StonePotatoes {
namespace GPU {

// Usage flags mirror GPU buffer semantics so the same code paths
// work when a real GPU backend lands.
// 用途旗標對齊 GPU 緩衝語意，日後接上真實 GPU 後端時程式碼路徑不變。
enum class BufferUsage : uint32_t {
    None      = 0,
    ReadOnly  = 1u << 0,  // shader reads (SRV-like)
    ReadWrite = 1u << 1,  // shader read/write (UAV-like)
    Staging   = 1u << 2,  // CPU<->device transfer
};
inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) |
                                  static_cast<uint32_t>(b));
}
inline bool HasFlag(BufferUsage v, BufferUsage f) {
    return (static_cast<uint32_t>(v) & static_cast<uint32_t>(f)) != 0;
}

// Type-erased compute buffer. Backed by host memory in the CPU
// reference implementation; the interface is backend-agnostic.
// 型別抹除計算緩衝。CPU 參考實作以主機記憶體為底層；
// 介面與後端無關。
class ComputeBuffer {
public:
    ComputeBuffer() = default;
    ComputeBuffer(size_t elementSize, size_t count, BufferUsage usage);
    ~ComputeBuffer() = default;

    ComputeBuffer(const ComputeBuffer&) = delete;
    ComputeBuffer& operator=(const ComputeBuffer&) = delete;
    ComputeBuffer(ComputeBuffer&&) = default;
    ComputeBuffer& operator=(ComputeBuffer&&) = default;

    void Allocate(size_t elementSize, size_t count, BufferUsage usage);
    void Release();

    void* Data() { return m_Data.data(); }
    const void* Data() const { return m_Data.data(); }
    size_t Count() const { return m_Count; }
    size_t ElementSize() const { return m_ElementSize; }
    size_t ByteSize() const { return m_Data.size(); }
    BufferUsage GetUsage() const { return m_Usage; }

    // Typed view — caller must match the allocated element size.
    // 有型別視圖 — 呼叫端必須與配置的元素大小一致。
    template <typename T>
    T* As() {
        return static_cast<T*>(Data());
    }
    template <typename T>
    const T* As() const {
        return static_cast<const T*>(Data());
    }

    // Host<->device transfer hooks. On the CPU backend these are
    // memcpy stubs; on a GPU backend they will enqueue transfers.
    // 主機<->裝置傳輸介面。CPU 後端為 memcpy；
    // GPU 後端將改為加入傳輸佇列。
    void Upload(const void* src, size_t bytes, size_t dstOffset = 0);
    void Download(void* dst, size_t bytes, size_t srcOffset = 0) const;

private:
    std::vector<uint8_t> m_Data;
    size_t m_ElementSize = 0;
    size_t m_Count = 0;
    BufferUsage m_Usage = BufferUsage::None;
};

// Typed convenience wrapper — mirrors StructuredBuffer<T>.
// 有型別包裝 — 對齊 StructuredBuffer<T> 語意。
template <typename T>
class TypedBuffer {
public:
    explicit TypedBuffer(size_t count,
                         BufferUsage usage = BufferUsage::ReadWrite)
        : m_Buffer(sizeof(T), count, usage) {}

    size_t Count() const { return m_Buffer.Count(); }
    T* Data() { return m_Buffer.As<T>(); }
    const T* Data() const { return m_Buffer.As<T>(); }
    ComputeBuffer& Raw() { return m_Buffer; }
    const ComputeBuffer& Raw() const { return m_Buffer; }

    T& operator[](size_t i) { return Data()[i]; }
    const T& operator[](size_t i) const { return Data()[i]; }

private:
    ComputeBuffer m_Buffer;
};

} // namespace GPU
} // namespace StonePotatoes
