#include <windows.h>

// Ensure we are compiling as 64-bit
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

// NOTE: Global for now
GLOBAL bool running{ false };

// Bitmap
GLOBAL BITMAPINFO bmInfo{};
GLOBAL void* bmMemory;
GLOBAL HBITMAP bmHandle;
GLOBAL HDC bmDeviceContext;

INTERNAL void
Win32ResizeDIBSection(int w, int h) {
    // Windows API is a bit of a hassle...

    // NOTE: think about the order of freeing and creating?
    if (bmHandle) {
        DeleteObject(bmHandle);
    }
    if (!bmDeviceContext) {
        bmDeviceContext = CreateCompatibleDC(0);
    }

    bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
    bmInfo.bmiHeader.biWidth = w;
    bmInfo.bmiHeader.biHeight = h;
    bmInfo.bmiHeader.biPlanes = 1;
    bmInfo.bmiHeader.biBitCount = 32;
    bmInfo.bmiHeader.biCompression = BI_RGB;

    // Create the device-independent bitmap
    bmHandle = CreateDIBSection(
        bmDeviceContext,
        &bmInfo,
        DIB_RGB_COLORS,
        &bmMemory,
        0,
        0
    );
}

INTERNAL void
Win32UpdateWindow(HDC deviceContext, int left, int top, int w, int h) {
    StretchDIBits(
        deviceContext,
        left, top, w, h, // dest
        left, top, w, h, // src
        bmMemory,
        &bmInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// Callback for messages
LRESULT CALLBACK
Win32MainWindowCallback(
    HWND    wnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
) {
    LRESULT result{ 0 };

    switch (msg) {
        case WM_CLOSE: {
            // TODO: show a message for closing
            OutputDebugStringA("WM_CLOSE\n");
            // Post a message to quit to the queue
            //PostQuitMessage(0);
            running = false;
            break;
        }
        case WM_DESTROY: {
            // NOTE: This might happen as an error?
            OutputDebugStringA("WM_DESTROY\n");
            running = false;
            break;
        }
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;
        }
        case WM_MOVE: {
            OutputDebugStringA("WM_MOVE\n");
            break;
        }
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
            RECT clientRect;
            GetClientRect(wnd, &clientRect);
            const int w{ clientRect.right - clientRect.left };
            const int h{ clientRect.bottom - clientRect.top };
            Win32ResizeDIBSection(w, h);
            break;
        }
        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");

            PAINTSTRUCT paint;
            const HDC deviceContext{ BeginPaint(wnd, &paint) };
            const int left{ paint.rcPaint.left };
            const int top{ paint.rcPaint.top };
            const int w{ paint.rcPaint.right - paint.rcPaint.left };
            const int h{ paint.rcPaint.bottom - paint.rcPaint.top };
            Win32UpdateWindow(deviceContext, left, top, w, h);
            EndPaint(wnd, &paint);

            break;
        }
        default: {
            //OutputDebugStringA("DEFAULT\n");
            result = DefWindowProc(wnd, msg, wParam, lParam);
            break;
        }
    }

    return result;
}

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI
WinMain(
    HINSTANCE hInstance, // A handle to the current instance of the application
    HINSTANCE hPrevInstance, // Not in use anymore
    PSTR lpCmdLine, // Command line arguments
    int nCmdShow // Window visibility option
) {
    // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages

    WNDCLASS windowClass{};
    // Check if redraws are needed anymore
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    // GetModuleHandle(0) returns hInstance
    windowClass.hInstance = hInstance;
    //HICON     hIcon;

    const char* name{ "Handmade Hero" };
    windowClass.lpszClassName = name;

    if (RegisterClass(&windowClass)) {
        HWND windowHandle = CreateWindowEx(
            0,
            name,
            name,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            // Window size and position
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );

        if (windowHandle) {
            running = true;
            // Windows message queue, everything has to go through the OS
            while (running) {
                MSG message;
                if (GetMessageA(&message, 0, 0, 0) > 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
                else {
                    break;
                }
            }
        }
        else {
            OutputDebugStringA("Failed to create windowHandle!\n");
        }
    }
    else {
        // NOTE: Log?
        OutputDebugStringA("Failed to register windowClass!\n");
    }

    return 0;
}
