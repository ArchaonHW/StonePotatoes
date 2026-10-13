#pragma once

namespace StonePotatoes {
namespace ECS {

// Marker base for world resources (singleton-scoped data such as maps,
// configs, global state). ResourceManager accepts any type — deriving from
// Resource is optional but documents intent.
// 世界資源（地圖、設定、全域狀態等單例資料）的標記基底。
// ResourceManager 接受任何型別 — 繼承 Resource 非必要，但可表達意圖。
class Resource {
public:
    virtual ~Resource() = default;
};

} // namespace ECS
} // namespace StonePotatoes
