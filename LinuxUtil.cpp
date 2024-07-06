#include "LinuxUtil.h"
#include "NativeUtil.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include <X11/X.h>
#include <X11/Xlib.h>
#undef min
#undef max
#undef None
#undef Data
#undef Always

using namespace std;

namespace
{

static_assert(is_same<Display, XDisplay>::value, "X11/Display and our XDisplay should be the same type");
static_assert(is_same<Window, XWindow>::value, "X11/Window and our XWindow should be the same type");

void closeXDisplay(XDisplay* display)
{
	if (!display)
		return;
	const int ret = XCloseDisplay(display);
	cout << "Closed X display: " << display << '\n';
}

unique_ptr<XDisplay, void (*)(XDisplay*)> connectX()
{
	XInitThreads();
	XDisplay* const display = XOpenDisplay(nullptr);
	if (!display)
	{
		cerr << "No X display available.";
		return {nullptr, closeXDisplay};
	}

	return {display, closeXDisplay};
}

enum XKeyCode
{
	ESC,
	Q,
	UNKNOWN,
};

XKeyCode mapKey(const unsigned int keycode)
{
	switch (keycode)
	{
	case 9:
		return XKeyCode::ESC;
	case 24:
		return XKeyCode::Q;
	default:
		break;
	};
	return XKeyCode::UNKNOWN;
};

} // namespace

// Displays a modal dialgo box with the given error message
void ShowErrorBox(const std::string& err)
{
	cerr << "Error: " << err << '\n'; // FIXME
}

class XlibWindowImpl
{
public:
	XlibWindowImpl()
		: m_display{connectX()}
	{
	}

	~XlibWindowImpl()
	{
		stopEventThread(); // safe even if thread is not up
		if (m_eventThread.joinable())
			m_eventThread.join();
	}

	Display* nativeHandle()
	{
		return m_display.get();
	}

	XDisplay* xDisplay() { return m_display.get(); }
	const XDisplay* xDisplay() const { return m_display.get(); }
	XWindow xWindow() const { return m_window; }

private:
	void startEventThread()
	{
		m_keepAlive = true;
		m_eventThread = thread{&XlibWindowImpl::eventThread, this};
	}

	void stopEventThread()
	{
		m_keepAlive = false;
	}

	void eventThread()
	{
		unique_lock<mutex> dataLock{m_mutex};
		while (m_keepAlive)
		{
			// Arbitrary sleep to not eat up too much CPU
			if (!XPending(nativeHandle()))
			{
				m_eventThreadCV.wait_for(dataLock, 25ms);
				continue;
			}

			XEvent event{};
			XNextEvent(nativeHandle(), &event);

			if (event.type == ClientMessage && event.xclient.data.l[0] == m_deleteMessage)
			{
				cout << "Window close requested through wm\n";
				m_keepAlive = false;
			}
			else if (event.type == ConfigureNotify)
			{
				// TODO
				// updateSize(&dataLock.data());
			}
			else if (event.type == KeyPress)
			{
				const XKeyCode code = mapKey(event.xkey.keycode);
				if (code == XKeyCode::ESC || code == XKeyCode::Q)
				{
					cout << "Window close requested via key press\n";
					m_keepAlive = false;
				}
			}
		}
	}

private:
	unique_ptr<XDisplay, void (*)(XDisplay*)> m_display;
	XWindow m_window;

	friend NativeWindow CreateNativeWindow(NativeLaunchInfo&, const string&);
	friend bool FlushWindowEvents(NativeWindow&);
	Atom m_deleteMessage;

	mutex m_mutex{}; // guards m_display
	condition_variable m_eventThreadCV{};
	thread m_eventThread{};
	atomic_bool m_keepAlive{true};
};

NativeWindow::NativeWindow() = default;
NativeWindow::NativeWindow(NativeWindow&&) = default;
NativeWindow& NativeWindow::operator=(NativeWindow&&) = default;
NativeWindow::~NativeWindow() = default;

XWindow NativeWindow::xWindow() const
{
	return m_impl->xWindow();
}

const XDisplay* NativeWindow::xDisplay() const
{
	return m_impl->xDisplay();
}

XDisplay* NativeWindow::xDisplay()
{
	return m_impl->xDisplay();
}

XWindowSize NativeWindow::windowSize()
{
	XWindowSize size{};
	Window root{};
	int xOffset{};
	int yOffset{};
	unsigned int border{};
	unsigned int depth{};
	XGetGeometry(xDisplay(), xWindow(), &root, &xOffset, &yOffset, &size.width, &size.height, &border, &depth);
	return size;
}

NativeWindow CreateNativeWindow(NativeLaunchInfo& nativeLaunchInfo, const string& windowTitle)
{
	unique_ptr<XlibWindowImpl> display = make_unique<XlibWindowImpl>();

	// Set up Xlib window
	const int screen = XDefaultScreen(display->nativeHandle());
	const Window rootWindow = RootWindow(display->nativeHandle(), screen);
	XSetWindowAttributes windowAttributes{};
	Window window = XCreateWindow(
		display->nativeHandle(),
		rootWindow,
		15, 15, // arbitrary window offsets
		windowSizeX,
		windowSizeY,
		10, // arbitrary border width
		CopyFromParent,
		InputOutput,
		CopyFromParent,
		CWColormap,
		&windowAttributes);
	XStoreName(display->nativeHandle(), window, windowTitle.c_str());

	Atom deleteMessage = XInternAtom(display->nativeHandle(), "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display->nativeHandle(), window, &deleteMessage, 1);
	XSelectInput(display->nativeHandle(), window, StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
	display->m_deleteMessage = XInternAtom(display->nativeHandle(), "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display->nativeHandle(), window, &display->m_deleteMessage, 1);

	display->m_window = window;

	// Display the window
	XMapWindow(display->nativeHandle(), window);
	XFlush(display->nativeHandle());
	display->startEventThread();

	NativeWindow nativeWindow{};
	nativeWindow.m_impl = std::move(display);

	return std::move(nativeWindow);
}

bool FlushWindowEvents(NativeWindow& window)
{
	XFlush(window.xDisplay());
	return window.m_impl->m_keepAlive;
}

// Main program entry point and loop
int main()
{
	NativeLaunchInfo info;
	Main(info);
	return 0;
}
