#include <windows.h>
#include <stdio.h>

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI WinMain(
    HINSTANCE hInstance, // A handle to the current instance of the application
    HINSTANCE hPrevInstance, // Not in use anymore
    PSTR lpCmdLine, // Command line arguments
    int nCmdShow // Window visibility option
) {
    printf("WinMain\n"); // Doesn't print?
    OutputDebugStringA("This will be printed\n");
    return 0;
}

int main() {
    printf("main!\n");
    return 0;
}
