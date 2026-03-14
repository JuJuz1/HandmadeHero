/*
Episode 11: Important topics about making code cross-platform
This is not a final platform layer!
*/

#include "handmade.h"

#include <dsound.h>
#include <windows.h>

#include <cstdio>

#include "win32_handmade.h"

GLOBAL bool32 gIsGameRunning;
GLOBAL win32::OffScreenBuffer gScreenBuff;
GLOBAL LPDIRECTSOUNDBUFFER gSecondaryBuff;
GLOBAL i64 gPerfCounterFreq;

namespace platform {

// TODO: make these more generic (and allow variadic arguments?)
// and much safer...
INTERNAL
DEBUG_PRINT_INT(DEBUGPrintInt) {
    char buf[32];
    sprintf_s(buf, "%s: %d\n", valueName, value);
    OutputDebugStringA(buf);
}

INTERNAL
DEBUG_PRINT_FLOAT(DEBUGPrintFloat) {
    char buf[32];
    sprintf_s(buf, "%s: %f\n", valueName, value);
    OutputDebugStringA(buf);
}

INTERNAL
DEBUG_FREE_FILE_MEMORY(DEBUGFreeFileMemory) {
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

INTERNAL
DEBUG_READ_FILE(DEBUGReadFile) {
    DEBUGFileReadResult result{};
    // What an atrocious name for a function which requests to read a file...
    HANDLE fileHandle{ CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0,
                                   0) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize)) {
            result.content =
                VirtualAlloc(0, fileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result.content) {
                const u32 bytesToRead{ SafeTruncateU64toU32(fileSize.QuadPart) };
                DWORD bytesRead;
                // Consider the case where one could truncate the file after reading
                if (ReadFile(fileHandle, result.content, bytesToRead, &bytesRead, 0) &&
                    bytesToRead == bytesRead) {
                    // Success
                    result.contentSize = bytesToRead;
                } else {
                    DEBUGFreeFileMemory(result.content);
                    result.content = 0;
                }
            } else {
                // TODO: log
            }
        } else {
            // TODO: log
        }

        CloseHandle(fileHandle);
    } else {
        // TODO: log
    }

    return result;
}

INTERNAL
DEBUG_WRITE_FILE(DEBUGWriteFile) {
    bool32 result{};
    HANDLE fileHandle{ CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, fileSize, &bytesWritten, 0)) {
            // Success
            result = bytesWritten == fileSize;
        } else {
            // TODO: log
        }

        CloseHandle(fileHandle);
    } else {
        // TODO: log
    }

    return result;
}

} //namespace platform

namespace win32 {

INTERNAL WindowDimension
GetWindowDimensions(HWND windowHandle) {
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    const i32 w{ clientRect.right - clientRect.left };
    const i32 h{ clientRect.bottom - clientRect.top };

    return WindowDimension{ w, h };
}

INTERNAL void
ResizeDIBSection(OffScreenBuffer* buff, i32 w, i32 h) {
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
DisplayBufferWindow(HDC deviceContext, const OffScreenBuffer* buff, i32 wndWidth, i32 wndHeight) {
    // TODO: aspect ratio correction
    StretchDIBits(deviceContext, 0, 0, wndWidth, wndHeight, // dest
                  0, 0, buff->width, buff->height,          // src
                  buff->memory, &buff->info, DIB_RGB_COLORS, SRCCOPY);
}

// Make use of runtime dynamic library linking
// Make a function pointer type for DirectSoundCreate (a function inside the dll)
// Feels like magic but really it's not when you understand it
// clang-format off
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
// clang-format on

//TODO: consider cleaning up, starting to become a mess
INTERNAL void
InitDSound(HWND windowHandle, u32 samplesPerSecond, u32 buffSize) {
    HMODULE dSoundLib{ LoadLibraryA("dsound.dll") };
    if (dSoundLib) {
        direct_sound_create* dSoundCreate{ reinterpret_cast<direct_sound_create*>(
            GetProcAddress(dSoundLib, "DirectSoundCreate")) };
        LPDIRECTSOUND dSound;

        // For "both" buffers
        WAVEFORMATEX waveFormat;
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 2;
        waveFormat.nSamplesPerSec = samplesPerSecond;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;

        if (dSoundCreate && SUCCEEDED(dSoundCreate(0, &dSound, 0))) {
            if (SUCCEEDED(dSound->SetCooperativeLevel(windowHandle, DSSCL_PRIORITY))) {
                DSBUFFERDESC primaryBuffDescription{};
                primaryBuffDescription.dwSize = sizeof(primaryBuffDescription);
                primaryBuffDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuff;
                if (SUCCEEDED(
                        dSound->CreateSoundBuffer(&primaryBuffDescription, &primaryBuff, 0))) {
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

            if (SUCCEEDED(
                    dSound->CreateSoundBuffer(&secondaryBuffDescription, &gSecondaryBuff, 0))) {
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

INTERNAL void
ClearSoundBuffer(SoundOutput* soundOutput) {
    LPVOID region1;
    DWORD region1Size;
    LPVOID region2;
    DWORD region2Size;
    if (SUCCEEDED(gSecondaryBuff->Lock(0, soundOutput->buffSize, &region1, &region1Size, &region2,
                                       &region2Size, 0))) {
        u8* destSample{ static_cast<u8*>(region1) };

        for (DWORD i{ 0 }; i < region1Size; ++i) {
            *destSample++ = 0;
        }

        destSample = static_cast<u8*>(region2);

        for (DWORD i{ 0 }; i < region2Size; ++i) {
            *destSample++ = 0;
        }

        gSecondaryBuff->Unlock(region1, region1Size, region2, region2Size);
    } else {
        // log, couldnt lock while clearing
    }
}

INTERNAL void
FillSoundBuffer(SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite,
                const game::SoundOutputBuffer* sourceBuff) {
    LPVOID region1;
    DWORD region1Size;
    LPVOID region2;
    DWORD region2Size;
    if (SUCCEEDED(gSecondaryBuff->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2,
                                       &region2Size, 0))) {
        // TODO: assert regionSizes
        //ASSERT(!"regionSizes are invalid!");

        const DWORD region1SampleCount{ region1Size / soundOutput->bytesPerSample };
        i16* destSample{ static_cast<i16*>(region1) };
        const i16* srcSample{ sourceBuff->samples };

        // TODO: collapse loops to one
        for (DWORD i{ 0 }; i < region1SampleCount; ++i) {
            *destSample++ = *srcSample++;
            *destSample++ = *srcSample++;
            ++soundOutput->runningSampleIndex;
        }

        const DWORD region2SampleCount{ region2Size / soundOutput->bytesPerSample };
        destSample = static_cast<i16*>(region2);

        for (DWORD i{ 0 }; i < region2SampleCount; ++i) {
            *destSample++ = *srcSample++;
            *destSample++ = *srcSample++;
            ++soundOutput->runningSampleIndex;
        }

        gSecondaryBuff->Unlock(region1, region1Size, region2, region2Size);
    } else {
        // log, couldnt lock
    }
}

// Callback for messages
INTERNAL LRESULT CALLBACK
MainWindowCallback(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result{};

    switch (msg) {
    case WM_CLOSE: {
        // TODO: show a message for closing
        OutputDebugStringA("WM_CLOSE\n");
        gIsGameRunning = false;
    } break;
    case WM_DESTROY: {
        // NOTE: This might happen as an error, recreate window?
        OutputDebugStringA("WM_DESTROY\n");
        gIsGameRunning = false;
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

    case WM_SYSKEYDOWN: {
        ASSERT(!"Keyboard input came through a non-dispatch message!");
    } break;
    case WM_SYSKEYUP: {
        ASSERT(!"Keyboard input came through a non-dispatch message!");
    } break;
    case WM_KEYDOWN: {
        ASSERT(!"Keyboard input came through a non-dispatch message!");
    } break;
    case WM_KEYUP: {
        ASSERT(!"Keyboard input came through a non-dispatch message!");
    } break;

    case WM_PAINT: {
        OutputDebugStringA("WM_PAINT\n");

        PAINTSTRUCT paint;
        HDC deviceContext{ BeginPaint(wnd, &paint) };

        auto wndDimension{ GetWindowDimensions(wnd) };
        DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width, wndDimension.height);

        EndPaint(wnd, &paint);
    } break;
    default: {
        result = DefWindowProcA(wnd, msg, wParam, lParam);
    } break;
    }

    return result;
}

INTERNAL void
ProcessKeyboardMessage(game::Button* button, bool32 isDown) {
    // The fired message should never have the same state (isDown)
    // This also catches scenarios when we forget to add the keypress handling to both WM_KEYUP and
    // WM_KEYDOWN
    ASSERT(button->endedDown != isDown);
    button->endedDown = isDown;
    ++button->halfTransitionCount;
}

INTERNAL void
ProcessPendingMessages(game::Input* input) {
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
        // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
        // message.lParam contains additional information about keystrokes
        // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
        const u32 vkCode{ static_cast<u32>(message.wParam) };
        const bool32 isDown{ (message.lParam & (1 << 31)) == 0 };
        const bool32 wasDown{ (message.lParam & (1 << 30)) != 0 };

        switch (message.message) {
        case WM_QUIT: {
            gIsGameRunning = false;
        } break;

        // SYSKEYDOWN is called whenever the key press includes alt
        // all other keypresses go to the non-sys versions below
        // This forces us to handle alt + f4 here...
        case WM_SYSKEYDOWN: {
            if (vkCode == VK_F4) {
                OutputDebugStringA("VK_F4 SYSKEYDOWN\n");
                // Should always be true here
                const bool32 altPressed{ (message.lParam & (1 << 29)) != 0 };
                ASSERT(altPressed && "SYSKEYDOWN did not have the alt key pressed");
                if (altPressed) {
                    gIsGameRunning = false;
                }
            }
        } break;
        case WM_SYSKEYUP: {
        } break;
        case WM_KEYDOWN: {
            //char buf[32];
            //sprintf_s(buf, "isDown: %d, wasDown: %d\n", isDown, wasDown);
            //OutputDebugStringA(buf);

            // Ignore continuous messages!
            if (wasDown == isDown) {
                break;
            }

            if (vkCode == 'W') {
                OutputDebugStringA("W\n");
                // This approach won't work for justPressed, we have to use halfTransitionCount
                //input->playerInputs->up.pressed = true;
                ProcessKeyboardMessage(&input->playerInputs->up, isDown);
            } else if (vkCode == 'S') {
                OutputDebugStringA("S\n");
                ProcessKeyboardMessage(&input->playerInputs->down, isDown);
            } else if (vkCode == 'A') {
                OutputDebugStringA("A\n");
                ProcessKeyboardMessage(&input->playerInputs->left, isDown);
            } else if (vkCode == 'D') {
                OutputDebugStringA("D\n");
                ProcessKeyboardMessage(&input->playerInputs->right, isDown);
            } else if (vkCode == 'Q') {
                OutputDebugStringA("Q\n");
                ProcessKeyboardMessage(&input->playerInputs->Q, isDown);
            } else if (vkCode == 'E') {
                OutputDebugStringA("E\n");
                ProcessKeyboardMessage(&input->playerInputs->E, isDown);
            } else if (vkCode == VK_SHIFT) {
                OutputDebugStringA("VK_SHIFT\n");
                ProcessKeyboardMessage(&input->playerInputs->shift, isDown);
            } else if (vkCode == VK_UP) {
                OutputDebugStringA("VK_UP\n");
            } else if (vkCode == VK_DOWN) {
                OutputDebugStringA("VK_DOWN\n");
            } else if (vkCode == VK_LEFT) {
                OutputDebugStringA("VK_LEFT\n");
            } else if (vkCode == VK_RIGHT) {
                OutputDebugStringA("VK_RIGHT\n");
            } else if (vkCode == VK_ESCAPE) {
                OutputDebugStringA("VK_ESCAPE\n");
            } else if (vkCode == VK_SPACE) {
                OutputDebugStringA("VK_SPACE\n");
            }
        } break;
        case WM_KEYUP: {
            if (wasDown == isDown) {
                break;
            }

            if (vkCode == 'W') {
                OutputDebugStringA("W\n");
                ProcessKeyboardMessage(&input->playerInputs->up, isDown);
            } else if (vkCode == 'S') {
                OutputDebugStringA("S\n");
                ProcessKeyboardMessage(&input->playerInputs->down, isDown);
            } else if (vkCode == 'A') {
                OutputDebugStringA("A\n");
                ProcessKeyboardMessage(&input->playerInputs->left, isDown);
            } else if (vkCode == 'D') {
                OutputDebugStringA("D\n");
                ProcessKeyboardMessage(&input->playerInputs->right, isDown);
            } else if (vkCode == 'Q') {
                OutputDebugStringA("Q\n");
                ProcessKeyboardMessage(&input->playerInputs->Q, isDown);
            } else if (vkCode == 'E') {
                OutputDebugStringA("E\n");
                ProcessKeyboardMessage(&input->playerInputs->E, isDown);
            } else if (vkCode == VK_SHIFT) {
                OutputDebugStringA("VK_SHIFT\n");
                ProcessKeyboardMessage(&input->playerInputs->shift, isDown);
            }
        } break;
        default: {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } break;
        }
    }
}

INTERNAL LARGE_INTEGER
GetWallClock() {
    LARGE_INTEGER res;
    QueryPerformanceCounter(&res);
    return res;
}

INTERNAL f64
GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    return static_cast<f64>(end.QuadPart - start.QuadPart) / static_cast<f64>(gPerfCounterFreq);
}

INTERNAL inline FILETIME
GetLastWriteTime(const char* filename) {
    FILETIME lastWriteTime{};

    WIN32_FIND_DATAA findData;
    HANDLE fileHandle{ FindFirstFileA(filename, &findData) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        lastWriteTime = findData.ftLastWriteTime;
        FindClose(fileHandle);
    }

    return lastWriteTime;
}

INTERNAL GameCode
LoadGameCode(const char* srcDll, const char* tempDll) {
    // NOTE: Delete the temp file when the program ends?
    CopyFileA(srcDll, tempDll, FALSE);

    GameCode gameCode{};
    gameCode.dll = LoadLibraryA(tempDll);
    gameCode.lastWritetime = GetLastWriteTime(srcDll);

    using namespace game::dll;
    if (gameCode.dll) {
        gameCode.updateAndRender =
            reinterpret_cast<update_and_render*>(GetProcAddress(gameCode.dll, "UpdateAndRender"));
        gameCode.getSoundSamples =
            reinterpret_cast<get_sound_samples*>(GetProcAddress(gameCode.dll, "GetSoundSamples"));

        gameCode.isValid = gameCode.updateAndRender && gameCode.getSoundSamples;
    } else {
        // TODO: log
        OutputDebugStringA("Failed to load handmade.dll!\n");
    }

    if (!gameCode.isValid) {
        gameCode.updateAndRender = UpdateAndRenderStub;
        gameCode.getSoundSamples = GetSoundSamplesStub;
    }

    return gameCode;
}

INTERNAL void
UnloadGameCode(GameCode* gamecode) {
    if (gamecode->dll) {
        FreeLibrary(gamecode->dll);
        gamecode->dll = 0;
    }

    gamecode->updateAndRender = game::dll::UpdateAndRenderStub;
    gamecode->getSoundSamples = game::dll::GetSoundSamplesStub;
    gamecode->isValid = false;
}

#if 0
INTERNAL void
DEBUGDrawVertical(OffScreenBuffer* buff, u32 x, u32 top, u32 bottom, u32 color) {
    u8* pixel{ static_cast<u8*>(buff->memory) + buff->bytesPerPixel * x + buff->pitch * top };
    for (u32 y{ top }; y < bottom; ++y) {
        *(reinterpret_cast<u32*>(pixel)) = color;
        pixel += buff->pitch;
    }
}

INTERNAL void
DEBUGDisplayAudioSync(OffScreenBuffer* buff, DWORD lastPlayCursorCount, DWORD* lastPlayCursor,
                      const SoundOutput* soundOutput) {
    constexpr u32 padX{ 16 };
    constexpr u32 padY{ 16 };

    constexpr u32 top = padY;
    const u32 bottom{ buff->height - padY };

    const f32 c{ static_cast<f32>(buff->width) / soundOutput->buffSize };
    for (u32 i{}; i < lastPlayCursorCount; ++i) {
        u32 x{ static_cast<u32>(c * lastPlayCursor[i]) + padX };
        DEBUGDrawVertical(buff, x, top, bottom, 0xFFFFFFFF);
    }
}
#endif

INTERNAL void
CatStrings(const char* srcA, u64 srcASize, const char* srcB, u64 srcBSize, char* dest,
           u64 destSize) {
    // Space for null terminator
    u64 total{ srcASize + srcBSize };
    if (total >= destSize) {
        total = destSize - 1;
    }

    for (u32 i{}; i < srcASize && i < total; ++i) {
        *dest++ = *srcA++;
    }

    for (u32 i{}; i < srcBSize && i < total; ++i) {
        *dest++ = *srcB++;
    }

    *dest++ = '\0';
}

} //namespace win32

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI
WinMain(
    // commenting out removed C4100 warnings for unreferenced parameters
    HINSTANCE hInstance,  // A handle to the current instance of the application
    HINSTANCE /*unused*/, // hPrevInstance, Not in use anymore
    LPSTR /*unused*/,     // lpCmdLine, Command line arguments
    int /*unused*/        // nCmdShow Window visibility option
) {
    // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages

    // NOTE: don't use MAX_PATH in user code as it can be dangerous
    char exePath[MAX_PATH];
    GetModuleFileNameA(0, exePath, sizeof(exePath));
    const char* exeName{ exePath };
    for (char* scan{ exePath }; *scan; ++scan) {
        if (*scan == '\\') {
            exeName = scan + 1;
        }
    }

    // NOTE: a little string processing cause we are handmade
    const char srcDllFilename[] = "handmade.dll";
    char srcDllPath[MAX_PATH];
    win32::CatStrings(exePath, exeName - exePath, srcDllFilename, sizeof(srcDllFilename) - 1,
                      srcDllPath, sizeof(srcDllPath));

    const char tempDllFilename[] = "handmade_temp.dll";
    char tempDllPath[MAX_PATH];
    win32::CatStrings(exePath, exeName - exePath, tempDllFilename, sizeof(tempDllFilename) - 1,
                      tempDllPath, sizeof(tempDllPath));

    constexpr i32 startingWidth{ 1280 };
    constexpr i32 startingHeight{ 720 };
    win32::ResizeDIBSection(&gScreenBuff, startingWidth, startingHeight);

    WNDCLASSA windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = win32::MainWindowCallback;
    //GetModuleHandle(0) returns hInstance
    windowClass.hInstance = hInstance;
    //HICON     hIcon;

    const char* name{ "Handmade Hero" };
    windowClass.lpszClassName = name;

    // TODO: query this via Windows
    // NOTE: in the future make the game function without a minimum frame rate
    constexpr u32 monitorHz{ 60 };
    constexpr u32 gameUpdateHz{ monitorHz / 2 }; // NOTE: Could this the same as monitorHz?
    constexpr f32 targetSecondsPerFrame{ 1.0f / gameUpdateHz };

    // NOTE: This will not be used if we recap episode 20 audio fixes
    constexpr u32 framesOfAudioLatency{ 4 }; // 3 seems to be enough for gameUpdateHz of 30, test 4

    constexpr u32 desiredSchedulerMS{ 1 };
    const bool32 isSleepGranular{ timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR };

    if (RegisterClassA(&windowClass)) {
        HWND windowHandle = CreateWindowExA(0, name, name, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                            // Window size and position
                                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                            CW_USEDEFAULT, 0, 0, hInstance, 0);

        if (windowHandle) {
            HDC deviceContext{ GetDC(windowHandle) };

            // DirectSound
            win32::SoundOutput soundOutput{};
            soundOutput.samplesPerSecond = 48000;
            soundOutput.bytesPerSample = sizeof(u16) * 2;
            soundOutput.buffSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount =
                framesOfAudioLatency * soundOutput.samplesPerSecond / gameUpdateHz;

            win32::InitDSound(windowHandle, soundOutput.samplesPerSecond, soundOutput.buffSize);
            win32::ClearSoundBuffer(&soundOutput);
            gSecondaryBuff->Play(0, 0, DSBPLAY_LOOPING);

            // TODO: pool with bitmap
            i16* samples{ static_cast<i16*>(
                VirtualAlloc(0, soundOutput.buffSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) };

#if HANDMADE_INTERNAL
            // Allocate at the same address if developer build
            LPVOID baseAddress{ reinterpret_cast<LPVOID>(TERABYTES(2)) };
#else
            LPVOID baseAddress{ 0 };
#endif
            game::GameMemory gameMemory{};
            gameMemory.permanentStorageSize = MEGABYTES(64);
            gameMemory.transientStorageSize = GIGABYTES(4);

            // TODO: Variable memory allocation based on platform statistics
            const u64 totalSize{ gameMemory.permanentStorageSize +
                                 gameMemory.transientStorageSize };
            gameMemory.permanentStorage =
                VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            gameMemory.transientStorage =
                static_cast<u8*>(gameMemory.permanentStorage) + gameMemory.permanentStorageSize;
            if (!(samples && gameMemory.permanentStorage && gameMemory.transientStorage)) {
                // at least one of these failed, the game will not run correctly (or at all!)
                // TODO: better assertion
                ASSERT(!"One or more of the game memory allocations failed!");
            }

            gameMemory.DEBUGFreeFileMemory = platform::DEBUGFreeFileMemory;
            gameMemory.DEBUGReadFile = platform::DEBUGReadFile;
            gameMemory.DEBUGWriteFile = platform::DEBUGWriteFile;
            gameMemory.DEBUGPrintInt = platform::DEBUGPrintInt;
            gameMemory.DEBUGPrintFloat = platform::DEBUGPrintFloat;

            game::Input input{};

            // Performance statistics
            LARGE_INTEGER freqCounter;
            QueryPerformanceFrequency(&freqCounter);
            gPerfCounterFreq = freqCounter.QuadPart;

            LARGE_INTEGER lastCounter{ win32::GetWallClock() };
            // RDTSC
            u64 lastCycleCount{ __rdtsc() };

#if 0
            DWORD DEBUGlastPlayCursorIndex{};
            DWORD DEBUGlastPlayCursor[gameUpdateHz / 2]{};
#endif

            DWORD lastPlayCursor{};
            bool32 isSoundValid{};

            // The game represented as a DLL which allows hot reloading and more fun stuff!
            win32::GameCode game{ win32::LoadGameCode(srcDllPath, tempDllPath) };

            gIsGameRunning = true;
            while (gIsGameRunning) {
                const FILETIME newDllWriteTime{ win32::GetLastWriteTime(srcDllPath) };
                if (CompareFileTime(&game.lastWritetime, &newDllWriteTime)) {
                    win32::UnloadGameCode(&game);
                    game = win32::LoadGameCode(srcDllPath, tempDllPath);
                }

                for (u32 i{}; i < ARRAY_COUNT(input.playerInputs[0].buttons); ++i) {
                    input.playerInputs[0].buttons[i].halfTransitionCount = 0;
                }

                win32::ProcessPendingMessages(&input);

                // Directsound
                // Circular buffer so we might get two regions to write to
                // A single sample is one LEFT and RIGHT together
                //  i16  i16 ...
                // [LEFT RIGHT] LEFT RIGHT ...
                DWORD byteToLock{};
                DWORD targetCursor;
                DWORD bytesToWrite{};

                if (isSoundValid) {
                    byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
                                 soundOutput.buffSize;

                    targetCursor = (lastPlayCursor +
                                    (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) %
                                   soundOutput.buffSize;
                    // To the end and wrap behind playCursor
                    if (byteToLock > targetCursor) {
                        bytesToWrite = soundOutput.buffSize - byteToLock;
                        bytesToWrite += targetCursor;
                    } else {
                        bytesToWrite = targetCursor - byteToLock;
                    }
                } else {
                    // log, couldnt get curr pos
                }

                // Call game code
                game::OffScreenBuffer screenBuff{};
                screenBuff.memory = gScreenBuff.memory;
                screenBuff.width = gScreenBuff.width;
                screenBuff.height = gScreenBuff.height;
                screenBuff.pitch = gScreenBuff.pitch;

                game.updateAndRender(&gameMemory, &screenBuff, &input);

                game::SoundOutputBuffer soundBuff{};
                soundBuff.samplesPerSecond = soundOutput.samplesPerSecond;
                soundBuff.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                soundBuff.samples = samples;

                game.getSoundSamples(&gameMemory, &soundBuff);

                if (isSoundValid) {
                    // soundBuff now contains game generated output
                    win32::FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuff);
                    // TODO: look up episode 20 if we want to continue fixing the audio sync
                    // For now we skip fixing the issues as it is very complicated and I think I
                    // wouldn't get much out of it...
                    //#if HANDMADE_INTERNAL
                    //                    DWORD playCursor;
                    //                    DWORD writeCursor;
                    //                    gSecondaryBuff->GetCurrentPosition(&playCursor,
                    //                    &writeCursor);

                    //                    DWORD unwrappedWriteCursor{ writeCursor };
                    //                    if (unwrappedWriteCursor < playCursor) {
                    //                        unwrappedWriteCursor += soundOutput.buffSize;
                    //                    }

                    //                    DWORD bytesBetweenCursors{ unwrappedWriteCursor -
                    //                    playCursor };

                    //                    char buf[256];
                    //                    sprintf_s(buf, "LPC: %u, BTL: %u, BTW: %u, - PC: %u, WC:
                    //                    %u, delta: %u\n",
                    //                              lastPlayCursor, byteToLock, targetCursor,
                    //                              bytesToWrite, playCursor, writeCursor);
                    //                    OutputDebugStringA(buf);

                    //                    // soundBuff now contains game generated output
                    //                    win32::FillSoundBuffer(&soundOutput, byteToLock,
                    //                    bytesToWrite, &soundBuff);
                    //#endif
                }

                // Performance
                // We only query the performance once per frame so that we don't leave out the time
                // between the frame's end and start

                LARGE_INTEGER endCounter{ win32::GetWallClock() };
                const f64 secondsElapsed{ win32::GetSecondsElapsed(lastCounter, endCounter) };
                f64 secondsElapedForFrame{ secondsElapsed };

#if 1
                // Wait if we are ahead of the target FPS
                if (secondsElapedForFrame < targetSecondsPerFrame) {
                    while (secondsElapedForFrame < targetSecondsPerFrame) {
                        if (isSleepGranular) {
                            const DWORD remainingMS{ static_cast<DWORD>(
                                (targetSecondsPerFrame - secondsElapedForFrame) * 1000.0f) };
                            if (remainingMS > 0) {
                                Sleep(remainingMS);
                            }
                        }

                        // NOTE: this would not always work...
                        //f32 testSecondsElapsedForFrame{ win32::GetSecondsElapsed(
                        //    lastCounter, win32::GetWallClock()) };
                        //ASSERT(testSecondsElapsedForFrame < targetSecondsPerFrame);

                        secondsElapedForFrame =
                            win32::GetSecondsElapsed(lastCounter, win32::GetWallClock());
                    }
                } else {
                    //TODO: log the missed frame!!!
                }
#endif

                endCounter = win32::GetWallClock();
                const f64 ms{ 1000 * win32::GetSecondsElapsed(lastCounter, endCounter) };
                const f64 FPS{ 1000 / ms };

                auto wndDimension{ win32::GetWindowDimensions(windowHandle) };

//#if HANDMADE_INTERNAL
#if 0
                win32::DEBUGDisplayAudioSync(&gScreenBuff, ARRAY_COUNT(DEBUGlastPlayCursor),
                                             DEBUGlastPlayCursor, &soundOutput);
#endif

                win32::DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width,
                                           wndDimension.height);

                DWORD playCursor;
                DWORD writeCursor;
                if (SUCCEEDED(gSecondaryBuff->GetCurrentPosition(&playCursor, &writeCursor))) {
                    lastPlayCursor = playCursor;
                    isSoundValid = true;
                } else {
                    isSoundValid = false;
                }

//#if HANDMADE_INTERNAL
#if 0
                DEBUGlastPlayCursor[DEBUGlastPlayCursorIndex++] = playCursor;
                if (DEBUGlastPlayCursorIndex > ARRAY_COUNT(DEBUGlastPlayCursor)) {
                    DEBUGlastPlayCursorIndex = 0;
                }
#endif

                const u64 endCycleCount{ __rdtsc() };
                const f64 cycleElapsedM{ static_cast<f64>((endCycleCount - lastCycleCount)) / 1000 *
                                         1000 };

                //const f64 ms{ 1000.0 * secondsElapsed };
                //const f64 FPS{ 1000 / ms };
#if 1
                char buf[64]; // yikes...
                sprintf_s(buf, "frame: %.5f ms | FPS: %.2f | cycles: %.4f M\n", ms, FPS,
                          cycleElapsedM);
                OutputDebugStringA(buf);
#endif

                lastCycleCount = endCycleCount;

                const LARGE_INTEGER resetCounter{ win32::GetWallClock() };
                lastCounter = resetCounter;
            }

            ReleaseDC(windowHandle, deviceContext);
        } else {
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
