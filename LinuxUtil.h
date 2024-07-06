#pragma once
#include <memory>

// Default window size
constexpr int windowSizeX = 1066;
constexpr int windowSizeY = 600;

// We really do not want to inlcude Xlib headers unless absolutely necessary.
using XWindow = unsigned long;     // Window = XID = init in X11/X.h
using XDisplay = struct _XDisplay; // Opaque handle to X11 display

// Linux version of NativeLaunchInfo
struct NativeLaunchInfo
{
	//	HINSTANCE instance = nullptr;
	//	int cmdShow = 0;
};

struct XWindowSize
{
	unsigned int width;
	unsigned int height;
};

// Windows version of NativeWindow
class XlibWindowImpl;

struct NativeWindow
{
	std::unique_ptr<XlibWindowImpl> m_impl;

	XDisplay* xDisplay();
	const XDisplay* xDisplay() const;
	XWindow xWindow() const;

	XWindowSize windowSize();

	~NativeWindow();
	NativeWindow();
	NativeWindow(NativeWindow&&);
	NativeWindow& operator=(NativeWindow&&);
	NativeWindow(const NativeWindow&) = delete;
	NativeWindow& operator=(const NativeWindow&) = delete;
};
