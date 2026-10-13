// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine MathUtils/Pathfinding.h (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine MathUtils/Pathfinding.h（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace StonePotatoes {
namespace Math {

// 8-direction neighbors (fixed order: orthogonal first, then diagonal for determinism)
namespace detail {
constexpr int kDirX[8] = {1, -1, 0, 0, 1, 1, -1, -1};
constexpr int kDirY[8] = {0, 0, 1, -1, 1, -1, 1, -1};
constexpr int kCost[8] = {10, 10, 10, 10, 14, 14, 14, 14};
} // namespace detail

// GridPathfinder: A* shortest path on a grid.
// Integer costs (orth=10, diag=14). Fully deterministic:
//   - open list tie-break: f -> h -> node id
//   - no float, no unordered_map iteration order dependence
//   - safe for lockstep reproducibility across platforms.
// Diagonal movement prevents corner-cutting: both orthogonal neighbors must be walkable.
// GridPathfinder：網格 A* 最短路徑。整數成本（orth=10、diag=14）。
// 全決定性：open list tie-break 明確（f → h → node id），無浮點、無
// unordered_map 迭代順序依賴，跨平台逐位一致，適用 lockstep。
// 對角移動防切角：兩正交鄰格皆需可走。
class GridPathfinder {
public:
    GridPathfinder(int width, int height)
        : m_Width(width), m_Height(height), m_Walkable(width * height, true) {}

    void SetWalkable(int x, int y, bool v) {
        if (Inside(x, y)) m_Walkable[static_cast<size_t>(y * m_Width + x)] = v;
    }
    bool IsWalkable(int x, int y) const {
        return Inside(x, y) && m_Walkable[static_cast<size_t>(y * m_Width + x)];
    }
    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

    // A* pathfinding. Returns empty vector if unreachable/unwalkable.
    // Path includes start and end as grid coordinates.
    std::vector<std::pair<int, int>> FindPath(int sx, int sy, int gx, int gy) const {
        std::vector<std::pair<int, int>> path;
        if (!IsWalkable(sx, sy) || !IsWalkable(gx, gy)) return path;
        if (sx == gx && sy == gy) { path.emplace_back(sx, sy); return path; }

        const int n = m_Width * m_Height;
        const int start = sy * m_Width + sx, goal = gy * m_Width + gx;
        std::vector<int> g(n, INT32_MAX);
        std::vector<int> parent(n, -1);
        std::vector<uint8_t> closed(n, 0);

        struct Node {
            int f, h, idx;
            bool operator>(const Node& o) const {
                if (f != o.f) return f > o.f;
                if (h != o.h) return h > o.h;
                return idx > o.idx;
            }
        };
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;

        g[start] = 0;
        open.push({Heuristic(sx, sy, gx, gy), Heuristic(sx, sy, gx, gy), start});
        while (!open.empty()) {
            const Node cur = open.top();
            open.pop();
            if (closed[static_cast<size_t>(cur.idx)]) continue;
            closed[static_cast<size_t>(cur.idx)] = 1;
            if (cur.idx == goal) break;
            const int cx = cur.idx % m_Width, cy = cur.idx / m_Width;
            for (int d = 0; d < 8; ++d) {
                const int nx = cx + detail::kDirX[d];
                const int ny = cy + detail::kDirY[d];
                if (!IsWalkable(nx, ny)) continue;
                if (d >= 4) {
                    // diagonal corner-cutting prevention
                    if (!IsWalkable(cx + detail::kDirX[d], cy) ||
                        !IsWalkable(cx, cy + detail::kDirY[d]))
                        continue;
                }
                const int ni = ny * m_Width + nx;
                const int ng = g[static_cast<size_t>(cur.idx)] + detail::kCost[d];
                if (ng < g[static_cast<size_t>(ni)]) {
                    g[static_cast<size_t>(ni)] = ng;
                    parent[static_cast<size_t>(ni)] = cur.idx;
                    const int hh = Heuristic(nx, ny, gx, gy);
                    open.push({ng + hh, hh, ni});
                }
            }
        }
        if (parent[static_cast<size_t>(goal)] == -1) return path;
        for (int i = goal; i != -1; i = parent[static_cast<size_t>(i)])
            path.emplace_back(i % m_Width, i / m_Width);
        std::reverse(path.begin(), path.end());
        return path;
    }

private:
    bool Inside(int x, int y) const {
        return x >= 0 && y >= 0 && x < m_Width && y < m_Height;
    }
    // Octile heuristic (integer): dx≥dy 时 10dx+4dy
    static int Heuristic(int x0, int y0, int x1, int y1) {
        const int dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
        const int dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);
        const int hi = (dx > dy) ? dx : dy, lo = (dx > dy) ? dy : dx;
        return 10 * (hi - lo) + 14 * lo;
    }

    int m_Width, m_Height;
    std::vector<uint8_t> m_Walkable;
};

// FlowField: goal-directed cost field.
// Compute(goalX, goalY) builds a reverse Dijkstra cost field from the goal.
// Direction(x, y, dx, dy) returns the downhill unit direction (8-way).
// Returns false if unreachable or already at goal.
// Efficient for many units sharing the same goal (O(1) query per unit).
// FlowField：目標導向成本場。Compute(goalX, goalY) 從目標反向 Dijkstra。
// Direction(x, y, dx, dy) 回傳下坡單位方向（8 向）。不可達/已在目標回 false。
// 大軍同目標時比每單位 A* 省（O(1) 查詢）。
class FlowField {
public:
    FlowField(int width, int height)
        : m_Width(width), m_Height(height),
          m_Walkable(width * height, true),
          m_Cost(width * height, INT32_MAX) {}

    void SetWalkable(int x, int y, bool v) {
        if (Inside(x, y)) m_Walkable[static_cast<size_t>(y * m_Width + x)] = v;
    }
    bool IsWalkable(int x, int y) const {
        return Inside(x, y) && m_Walkable[static_cast<size_t>(y * m_Width + x)];
    }

    // Reverse Dijkstra from goal. Returns false if goal not walkable.
    bool Compute(int gx, int gy) {
        if (!IsWalkable(gx, gy)) return false;
        std::fill(m_Cost.begin(), m_Cost.end(), INT32_MAX);
        struct Node {
            int c, idx;
            bool operator>(const Node& o) const {
                return c != o.c ? c > o.c : idx > o.idx;
            }
        };
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
        const int goal = gy * m_Width + gx;
        m_Cost[static_cast<size_t>(goal)] = 0;
        open.push({0, goal});
        while (!open.empty()) {
            const Node cur = open.top();
            open.pop();
            if (cur.c > m_Cost[static_cast<size_t>(cur.idx)]) continue;
            const int cx = cur.idx % m_Width, cy = cur.idx / m_Width;
            for (int d = 0; d < 8; ++d) {
                const int nx = cx + detail::kDirX[d];
                const int ny = cy + detail::kDirY[d];
                if (!IsWalkable(nx, ny)) continue;
                if (d >= 4 &&
                    (!IsWalkable(cx + detail::kDirX[d], cy) ||
                     !IsWalkable(cx, cy + detail::kDirY[d])))
                    continue;
                const int ni = ny * m_Width + nx;
                const int nc = cur.c + detail::kCost[d];
                if (nc < m_Cost[static_cast<size_t>(ni)]) {
                    m_Cost[static_cast<size_t>(ni)] = nc;
                    open.push({nc, ni});
                }
            }
        }
        return true;
    }

    // Cost value; INT32_MAX if unreachable/unwalkable
    int Cost(int x, int y) const {
        return Inside(x, y) ? m_Cost[static_cast<size_t>(y * m_Width + x)] : INT32_MAX;
    }

    // Downhill unit direction (8-way). Returns false if unreachable/at goal.
    bool Direction(int x, int y, float& dx, float& dy) const {
        if (!Inside(x, y) || m_Cost[static_cast<size_t>(y * m_Width + x)] == INT32_MAX ||
            m_Cost[static_cast<size_t>(y * m_Width + x)] == 0) {
            return false;
        }
        const int best = BestNeighborCost(x, y);
        if (best == INT32_MAX || best >= m_Cost[static_cast<size_t>(y * m_Width + x)]) return false;
        for (int d = 0; d < 8; ++d) {
            const int nx = x + detail::kDirX[d];
            const int ny = y + detail::kDirY[d];
            if (!Inside(nx, ny)) continue;
            if (d >= 4 &&
                (!IsWalkable(x + detail::kDirX[d], y) ||
                 !IsWalkable(x, y + detail::kDirY[d])))
                continue;
            if (m_Cost[static_cast<size_t>(ny * m_Width + nx)] == best) {
                dx = static_cast<float>(detail::kDirX[d]);
                dy = static_cast<float>(detail::kDirY[d]);
                const float len = (d >= 4) ? 1.41421356f : 1.0f;
                dx /= len; dy /= len;
                return true;
            }
        }
        return false;
    }

private:
    bool Inside(int x, int y) const {
        return x >= 0 && y >= 0 && x < m_Width && y < m_Height;
    }
    int BestNeighborCost(int x, int y) const {
        int best = INT32_MAX;
        for (int d = 0; d < 8; ++d) {
            const int nx = x + detail::kDirX[d];
            const int ny = y + detail::kDirY[d];
            if (!Inside(nx, ny)) continue;
            if (d >= 4 &&
                (!IsWalkable(x + detail::kDirX[d], y) ||
                 !IsWalkable(x, y + detail::kDirY[d])))
                continue;
            best = (best < m_Cost[static_cast<size_t>(ny * m_Width + nx)]) ?
                   best : m_Cost[static_cast<size_t>(ny * m_Width + nx)];
        }
        return best;
    }

    int m_Width, m_Height;
    std::vector<uint8_t> m_Walkable;
    std::vector<int> m_Cost;
};

} // namespace Math
} // namespace StonePotatoes
