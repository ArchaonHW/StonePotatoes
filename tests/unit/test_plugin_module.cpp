// Shared-library plugin used by test_plugin.cpp. | test_plugin.cpp 使用的共享函式庫外掛。
#include "core/plugin/Plugin.h"

namespace {
class DynamicTestPlugin : public StonePotatoes::Plugin::Plugin {
public:
    DynamicTestPlugin() : Plugin("dynamic_test", "1.2.3", {"base"}) {}
protected:
    bool OnLoad() override { return true; }
    void OnUnload() override {}
};
} // namespace

STONEPOTATOES_DEFINE_PLUGIN(DynamicTestPlugin)
