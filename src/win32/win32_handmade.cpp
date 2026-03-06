/*
Episode 11: Important topics about making code cross-platform
This is not a final platform layer!
*/

#include "handmade.cpp"

#include <dsound.h>
#include <windows.h>

#include <cstdio>

#include "win32_handmade.h"

GLOBAL bool32 gIsGameRunning;
GLOBAL Win32OffScreenBuffer gScreenBuff;
GLOBAL LPDIRECTSOUNDBUFFER gSecondaryBuff;

// TODO: will be refactored in episode 17
// as it currently doesn't support multiple key pressed per (Windows) poll time
// Currently the polling goes through Win32MainWindowCallback, which is not good!
GLOBAL game::Input gInput;

INTERNAL void*
DEBUGPlatformReadFile(const char* filename) {
    void* result{};
    HANDLE fileHandle{ CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0,
                                   0) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize)) {
            // Check
            result = VirtualAlloc(0, fileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result) {
                u32 bytesToRead{ safeTrunateU64toU32(fileSize.QuadPart) };
                DWORD bytesRead;
                if (ReadFile(fileHandle, result, bytesToRead, &bytesRead, 0)) {
                    // Success
                } else {
                    DEBUGPlatformFreeFileMemory(result);
                    result = 0;
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
INTERNAL void
DEBUGPlatformFreeFileMemory(void* memory) {
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}
INTERNAL bool32
DEBUGPlatformWriteFile(const char* filename, u32 fileSize, void* memory) {}

INTERNAL WindowDimension
Win32GetWindowDimensions(HWND windowHandle) {
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    const i32 w{ clientRect.right - clientRect.left };
    const i32 h{ clientRect.bottom - clientRect.top };

    return WindowDimension{ w, h };
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
Win32DisplayBufferWindow(const HDC deviceContext, const Win32OffScreenBuffer* buff, i32 wndWidth,
                         i32 wndHeight) {
    // TODO: aspect ratio correction
    StretchDIBits(deviceContext, 0, 0, wndWidth, wndHeight, // dest
                  0, 0, buff->width, buff->height,          // src
                  buff->memory, &buff->info, DIB_RGB_COLORS, SRCCOPY);
}

// Callback for messages
LRESULT CALLBACK
Win32MainWindowCallback(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result{ 0 };

    switch (msg) {
    case WM_CLOSE: {
        // TODO: show a message for closing
        OutputDebugStringA("WM_CLOSE\n");
        gIsGameRunning = false;
    } break;
    case WM_DESTROY: {
        // NOTE: This might happen as an error?
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

#ifdef _DEBUG

#endif
    // Key presses

    // SYSKEYDOWN is called whenever the key press includes alt
    // all other keypresses go to the non-sys versions below
    // This forces us to handle alt + f4 here...
    case WM_SYSKEYDOWN: {
        u32 vkCode{ static_cast<u32>(wParam) };
        if (vkCode == VK_F4) {
            OutputDebugStringA("VK_F4 SYSKEYDOWN\n");
            // Should always be true here
            bool32 altPressed{ (lParam & (1 << 29)) != 0 };
            ASSERT(altPressed && "SYSKEYDOWN did not have the alt key pressed");
            if (altPressed) {
                gIsGameRunning = false;
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
        bool32 isDown{ (lParam & (1 << 31)) == 0 };
        bool32 wasDown{ (lParam & (1 << 30)) != 0 };

        //char buf[32];
        //sprintf_s(buf, "isDown: %d, wasDown: %d\n", isDown, wasDown);
        //OutputDebugStringA(buf);

        // If continuously pressing
        // TODO: should change to handle that case also instead of breaking?
        //if (wasDown != isDown) {
        if (vkCode == 'W') {
            OutputDebugStringA("W\n");
            gInput.playerInputs->up.endedDown = isDown;
        } else if (vkCode == 'S') {
            OutputDebugStringA("S\n");
            gInput.playerInputs->down.endedDown = isDown;
        } else if (vkCode == 'A') {
            OutputDebugStringA("A\n");
            gInput.playerInputs->left.endedDown = isDown;
        } else if (vkCode == 'D') {
            OutputDebugStringA("D\n");
            gInput.playerInputs->right.endedDown = isDown;
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
            OutputDebugStringA("VK_ESCAPE\n");
        } else if (vkCode == VK_SPACE) {
            OutputDebugStringA("VK_SPACE\n");
        }
        //}
    } break;
    case WM_KEYUP: {
    } break;

    case WM_PAINT: {
        OutputDebugStringA("WM_PAINT\n");

        PAINTSTRUCT paint;
        const HDC deviceContext{ BeginPaint(wnd, &paint) };

        auto wndDimension{ Win32GetWindowDimensions(wnd) };
        Win32DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width,
                                 wndDimension.height);

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
// clang-format off
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
// clang-format on

//TODO: consider cleaning up, starting to become a mess
INTERNAL void
Win32InitDSound(HWND windowHandle, u32 samplesPerSecond, u32 buffSize) {
    HMODULE dSoundLib{ LoadLibraryA("dsound.dll") };
    if (dSoundLib) {
        direct_sound_create* dSoundCreate =
            reinterpret_cast<direct_sound_create*>(GetProcAddress(dSoundLib, "DirectSoundCreate"));
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
Win32ClearSoundBuffer(Win32SoundOutput* soundOutput) {
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
Win32FillSoundBuffer(Win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite,
                     const game::SoundOutputBuffer* sourceBuff) {
    LPVOID region1;
    DWORD region1Size;
    LPVOID region2;
    DWORD region2Size;
    if (SUCCEEDED(gSecondaryBuff->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2,
                                       &region2Size, 0))) {
        // TODO: assert regionSizes
        //ASSERT(false && "regionSizes are invalid!");

        const DWORD region1SampleCount{ region1Size / soundOutput->bytesPerSample };
        i16* destSample{ static_cast<i16*>(region1) };
        i16* srcSample{ sourceBuff->samples };

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

// https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI
WinMain(
    // commenting out removed C4100 warnings for unreferenced parameters
    HINSTANCE hInstance, // A handle to the current instance of the application
    HINSTANCE,           // hPrevInstance, Not in use anymore
    PSTR,                // lpCmdLine, Command line arguments
    int                  // nCmdShow Window visibility option
) {
    // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages

    constexpr i32 startingWidth{ 1280 };
    constexpr i32 startingHeight{ 720 };
    Win32ResizeDIBSection(&gScreenBuff, startingWidth, startingHeight);

    WNDCLASSA windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    //GetModuleHandle(0) returns hInstance
    windowClass.hInstance = hInstance;
    //HICON     hIcon;

    const char* name{ "Handmade Hero" };
    windowClass.lpszClassName = name;

    if (RegisterClass(&windowClass)) {
        const HWND windowHandle = CreateWindowExA(0, name, name, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                                  // Window size and position
                                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                  CW_USEDEFAULT, 0, 0, hInstance, 0);

        if (windowHandle) {
            // DirectSound
            Win32SoundOutput soundOutput{};
            soundOutput.samplesPerSecond = 48000;
            soundOutput.bytesPerSample = sizeof(u16) * 2;
            soundOutput.buffSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

            Win32InitDSound(windowHandle, soundOutput.samplesPerSecond, soundOutput.buffSize);
            Win32ClearSoundBuffer(&soundOutput);
            gSecondaryBuff->Play(0, 0, DSBPLAY_LOOPING);

            // TODO: pool with bitmap
            i16* samples{ static_cast<i16*>(
                VirtualAlloc(0, soundOutput.buffSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) };

#if HANDMADE_INTERNAL
            // allocate at the same address if developer build
            LPVOID baseAddress{ reinterpret_cast<LPVOID>(TERABYTES(2)) };
#else
            LPVOID baseAddress{ 0 };
#endif
            game::GameMemory gameMemory{};
            gameMemory.permanentStorageSize = MEGABYTES(64);
            // Integral promotion, otherwise we wrap to 0 because we hit u32 max...
            gameMemory.transientStorageSize = GIGABYTES(4);

            // TODO: Variable memory allocation based on platform statistics
            u64 totalSize{ gameMemory.permanentStorageSize + gameMemory.transientStorageSize };
            gameMemory.permanentStorage =
                VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            gameMemory.transientStorage =
                static_cast<u8*>(gameMemory.permanentStorage) + gameMemory.permanentStorageSize;
            if (!(samples && gameMemory.permanentStorage && gameMemory.transientStorage)) {
                // at least one of these failed, the game will not run correctly (or at all!)
                // TODO: better assertion
                ASSERT(false && "One or more of the game memory allocations failed!");
            }

            // Performance statistics
            LARGE_INTEGER freqCounter;
            QueryPerformanceFrequency(&freqCounter);
            i64 perfCounterFreq{ freqCounter.QuadPart };
            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);
            // RDTSC
            u64 lastCycleCount{ __rdtsc() };

            // The main loop!
            gIsGameRunning = true;

            while (gIsGameRunning) {
                // Clear input
                gInput = game::Input{};

                // Windows message queue, everything has to go through the OS
                MSG message;
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        gIsGameRunning = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                // Directsound
                // Circular buffer so we might get two regions to write to
                // A single sample is one LEFT and RIGHT together
                //  i16  i16 ...
                // [LEFT RIGHT] LEFT RIGHT ...
                DWORD byteToLock{};
                DWORD targetCursor;
                DWORD bytesToWrite{};
                DWORD playCursor;
                DWORD writeCursor;
                bool32 isSoundValid{};

                if (SUCCEEDED(gSecondaryBuff->GetCurrentPosition(&playCursor, &writeCursor))) {
                    byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
                                 soundOutput.buffSize;

                    targetCursor =
                        (playCursor + soundOutput.latencySampleCount * soundOutput.bytesPerSample) %
                        soundOutput.buffSize;
                    // To the end and wrap behind playCursor
                    if (byteToLock > targetCursor) {
                        bytesToWrite = soundOutput.buffSize - byteToLock;
                        bytesToWrite += targetCursor;
                    } else {
                        bytesToWrite = targetCursor - byteToLock;
                    }

                    isSoundValid = true;
                } else {
                    // log, couldnt get curr pos
                }

                // Call game code
                game::OffScreenBuffer screenBuff{};
                screenBuff.memory = gScreenBuff.memory;
                screenBuff.width = gScreenBuff.width;
                screenBuff.height = gScreenBuff.height;
                screenBuff.pitch = gScreenBuff.pitch;

                game::SoundOutputBuffer soundBuff{};
                soundBuff.samplesPerSecond = soundOutput.samplesPerSecond;
                soundBuff.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                soundBuff.samples = samples;

                game::UpdateAndRender(&gameMemory, &screenBuff, &soundBuff, &gInput);

                if (isSoundValid) {
                    // soundBuff now contains game generated output
                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuff);
                }

                const HDC deviceContext{ GetDC(windowHandle) };
                auto wndDimension{ Win32GetWindowDimensions(windowHandle) };
                Win32DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width,
                                         wndDimension.height);
                ReleaseDC(windowHandle, deviceContext);
#if 0
                // Performance

                // We only query the performance once per frame so that we don't leave out the time
                // between the frame's end and start
                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);
                const u64 endCycleCount{ __rdtsc() };

                const i64 counterElapsed{ endCounter.QuadPart - lastCounter.QuadPart };
                lastCounter = endCounter;
                const f64 cycleElapsedM{ static_cast<f64>(endCycleCount - lastCycleCount) /
                                         (1000.0 * 1000.0) };
                lastCycleCount = endCycleCount;

                // Multiply by 1000 so that the integer divide doesn't always result in 0, assuming
                // the frame took less than 1 second
                const f64 ms{ (1000.0 * counterElapsed) / perfCounterFreq };
                const f64 FPS{ 1000.0 / ms };
                char buf[64]; // yikes...
                sprintf_s(buf, "frame: %.5f ms | FPS: %.2f | cycles: %.4f M\n", ms, FPS,
                          cycleElapsedM);
                OutputDebugStringA(buf);
#endif
            }
        } else {
            // NOTE: Log?
            OutputDebugStringA("Failed to create windowHandle!\n");
        }
    } else {
        // NOTE: Log?
        OutputDebugStringA("Failed to register windowClass!\n");
    }

    return 0;
}
