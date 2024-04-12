#pragma once

class HighResolutionTimerImpl;

class HighResolutionTimer
{
public:
    HighResolutionTimer(void);
    ~HighResolutionTimer(void);

    // "Tick" the timer to compute the amount of time since the last it was ticked (or since the timer was created).
    void Tick();

    double ElapsedSeconds() const;
    double ElapsedMilliSeconds() const;
    double ElapsedMicroSeconds() const;

private:
    HighResolutionTimerImpl* pImpl;
};
