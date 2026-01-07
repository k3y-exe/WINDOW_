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

	/* Backend-specific handle */
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
	WINDOW_X11Window *p_x11;
	p_x11 = (WINDOW_X11Window*)malloc(sizeof(WINDOW_X11Window));
	if (!p_x11)
		return 0;

	p_x11->display = XOpenDisplay(NULL);
	if (!p_x11->display) {
		free(p_x11);
		return 0;
	}

	p_x11->screen = DefaultScreen(p_x11->display);

	p_x11->window = XCreateSimpleWindow(
		p_x11->display,
		RootWindow(p_x11->display, p_x11->screen),
		0, 0,
		(unsigned int)p_width,
		(unsigned int)p_height,
		1,
		BlackPixel(p_x11->display, p_x11->screen),
		WhitePixel(p_x11->display, p_x11->screen)
	);

	XStoreName(p_x11->display, p_x11->window, p_title);

	XSelectInput(
		p_x11->display,
		p_x11->window,
		ExposureMask |
		KeyPressMask |
		KeyReleaseMask |
		ButtonPressMask |
		ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask
	);

	p_x11->wmDelete = XInternAtom(p_x11->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(p_x11->display, p_x11->window, &p_x11->wmDelete, 1);

	XMapWindow(p_x11->display, p_x11->window);

	p_window->handle = p_x11;
	p_window->width = p_width;
	p_window->height = p_height;
	p_window->shouldClose = 0;

	return 1;
}

static void WINDOW_WindowPoll(WINDOW_Window *p_window)
{
	WINDOW_X11Window *p_x11;
	XEvent event;
	p_x11 = (WINDOW_X11Window*)p_window->handle;

	while (XPending(p_x11->display)) {
		XNextEvent(p_x11->display, &event);

		if (event.type == ClientMessage) {
			if ((Atom)event.xclient.data.l[0] == p_x11->wmDelete) {
				p_window->shouldClose = 1;
			}
		} else if (event.type == ConfigureNotify) {
			p_window->width = event.xconfigure.width;
			p_window->height = event.xconfigure.height;
		}
	}
}

static void WINDOW_WindowDestroy(WINDOW_Window *p_window)
{
	WINDOW_X11Window *p_x11;
	p_x11 = (WINDOW_X11Window*)p_window->handle;
	if (!p_x11)
		return;

	XDestroyWindow(p_x11->display, p_x11->window);
	XCloseDisplay(p_x11->display);
	free(p_x11);
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
	WINDOW_Window *p_window;
	p_window = (WINDOW_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg) {
	case WM_CLOSE:
		if (p_window) p_window->shouldClose = 1;
		return 0;
	case WM_SIZE:
		if (p_window) {
			p_window->width = LOWORD(lParam);
			p_window->height = HIWORD(lParam);
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static int WINDOW_WindowCreate(WINDOW_Window *p_window, const char *p_title, int p_width, int p_height)
{
	WINDOW_Win32Window *p_win;
	p_win = (WINDOW_Win32Window*)malloc(sizeof(WINDOW_Win32Window));
	if (!p_win)
		return 0;

	p_win->hInstance = GetModuleHandle(NULL);
	p_win->wc.style = CS_HREDRAW | CS_VREDRAW;
	p_win->wc.lpfnWndProc = WINDOW_WinProc;
	p_win->wc.cbClsExtra = 0;
	p_win->wc.cbWndExtra = 0;
	p_win->wc.hInstance = p_win->hInstance;
	p_win->wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	p_win->wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	p_win->wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	p_win->wc.lpszMenuName = NULL;
	p_win->wc.lpszClassName = "WINDOW_WinClass";

	if (!RegisterClass(&p_win->wc)) {
		free(p_win);
		return 0;
	}

	p_win->hwnd = CreateWindow(
		p_win->wc.lpszClassName,
		p_title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		p_width, p_height,
		NULL, NULL, p_win->hInstance, NULL
	);

	SetWindowLongPtr(p_win->hwnd, GWLP_USERDATA, (LONG_PTR)p_window);

	ShowWindow(p_win->hwnd, SW_SHOW);

	p_window->handle = p_win;
	p_window->width = p_width;
	p_window->height = p_height;
	p_window->shouldClose = 0;

	return 1;
}

static void WINDOW_WindowPoll(WINDOW_Window *p_window)
{
	WINDOW_Win32Window *p_win;
	MSG msg;
	p_win = (WINDOW_Win32Window*)p_window->handle;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static void WINDOW_WindowDestroy(WINDOW_Window *p_window)
{
	WINDOW_Win32Window *p_win;
	p_win = (WINDOW_Win32Window*)p_window->handle;
	if (!p_win)
		return;

	DestroyWindow(p_win->hwnd);
	UnregisterClass(p_win->wc.lpszClassName, p_win->hInstance);
	free(p_win);
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
	WINDOW_MacWindow *p_mac;
	p_mac = (WINDOW_MacWindow*)malloc(sizeof(WINDOW_MacWindow));
	if (!p_mac) return 0;

	Class NSApplicationClass = (Class)objc_getClass("NSApplication");
	p_mac->nsapp = ((id(*)(Class, SEL))objc_msgSend)(NSApplicationClass, sel_registerName("sharedApplication"));

	Class NSWindowClass = (Class)objc_getClass("NSWindow");
	NSRect frame;
	frame.origin.x = 0;
	frame.origin.y = 0;
	frame.size.width = p_width;
	frame.size.height = p_height;

	p_mac->nswindow = ((id(*)(Class, SEL))objc_msgSend)(NSWindowClass, sel_registerName("alloc"));
	p_mac->nswindow = ((id(*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(
		p_mac->nswindow,
		sel_registerName("initWithContentRect:styleMask:backing:defer:"),
		frame,
		1, 0, 1
	);

	CFStringRef nsTitle;
	nsTitle = CFStringCreateWithCString(NULL, p_title, kCFStringEncodingUTF8);
	((void(*)(id, SEL, id))objc_msgSend)(p_mac->nswindow, sel_registerName("setTitle:"), nsTitle);
	CFRelease(nsTitle);

	((void(*)(id, SEL))objc_msgSend)(p_mac->nswindow, sel_registerName("makeKeyAndOrderFront:"), p_mac->nswindow);

	p_window->handle = p_mac;
	p_window->width = p_width;
	p_window->height = p_height;
	p_window->shouldClose = 0;

	return 1;
}

static void WINDOW_WindowPoll(WINDOW_Window *p_window)
{
	WINDOW_MacWindow *p_mac;
	p_mac = (WINDOW_MacWindow*)p_window->handle;

	Class NSAppClass = (Class)objc_getClass("NSApplication");
	id sharedApp = ((id(*)(Class, SEL))objc_msgSend)(NSAppClass, sel_registerName("sharedApplication"));
	((void(*)(id, SEL))objc_msgSend)(sharedApp, sel_registerName("updateWindows"));
}

static void WINDOW_WindowDestroy(WINDOW_Window *p_window)
{
	WINDOW_MacWindow *p_mac;
	p_mac = (WINDOW_MacWindow*)p_window->handle;
	if (!p_mac) return;

	((void(*)(id, SEL))objc_msgSend)(p_mac->nswindow, sel_registerName("close"));
	free(p_mac);
	p_window->handle = NULL;
}

static int WINDOW_WindowShouldClose(const WINDOW_Window *p_window)
{
	return p_window->shouldClose;
}

#endif /* WINDOW_MACOS */

#endif /* WINDOW_H */

