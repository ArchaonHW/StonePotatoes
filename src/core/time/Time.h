#pragma once

#include <cstdint>

namespace StonePotatoes {
namespace Time {

class Timer {
public:
    Timer();
    ~Timer();

    void Reset();
    double GetElapsedSeconds() const;
    double GetElapsedMilliseconds() const;

private:
    uint64_t m_StartTime;
};

class TimeManager {
public:
    static TimeManager& GetInstance();

    void Initialize();
    void Shutdown();

    void Update();
    // Advance with an explicit real-time delta. This makes simulation updates
    // reproducible in tests, replays, and lockstep callers.
    // 使用明確的真實時間差推進；測試、重播與 lockstep 呼叫端可重現更新。
    bool Advance(double realDeltaSeconds);
    void SetFixedDeltaTime(double deltaTime);
    double GetDeltaTime() const;
    double GetFixedDeltaTime() const;
    double GetTotalTime() const;

    // Multi-scale time
    double GetGameMinute() const;
    double GetGameHour() const;
    double GetGameDay() const;
    double GetGameMonth() const;
    double GetGameYear() const;

    void SetTimeScale(double scale);
    double GetTimeScale() const;

private:
    TimeManager();
    ~TimeManager();

    double m_DeltaTime;
    double m_FixedDeltaTime;
    double m_TotalTime;
    double m_TimeScale;
    double m_LastWallTime;

    // Multi-scale time
    double m_GameMinute;
    double m_GameHour;
    double m_GameDay;
    double m_GameMonth;
    double m_GameYear;

    Timer m_Timer;
};

} // namespace Time
} // namespace StonePotatoes
