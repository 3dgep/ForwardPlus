#include <GraphicsTestPCH.h>

#include <Statistic.h>

Statistic::Statistic()
    : m_Total( 0.0 )
    , m_NumSamples( 0.0 )
    , m_Min( std::numeric_limits<double>::max() )
    , m_Max( std::numeric_limits<double>::min() )
{

}

// Reset all statistic values to 0.
void Statistic::Reset()
{
    m_Total = 0.0;
    m_NumSamples = 0.0;
    m_Min = std::numeric_limits<double>::max();
    m_Max = std::numeric_limits<double>::min();
}

void Statistic::Sample( double value )
{
    m_Total += value;
    m_NumSamples++;
    if ( value > m_Max )
    {
        m_Max = value;
    }
    if ( value < m_Min )
    {
        m_Min = value;
    }
}

double Statistic::GetNumSamples()
{
    return m_NumSamples;
}

double Statistic::GetAverage()
{
    if ( m_NumSamples > 0 )
    {
        return m_Total / m_NumSamples;
    }

    return 0.0;
}

double Statistic::GetMinValue()
{
    return m_Min;
}

double Statistic::GetMaxValue()
{
    return m_Max;
}
