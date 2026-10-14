#include "data/AssetCache.h"

#include "core/io/FileSystem.h"

#include <system_error>
#include <utility>

namespace StonePotatoes::Data {

namespace {

std::string PathToUtf8(const std::filesystem::path& path) {
    const auto encoded = path.u8string();
    return std::string(reinterpret_cast<const char*>(encoded.data()),
                       encoded.size());
}

bool IsWithinRoot(const std::filesystem::path& root,
                  const std::filesystem::path& candidate,
                  std::filesystem::path& relative) {
    relative = candidate.lexically_relative(root);
    if (relative.empty() || relative.is_absolute()) return false;
    const auto first = relative.begin();
    return first != relative.end() && *first != ".." && *first != ".";
}

} // namespace

AssetCache::AssetCache(std::string rootDirectory)
    : m_RootDirectory(rootDirectory.empty()
                          ? std::filesystem::path(".")
                          : std::filesystem::u8path(rootDirectory)) {
}

bool AssetCache::Resolve(std::string_view relativePath, std::string& cacheKey,
                         std::string& fullPath) const {
    if (relativePath.empty() ||
        relativePath.find('\0') != std::string_view::npos) {
        return false;
    }

    const auto requested = std::filesystem::u8path(relativePath.begin(),
                                                   relativePath.end());
    if (requested.is_absolute() || requested.has_root_name() ||
        requested.has_root_directory()) {
        return false;
    }
    const auto normalized = requested.lexically_normal();
    if (normalized.empty() || normalized == ".") return false;

    std::error_code ec;
    const auto absoluteRoot = std::filesystem::absolute(m_RootDirectory, ec);
    if (ec) return false;
    const auto canonicalRoot = std::filesystem::weakly_canonical(absoluteRoot, ec);
    if (ec) return false;
    const auto candidate =
        std::filesystem::weakly_canonical(canonicalRoot / normalized, ec);
    if (ec) return false;

    std::filesystem::path canonicalRelative;
    if (!IsWithinRoot(canonicalRoot, candidate, canonicalRelative)) return false;

    cacheKey = PathToUtf8(canonicalRelative);
    fullPath = PathToUtf8(candidate);
    return true;
}

bool AssetCache::LoadEntry(const std::string& cacheKey,
                           const std::string& fullPath, Entry*& entry) {
    auto [it, inserted] = m_Cache.try_emplace(cacheKey);
    entry = &it->second;
    if (inserted) {
        BinaryAsset bytes;
        if (!IO::FileSystem::IsFile(fullPath.c_str()) ||
            !IO::FileSystem::ReadBinaryFile(fullPath.c_str(), bytes)) {
            m_Cache.erase(it);
            entry = nullptr;
            return false;
        }
        entry->binary = std::make_shared<const BinaryAsset>(std::move(bytes));
    }
    return true;
}

AssetCache::BinaryAssetPtr AssetCache::LoadBinary(
    std::string_view relativePath) {
    std::lock_guard lock(m_Mutex);
    std::string cacheKey, fullPath;
    if (!Resolve(relativePath, cacheKey, fullPath)) return {};
    Entry* entry = nullptr;
    return LoadEntry(cacheKey, fullPath, entry) ? entry->binary : BinaryAssetPtr{};
}

AssetCache::TextAssetPtr AssetCache::LoadText(std::string_view relativePath) {
    std::lock_guard lock(m_Mutex);
    std::string cacheKey, fullPath;
    if (!Resolve(relativePath, cacheKey, fullPath)) return {};
    Entry* entry = nullptr;
    if (!LoadEntry(cacheKey, fullPath, entry)) return {};
    if (!entry->text) {
        const auto& bytes = *entry->binary;
        std::string text;
        if (!bytes.empty()) {
            text.assign(reinterpret_cast<const char*>(bytes.data()),
                        bytes.size());
        }
        entry->text = std::make_shared<const std::string>(std::move(text));
    }
    return entry->text;
}

bool AssetCache::Invalidate(std::string_view relativePath) {
    std::lock_guard lock(m_Mutex);
    std::string cacheKey, fullPath;
    if (!Resolve(relativePath, cacheKey, fullPath)) return false;
    return m_Cache.erase(cacheKey) != 0;
}

void AssetCache::Clear() {
    std::lock_guard lock(m_Mutex);
    m_Cache.clear();
}

std::size_t AssetCache::CachedAssetCount() const {
    std::lock_guard lock(m_Mutex);
    return m_Cache.size();
}

} // namespace StonePotatoes::Data
