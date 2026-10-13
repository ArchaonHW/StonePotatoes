#pragma once

#include <string>

namespace StonePotatoes {
namespace Plugin {

class Plugin {
public:
    Plugin(const char* name);
    virtual ~Plugin();

    virtual bool Load() = 0;
    virtual void Unload() = 0;

    const char* GetName() const;
    bool IsLoaded() const;

protected:
    std::string m_Name;
    bool m_Loaded;
};

class PluginManager {
public:
    static PluginManager& GetInstance();

    bool RegisterPlugin(Plugin* plugin);
    bool UnregisterPlugin(const char* name);
    Plugin* GetPlugin(const char* name);

    bool LoadAllPlugins();
    void UnloadAllPlugins();

private:
    PluginManager();
    ~PluginManager();

    // TODO: Implement plugin storage
};

} // namespace Plugin
} // namespace StonePotatoes
