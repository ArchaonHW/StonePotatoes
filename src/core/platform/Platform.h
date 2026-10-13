#pragma once

namespace StonePotatoes {
namespace Platform {

// Platform initialization
bool Initialize();
void Shutdown();

// Platform information
const char* GetPlatformName();
const char* GetOSVersion();

// Error handling
void SetErrorCallback(void (*callback)(const char* message));
void ReportError(const char* message);

} // namespace Platform
} // namespace StonePotatoes
