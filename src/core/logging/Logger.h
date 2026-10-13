#pragma once

#include <cstdio>
#include <string>

namespace StonePotatoes {
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
    void EnableFileOutput(bool enable, const char* filename = "stonepotatoes.log");

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
    FILE* m_FileHandle = nullptr;
};

} // namespace Logging
} // namespace StonePotatoes

// Convenience macros
#define STONEPOTATOES_LOG_DEBUG(...) StonePotatoes::Logging::Logger::GetInstance().LogFormat(StonePotatoes::Logging::LogLevel::Debug, __VA_ARGS__)
#define STONEPOTATOES_LOG_INFO(...) StonePotatoes::Logging::Logger::GetInstance().LogFormat(StonePotatoes::Logging::LogLevel::Info, __VA_ARGS__)
#define STONEPOTATOES_LOG_WARNING(...) StonePotatoes::Logging::Logger::GetInstance().LogFormat(StonePotatoes::Logging::LogLevel::Warning, __VA_ARGS__)
#define STONEPOTATOES_LOG_ERROR(...) StonePotatoes::Logging::Logger::GetInstance().LogFormat(StonePotatoes::Logging::LogLevel::Error, __VA_ARGS__)
#define STONEPOTATOES_LOG_FATAL(...) StonePotatoes::Logging::Logger::GetInstance().LogFormat(StonePotatoes::Logging::LogLevel::Fatal, __VA_ARGS__)
