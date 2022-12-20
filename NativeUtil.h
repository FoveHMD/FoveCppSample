#pragma once
#include <string>

// This header declares structs and functions that are implemented
// per platform. Each item here is implemented by the corresponding Util header

// Stores information about the launching of the program
// The contents of this struct are platform specific
struct NativeLaunchInfo;

// Stores information about a window
// The contents of this struct are platform specific
struct NativeWindow;

// This is the main function
// Each platform defines the proper main (eg, WinMain() or main()), then calls into this function
// This function is implented by each example program that uses native gui (eg, opening windows)
void Main(NativeLaunchInfo);

// Creates a window
NativeWindow CreateNativeWindow(NativeLaunchInfo&, const std::string& windowTitle);

// Flushes the event queue for the window if needed
// Returns false if the window has received a close signal, otherwise true
bool FlushWindowEvents(NativeWindow&);

// Displays a modal dialgo box with the given error message
void ShowErrorBox(const std::string& msg);

// Include the platform-specific version of this header based on the current platform
#ifdef _WIN32
#include "WindowsUtil.h"
#elif defined(__APPLE__)
#else
#include "LinuxUtil.h"
#endif
