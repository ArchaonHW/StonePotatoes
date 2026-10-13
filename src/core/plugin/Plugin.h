#pragma once

// Plugin system | 外掛系統
//
// Design | 設計
// - A Plugin has a name, version, declared dependencies and two lifecycle
//   hooks: OnLoad() (may fail) and OnUnload().
//   外掛具名稱、版本、相依宣告，以及兩個生命週期掛勾：OnLoad()（可失敗）與 OnUnload()。
// - PluginManager owns registered plugins, resolves load order by a stable
//   topological sort over dependencies (registration order breaks ties),
//   and unloads in exact reverse load order.
//   PluginManager 持有已註冊外掛，以穩定拓撲排序決定載入順序（同層依註冊順序），
//   卸載時嚴格依載入順序反向。
// - Missing dependencies and cycles are reported via GetLastError(); a
//   plugin whose dependency fails to load is skipped (not loaded).
//   缺少相依或循環相依由 GetLastError() 回報；相依載入失敗的外掛會被略過。
// - Dynamic libraries are optional and go through the IPluginLoader
//   interface (SharedLibraryPluginLoader = dlopen / LoadLibrary).
//   動態函式庫為可選功能，經由 IPluginLoader 介面（dlopen / LoadLibrary）。
// - Not thread-safe: call from the main thread. | 非執行緒安全：請於主執行緒呼叫。

#include <memory>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Plugin {

class PluginManager;

class Plugin {
public:
    // Inline so shared-library plugins need not link StonePotatoesCore.
    // inline 定義：共享函式庫外掛不必連結 StonePotatoesCore。
    explicit Plugin(std::string name, std::string version = "0.0.0",
                    std::vector<std::string> dependencies = {})
        : m_Name(std::move(name)), m_Version(std::move(version)),
          m_Dependencies(std::move(dependencies)) {}
    // Never calls OnUnload(): the derived part is gone by then; the manager
    // unloads before releasing. | 不呼叫 OnUnload()；管理器會在釋放前先卸載。
    virtual ~Plugin() = default;

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    const char* GetName() const { return m_Name.c_str(); }
    const std::string& GetVersion() const { return m_Version; }
    const std::vector<std::string>& GetDependencies() const { return m_Dependencies; }
    bool IsLoaded() const { return m_Loaded; }

protected:
    // Lifecycle hooks, called only by PluginManager.
    // 生命週期掛勾，僅由 PluginManager 呼叫。
    virtual bool OnLoad() = 0;   // return false → load failed | 回傳 false 表示載入失敗
    virtual void OnUnload() = 0;

private:
    friend class PluginManager;
    std::string m_Name;
    std::string m_Version;
    std::vector<std::string> m_Dependencies;
    bool m_Loaded = false;
};

// Creates plugins from an external source (e.g. a shared library).
// 從外部來源（例如共享函式庫）建立外掛。
class IPluginLoader {
public:
    virtual ~IPluginLoader() = default;
    // Returns nullptr on failure; `error` receives a message.
    // 失敗回傳 nullptr，`error` 填入錯誤訊息。
    virtual std::shared_ptr<Plugin> LoadFromFile(const std::string& path,
                                                 std::string& error) = 0;
};

// Shared-library loader. The library must export (extern "C"):
//   StonePotatoes::Plugin::Plugin* StonePotatoes_CreatePlugin();
//   void StonePotatoes_DestroyPlugin(StonePotatoes::Plugin::Plugin*);
// The library stays loaded until the returned plugin is destroyed.
// Use STONEPOTATOES_DEFINE_PLUGIN(Type) to generate the exports.
// 共享函式庫載入器。函式庫須匯出上述 C 符號；外掛銷毀前函式庫保持載入。
class SharedLibraryPluginLoader : public IPluginLoader {
public:
    std::shared_ptr<Plugin> LoadFromFile(const std::string& path,
                                         std::string& error) override;
};

class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Process-wide instance (optional; managers can also be created locally).
    // 全域實例（可選；也可自行建立區域管理器）。
    static PluginManager& GetInstance();

    // Fails on null or duplicate name. | null 或名稱重複時失敗。
    bool RegisterPlugin(std::unique_ptr<Plugin> plugin);
    bool RegisterPlugin(std::shared_ptr<Plugin> plugin);
    // Load via loader then register. | 經載入器載入後註冊。
    bool RegisterFromFile(IPluginLoader& loader, const std::string& path);

    // Unloads it (and any loaded dependents first), then removes it.
    // 先卸載（含已載入的相依者），再移除。
    bool UnregisterPlugin(const char* name);
    Plugin* GetPlugin(const char* name) const;
    size_t GetPluginCount() const { return m_Plugins.size(); }

    // Loads one plugin, loading its dependencies first.
    // 載入單一外掛，先載入其相依。
    bool LoadPlugin(const char* name);
    // Unloads one plugin, unloading loaded dependents first.
    // 卸載單一外掛，先卸載依賴它的已載入外掛。
    void UnloadPlugin(const char* name);

    // Loads all in dependency order; true only if every plugin loaded.
    // 依相依順序載入全部；全部成功才回傳 true。
    bool LoadAllPlugins();
    // Unloads all in reverse load order. | 依載入順序反向卸載全部。
    void UnloadAllPlugins();

    // Dependency-resolved order of all registered plugins (empty + error on
    // missing dependency / cycle). | 解析後的載入順序（失敗時為空並設定錯誤）。
    std::vector<std::string> ResolveLoadOrder();
    const std::vector<std::string>& GetLoadOrder() const { return m_LoadOrder; }
    const std::string& GetLastError() const { return m_LastError; }

private:
    struct Entry { std::shared_ptr<Plugin> plugin; };
    Entry* Find(const std::string& name);
    const Entry* Find(const std::string& name) const;
    bool LoadRecursive(const std::string& name, std::vector<std::string>& stack);
    void DoUnload(Plugin& p);

    std::vector<Entry> m_Plugins;          // registration order | 註冊順序
    std::vector<std::string> m_LoadOrder;  // actual load order | 實際載入順序
    std::string m_LastError;
    std::vector<std::string> m_FailedThisPass;  // avoid retrying OnLoad within one LoadAllPlugins | 同一輪不重試
};

} // namespace Plugin
} // namespace StonePotatoes

#if defined(_WIN32)
#define STONEPOTATOES_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define STONEPOTATOES_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Define the C entry points for a plugin type in a shared library.
// 為共享函式庫中的外掛型別定義 C 進入點。
#define STONEPOTATOES_DEFINE_PLUGIN(PluginType)                                     \
    STONEPOTATOES_PLUGIN_EXPORT ::StonePotatoes::Plugin::Plugin*                    \
    StonePotatoes_CreatePlugin() { return new PluginType(); }                       \
    STONEPOTATOES_PLUGIN_EXPORT void                                                \
    StonePotatoes_DestroyPlugin(::StonePotatoes::Plugin::Plugin* p) { delete p; }
