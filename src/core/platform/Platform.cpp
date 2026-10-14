#include "Platform.h"
#include "../logging/Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <GLFW/glfw3.h>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/utsname.h>
#endif

#include <string>

namespace StonePotatoes {
namespace Platform {

static void (*g_ErrorCallback)(const char* message) = nullptr;

bool Initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        STONEPOTATOES_LOG_ERROR("Failed to initialize GLFW");
        return false;
    }
    STONEPOTATOES_LOG_INFO("Platform: Initialized with GLFW");
    return true;
}

void Shutdown() {
    // Shutdown GLFW
    glfwTerminate();
    STONEPOTATOES_LOG_INFO("Platform: Shutdown complete");
}

const char* GetPlatformName() {
#ifdef _WIN32
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

const char* GetOSVersion() {
    static const std::string version = [] {
#ifdef _WIN32
        using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
        const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        const auto rtlGetVersion = ntdll
            ? reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"))
            : nullptr;
        if (!rtlGetVersion) {
            return std::string("Windows (version unavailable)");
        }

        OSVERSIONINFOW info{};
        info.dwOSVersionInfoSize = sizeof(info);
        if (rtlGetVersion(&info) != 0) {
            return std::string("Windows (version unavailable)");
        }

        return std::string("Windows ") + std::to_string(info.dwMajorVersion) + "." +
            std::to_string(info.dwMinorVersion) + "." + std::to_string(info.dwBuildNumber);
#elif defined(__linux__) || defined(__APPLE__)
        struct utsname info{};
        if (uname(&info) != 0) {
            return std::string(GetPlatformName()) + " (version unavailable)";
        }
#ifdef __APPLE__
        // macOS exposes its Darwin kernel release through uname.
        return std::string("Darwin ") + info.release;
#else
        return std::string(info.release);
#endif
#else
        return std::string("Unknown");
#endif
    }();
    return version.c_str();
}

void SetErrorCallback(void (*callback)(const char* message)) {
    g_ErrorCallback = callback;
}

void ReportError(const char* message) {
    if (g_ErrorCallback) {
        g_ErrorCallback(message);
    } else {
        // Default error reporting - use logger
        STONEPOTATOES_LOG_ERROR("%s", message);
    }
}

} // namespace Platform
} // namespace StonePotatoes
