#include <windows.h>
#include <stdint.h>

// Ensure we are compiling as 64-bit
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

#define SCAST static_cast

// Typedefs for common types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// NOTE: Global for now
GLOBAL bool running{ false };

// Bitmap
GLOBAL BITMAPINFO bmInfo{};
GLOBAL void* bmMemory;
GLOBAL u32 bmWidth;
GLOBAL u32 bmHeight;
GLOBAL u32 bytesPerPixel{ 4 };

INTERNAL void
DrawGradient(u32 xOffset, u32 yOffset) {
    // Pitch (length width-wise)
    const u32 pitch{ bmWidth * bytesPerPixel };
    u8* row{ SCAST<u8 *>(bmMemory) };

    // Drawing
    for (u32 y{ 0 }; y < bmHeight; ++y) {
        // pixel is the start of the current row
        u8* pixel{ SCAST<u8 *>(row) };
        for (u32 x{ 0 }; x < bmWidth; ++x) {
            // Windows flipped the order of rbg
            // BB GG RR xx

            *pixel = SCAST<u8>(x) + xOffset;
            ++pixel;

            *pixel = SCAST<u8>(y) + yOffset;
            ++pixel;

            *pixel = 0;
            ++pixel;

            // Padding
            *pixel = 0;
            ++pixel;
        }

        row += pitch;
    }
}

INTERNAL void
Win32ResizeDIBSection(u32 w, u32 h) {
    if (bmMemory) {
        VirtualFree(bmMemory, 0, MEM_RELEASE);
    }

    bmWidth = w;
    bmHeight = h;

    bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
    bmInfo.bmiHeader.biWidth = bmWidth;
    bmInfo.bmiHeader.biHeight = -bmHeight; // top-down by assigning negative
    bmInfo.bmiHeader.biPlanes = 1;
    bmInfo.bmiHeader.biBitCount = 32; // 8 padding
    bmInfo.bmiHeader.biCompression = BI_RGB;

    const u32 bmMemorySize{ bmWidth * bmHeight * bytesPerPixel };
    // Allocate the memory for use immediately (MEM_COMMIT)
    bmMemory = VirtualAlloc(0, bmMemorySize, MEM_COMMIT, PAGE_READWRITE);

    DrawGradient(128, 0);
}

INTERNAL void
Win32UpdateWindow(HDC deviceContext, RECT* windowRect, u32 left, u32 top, u32 w, u32 h) {
    const i32 wndW{ windowRect->right - windowRect->left };
    const i32 wndH{ windowRect->bottom - windowRect->top };
    StretchDIBits(
        deviceContext,
        0, 0, bmWidth, bmHeight, // dest
        0, 0, wndW, wndH, // src
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
            const i32 w{ clientRect.right - clientRect.left };
            const i32 h{ clientRect.bottom - clientRect.top };
            Win32ResizeDIBSection(w, h);
            break;
        }
        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");

            PAINTSTRUCT paint;
            const HDC deviceContext{ BeginPaint(wnd, &paint) };
            const i32 left{ paint.rcPaint.left };
            const i32 top{ paint.rcPaint.top };
            const i32 w{ paint.rcPaint.right - paint.rcPaint.left };
            const i32 h{ paint.rcPaint.bottom - paint.rcPaint.top };

            RECT clientRect;
            GetClientRect(wnd, &clientRect);

            Win32UpdateWindow(deviceContext, &clientRect, left, top, w, h);
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
        const HWND windowHandle = CreateWindowEx(
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
