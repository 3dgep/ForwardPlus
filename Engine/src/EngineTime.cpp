#include <EnginePCH.h>
#include <EngineTime.h>

extern float g_GameDeltaTime;
extern float g_ApplicationTime;

float EngineTime::GetDeltaTime()
{
	return g_GameDeltaTime;
}

float EngineTime::GetTotalTime()
{
	return g_ApplicationTime;
}