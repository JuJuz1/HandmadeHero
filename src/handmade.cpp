#include <windows.h>
#include <dsound.h>

#include <stdint.h>
#include <cassert>
#include <math.h>

#include <cstdio>


// Ensure we are compiling as 64-bit
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");


// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

#define PI32 3.14159265359f

// Typedefs for common types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;


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
GLOBAL Win32OffScreenBuffer gScreenBuff;

GLOBAL LPDIRECTSOUNDBUFFER gSecondaryBuff;


INTERNAL WindowDimension
Win32GetWindowDimensions(HWND windowHandle) {
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    const i32 w{ clientRect.right - clientRect.left };
    const i32 h{ clientRect.bottom - clientRect.top };

    return WindowDimension{ w, h };
}

INTERNAL void
DrawGradient(const Win32OffScreenBuffer* buff, u32 xOffset, u32 yOffset) {
    // NOTE: maybe see what the optimizer does to buff (passing by value vs pointer)
    // remember to not get fixated on micro-optimizations before actually doing optimization though...

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
    buff->memory = VirtualAlloc(0, bmMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buff->pitch = buff->width * buff->bytesPerPixel;

    // TODO: clear to black probably
}

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

        // SYSKEYDOWN is called whenever the key press includes alt
        // all other keypresses go to the non-sys versions below
        // This forces us to handle alt + f4 here...
        case WM_SYSKEYDOWN: {
            u32 vkCode{ static_cast<u32>(wParam) };
            if (vkCode == VK_F4) {
                OutputDebugStringA("VK_F4 SYSKEYDOWN\n");
                // Should always be true here
                bool altPressed{ (lParam & (1 << 29)) != 0 };
                if (altPressed) {
                    gRunning = false;
                } else {
                    assert(false && "SYSKEYDOWN did not have the alt key pressed");
                }
            }
        } break;
        case WM_SYSKEYUP: {
        } break;
        case WM_KEYDOWN: {
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
            u32 vkCode{ static_cast<u32>(wParam) };
            // lParam contains additional information about keystrokes
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
            bool wasDown{ (lParam & (1 << 30)) != 0 };
            bool isDown{ (lParam & (1 << 31)) == 0 };

            // If continuously pressing
            // TODO: should change to handle that case also instead of breaking?
            if (wasDown == isDown) {
                break;
            }

            if (vkCode == 'W') {
                OutputDebugStringA("W\n");
            } else if (vkCode == 'S') {
                OutputDebugStringA("S\n");
            } else if (vkCode == 'A') {
                OutputDebugStringA("A\n");
            } else if (vkCode == 'D') {
                OutputDebugStringA("D\n");
            } else if (vkCode == 'Q') {
                OutputDebugStringA("Q\n");
            } else if (vkCode == 'E') {
                OutputDebugStringA("E\n");
            } else if (vkCode == VK_UP) {
                OutputDebugStringA("VK_UP\n");
            } else if (vkCode == VK_DOWN) {
                OutputDebugStringA("VK_DOWN\n");
            } else if (vkCode == VK_LEFT) {
                OutputDebugStringA("VK_LEFT\n");
            } else if (vkCode == VK_RIGHT) {
                OutputDebugStringA("VK_RIGHT\n");
            } else if (vkCode == VK_ESCAPE) {
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
            Win32DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width, wndDimension.height);

            EndPaint(wnd, &paint);
        } break;
        default: {
            //OutputDebugStringA("DEFAULT\n");
            result = DefWindowProcA(wnd, msg, wParam, lParam);
        } break;
    }

    return result;
}

// Make use of runtime dynamic library linking
// Make a function pointer type for DirectSoundCreate (a function inside the dll)
// Feels like magic but really it's not when you understand it
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// TODO: consider cleaning up, starting to become a mess
INTERNAL void
Win32InitDSound(HWND windowHandle, u32 samplesPerSecond, u32 buffSize) {
    HMODULE dSoundLib{ LoadLibraryA("dsound.dll") };
    if (dSoundLib) {
        direct_sound_create* dSoundCreate = reinterpret_cast<direct_sound_create *>
            (GetProcAddress(dSoundLib, "DirectSoundCreate"));
        LPDIRECTSOUND dSound;

        // For "both" buffers
        WAVEFORMATEX waveFormat{};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 2;
        waveFormat.nSamplesPerSec = samplesPerSecond;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;;
        waveFormat.cbSize = 0;

        if (dSoundCreate && SUCCEEDED(dSoundCreate(0, &dSound, 0))) {
            if (SUCCEEDED(dSound->SetCooperativeLevel(windowHandle, DSSCL_PRIORITY))) {
                DSBUFFERDESC primaryBuffDescription{};
                primaryBuffDescription.dwSize = sizeof(primaryBuffDescription);
                primaryBuffDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuff;
                if (SUCCEEDED(dSound->CreateSoundBuffer(&primaryBuffDescription, &primaryBuff, 0))) {
                    if (SUCCEEDED(primaryBuff->SetFormat(&waveFormat))) {
                        OutputDebugStringA("Primary buffer format was set\n");
                    } else {
                        // log, setting format failed
                    }
                } else {
                    // log, creating primary sound buffer failed
                }
            } else {
                // log, couldn't set cooperative
            }

            // Secondary buffer, this is where we write to!
            DSBUFFERDESC secondaryBuffDescription{};
            secondaryBuffDescription.dwSize = sizeof(secondaryBuffDescription);
            secondaryBuffDescription.dwFlags = 0;
            secondaryBuffDescription.dwBufferBytes = buffSize;
            secondaryBuffDescription.lpwfxFormat = &waveFormat;

            if (SUCCEEDED(dSound->CreateSoundBuffer(&secondaryBuffDescription, &gSecondaryBuff, 0))) {
                OutputDebugStringA("Secondary buffer format created\n");
            } else {
                // log, creating secondary sound buffer failed
            }
        } else {
            // log, couldn't create dsound
        }
    } else {
        // TODO: log, couldn't load dll
    }
}

// Secondary buffer values
struct Win32SoundOutput {
    u32 samplesPerSecond{ 48000 };
    u32 toneHz{ 256 }; // Pitch
    i32 toneVolume{ 6000 }; // Amplitude
    u32 wavePeriod{ samplesPerSecond / toneHz };
    u32 bytesPerSample{ sizeof(u16) * 2 };
    u32 buffSize{ samplesPerSecond * bytesPerSample };
    u32 runningSampleIndex;
};

INTERNAL void
Win32FillSoundBuffer(Win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite) {
    LPVOID region1;
    DWORD region1Size;
    LPVOID region2;
    DWORD region2Size;
    if (SUCCEEDED(gSecondaryBuff->Lock(
        byteToLock, bytesToWrite,
        &region1, &region1Size,
        &region2, &region2Size,
        0
    ))) {
        // TODO: assert regionSizes
        //assert(false && "regionSizes are invalid!");

        const DWORD region1SampleCount{ region1Size / soundOutput->bytesPerSample };
        i16* sampleOut{ static_cast<i16 *>(region1) };

        // TODO: collapse loops to one
        for (DWORD i{ 0 }; i < region1SampleCount; ++i) {
            // Sine wave
            const f32 t{ static_cast<f32>(soundOutput->runningSampleIndex) / static_cast<f32>(soundOutput->wavePeriod)
                * 2 * PI32 };
            const f32 sineValue{ sinf(t) };
            const i16 sampleValue{ static_cast<i16>(sineValue * soundOutput->toneVolume) };

            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            ++soundOutput->runningSampleIndex;
        }

        const DWORD region2SampleCount{ region2Size / soundOutput->bytesPerSample };
        sampleOut = static_cast<i16 *>(region2);

        for (DWORD i{ 0 }; i < region2SampleCount; ++i) {
            const f32 t{ static_cast<f32>(soundOutput->runningSampleIndex) / static_cast<f32>(soundOutput->wavePeriod)
                * 2 * PI32 };
            const f32 sineValue{ sinf(t) };
            const i16 sampleValue{ static_cast<i16>(sineValue * soundOutput->toneVolume) };

            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            ++soundOutput->runningSampleIndex;
        }

        gSecondaryBuff->Unlock(region1, region1Size, region2, region2Size);
    } else {
        // log, couldnt lock
    }
}

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI
WinMain(
    // commenting out removed C4100 warnings for unreferenced parameters
    HINSTANCE hInstance, // A handle to the current instance of the application
    HINSTANCE, //hPrevInstance, Not in use anymore
    PSTR, //lpCmdLine, Command line arguments
    int //nCmdShow Window visibility option
) {
    // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages

    constexpr i32 startingWidth{ 1280 };
    constexpr i32 startingHeight{ 720 };
    Win32ResizeDIBSection(&gScreenBuff, startingWidth, startingHeight);

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
            0, name, name,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            // Window size and position
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0, hInstance, 0
        );

        if (windowHandle) {
            Win32SoundOutput soundOutput{};
            Win32InitDSound(windowHandle, soundOutput.samplesPerSecond, soundOutput.buffSize);
            Win32FillSoundBuffer(&soundOutput, 0, soundOutput.buffSize);
            bool isSoundPlaying{ false };

            // Animating gradient
            u32 xOffsetGradient{};
            u32 yOffsetGradient{};

            // The main loop!
            gRunning = true;
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

                DrawGradient(&gScreenBuff, xOffsetGradient, yOffsetGradient);
                // Overflows to zero
                ++xOffsetGradient;
                ++yOffsetGradient;


                // NOTE: DirectSound output test
                // Circular buffer so we might get two regions to write to

                // A single sample is one LEFT and RIGHT together
                //  i16  i16 ...
                // [LEFT RIGHT] LEFT RIGHT ...

                DWORD playCursor;
                DWORD writeCursor;

                if (SUCCEEDED(gSecondaryBuff->GetCurrentPosition(&playCursor, &writeCursor))) {
                    const DWORD byteToLock{ (soundOutput.runningSampleIndex * soundOutput.bytesPerSample)
                        % soundOutput.buffSize };
                    DWORD bytesToWrite;

                    // NOTE: a way to log variables
                    //OutputDebugStringA("-----\n");
                    //char buf[256];
                    //sprintf_s(buf, "playCursor=%u, writeCursor=%u, byteToLock=%u, runningIndex=%u\n",
                    //    playCursor, writeCursor, byteToLock, soundOutput.runningSampleIndex);
                    //OutputDebugStringA(buf);

                    // TODO: change to a lower latency offset
                    // To the end and wrap behind playCursor
                    if (byteToLock > playCursor) {
                        bytesToWrite = soundOutput.buffSize - byteToLock;
                        bytesToWrite += playCursor;
                    } else {
                        bytesToWrite = playCursor - byteToLock;
                    }

                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
                } else {
                    // log, couldnt get curr pos
                }

                if (!isSoundPlaying) {
                    isSoundPlaying = true;
                    gSecondaryBuff->Play(0, 0, DSBPLAY_LOOPING);
                }

                const HDC deviceContext{ GetDC(windowHandle) };
                auto wndDimension{ Win32GetWindowDimensions(windowHandle) };
                Win32DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width, wndDimension.height);
                ReleaseDC(windowHandle, deviceContext);
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
