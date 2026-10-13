#include "Logger.h"
#include "../platform/Platform.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace StonePotatoes {
namespace Logging {

Logger::Logger()
    : m_LogLevel(LogLevel::Info)
    , m_ConsoleOutput(true)
    , m_FileOutput(false)
    , m_LogFilename("stonepotatoes.log") {
}

Logger::~Logger() {
    if (m_FileHandle) {
        std::fclose(m_FileHandle);
        m_FileHandle = nullptr;
    }
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
    if (!enable && m_FileHandle) {
        std::fclose(m_FileHandle);
        m_FileHandle = nullptr;
    }
    m_FileOutput = enable;
    if (filename) {
        // Reopen under the new name on next write.
        if (m_FileHandle) {
            std::fclose(m_FileHandle);
            m_FileHandle = nullptr;
        }
        m_LogFilename = filename;
    }
}

namespace {

const char* TimestampNow() {
    static thread_local char buf[32];
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

} // namespace

void Logger::WriteToConsole(LogLevel level, const char* message) {
    printf("[%s] [%s] %s\n", TimestampNow(), GetLevelString(level), message);
}

void Logger::WriteToFile(LogLevel level, const char* message) {
    if (!m_FileHandle) {
#ifdef _WIN32
        if (fopen_s(&m_FileHandle, m_LogFilename.c_str(), "a") != 0) {
            m_FileHandle = nullptr;
        }
#else
        m_FileHandle = std::fopen(m_LogFilename.c_str(), "a");
#endif
        if (!m_FileHandle) {
            m_FileOutput = false;
            WriteToConsole(LogLevel::Error, "Logger: failed to open log file");
            return;
        }
    }
    std::fprintf(m_FileHandle, "[%s] [%s] %s\n",
                 TimestampNow(), GetLevelString(level), message);
    // Flush every line so logs survive a crash.
    std::fflush(m_FileHandle);
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
} // namespace StonePotatoes
