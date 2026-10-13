#include "Plugin.h"
#include "../logging/Logger.h"

#include <algorithm>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace StonePotatoes {
namespace Plugin {

// ------------------------------------------------- SharedLibraryPluginLoader
namespace {
#if defined(_WIN32)
using LibHandle = HMODULE;
LibHandle OpenLib(const std::string& p) { return LoadLibraryA(p.c_str()); }
void* Sym(LibHandle h, const char* n) {
    return reinterpret_cast<void*>(GetProcAddress(h, n));
}
void CloseLib(LibHandle h) { FreeLibrary(h); }
std::string LibError() { return "LoadLibrary error " + std::to_string(GetLastError()); }
#else
using LibHandle = void*;
LibHandle OpenLib(const std::string& p) { return dlopen(p.c_str(), RTLD_NOW | RTLD_LOCAL); }
void* Sym(LibHandle h, const char* n) { return dlsym(h, n); }
void CloseLib(LibHandle h) { dlclose(h); }
std::string LibError() { const char* e = dlerror(); return e ? e : "unknown dlopen error"; }
#endif
using CreateFn = Plugin* (*)();
using DestroyFn = void (*)(Plugin*);
} // namespace

std::shared_ptr<Plugin> SharedLibraryPluginLoader::LoadFromFile(
    const std::string& path, std::string& error) {
    LibHandle h = OpenLib(path);
    if (!h) { error = "cannot open '" + path + "': " + LibError(); return nullptr; }
    auto create = reinterpret_cast<CreateFn>(Sym(h, "StonePotatoes_CreatePlugin"));
    auto destroy = reinterpret_cast<DestroyFn>(Sym(h, "StonePotatoes_DestroyPlugin"));
    if (!create || !destroy) {
        error = "'" + path + "' does not export StonePotatoes_CreatePlugin/DestroyPlugin";
        CloseLib(h);
        return nullptr;
    }
    Plugin* raw = create();
    if (!raw) { error = "StonePotatoes_CreatePlugin returned null"; CloseLib(h); return nullptr; }
    // Destroy inside the library, then unload the library.
    // 於函式庫內銷毀，再卸載函式庫。
    return std::shared_ptr<Plugin>(raw, [h, destroy](Plugin* p) {
        destroy(p);
        CloseLib(h);
    });
}

// ---------------------------------------------------------- PluginManager
PluginManager::~PluginManager() { UnloadAllPlugins(); }

PluginManager& PluginManager::GetInstance() {
    static PluginManager instance;
    return instance;
}

PluginManager::Entry* PluginManager::Find(const std::string& name) {
    for (auto& e : m_Plugins)
        if (name == e.plugin->GetName()) return &e;
    return nullptr;
}
const PluginManager::Entry* PluginManager::Find(const std::string& name) const {
    for (const auto& e : m_Plugins)
        if (name == e.plugin->GetName()) return &e;
    return nullptr;
}

bool PluginManager::RegisterPlugin(std::unique_ptr<Plugin> plugin) {
    return RegisterPlugin(std::shared_ptr<Plugin>(std::move(plugin)));
}

bool PluginManager::RegisterPlugin(std::shared_ptr<Plugin> plugin) {
    if (!plugin) { m_LastError = "RegisterPlugin: null plugin"; return false; }
    if (Find(plugin->GetName())) {
        m_LastError = std::string("RegisterPlugin: duplicate name '") + plugin->GetName() + "'";
        STONEPOTATOES_LOG_ERROR("%s", m_LastError.c_str());
        return false;
    }
    STONEPOTATOES_LOG_INFO("Registered plugin %s %s", plugin->GetName(),
                           plugin->GetVersion().c_str());
    m_Plugins.push_back({std::move(plugin)});
    return true;
}

bool PluginManager::RegisterFromFile(IPluginLoader& loader, const std::string& path) {
    std::string err;
    auto p = loader.LoadFromFile(path, err);
    if (!p) {
        m_LastError = err;
        STONEPOTATOES_LOG_ERROR("RegisterFromFile: %s", err.c_str());
        return false;
    }
    return RegisterPlugin(std::move(p));
}

Plugin* PluginManager::GetPlugin(const char* name) const {
    if (!name) return nullptr;
    const Entry* e = Find(name);
    return e ? e->plugin.get() : nullptr;
}

std::vector<std::string> PluginManager::ResolveLoadOrder() {
    // Stable Kahn: repeatedly pick the earliest-registered plugin whose
    // dependencies are all already placed.
    // 穩定 Kahn：每輪挑選相依皆已排入、且註冊最早的外掛。
    for (const auto& e : m_Plugins)
        for (const auto& d : e.plugin->GetDependencies())
            if (!Find(d)) {
                m_LastError = std::string("plugin '") + e.plugin->GetName() +
                              "' depends on missing plugin '" + d + "'";
                return {};
            }
    std::vector<std::string> order;
    std::vector<bool> placed(m_Plugins.size(), false);
    auto isPlaced = [&](const std::string& n) {
        return std::find(order.begin(), order.end(), n) != order.end();
    };
    while (order.size() < m_Plugins.size()) {
        bool progress = false;
        for (size_t i = 0; i < m_Plugins.size(); ++i) {
            if (placed[i]) continue;
            const auto& deps = m_Plugins[i].plugin->GetDependencies();
            if (std::all_of(deps.begin(), deps.end(), isPlaced)) {
                order.push_back(m_Plugins[i].plugin->GetName());
                placed[i] = true;
                progress = true;
                break;  // restart scan → registration order wins ties
            }
        }
        if (!progress) {
            std::string cyc;
            for (size_t i = 0; i < m_Plugins.size(); ++i)
                if (!placed[i]) cyc += std::string(cyc.empty() ? "" : ", ") + m_Plugins[i].plugin->GetName();
            m_LastError = "dependency cycle among: " + cyc;
            return {};
        }
    }
    return order;
}

bool PluginManager::LoadRecursive(const std::string& name,
                                  std::vector<std::string>& stack) {
    Entry* e = Find(name);
    if (!e) { m_LastError = "missing plugin '" + name + "'"; return false; }
    Plugin& p = *e->plugin;
    if (p.m_Loaded) return true;
    if (std::find(m_FailedThisPass.begin(), m_FailedThisPass.end(), name) != m_FailedThisPass.end()) {
        m_LastError = "plugin '" + name + "' failed earlier in this pass";
        return false;
    }
    if (std::find(stack.begin(), stack.end(), name) != stack.end()) {
        m_LastError = "dependency cycle at '" + name + "'";
        return false;
    }
    stack.push_back(name);
    for (const auto& d : p.GetDependencies()) {
        if (!LoadRecursive(d, stack)) {
            m_LastError = "plugin '" + name + "' skipped: dependency '" + d +
                          "' failed (" + m_LastError + ")";
            stack.pop_back();
            return false;
        }
    }
    stack.pop_back();
    if (!p.OnLoad()) {
        m_FailedThisPass.push_back(name);
        m_LastError = "plugin '" + name + "' OnLoad() failed";
        STONEPOTATOES_LOG_ERROR("%s", m_LastError.c_str());
        return false;
    }
    p.m_Loaded = true;
    m_LoadOrder.push_back(name);
    STONEPOTATOES_LOG_INFO("Loaded plugin %s", p.GetName());
    return true;
}

bool PluginManager::LoadPlugin(const char* name) {
    if (!name) return false;
    std::vector<std::string> stack;
    m_FailedThisPass.clear();
    const bool ok = LoadRecursive(name, stack);
    m_FailedThisPass.clear();
    return ok;
}

bool PluginManager::LoadAllPlugins() {
    auto order = ResolveLoadOrder();
    if (order.empty() && !m_Plugins.empty()) {
        STONEPOTATOES_LOG_ERROR("LoadAllPlugins: %s", m_LastError.c_str());
        return false;
    }
    bool ok = true;
    std::string firstError;
    m_FailedThisPass.clear();
    for (const auto& n : order) {
        std::vector<std::string> stack;
        if (!LoadRecursive(n, stack)) {
            ok = false;
            if (firstError.empty()) firstError = m_LastError;
        }
    }
    m_FailedThisPass.clear();
    if (!ok) m_LastError = firstError;
    return ok;
}

void PluginManager::DoUnload(Plugin& p) {
    p.OnUnload();
    p.m_Loaded = false;
    m_LoadOrder.erase(std::remove(m_LoadOrder.begin(), m_LoadOrder.end(),
                                  std::string(p.GetName())),
                      m_LoadOrder.end());
    STONEPOTATOES_LOG_INFO("Unloaded plugin %s", p.GetName());
}

void PluginManager::UnloadPlugin(const char* name) {
    if (!name) return;
    Entry* e = Find(name);
    if (!e || !e->plugin->m_Loaded) return;
    // Unload loaded dependents first, newest first.
    // 先卸載依賴它的已載入外掛（由新到舊）。
    const std::string target = name;
    for (auto it = m_LoadOrder.rbegin(); it != m_LoadOrder.rend();) {
        Entry* d = Find(*it);
        const auto& deps = d->plugin->GetDependencies();
        if (std::find(deps.begin(), deps.end(), target) != deps.end()) {
            UnloadPlugin(d->plugin->GetName());
            it = m_LoadOrder.rbegin();  // order changed → restart
        } else {
            ++it;
        }
    }
    DoUnload(*e->plugin);
}

void PluginManager::UnloadAllPlugins() {
    while (!m_LoadOrder.empty()) {
        Entry* e = Find(m_LoadOrder.back());
        DoUnload(*e->plugin);
    }
}

bool PluginManager::UnregisterPlugin(const char* name) {
    if (!name || !Find(name)) {
        m_LastError = std::string("UnregisterPlugin: unknown plugin '") + (name ? name : "(null)") + "'";
        return false;
    }
    UnloadPlugin(name);
    m_Plugins.erase(std::remove_if(m_Plugins.begin(), m_Plugins.end(),
                                   [&](const Entry& e) {
                                       return std::string(name) == e.plugin->GetName();
                                   }),
                    m_Plugins.end());
    return true;
}

} // namespace Plugin
} // namespace StonePotatoes
