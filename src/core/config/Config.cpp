#include "Config.h"
#include "../io/FileSystem.h"
#include "../logging/Logger.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace StonePotatoes {
namespace Config {

namespace {

// Minimal recursive-descent JSON parser that flattens a document into
// "dotted.path" -> (value, type) entries. Supports objects, arrays, strings,
// numbers, booleans and null. Type tag: 's' string, 'n' number, 'b' bool.
// 最小遞迴下降 JSON 解析器，將文件壓平為 "dotted.path" -> (值, 型別) 條目。
// 支援物件、陣列、字串、數字、布林與 null。型別標記：'s' 字串、'n' 數字、'b' 布林。
struct FlatEntry {
    std::string value;
    char type;
};

class JsonFlattener {
public:
    explicit JsonFlattener(const std::string& text)
        : m_Text(text)
        , m_Pos(0) {
    }

    bool Parse(std::unordered_map<std::string, FlatEntry>& out) {
        SkipWhitespace();
        if (!ParseValue("", out)) {
            return false;
        }
        SkipWhitespace();
        return m_Pos == m_Text.size();
    }

    const char* Error() const { return "JSON parse error"; }

private:
    void SkipWhitespace() {
        while (m_Pos < m_Text.size() &&
               std::isspace(static_cast<unsigned char>(m_Text[m_Pos]))) {
            ++m_Pos;
        }
        // Support // and /* */ comments (common in config files).
        while (m_Pos + 1 < m_Text.size() && m_Text[m_Pos] == '/') {
            if (m_Text[m_Pos + 1] == '/') {
                while (m_Pos < m_Text.size() && m_Text[m_Pos] != '\n') ++m_Pos;
            } else if (m_Text[m_Pos + 1] == '*') {
                m_Pos += 2;
                while (m_Pos + 1 < m_Text.size() &&
                       !(m_Text[m_Pos] == '*' && m_Text[m_Pos + 1] == '/')) {
                    ++m_Pos;
                }
                m_Pos += 2;
            } else {
                break;
            }
            while (m_Pos < m_Text.size() &&
                   std::isspace(static_cast<unsigned char>(m_Text[m_Pos]))) {
                ++m_Pos;
            }
        }
    }

    bool ParseValue(const std::string& key,
                    std::unordered_map<std::string, FlatEntry>& out) {
        SkipWhitespace();
        if (m_Pos >= m_Text.size()) {
            return false;
        }
        const char c = m_Text[m_Pos];
        if (c == '{') return ParseObject(key, out);
        if (c == '[') return ParseArray(key, out);
        if (c == '"') {
            std::string s;
            if (!ParseString(s)) return false;
            if (!key.empty()) out[key] = {s, 's'};
            return true;
        }
        if (c == 't' || c == 'f') {
            return ParseBool(key, out);
        }
        if (c == 'n') {
            if (m_Text.compare(m_Pos, 4, "null") == 0) {
                m_Pos += 4;
                return true; // null: store nothing
            }
            return false;
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return ParseNumber(key, out);
        }
        return false;
    }

    bool ParseObject(const std::string& prefix,
                     std::unordered_map<std::string, FlatEntry>& out) {
        ++m_Pos; // '{'
        SkipWhitespace();
        if (m_Pos < m_Text.size() && m_Text[m_Pos] == '}') {
            ++m_Pos;
            return true;
        }
        while (m_Pos < m_Text.size()) {
            SkipWhitespace();
            if (m_Text[m_Pos] != '"') {
                return false;
            }
            std::string name;
            if (!ParseString(name)) {
                return false;
            }
            SkipWhitespace();
            if (m_Pos >= m_Text.size() || m_Text[m_Pos] != ':') {
                return false;
            }
            ++m_Pos;
            const std::string childKey =
                prefix.empty() ? name : prefix + "." + name;
            if (!ParseValue(childKey, out)) {
                return false;
            }
            SkipWhitespace();
            if (m_Pos >= m_Text.size()) {
                return false;
            }
            if (m_Text[m_Pos] == ',') {
                ++m_Pos;
                continue;
            }
            if (m_Text[m_Pos] == '}') {
                ++m_Pos;
                return true;
            }
            return false;
        }
        return false;
    }

    bool ParseArray(const std::string& prefix,
                    std::unordered_map<std::string, FlatEntry>& out) {
        ++m_Pos; // '['
        SkipWhitespace();
        if (m_Pos < m_Text.size() && m_Text[m_Pos] == ']') {
            ++m_Pos;
            return true;
        }
        int index = 0;
        while (m_Pos < m_Text.size()) {
            const std::string childKey =
                prefix + "." + std::to_string(index++);
            if (!ParseValue(childKey, out)) {
                return false;
            }
            SkipWhitespace();
            if (m_Pos >= m_Text.size()) {
                return false;
            }
            if (m_Text[m_Pos] == ',') {
                ++m_Pos;
                SkipWhitespace();
                continue;
            }
            if (m_Text[m_Pos] == ']') {
                ++m_Pos;
                return true;
            }
            return false;
        }
        return false;
    }

    bool ParseString(std::string& out) {
        ++m_Pos; // opening quote
        out.clear();
        while (m_Pos < m_Text.size()) {
            const char c = m_Text[m_Pos++];
            if (c == '"') {
                return true;
            }
            if (c == '\\') {
                if (m_Pos >= m_Text.size()) {
                    return false;
                }
                const char e = m_Text[m_Pos++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': {
                        // Keep \uXXXX raw UTF-16 code unit -> UTF-8 (BMP only)
                        if (m_Pos + 4 > m_Text.size()) return false;
                        const unsigned cp = std::stoul(
                            m_Text.substr(m_Pos, 4), nullptr, 16);
                        m_Pos += 4;
                        if (cp < 0x80) {
                            out += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            out += static_cast<char>(0xC0 | (cp >> 6));
                            out += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (cp >> 12));
                            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        break;
                    }
                    default: out += e; break;
                }
            } else {
                out += c;
            }
        }
        return false;
    }

    bool ParseBool(const std::string& key,
                   std::unordered_map<std::string, FlatEntry>& out) {
        if (m_Text.compare(m_Pos, 4, "true") == 0) {
            m_Pos += 4;
            if (!key.empty()) out[key] = {"true", 'b'};
            return true;
        }
        if (m_Text.compare(m_Pos, 5, "false") == 0) {
            m_Pos += 5;
            if (!key.empty()) out[key] = {"false", 'b'};
            return true;
        }
        return false;
    }

    bool ParseNumber(const std::string& key,
                     std::unordered_map<std::string, FlatEntry>& out) {
        const size_t start = m_Pos;
        if (m_Text[m_Pos] == '-') ++m_Pos;
        while (m_Pos < m_Text.size() &&
               (std::isdigit(static_cast<unsigned char>(m_Text[m_Pos])) ||
                m_Text[m_Pos] == '.' || m_Text[m_Pos] == 'e' ||
                m_Text[m_Pos] == 'E' || m_Text[m_Pos] == '+' ||
                m_Text[m_Pos] == '-')) {
            ++m_Pos;
        }
        if (m_Pos == start || (m_Pos - start) == 1 && m_Text[start] == '-') {
            return false;
        }
        if (!key.empty()) {
            out[key] = {m_Text.substr(start, m_Pos - start), 'n'};
        }
        return true;
    }

    const std::string& m_Text;
    size_t m_Pos;
};

std::string JsonEscapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;
        }
    }
    return out;
}

} // namespace

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {
}

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadFromFile(const char* filename) {
    std::string text;
    if (!IO::FileSystem::ReadTextFile(filename, text)) {
        STONEPOTATOES_LOG_ERROR("ConfigManager: cannot read %s", filename);
        return false;
    }

    std::unordered_map<std::string, FlatEntry> flat;
    JsonFlattener parser(text);
    if (!parser.Parse(flat)) {
        STONEPOTATOES_LOG_ERROR("ConfigManager: %s is not valid JSON",
                                filename);
        return false;
    }

    for (const auto& kv : flat) {
        m_Values[kv.first] = kv.second.value;
        m_Types[kv.first] = kv.second.type;
    }
    STONEPOTATOES_LOG_INFO("ConfigManager: loaded %zu keys from %s",
                           flat.size(), filename);
    return true;
}

bool ConfigManager::SaveToFile(const char* filename) {
    std::ostringstream out;
    out << "{\n";
    size_t i = 0;
    for (const auto& kv : m_Values) {
        const char type = [&]() {
            auto it = m_Types.find(kv.first);
            return it != m_Types.end() ? it->second : 's';
        }();
        out << "  \"" << JsonEscapeString(kv.first) << "\": ";
        if (type == 's') {
            out << '"' << JsonEscapeString(kv.second) << '"';
        } else {
            out << kv.second;
        }
        if (++i < m_Values.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "}\n";
    if (!IO::FileSystem::WriteTextFile(filename, out.str())) {
        STONEPOTATOES_LOG_ERROR("ConfigManager: cannot write %s", filename);
        return false;
    }
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
    return std::atoi(it->second.c_str());
}

double ConfigManager::GetDouble(const char* key, double defaultValue) const {
    auto it = m_Values.find(key);
    if (it == m_Values.end()) {
        return defaultValue;
    }
    return std::atof(it->second.c_str());
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
    m_Types[key] = 'b';
}

void ConfigManager::SetInt(const char* key, int value) {
    m_Values[key] = std::to_string(value);
    m_Types[key] = 'n';
}

void ConfigManager::SetDouble(const char* key, double value) {
    std::ostringstream ss;
    ss << value;
    m_Values[key] = ss.str();
    m_Types[key] = 'n';
}

void ConfigManager::SetString(const char* key, const char* value) {
    m_Values[key] = value;
    m_Types[key] = 's';
}

bool ConfigManager::HasKey(const char* key) const {
    return m_Values.find(key) != m_Values.end();
}

void ConfigManager::Clear() {
    m_Values.clear();
    m_Types.clear();
}

} // namespace Config
} // namespace StonePotatoes
