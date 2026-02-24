#include <windows.h>
#include <stdint.h>

// Ensure we are compiling as 64-bit
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");


// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static


// Typedefs for common types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


// Struct to hold buffer info
struct Win32OffScreenBuffer {
    BITMAPINFO info;
    void* memory;
    i32 width;
    i32 height;
    u32 bytesPerPixel;
    u32 pitch;
};

struct WindowDimension {
    i32 width;
    i32 height;
};


GLOBAL bool gRunning{ false };
GLOBAL Win32OffScreenBuffer gBuff{};


INTERNAL WindowDimension
Win32GetWindowDimensions(HWND wnd) {
    RECT clientRect;
    GetClientRect(wnd, &clientRect);
    const i32 w{ clientRect.right - clientRect.left };
    const i32 h{ clientRect.bottom - clientRect.top };

    return WindowDimension{ w, h };
}

INTERNAL void
DrawGradient(const Win32OffScreenBuffer* buff, u32 xOffset, u32 yOffset) {
    // TODO: see what the optimizer does to buff (passing by value vs pointer)

    // Pitch (length width-wise)
    u8* row{ static_cast<u8 *>(buff->memory) };

    // Drawing
    for (i32 y{ 0 }; y < buff->height; ++y) {
        u32* pixel{ reinterpret_cast<u32 *>(row) };
        for (i32 x{ 0 }; x < buff->width; ++x) {
            // Windows flipped the order of rbg
            // Memory: BB GG RR xx
            // !little endianness!

            u8 blue{ static_cast<u8>(x + xOffset) };
            u8 green{ static_cast<u8>(y + yOffset) };

            // Register: xx RR GG BB
            *pixel++ = ((green << 8) | blue);
        }

        row += buff->pitch;
    }
}

INTERNAL void
Win32ResizeDIBSection(Win32OffScreenBuffer* buff, i32 w, i32 h) {
    if (buff->memory) {
        VirtualFree(buff->memory, 0, MEM_RELEASE);
    }

    buff->width = w;
    buff->height = h;
    buff->bytesPerPixel = 4;

    buff->info.bmiHeader.biSize = sizeof(buff->info.bmiHeader);
    buff->info.bmiHeader.biWidth = buff->width;
    buff->info.bmiHeader.biHeight = -buff->height; // top-down by assigning negative
    buff->info.bmiHeader.biPlanes = 1;
    buff->info.bmiHeader.biBitCount = 32; // 8 padding
    buff->info.bmiHeader.biCompression = BI_RGB;

    const u32 bmMemorySize{ buff->width * buff->height * buff->bytesPerPixel };
    // Allocate the memory for use immediately (MEM_COMMIT)
    buff->memory = VirtualAlloc(0, bmMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buff->pitch = buff->width * buff->bytesPerPixel;

    // TODO: clear to black probably
}

// TODO: cleanup functions and other things
INTERNAL void
Win32DisplayBufferWindow(
    const HDC deviceContext,
    const Win32OffScreenBuffer* buff,
    i32 wndWidth,
    i32 wndHeight
) {
    // TODO: aspect ratio correction
    StretchDIBits(
        deviceContext,
        0, 0, wndWidth, wndHeight, // dest
        0, 0, buff->width, buff->height, // src
        buff->memory,
        &buff->info,
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
            gRunning = false;
        } break;
        case WM_DESTROY: {
            // NOTE: This might happen as an error?
            OutputDebugStringA("WM_DESTROY\n");
            gRunning = false;
        } break;
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_MOVE: {
            OutputDebugStringA("WM_MOVE\n");
        } break;
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        // Key presses
        case WM_SYSKEYDOWN: {

        } break;
        case WM_SYSKEYUP: {

        } break;
        case WM_KEYDOWN: {
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
            u32 vkCode{ static_cast<u32>(wParam) };
            bool wasDown{ (lParam & (1 << 30)) != 0 };
            bool isDown{ (lParam & (1 << 31)) == 0 };

            // If continuously pressing
            if (wasDown == isDown) {
                break;
            }

            if (vkCode == 'W') {
                OutputDebugStringA("W\n");
            }
            else if (vkCode == 'S') {
                OutputDebugStringA("S\n");
            }
            else if (vkCode == 'A') {
                OutputDebugStringA("A\n");
            }
            else if (vkCode == 'D') {
                OutputDebugStringA("D\n");
            }
            else if (vkCode == 'Q') {
                OutputDebugStringA("Q\n");
            }
            else if (vkCode == 'E') {
                OutputDebugStringA("E\n");
            }
            else if (vkCode == VK_UP) {
                OutputDebugStringA("VK_UP\n");
            }
            else if (vkCode == VK_DOWN) {
                OutputDebugStringA("VK_DOWN\n");
            }
            else if (vkCode == VK_LEFT) {
                OutputDebugStringA("VK_LEFT\n");
            }
            else if (vkCode == VK_RIGHT) {
                OutputDebugStringA("VK_RIGHT\n");
            }
            else if (vkCode == VK_ESCAPE) {
                OutputDebugStringA("VK_ESCAPE ");
                if (isDown) {
                    OutputDebugStringA("IS DOWN");
                }
                if (wasDown) {
                    OutputDebugStringA("WAS DOWN");
                }
                OutputDebugStringA("\n");
            }
            else if (vkCode == VK_SPACE) {
                OutputDebugStringA("VK_SPACE\n");
            }
        } break;
        case WM_KEYUP: {

        } break;

        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");

            PAINTSTRUCT paint;
            const HDC deviceContext{ BeginPaint(wnd, &paint) };

            auto wndDimension{ Win32GetWindowDimensions(wnd) };
            Win32DisplayBufferWindow(deviceContext, &gBuff, wndDimension.width, wndDimension.height);

            EndPaint(wnd, &paint);
        } break;
        default: {
            //OutputDebugStringA("DEFAULT\n");
            result = DefWindowProcA(wnd, msg, wParam, lParam);
        } break;
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

    constexpr i32 startingWidth{ 1280 };
    constexpr i32 startingHeight{ 720 };
    Win32ResizeDIBSection(&gBuff, startingWidth, startingHeight);

    WNDCLASSA windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    // GetModuleHandle(0) returns hInstance
    windowClass.hInstance = hInstance;
    //HICON     hIcon;

    const char* name{ "Handmade Hero" };
    windowClass.lpszClassName = name;

    if (RegisterClass(&windowClass)) {
        const HWND windowHandle = CreateWindowExA(
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
            gRunning = true;

            // Animating
            u32 xOffset{};
            u32 yOffset{};

            while (gRunning) {
                // Windows message queue, everything has to go through the OS
                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        gRunning = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
                // If no messages or all are finished


                const HDC deviceContext{ GetDC(windowHandle) };
                auto wndDimension{ Win32GetWindowDimensions(windowHandle) };
                Win32DisplayBufferWindow(deviceContext, &gBuff, wndDimension.width, wndDimension.height);
                ReleaseDC(windowHandle, deviceContext);

                DrawGradient(&gBuff, xOffset, yOffset);
                // Overflows to zero
                ++xOffset;
                ++yOffset;
            }
        }
        else {
            // NOTE: Log?
            OutputDebugStringA("Failed to create windowHandle!\n");
        }
    }
    else {
        // NOTE: Log?
        OutputDebugStringA("Failed to register windowClass!\n");
    }

    return 0;
}
