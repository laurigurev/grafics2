#include "grafics2.h"

Window* WINDOW;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_DESTROY:
			// DestroyWindow(hwnd);
			// PostQuitMessage(0);
			WINDOW->should_close = true;
			logt("WM_DESTROY\n");
			break;
		case WM_PAINT:
			ValidateRect(hwnd, NULL);
			break;
		default:
			break;
	}
	
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void windowc(Window* win, const char* name, const uint32_t width, const uint32_t height)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = win->hinstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = name;
	wcex.hIconSm = NULL;
	
	RegisterClassEx(&wcex);
	
	win->hwnd = CreateWindow(
		name, 
		name, 
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
		0,
		0,
		width,
		height,
		NULL,
		NULL,
		win->hinstance,
		NULL
	);
	
	if (!win->hwnd)
	{
		loge("failed to create window.hwnd\n");
		return;
	}
	
	win->should_close = false;
	WINDOW = win;
	
	ShowWindow(win->hwnd, SW_SHOW);
	SetForegroundWindow(win->hwnd);
	SetFocus(win->hwnd);
}

void windowd(Window* win)
{
	DestroyWindow(win->hwnd);
	PostQuitMessage(0);
	WINDOW = NULL;
}

void wpoll_events(Window* win)
{
	MSG msg;
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void wframebuffer_size(Window* win, uint32_t* width, uint32_t* height)
{
	RECT rect;
	GetClientRect(win->hwnd, &rect);
	*width = rect.right;
	*height = rect.bottom;
}
