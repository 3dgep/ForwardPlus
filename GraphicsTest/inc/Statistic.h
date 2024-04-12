#pragma once

class Statistic
{
public:
    Statistic();

    // Reset all statistic values to 0.
    void Reset();

    void Sample( double value );

    double GetNumSamples();
    double GetAverage();
    double GetMinValue();
    double GetMaxValue();

private:

    double m_Total;
    double m_NumSamples;
    double m_Min;
    double m_Max;
};