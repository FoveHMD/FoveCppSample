#include "NativeUtil.h"
#include "Util.h"

using namespace std;

// Handles window messages
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

NativeWindow createNativeWindow(NativeLaunchInfo& nativeLaunchInfo, const string& windowTitle)
{
	// Register class
	WNDCLASSEX windowClass;
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = nativeLaunchInfo.instance;
	windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = L"FoveWindowClass";
	windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	if (!RegisterClassEx(&windowClass))
		throw "Unable to register window class: " + getLastErrorAsString();

	// Create window
	RECT r = {0, 0, windowSizeX, windowSizeY};
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
	HWND window = CreateWindow(L"FoveWindowClass", toUtf16(windowTitle).c_str(),
							   WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
							   CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, nativeLaunchInfo.instance, nullptr);
	if (!window)
		throw "Unable to create window: " + getLastErrorAsString();

	// Display the window on screen
	ShowWindow(window, nativeLaunchInfo.cmdShow);

	NativeWindow nativeWindow;
	nativeWindow.window = window;
	return nativeWindow;
}

bool flushWindowEvents(NativeWindow& window)
{
	while (true)
	{
		MSG msg = {0};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				return false;
			}
		}
		else
			return true;
	}
}

void showErrorBox(const string& msg)
{
	MessageBox(0, toUtf16(msg).c_str(), L"Error", MB_OK);
}

// Main program entry point and loop
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int cmdShow)
{
	NativeLaunchInfo info;
	info.instance = instance;
	info.cmdShow = cmdShow;
	programMain(info);
	return 0;
}
