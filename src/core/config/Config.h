#pragma once

#include <string>
#include <unordered_map>

namespace Genesis {
namespace Config {

class ConfigManager {
public:
    static ConfigManager& GetInstance();

    bool LoadFromFile(const char* filename);
    bool SaveToFile(const char* filename);

    bool GetBool(const char* key, bool defaultValue = false) const;
    int GetInt(const char* key, int defaultValue = 0) const;
    double GetDouble(const char* key, double defaultValue = 0.0) const;
    const char* GetString(const char* key, const char* defaultValue = "") const;

    void SetBool(const char* key, bool value);
    void SetInt(const char* key, int value);
    void SetDouble(const char* key, double value);
    void SetString(const char* key, const char* value);

    bool HasKey(const char* key) const;
    void Clear();

private:
    ConfigManager();
    ~ConfigManager();

    std::unordered_map<std::string, std::string> m_Values;
};

} // namespace Config
} // namespace Genesis
