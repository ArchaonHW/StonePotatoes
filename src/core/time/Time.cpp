#include "Time.h"
#include "../platform/Platform.h"
#include "../logging/Logger.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <time.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#endif

namespace StonePotatoes {
namespace Time {

static uint64_t GetHighResolutionTime() {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
#elif defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#elif defined(__APPLE__)
    return mach_absolute_time();
#else
    return 0;
#endif
}

static double GetTimeResolution() {
#ifdef _WIN32
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return 1.0 / frequency.QuadPart;
#elif defined(__linux__)
    return 1e-9; // nanoseconds
#elif defined(__APPLE__)
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    return (double)info.numer / (double)info.denom * 1e-9;
#else
    return 1.0;
#endif
}

// Timer implementation
Timer::Timer() {
    Reset();
}

Timer::~Timer() = default;

void Timer::Reset() {
    m_StartTime = GetHighResolutionTime();
}

double Timer::GetElapsedSeconds() const {
    uint64_t current = GetHighResolutionTime();
    uint64_t elapsed = current - m_StartTime;
    return elapsed * GetTimeResolution();
}

double Timer::GetElapsedMilliseconds() const {
    return GetElapsedSeconds() * 1000.0;
}

// TimeManager implementation
TimeManager::TimeManager()
    : m_DeltaTime(0.0)
    , m_FixedDeltaTime(1.0 / 60.0)
    , m_TotalTime(0.0)
    , m_TimeScale(1.0)
    , m_GameMinute(0.0)
    , m_GameHour(0.0)
    , m_GameDay(0.0)
    , m_GameMonth(0.0)
    , m_GameYear(0.0) {
}

TimeManager::~TimeManager() {
}

TimeManager& TimeManager::GetInstance() {
    static TimeManager instance;
    return instance;
}

void TimeManager::Initialize() {
    m_Timer.Reset();
    STONEPOTATOES_LOG_INFO("TimeManager: Initialized");
}

void TimeManager::Shutdown() {
    STONEPOTATOES_LOG_INFO("TimeManager: Shutdown");
}

void TimeManager::Update() {
    double previousTime = m_TotalTime;
    m_TotalTime = m_Timer.GetElapsedSeconds();
    m_DeltaTime = (m_TotalTime - previousTime) * m_TimeScale;

    // Update multi-scale time
    // TODO: Implement proper multi-scale time system
    m_GameMinute += m_DeltaTime * 60.0; // 1 real second = 1 game minute
    m_GameHour = m_GameMinute / 60.0;
    m_GameDay = m_GameHour / 24.0;
    m_GameMonth = m_GameDay / 30.0;
    m_GameYear = m_GameMonth / 12.0;
}

void TimeManager::SetFixedDeltaTime(double deltaTime) {
    m_FixedDeltaTime = deltaTime;
}

double TimeManager::GetDeltaTime() const {
    return m_DeltaTime;
}

double TimeManager::GetFixedDeltaTime() const {
    return m_FixedDeltaTime;
}

double TimeManager::GetTotalTime() const {
    return m_TotalTime;
}

double TimeManager::GetGameMinute() const {
    return m_GameMinute;
}

double TimeManager::GetGameHour() const {
    return m_GameHour;
}

double TimeManager::GetGameDay() const {
    return m_GameDay;
}

double TimeManager::GetGameMonth() const {
    return m_GameMonth;
}

double TimeManager::GetGameYear() const {
    return m_GameYear;
}

void TimeManager::SetTimeScale(double scale) {
    m_TimeScale = scale;
}

double TimeManager::GetTimeScale() const {
    return m_TimeScale;
}

} // namespace Time
} // namespace StonePotatoes
