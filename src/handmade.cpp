#include <windows.h>

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI WinMain(
    HINSTANCE hInstance, // A handle to the current instance of the application
    HINSTANCE hPrevInstance, // Not in use anymore
    PSTR lpCmdLine, // Command line arguments
    int nCmdShow // Window visibility option
) {
    https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox#requirements
    MessageBoxA(0, "It works!", "Handmade Hero", MB_OK);
    OutputDebugStringA("This will be printed\n");
    return 0;
}
