#include "Config.h"
#include "../platform/Platform.h"
#include <fstream>
#include <sstream>

namespace Genesis {
namespace Config {

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {
}

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadFromFile(const char* filename) {
    (void)filename; // Suppress unused parameter warning
    // TODO: Implement actual config file loading (JSON, TOML, YAML)
    Platform::ReportError("ConfigManager::LoadFromFile: TODO - Implement actual config loading");
    return true;
}

bool ConfigManager::SaveToFile(const char* filename) {
    (void)filename; // Suppress unused parameter warning
    // TODO: Implement actual config file saving
    Platform::ReportError("ConfigManager::SaveToFile: TODO - Implement actual config saving");
    return true;
}

bool ConfigManager::GetBool(const char* key, bool defaultValue) const {
    auto it = m_Values.find(key);
    if (it == m_Values.end()) {
        return defaultValue;
    }
    return it->second == "true" || it->second == "1";
}

int ConfigManager::GetInt(const char* key, int defaultValue) const {
    auto it = m_Values.find(key);
    if (it == m_Values.end()) {
        return defaultValue;
    }
    return std::stoi(it->second);
}

double ConfigManager::GetDouble(const char* key, double defaultValue) const {
    auto it = m_Values.find(key);
    if (it == m_Values.end()) {
        return defaultValue;
    }
    return std::stod(it->second);
}

const char* ConfigManager::GetString(const char* key, const char* defaultValue) const {
    auto it = m_Values.find(key);
    if (it == m_Values.end()) {
        return defaultValue;
    }
    return it->second.c_str();
}

void ConfigManager::SetBool(const char* key, bool value) {
    m_Values[key] = value ? "true" : "false";
}

void ConfigManager::SetInt(const char* key, int value) {
    m_Values[key] = std::to_string(value);
}

void ConfigManager::SetDouble(const char* key, double value) {
    m_Values[key] = std::to_string(value);
}

void ConfigManager::SetString(const char* key, const char* value) {
    m_Values[key] = value;
}

bool ConfigManager::HasKey(const char* key) const {
    return m_Values.find(key) != m_Values.end();
}

void ConfigManager::Clear() {
    m_Values.clear();
}

} // namespace Config
} // namespace Genesis
