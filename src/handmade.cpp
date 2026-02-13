#include <windows.h>

// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

LRESULT CALLBACK MainWindowCallback(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
) {
    LRESULT result{ 0 };

    switch (Msg) {
        case WM_CLOSE: {
            OutputDebugStringA("WM_CLOSE\n");
            exit(0);
            break;
        }
        case WM_DESTROY: {
            OutputDebugStringA("WM_DESTROY\n");
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
            break;
        }
        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");

            PAINTSTRUCT paint;
            const HDC deviceContext{ BeginPaint(hWnd, &paint) };
            const int left{ paint.rcPaint.left };
            const int top{ paint.rcPaint.top };
            const int w{ paint.rcPaint.right - paint.rcPaint.left };
            const int h{ paint.rcPaint.bottom - paint.rcPaint.top };

            static DWORD operation{ WHITENESS };
            PatBlt(deviceContext, left, top, w, h, operation);
            operation = (operation == WHITENESS) ? BLACKNESS : WHITENESS;
            EndPaint(hWnd, &paint);

            break;
        }
        default: {
            //OutputDebugStringA("DEFAULT\n");
            result = DefWindowProc(hWnd, Msg, wParam, lParam);
            break;
        }
    }

    return result;
}

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI WinMain(
    HINSTANCE hInstance, // A handle to the current instance of the application
    HINSTANCE hPrevInstance, // Not in use anymore
    PSTR lpCmdLine, // Command line arguments
    int nCmdShow // Window visibility option
) {
    // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages

    WNDCLASS windowClass{};
    // Check if redraws are needed anymore
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    // GetModuleHandle(0) returns hInstance
    windowClass.hInstance = hInstance;
    //HICON     hIcon;

    constexpr char* name{ "Handmade Hero "};
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
            // Windows message queue
            MSG message;
            while (GetMessage(&message, 0, 0, 0)) {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
        else {
            OutputDebugStringA("Failed to create windowHandle!\n");
        }
    }

    return 0;
}
