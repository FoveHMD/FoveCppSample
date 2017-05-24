#pragma once
#include <string>
#include <windows.h>

// Windows version of NativeLaunchInfo
struct NativeLaunchInfo {
	HINSTANCE instance = nullptr;
	int cmdShow = 0;
};

// Windows version of NativeWindow
struct NativeWindow {
	HWND window = nullptr;
};
