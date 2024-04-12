#pragma once

// The Graphics class is a Facade to our Graphics device.
class Graphics
{
	static Graphics& Instance();
	static void CreateInstance();
	static void DestroyInstance();

private:
	Graphics(void);
	virtual ~Graphics(void);
};

