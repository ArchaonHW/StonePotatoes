#pragma once

#include <string>

namespace Genesis {
namespace Logging {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    static Logger& GetInstance();

    void Log(LogLevel level, const char* message);
    void LogFormat(LogLevel level, const char* format, ...);

    void SetLogLevel(LogLevel level);
    LogLevel GetLogLevel() const;

    void EnableConsoleOutput(bool enable);
    void EnableFileOutput(bool enable, const char* filename = "genesis.log");

private:
    Logger();
    ~Logger();

    void WriteToConsole(LogLevel level, const char* message);
    void WriteToFile(LogLevel level, const char* message);
    const char* GetLevelString(LogLevel level) const;

    LogLevel m_LogLevel;
    bool m_ConsoleOutput;
    bool m_FileOutput;
    std::string m_LogFilename;
};

} // namespace Logging
} // namespace Genesis

// Convenience macros
#define GENESIS_LOG_DEBUG(...) Genesis::Logging::Logger::GetInstance().LogFormat(Genesis::Logging::LogLevel::Debug, __VA_ARGS__)
#define GENESIS_LOG_INFO(...) Genesis::Logging::Logger::GetInstance().LogFormat(Genesis::Logging::LogLevel::Info, __VA_ARGS__)
#define GENESIS_LOG_WARNING(...) Genesis::Logging::Logger::GetInstance().LogFormat(Genesis::Logging::LogLevel::Warning, __VA_ARGS__)
#define GENESIS_LOG_ERROR(...) Genesis::Logging::Logger::GetInstance().LogFormat(Genesis::Logging::LogLevel::Error, __VA_ARGS__)
#define GENESIS_LOG_FATAL(...) Genesis::Logging::Logger::GetInstance().LogFormat(Genesis::Logging::LogLevel::Fatal, __VA_ARGS__)
