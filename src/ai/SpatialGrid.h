#pragma once

// Uniform-hash spatial index for locality queries. Entities are bucketed
// by cell; a radius query visits only the cells the circle overlaps, so
// perception costs O(occupied neighborhood) instead of O(N) per agent.
// Insert order inside a cell is deterministic (registration order), so
// query results are stable for replay.
// 感知用的均勻雜湊空間索引。實體按格子分桶；半徑查詢只走訪
// 圓域覆蓋的格子，感知成本為 O(鄰域) 而非每代理 O(N)。
// 桶內插入順序確定（依註冊序），查詢結果可重放。

#include "../ecs/entity/Entity.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace StonePotatoes {
namespace AI {

class SpatialGrid {
public:
    struct Entry {
        ECS::EntityID id = 0;
        float x = 0.0f, y = 0.0f;
        uint8_t tag = 0;          // caller-defined kind | 呼叫方自訂類別
    };

    explicit SpatialGrid(float cellSize = 8.0f)
        : m_Cell(cellSize > 0.0f ? cellSize : 8.0f) {}

    float CellSize() const { return m_Cell; }
    size_t CellCount() const { return m_Cells.size(); }
    size_t EntryCount() const { return m_Count; }

    void Clear() {
        m_Cells.clear();
        m_Count = 0;
    }

    void Insert(ECS::EntityID id, float x, float y, uint8_t tag = 0) {
        m_Cells[Key(CellOf(x), CellOf(y))].push_back({id, x, y, tag});
        ++m_Count;
    }

    // Visit every entry whose position lies within r of (x, y).
    // Iterates cells in a fixed scan order → deterministic results.
    // 走訪距 (x,y) 半徑 r 內的所有項目；格子掃描序固定 → 結果確定。
    template <typename Fn>
    void QueryCircle(float x, float y, float r, Fn&& fn) const {
        const int x0 = CellOf(x - r), x1 = CellOf(x + r);
        const int y0 = CellOf(y - r), y1 = CellOf(y + r);
        const float r2 = r * r;
        for (int cy = y0; cy <= y1; ++cy) {
            for (int cx = x0; cx <= x1; ++cx) {
                auto it = m_Cells.find(Key(cx, cy));
                if (it == m_Cells.end()) continue;
                for (const Entry& e : it->second) {
                    const float dx = e.x - x, dy = e.y - y;
                    if (dx * dx + dy * dy <= r2) fn(e);
                }
            }
        }
    }

    // Visit all entries in the cells overlapped by the circle's bounding
    // box WITHOUT distance filtering — cheaper when the caller filters.
    // 走訪圓域外接框覆蓋的所有格子（不做距離過濾，由呼叫方過濾）。
    template <typename Fn>
    void QueryCells(float x, float y, float r, Fn&& fn) const {
        const int x0 = CellOf(x - r), x1 = CellOf(x + r);
        const int y0 = CellOf(y - r), y1 = CellOf(y + r);
        for (int cy = y0; cy <= y1; ++cy) {
            for (int cx = x0; cx <= x1; ++cx) {
                auto it = m_Cells.find(Key(cx, cy));
                if (it == m_Cells.end()) continue;
                for (const Entry& e : it->second) fn(e);
            }
        }
    }

private:
    // floor(v / cell) without libm — symmetric around the origin.
    // floor(v / cell) 不依賴 libm — 以原點對稱。
    int CellOf(float v) const {
        const float q = v / m_Cell;
        const int i = static_cast<int>(q);
        return (q < 0.0f && static_cast<float>(i) != q) ? i - 1 : i;
    }

    static int64_t Key(int cx, int cy) {
        return (static_cast<int64_t>(cx) << 32) |
               static_cast<uint32_t>(cy);
    }

    float m_Cell;
    std::unordered_map<int64_t, std::vector<Entry>> m_Cells;
    size_t m_Count = 0;
};

} // namespace AI
} // namespace StonePotatoes
