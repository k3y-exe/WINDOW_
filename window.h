#ifndef WINDOW_H
#define WINDOW_H

/* ============================================================
	Platform detection
	============================================================ */
#if defined(_WIN32)
	#define WINDOW_WIN32
#elif defined(__APPLE__)
	#define WINDOW_MACOS
#else
	#define WINDOW_X11
#endif

/* ============================================================
	Common API
	============================================================ */
typedef struct WINDOW_Window {
	int width;
	int height;
	int shouldClose;
	void *handle;
} WINDOW_Window;

/* Create / destroy / poll events */
static int WINDOW_WindowCreate(WINDOW_Window *p_window, const char *p_title, int p_width, int p_height);
static void WINDOW_WindowPoll(WINDOW_Window *p_window);
static void WINDOW_WindowDestroy(WINDOW_Window *p_window);
static int WINDOW_WindowShouldClose(const WINDOW_Window *p_window);

/* ============================================================
	WINDOW_X11 (Linux)
	============================================================ */
#if defined(WINDOW_X11)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct WINDOW_X11Window {
	Display *display;
	int screen;
	Window window;
	Atom wmDelete;
} WINDOW_X11Window;

static int WINDOW_WindowCreate(WINDOW_Window *p_window, const char *p_title, int p_width, int p_height)
{
	WINDOW_X11Window *x11Window;
	x11Window = (WINDOW_X11Window*)malloc(sizeof(WINDOW_X11Window));
	if (!x11Window)
		return 0;

	x11Window->display = XOpenDisplay(NULL);
	if (!x11Window->display) {
		free(x11Window);
		return 0;
	}

	x11Window->screen = DefaultScreen(x11Window->display);

	x11Window->window = XCreateSimpleWindow(
		x11Window->display,
		RootWindow(x11Window->display, x11Window->screen),
		0, 0,
		(unsigned int)p_width,
		(unsigned int)p_height,
		1,
		BlackPixel(x11Window->display, x11Window->screen),
		WhitePixel(x11Window->display, x11Window->screen)
	);

	XStoreName(x11Window->display, x11Window->window, p_title);

	XSelectInput(
		x11Window->display,
		x11Window->window,
		ExposureMask |
		KeyPressMask |
		KeyReleaseMask |
		ButtonPressMask |
		ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask
	);

	x11Window->wmDelete = XInternAtom(x11Window->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(x11Window->display, x11Window->window, &x11Window->wmDelete, 1);

	XMapWindow(x11Window->display, x11Window->window);

	p_window->handle = x11Window;
	p_window->width = p_width;
	p_window->height = p_height;
	p_window->shouldClose = 0;

	return 1;
}

static void WINDOW_WindowPoll(WINDOW_Window *p_window)
{
	WINDOW_X11Window *x11Window;
	XEvent event;
	x11Window = (WINDOW_X11Window*)p_window->handle;

	while (XPending(x11Window->display)) {
		XNextEvent(x11Window->display, &event);

		if (event.type == ClientMessage) {
			if ((Atom)event.xclient.data.l[0] == x11Window->wmDelete)
				p_window->shouldClose = 1;
		} else if (event.type == ConfigureNotify) {
			p_window->width = event.xconfigure.width;
			p_window->height = event.xconfigure.height;
		}
	}
}

static void WINDOW_WindowDestroy(WINDOW_Window *p_window)
{
	WINDOW_X11Window *x11Window;
	x11Window = (WINDOW_X11Window*)p_window->handle;
	if (!x11Window)
		return;

	XDestroyWindow(x11Window->display, x11Window->window);
	XCloseDisplay(x11Window->display);
	free(x11Window);
	p_window->handle = NULL;
}

static int WINDOW_WindowShouldClose(const WINDOW_Window *p_window)
{
	return p_window->shouldClose;
}

#endif /* WINDOW_X11 */

/* ============================================================
	WINDOW_WIN32
	============================================================ */
#if defined(WINDOW_WIN32)

#include <windows.h>
#include <stdlib.h>

typedef struct WINDOW_Win32Window {
	HWND hwnd;
	WNDCLASS wc;
	HINSTANCE hInstance;
} WINDOW_Win32Window;

static LRESULT CALLBACK WINDOW_WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WINDOW_Window *window;
	window = (WINDOW_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg) {
	case WM_CLOSE:
		if (window) window->shouldClose = 1;
		return 0;
	case WM_SIZE:
		if (window) {
			window->width = LOWORD(lParam);
			window->height = HIWORD(lParam);
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static int WINDOW_WindowCreate(WINDOW_Window *p_window, const char *p_title, int p_width, int p_height)
{
	WINDOW_Win32Window *win32Window;
	win32Window = (WINDOW_Win32Window*)malloc(sizeof(WINDOW_Win32Window));
	if (!win32Window)
		return 0;

	win32Window->hInstance = GetModuleHandle(NULL);
	win32Window->wc.style = CS_HREDRAW | CS_VREDRAW;
	win32Window->wc.lpfnWndProc = WINDOW_WinProc;
	win32Window->wc.cbClsExtra = 0;
	win32Window->wc.cbWndExtra = 0;
	win32Window->wc.hInstance = win32Window->hInstance;
	win32Window->wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win32Window->wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	win32Window->wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	win32Window->wc.lpszMenuName = NULL;
	win32Window->wc.lpszClassName = "WINDOW_WinClass";

	if (!RegisterClass(&win32Window->wc)) {
		free(win32Window);
		return 0;
	}

	win32Window->hwnd = CreateWindow(
		win32Window->wc.lpszClassName,
		p_title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		p_width, p_height,
		NULL, NULL, win32Window->hInstance, NULL
	);

	SetWindowLongPtr(win32Window->hwnd, GWLP_USERDATA, (LONG_PTR)p_window);

	ShowWindow(win32Window->hwnd, SW_SHOW);

	p_window->handle = win32Window;
	p_window->width = p_width;
	p_window->height = p_height;
	p_window->shouldClose = 0;

	return 1;
}

static void WINDOW_WindowPoll(WINDOW_Window *p_window)
{
	WINDOW_Win32Window *win32Window;
	MSG msg;
	win32Window = (WINDOW_Win32Window*)p_window->handle;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static void WINDOW_WindowDestroy(WINDOW_Window *p_window)
{
	WINDOW_Win32Window *win32Window;
	win32Window = (WINDOW_Win32Window*)p_window->handle;
	if (!win32Window)
		return;

	DestroyWindow(win32Window->hwnd);
	UnregisterClass(win32Window->wc.lpszClassName, win32Window->hInstance);
	free(win32Window);
	p_window->handle = NULL;
}

static int WINDOW_WindowShouldClose(const WINDOW_Window *p_window)
{
	return p_window->shouldClose;
}

#endif /* WINDOW_WIN32 */

/* ============================================================
	WINDOW_MACOS
	============================================================ */
#if defined(WINDOW_MACOS)

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
#include <stdlib.h>

typedef struct WINDOW_MacWindow {
	id nswindow;
	id nsapp;
} WINDOW_MacWindow;

static int WINDOW_WindowCreate(WINDOW_Window *p_window, const char *p_title, int p_width, int p_height)
{
	WINDOW_MacWindow *macWindow;
	macWindow = (WINDOW_MacWindow*)malloc(sizeof(WINDOW_MacWindow));
	if (!macWindow) return 0;

	Class NSApplicationClass = (Class)objc_getClass("NSApplication");
	macWindow->nsapp = ((id(*)(Class, SEL))objc_msgSend)(NSApplicationClass, sel_registerName("sharedApplication"));

	Class NSWindowClass = (Class)objc_getClass("NSWindow");
	NSRect frame;
	frame.origin.x = 0;
	frame.origin.y = 0;
	frame.size.width = p_width;
	frame.size.height = p_height;

	macWindow->nswindow = ((id(*)(Class, SEL))objc_msgSend)(NSWindowClass, sel_registerName("alloc"));
	macWindow->nswindow = ((id(*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(
		macWindow->nswindow,
		sel_registerName("initWithContentRect:styleMask:backing:defer:"),
		frame,
		1, 0, 1
	);

	CFStringRef nsTitle;
	nsTitle = CFStringCreateWithCString(NULL, p_title, kCFStringEncodingUTF8);
	((void(*)(id, SEL, id))objc_msgSend)(macWindow->nswindow, sel_registerName("setTitle:"), nsTitle);
	CFRelease(nsTitle);

	((void(*)(id, SEL))objc_msgSend)(macWindow->nswindow, sel_registerName("makeKeyAndOrderFront:"), macWindow->nswindow);

	p_window->handle = macWindow;
	p_window->width = p_width;
	p_window->height = p_height;
	p_window->shouldClose = 0;

	return 1;
}

static void WINDOW_WindowPoll(WINDOW_Window *p_window)
{
	WINDOW_MacWindow *macWindow;
	macWindow = (WINDOW_MacWindow*)p_window->handle;

	Class NSAppClass = (Class)objc_getClass("NSApplication");
	id sharedApp = ((id(*)(Class, SEL))objc_msgSend)(NSAppClass, sel_registerName("sharedApplication"));
	((void(*)(id, SEL))objc_msgSend)(sharedApp, sel_registerName("updateWindows"));
}

static void WINDOW_WindowDestroy(WINDOW_Window *p_window)
{
	WINDOW_MacWindow *macWindow;
	macWindow = (WINDOW_MacWindow*)p_window->handle;
	if (!macWindow) return;

	((void(*)(id, SEL))objc_msgSend)(macWindow->nswindow, sel_registerName("close"));
	free(macWindow);
	p_window->handle = NULL;
}

static int WINDOW_WindowShouldClose(const WINDOW_Window *p_window)
{
	return p_window->shouldClose;
}

#endif /* WINDOW_MACOS */

#endif /* WINDOW_H */
