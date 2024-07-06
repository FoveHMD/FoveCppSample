#pragma once
#include <string>
#include <windows.h>

// Default window size
constexpr int windowSizeX = 1066;
constexpr int windowSizeY = 600;

// Windows version of NativeLaunchInfo
struct NativeLaunchInfo
{
	HINSTANCE instance = nullptr;
	int cmdShow = 0;
};

// Windows version of NativeWindow
struct NativeWindow
{
	HWND window = nullptr;
};
