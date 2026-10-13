#include "Platform.h"
#include "../logging/Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <GLFW/glfw3.h>

#if defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/utsname.h>
#endif

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
    // TODO: Implement OS version detection
    return "Unknown";
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
