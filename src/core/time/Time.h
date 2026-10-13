#pragma once

#include <cstdint>

namespace Genesis {
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

    // Multi-scale time
    double m_GameMinute;
    double m_GameHour;
    double m_GameDay;
    double m_GameMonth;
    double m_GameYear;

    Timer m_Timer;
};

} // namespace Time
} // namespace Genesis
