#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace StonePotatoes::Data {

// Synchronous, root-scoped cache for immutable binary and text assets.
// 同步、限制於根目錄的唯讀二進位與文字資產快取。
class AssetCache {
public:
    using BinaryAsset = std::vector<uint8_t>;
    using BinaryAssetPtr = std::shared_ptr<const BinaryAsset>;
    using TextAssetPtr = std::shared_ptr<const std::string>;

    explicit AssetCache(std::string rootDirectory);

    BinaryAssetPtr LoadBinary(std::string_view relativePath);
    TextAssetPtr LoadText(std::string_view relativePath);

    // Remove one cached asset; existing shared handles remain valid.
    // 移除單筆快取；既有 shared handle 仍有效。
    bool Invalidate(std::string_view relativePath);
    void Clear();
    std::size_t CachedAssetCount() const;

private:
    struct Entry {
        BinaryAssetPtr binary;
        TextAssetPtr text;
    };

    bool Resolve(std::string_view relativePath, std::string& cacheKey,
                 std::string& fullPath) const;
    bool LoadEntry(const std::string& cacheKey, const std::string& fullPath,
                   Entry*& entry);

    std::filesystem::path m_RootDirectory;
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, Entry> m_Cache;
};

} // namespace StonePotatoes::Data
