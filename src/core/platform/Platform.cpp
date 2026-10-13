#include "Platform.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/utsname.h>
#endif

namespace Genesis {
namespace Platform {

static void (*g_ErrorCallback)(const char* message) = nullptr;

bool Initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        Platform::ReportError("Failed to initialize GLFW");
        return false;
    }
    Platform::ReportError("Platform: Initialized with GLFW");
    return true;
}

void Shutdown() {
    // Shutdown GLFW
    glfwTerminate();
    Platform::ReportError("Platform: Shutdown complete");
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
        // Default error reporting
#ifdef _WIN32
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
#else
        fprintf(stderr, "ERROR: %s\n", message);
#endif
    }
}

} // namespace Platform
} // namespace Genesis
