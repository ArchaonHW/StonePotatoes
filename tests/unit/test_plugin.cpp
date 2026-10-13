// PluginManager tests | 外掛管理器測試
#include "core/plugin/Plugin.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace StonePotatoes::Plugin;

static int g_Passed = 0, g_Failed = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) ++g_Passed;                                             \
        else { ++g_Failed; std::printf("[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static std::vector<std::string> g_Log;  // "+name" load, "-name" unload

class TestPlugin : public Plugin {
public:
    TestPlugin(std::string n, std::vector<std::string> deps = {}, bool failLoad = false)
        : Plugin(n, "1.0.0", std::move(deps)), m_Fail(failLoad) {}
protected:
    bool OnLoad() override {
        if (m_Fail) return false;
        g_Log.push_back(std::string("+") + GetName());
        return true;
    }
    void OnUnload() override { g_Log.push_back(std::string("-") + GetName()); }
private:
    bool m_Fail;
};

static std::unique_ptr<Plugin> P(const char* n, std::vector<std::string> d = {}, bool fail = false) {
    return std::make_unique<TestPlugin>(n, std::move(d), fail);
}

void TestRegisterLookup() {
    PluginManager pm;
    CHECK(pm.RegisterPlugin(P("a")));
    CHECK(!pm.RegisterPlugin(P("a")));                 // duplicate
    CHECK(!pm.RegisterPlugin(std::unique_ptr<Plugin>())); // null
    CHECK(pm.GetPluginCount() == 1);
    CHECK(pm.GetPlugin("a") != nullptr);
    CHECK(pm.GetPlugin("zzz") == nullptr);
    CHECK(pm.GetPlugin(nullptr) == nullptr);
    CHECK(pm.UnregisterPlugin("a"));
    CHECK(!pm.UnregisterPlugin("a"));
    CHECK(pm.GetPluginCount() == 0);
}

void TestDependencyOrder() {
    g_Log.clear();
    PluginManager pm;
    // Registered out of order: render needs core+gfx, gfx needs core.
    pm.RegisterPlugin(P("render", {"core", "gfx"}));
    pm.RegisterPlugin(P("gfx", {"core"}));
    pm.RegisterPlugin(P("core"));
    pm.RegisterPlugin(P("audio"));
    auto order = pm.ResolveLoadOrder();
    CHECK((order == std::vector<std::string>{"core", "gfx", "render", "audio"}));
    CHECK(pm.LoadAllPlugins());
    CHECK((g_Log == std::vector<std::string>{"+core", "+gfx", "+render", "+audio"}));
    CHECK(pm.GetPlugin("render")->IsLoaded());

    g_Log.clear();
    pm.UnloadAllPlugins();  // exact reverse
    CHECK((g_Log == std::vector<std::string>{"-audio", "-render", "-gfx", "-core"}));
    CHECK(!pm.GetPlugin("core")->IsLoaded());
}

void TestLoadSingleAndUnloadDependents() {
    g_Log.clear();
    PluginManager pm;
    pm.RegisterPlugin(P("core"));
    pm.RegisterPlugin(P("gfx", {"core"}));
    pm.RegisterPlugin(P("ui", {"gfx"}));
    CHECK(pm.LoadPlugin("ui"));                         // pulls deps
    CHECK((g_Log == std::vector<std::string>{"+core", "+gfx", "+ui"}));
    g_Log.clear();
    pm.UnloadPlugin("core");                            // dependents first
    CHECK((g_Log == std::vector<std::string>{"-ui", "-gfx", "-core"}));

    g_Log.clear();
    CHECK(pm.LoadAllPlugins());
    CHECK(pm.UnregisterPlugin("gfx"));                  // unloads ui, gfx
    CHECK((g_Log == std::vector<std::string>{"+core", "+gfx", "+ui", "-ui", "-gfx"}));
    CHECK(pm.GetPlugin("core")->IsLoaded());
    CHECK(!pm.GetPlugin("ui")->IsLoaded());
}

void TestMissingAndCycle() {
    {
        PluginManager pm;
        pm.RegisterPlugin(P("a", {"ghost"}));
        CHECK(pm.ResolveLoadOrder().empty());
        CHECK(pm.GetLastError().find("ghost") != std::string::npos);
        CHECK(!pm.LoadAllPlugins());
        CHECK(!pm.GetPlugin("a")->IsLoaded());
    }
    {
        PluginManager pm;
        pm.RegisterPlugin(P("x", {"y"}));
        pm.RegisterPlugin(P("y", {"x"}));
        pm.RegisterPlugin(P("z"));
        CHECK(pm.ResolveLoadOrder().empty());
        CHECK(pm.GetLastError().find("cycle") != std::string::npos);
        CHECK(!pm.LoadAllPlugins());
        CHECK(!pm.LoadPlugin("x"));
    }
}

void TestFailedLoadSkipsDependents() {
    g_Log.clear();
    PluginManager pm;
    pm.RegisterPlugin(P("base", {}, /*failLoad=*/true));
    pm.RegisterPlugin(P("child", {"base"}));
    pm.RegisterPlugin(P("other"));
    CHECK(!pm.LoadAllPlugins());
    CHECK(!pm.GetPlugin("base")->IsLoaded());
    CHECK(!pm.GetPlugin("child")->IsLoaded());
    CHECK(pm.GetPlugin("other")->IsLoaded());           // independent still loads
    CHECK((g_Log == std::vector<std::string>{"+other"}));
}

void TestDestructorUnloads() {
    g_Log.clear();
    {
        PluginManager pm;
        pm.RegisterPlugin(P("a"));
        pm.RegisterPlugin(P("b", {"a"}));
        pm.LoadAllPlugins();
    }
    CHECK((g_Log == std::vector<std::string>{"+a", "+b", "-b", "-a"}));
}

void TestSharedLibrary() {
    PluginManager pm;
    SharedLibraryPluginLoader loader;
    CHECK(!pm.RegisterFromFile(loader, "/nonexistent/libnope.so"));
    CHECK(!pm.GetLastError().empty());
#ifdef SP_TEST_PLUGIN_PATH
    pm.RegisterPlugin(P("base"));
    CHECK(pm.RegisterFromFile(loader, SP_TEST_PLUGIN_PATH));
    Plugin* d = pm.GetPlugin("dynamic_test");
    CHECK(d != nullptr);
    if (d) {
        CHECK(d->GetVersion() == "1.2.3");
        CHECK(pm.LoadAllPlugins());
        CHECK(d->IsLoaded());
        CHECK(pm.UnregisterPlugin("dynamic_test"));     // destroy in-library, dlclose
        CHECK(pm.GetPlugin("dynamic_test") == nullptr);
    }
#endif
}

int main() {
    std::printf("=== StonePotatoes Plugin Tests ===\n");
    TestRegisterLookup();
    TestDependencyOrder();
    TestLoadSingleAndUnloadDependents();
    TestMissingAndCycle();
    TestFailedLoadSkipsDependents();
    TestDestructorUnloads();
    TestSharedLibrary();
    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
