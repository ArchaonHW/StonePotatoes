#include "Plugin.h"
#include "../platform/Platform.h"
#include "../logging/Logger.h"

namespace StonePotatoes {
namespace Plugin {

Plugin::Plugin(const char* name)
    : m_Name(name)
    , m_Loaded(false) {
}

Plugin::~Plugin() {
    if (m_Loaded) {
        Unload();
    }
}

const char* Plugin::GetName() const {
    return m_Name.c_str();
}

bool Plugin::IsLoaded() const {
    return m_Loaded;
}

PluginManager::PluginManager() {
}

PluginManager::~PluginManager() {
    UnloadAllPlugins();
}

PluginManager& PluginManager::GetInstance() {
    static PluginManager instance;
    return instance;
}

bool PluginManager::RegisterPlugin(Plugin* plugin) {
    (void)plugin; // Suppress unused parameter warning
    // TODO: Implement plugin registration
    STONEPOTATOES_LOG_ERROR("PluginManager::RegisterPlugin: TODO - Implement plugin registration");
    return true;
}

bool PluginManager::UnregisterPlugin(const char* name) {
    (void)name; // Suppress unused parameter warning
    // TODO: Implement plugin unregistration
    STONEPOTATOES_LOG_ERROR("PluginManager::UnregisterPlugin: TODO - Implement plugin unregistration");
    return true;
}

Plugin* PluginManager::GetPlugin(const char* name) {
    (void)name; // Suppress unused parameter warning
    // TODO: Implement plugin lookup
    STONEPOTATOES_LOG_ERROR("PluginManager::GetPlugin: TODO - Implement plugin lookup");
    return nullptr;
}

bool PluginManager::LoadAllPlugins() {
    // TODO: Implement loading all registered plugins
    STONEPOTATOES_LOG_ERROR("PluginManager::LoadAllPlugins: TODO - Implement loading all plugins");
    return true;
}

void PluginManager::UnloadAllPlugins() {
    // TODO: Implement unloading all plugins
    STONEPOTATOES_LOG_ERROR("PluginManager::UnloadAllPlugins: TODO - Implement unloading all plugins");
}

} // namespace Plugin
} // namespace StonePotatoes
