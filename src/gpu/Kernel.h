#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace GPU {

// A compute kernel: a function applied per-thread over a 1D dispatch.
// Mirrors HLSL compute shader semantics (SV_DispatchThreadID).
// 計算核心：對一維 dispatch 中每個執行緒套用的函式。
// 對齊 HLSL 計算著色器語意（SV_DispatchThreadID）。
class Kernel {
public:
    // gid = global thread index in [0, threads)
    // gid = 全域執行緒索引，範圍 [0, threads)
    using Fn = std::function<void(uint32_t gid, void* userData)>;

    Kernel() = default;
    Kernel(std::string name, Fn fn) : m_Name(std::move(name)), m_Fn(std::move(fn)) {}

    void SetName(std::string name) { m_Name = std::move(name); }
    const std::string& GetName() const { return m_Name; }

    void SetFunction(Fn fn) { m_Fn = std::move(fn); }
    bool IsValid() const { return static_cast<bool>(m_Fn); }

    void Invoke(uint32_t gid, void* userData) const {
        if (m_Fn) m_Fn(gid, userData);
    }

private:
    std::string m_Name;
    Fn m_Fn;
};

// Dispatch parameters — mirrors [numthreads(x,y,z)] + Dispatch(x,y,z).
// Dispatch 參數 — 對齊 [numthreads(x,y,z)] + Dispatch(x,y,z)。
struct DispatchParams {
    uint32_t threadCount = 0;     // total global threads
    uint32_t groupSize = 64;      // threads per group (scheduling hint)
    void* userData = nullptr;
};

} // namespace GPU
} // namespace StonePotatoes
