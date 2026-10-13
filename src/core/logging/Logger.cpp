#include "Logger.h"
#include "../platform/Platform.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace Genesis {
namespace Logging {

Logger::Logger()
    : m_LogLevel(LogLevel::Info)
    , m_ConsoleOutput(true)
    , m_FileOutput(false)
    , m_LogFilename("genesis.log") {
}

Logger::~Logger() {
}

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::Log(LogLevel level, const char* message) {
    if (level < m_LogLevel) {
        return;
    }

    if (m_ConsoleOutput) {
        WriteToConsole(level, message);
    }

    if (m_FileOutput) {
        WriteToFile(level, message);
    }
}

void Logger::LogFormat(LogLevel level, const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Call the non-variadic version explicitly
    Logger::Log(level, static_cast<const char*>(buffer));
}

void Logger::SetLogLevel(LogLevel level) {
    m_LogLevel = level;
}

LogLevel Logger::GetLogLevel() const {
    return m_LogLevel;
}

void Logger::EnableConsoleOutput(bool enable) {
    m_ConsoleOutput = enable;
}

void Logger::EnableFileOutput(bool enable, const char* filename) {
    m_FileOutput = enable;
    if (filename) {
        m_LogFilename = filename;
    }
}

void Logger::WriteToConsole(LogLevel level, const char* message) {
    // TODO: Add timestamp and coloring
    printf("[%s] %s\n", GetLevelString(level), message);
}

void Logger::WriteToFile(LogLevel level, const char* message) {
    (void)level; // Suppress unused parameter warning
    (void)message; // Suppress unused parameter warning
    // TODO: Implement file logging
    Platform::ReportError("Logger::WriteToFile: TODO - Implement file logging");
}

const char* Logger::GetLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}

} // namespace Logging
} // namespace Genesis
