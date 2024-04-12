#pragma once

class Rect
{
public:
	Rect( float x = 0.0f, float y = 0.0f, float width = 0.0f, float height = 0.0f )
		: X(x)
		, Y(y)
		, Width(width)
		, Height(height)
	{}

	float X;
	float Y;
	float Width;
	float Height;
};