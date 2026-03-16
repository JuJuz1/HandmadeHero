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
GLOBAL bool32 gIsGamePaused;

GLOBAL win32::OffScreenBuffer gScreenBuff;
GLOBAL LPDIRECTSOUNDBUFFER gSecondaryBuff;
GLOBAL i64 gPerfCounterFreq;

namespace platform_export {

// TODO: make these more generic (and allow variadic arguments?)
// and much safer...
INTERNAL
DEBUG_PRINT_INT(DEBUGPrintInt) {
    UNUSED_PARAMS(threadContext);

    char buf[32];
    sprintf_s(buf, "%s: %d\n", valueName, value);
    OutputDebugStringA(buf);
}

INTERNAL
DEBUG_PRINT_FLOAT(DEBUGPrintFloat) {
    UNUSED_PARAMS(threadContext);

    char buf[32];
    sprintf_s(buf, "%s: %f\n", valueName, value);
    OutputDebugStringA(buf);
}

INTERNAL
DEBUG_FREE_FILE_MEMORY(DEBUGFreeFileMemory) {
    UNUSED_PARAMS(threadContext);

    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

INTERNAL
DEBUG_READ_FILE(DEBUGReadFile) {
    UNUSED_PARAMS(threadContext);

    DEBUGFileReadResult result{};
    // What an atrocious name for a function which requests to read a file...
    HANDLE fileHandle{ CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0,
                                   0) };
    if (fileHandle == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("DEBUGReadFile file read failed!\n");
        return result;
    }

    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(fileHandle, &fileSize)) {
        result.content =
            VirtualAlloc(0, fileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!result.content) {
            OutputDebugStringA("DEBUGReadFile VirtualAlloc failed!\n");
            return result;
        }

        const u32 bytesToRead{ SafeTruncateU64toU32(fileSize.QuadPart) };
        DWORD bytesRead;
        // Consider the case where one could truncate the file after reading
        if (ReadFile(fileHandle, result.content, bytesToRead, &bytesRead, 0) &&
            bytesToRead == bytesRead) {
            result.contentSize = bytesToRead;
        } else {
            DEBUGFreeFileMemory(threadContext, result.content);
            result.content = 0;
        }
    } else {
        // log
    }

    CloseHandle(fileHandle);

    return result;
}

INTERNAL
DEBUG_WRITE_FILE(DEBUGWriteFile) {
    UNUSED_PARAMS(threadContext);

    bool32 result{};
    HANDLE fileHandle{ CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, fileSize, &bytesWritten, 0)) {
            // Success
            result = bytesWritten == fileSize;
        } else {
            // log
        }

        CloseHandle(fileHandle);
    } else {
        // log
    }

    return result;
}

} //namespace platform_export

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
ResizeDIBSection(OffScreenBuffer* screenBuff, i32 w, i32 h) {
    if (screenBuff->memory) {
        VirtualFree(screenBuff->memory, 0, MEM_RELEASE);
    }

    screenBuff->width = w;
    screenBuff->height = h;
    screenBuff->bytesPerPixel = 4;

    screenBuff->info.bmiHeader.biSize = sizeof(screenBuff->info.bmiHeader);
    screenBuff->info.bmiHeader.biWidth = screenBuff->width;
    screenBuff->info.bmiHeader.biHeight = -screenBuff->height; // top-down by assigning negative
    screenBuff->info.bmiHeader.biPlanes = 1;
    screenBuff->info.bmiHeader.biBitCount = 32; // 8 padding
    screenBuff->info.bmiHeader.biCompression = BI_RGB;

    const u32 bmMemorySize{ screenBuff->width * screenBuff->height * screenBuff->bytesPerPixel };
    // Allocate the memory for use immediately (MEM_COMMIT)
    screenBuff->memory = VirtualAlloc(0, bmMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    screenBuff->pitch = screenBuff->width * static_cast<i32>(screenBuff->bytesPerPixel);

    // TODO: clear to black probably
}

INTERNAL void
DisplayBufferWindow(const HDC deviceContext, // clang-tidy NOLINT
                    const OffScreenBuffer* screenBuff, i32 wndWidth, i32 wndHeight) {
    // TODO: aspect ratio correction
    // NOTE: buffer is locked for prototyping
    // wndWidth, wndHeight
    StretchDIBits(deviceContext, 0, 0, screenBuff->width, screenBuff->height, // dest
                  0, 0, screenBuff->width, screenBuff->height,                // src
                  screenBuff->memory, &screenBuff->info, DIB_RGB_COLORS, SRCCOPY);
}

// Make use of runtime dynamic library linking
// Make a function pointer type for DirectSoundCreate (a function inside the dll)
// Feels like magic but really it's not when you understand it
// clang-format off
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
// clang-format on

INTERNAL void
InitDSound(HWND windowHandle, u32 samplesPerSecond, u32 buffSize) {
    HMODULE dSoundLib{ LoadLibraryA("dsound.dll") };
    if (!dSoundLib) {
        // log, couldn't load dll
        return;
    }

    direct_sound_create* dSoundCreate{ reinterpret_cast<direct_sound_create*>(
        GetProcAddress(dSoundLib, "DirectSoundCreate")) };
    LPDIRECTSOUND dSound;

    if (!dSoundCreate || !SUCCEEDED(dSoundCreate(0, &dSound, 0))) {
        // log, function pointer invalid or the call failed
        return;
    }

    if (!SUCCEEDED(dSound->SetCooperativeLevel(windowHandle, DSSCL_PRIORITY))) {
        // log, setcooperativelevel failed
        return;
    }

    DSBUFFERDESC primaryBuffDescription{};
    primaryBuffDescription.dwSize = sizeof(primaryBuffDescription);
    primaryBuffDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

    // For "both" buffers
    WAVEFORMATEX waveFormat{};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = samplesPerSecond;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    LPDIRECTSOUNDBUFFER primaryBuff;
    if (SUCCEEDED(dSound->CreateSoundBuffer(&primaryBuffDescription, &primaryBuff, 0))) {
        if (SUCCEEDED(primaryBuff->SetFormat(&waveFormat))) {
            OutputDebugStringA("Primary buffer format was set\n");
        } else {
            // log, setting format failed
        }
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

        // NOTE: collapse loops to one?
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

// Circular buffer so we might get two regions to write to
INTERNAL DSoundParams
ProcessDSoundParams(const SoundOutput* soundOutput, DWORD lastPlayCursor, bool32 isSoundValid) {
    DSoundParams dSoundParams{};

    if (isSoundValid) {
        dSoundParams.byteToLock =
            (soundOutput->runningSampleIndex * soundOutput->bytesPerSample) % soundOutput->buffSize;

        dSoundParams.targetCursor =
            (lastPlayCursor + (soundOutput->latencySampleCount * soundOutput->bytesPerSample)) %
            soundOutput->buffSize;
        // To the end and wrap behind playCursor
        if (dSoundParams.byteToLock > dSoundParams.targetCursor) {
            dSoundParams.bytesToWrite = soundOutput->buffSize - dSoundParams.byteToLock;
            dSoundParams.bytesToWrite += dSoundParams.targetCursor;
        } else {
            dSoundParams.bytesToWrite = dSoundParams.targetCursor - dSoundParams.byteToLock;
        }
    } else {
        // log, couldnt get curr pos
    }

    return dSoundParams;
}

// Callback for messages
INTERNAL LRESULT CALLBACK
MainWindowCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        // NOTE: modify these to preference, now disabled
        if (wParam) {
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        } else {
            //SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 64, LWA_ALPHA);
        }
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
        const HDC deviceContext{ BeginPaint(hWnd, &paint) }; // clang-tidy NOLINT

        auto wndDimension{ GetWindowDimensions(hWnd) };
        DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width, wndDimension.height);

        EndPaint(hWnd, &paint);
    } break;
    default: {
        result = DefWindowProcA(hWnd, msg, wParam, lParam);
    } break;
    }

    return result;
}

INTERNAL void
GetExePathAndFilename(AllState* allState) {
    GetModuleFileNameA(0, allState->exePath, sizeof(allState->exePath));
    allState->exeFilename = allState->exePath;

    for (char* scan{ allState->exePath }; *scan; ++scan) {
        if (*scan == '\\') {
            allState->exeFilename = scan + 1;
        }
    }
}

INTERNAL void
BuildGamePathFilename(const AllState* allState, const char* filename, char* dest, u32 destCount) {
    CatStrings(allState->exePath, allState->exeFilename - allState->exePath, filename,
               StrLength(filename), dest, destCount);
}

INTERNAL void
GetInputFilePath(const AllState* allState, bool32 inputStream, u32 slotIndex, char* dest,
                 u32 destCount) {
    char temp[32];
    wsprintfA(temp, "loop_edit%d_%s.hmi", slotIndex, inputStream ? "input" : "state");
    BuildGamePathFilename(allState, temp, dest, destCount);
}

INTERNAL ReplayBuffer*
GetReplayBuffer(AllState* allState, u32 index) {
    ASSERT(index < ARRAY_COUNT(allState->replayBuffers));
    ReplayBuffer* replayBuffer{ &allState->replayBuffers[index] };
    return replayBuffer;
}

INTERNAL void
BeginRecordInput(AllState* allState, u32 recordingIndex) {
    // TODO: cleanup the files after exiting the game?
    allState->recordingIndex = recordingIndex;

    char filePath[win32::allStateFileNameCount];
    GetInputFilePath(allState, true, recordingIndex, filePath, sizeof(filePath));
    HANDLE fileHandle{ CreateFileA(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        allState->recordingHandle = fileHandle;
    } else {
        OutputDebugStringA("BeginRecordInput CreateFileA writing to file failed!\n");
    }

    OutputDebugStringA("BeginRecordInput start recording input!\n");

    // Storing the replay buffers in memory now!
    const ReplayBuffer* replayBuffer{ GetReplayBuffer(allState, recordingIndex) };
    CopyMemory(replayBuffer->memoryBlock, allState->gameMemory, allState->memorySize);
}

INTERNAL void
EndRecordInput(AllState* allState) {
    if (allState->recordingHandle) {
        CloseHandle(allState->recordingHandle);
        allState->recordingHandle = INVALID_HANDLE_VALUE;
    }

    allState->recordingIndex = 0;
    OutputDebugStringA("RecordInput ended!\n");
}

INTERNAL void
BeginInputPlayback(AllState* allState, u32 playingIndex) {
    allState->playingIndex = playingIndex;

    char filePath[allStateFileNameCount];
    GetInputFilePath(allState, true, playingIndex, filePath, sizeof(filePath));
    HANDLE fileHandle{ CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0,
                                   0) };
    if (fileHandle != INVALID_HANDLE_VALUE) {
        allState->playingHandle = fileHandle;
    } else {
        OutputDebugStringA("BeginInputPlayback CreateFileA reading file failed!\n");
    }

    const ReplayBuffer* replayBuffer{ GetReplayBuffer(allState, playingIndex) };
    CopyMemory(allState->gameMemory, replayBuffer->memoryBlock, allState->memorySize);
}

INTERNAL void
EndInputPlayback(AllState* allState) {
    if (allState->playingHandle) {
        CloseHandle(allState->playingHandle);
        // NOTE: not needed, helps debugging though
        allState->playingHandle = INVALID_HANDLE_VALUE;
    }

    allState->playingIndex = 0;
    OutputDebugStringA("PlaybackInput ended!\n");
}

// These functions are the ones called in the loop

INTERNAL void
RecordInput(const game::Input* input, AllState* allState) {
    DWORD bytesWritten;
    if (WriteFile(allState->recordingHandle, input, sizeof(*input), &bytesWritten, 0) &&
        bytesWritten == sizeof(*input)) {
        // Success
    } else {
        OutputDebugStringA("RecordInput RecordInput writing to file failed!\n");
    }
}

INTERNAL void
PlaybackInput(game::Input* input, AllState* allState) {
    DWORD bytesRead;
    if ((ReadFile(allState->playingHandle, input, sizeof(*input), &bytesRead, 0) &&
         bytesRead == sizeof(*input))) {
        // Still input left
    } else {
        // EOF (bytesRead != sizeof(*input)) or error while reading
        const u32 playingIndex{ allState->playingIndex };
        EndInputPlayback(allState);

        // NOTE: add a check if we want to prevent looping, like some bool
        OutputDebugStringA("PlaybackInput starting loop again!\n");
        BeginInputPlayback(allState, playingIndex);
        // NOTE: Read the first frame again to fix 1 leaky frame while the playback loops, maybe not
        // needed?
        ReadFile(allState->playingHandle, input, sizeof(*input), &bytesRead, 0);
    }
}

INTERNAL void
ClearInputMemory(game::Input* input) {
    ZeroMemory(input, sizeof(*input));
}

INTERNAL void
HandleRecordButton(AllState* allState, game::Input* input) {
    if (input->playerInputs->shift.endedDown) {
        // TODO: Clear recorded input, or switch index or something
    } else {
        ClearInputMemory(input);

        if (allState->playingIndex == 0) {
            if (allState->recordingIndex == 0) {
                OutputDebugStringA("L: Recording STARTED!\n");
                BeginRecordInput(allState, 1);
            } else {
                OutputDebugStringA("L: Recording STOPPED!\n");
                EndRecordInput(allState);
                BeginInputPlayback(allState, 1);
            }
        } else {
            OutputDebugStringA("L: Playing STOPPED!\n");
            EndInputPlayback(allState);
        }
    }
}

// The fired message should never have the same state (wasDown == isDown)
INTERNAL void
ProcessKeyboardMessage(game::Button* button, bool32 isDown) {
    // NOTE: maybe just use if instead of ASSERT due to the way we do mouse input polling atm
    if (button->endedDown != isDown) {
        button->endedDown = isDown;
        ++button->halfTransitionCount;
    }
}

INTERNAL void
ProcessPendingMessages(game::Input* input, AllState* allState) {
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

        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_KEYDOWN: {
            // Ignore continuous messages
            if (isDown == wasDown) {
                break;
            }

            //char buf[32];
            //sprintf_s(buf, "isDown: %d, wasDown: %d\n", isDown, wasDown);
            //OutputDebugStringA(buf);

            switch (vkCode) {
            case 'W': {
                OutputDebugStringA("W\n");
                // This approach won't work for justPressed, we have to use halfTransitionCount
                //input->playerInputs->up.pressed = true;
                ProcessKeyboardMessage(&input->playerInputs->up, isDown);
            } break;
            case 'S': {
                OutputDebugStringA("S\n");
                ProcessKeyboardMessage(&input->playerInputs->down, isDown);
            } break;
            case 'A': {
                OutputDebugStringA("A\n");
                ProcessKeyboardMessage(&input->playerInputs->left, isDown);
            } break;
            case 'D': {
                OutputDebugStringA("D\n");
                ProcessKeyboardMessage(&input->playerInputs->right, isDown);
            } break;
            case 'Q': {
                OutputDebugStringA("Q\n");
                ProcessKeyboardMessage(&input->playerInputs->Q, isDown);
            } break;
            case 'E': {
                OutputDebugStringA("E\n");
                ProcessKeyboardMessage(&input->playerInputs->E, isDown);
            } break;
            case VK_SHIFT: {
                OutputDebugStringA("VK_SHIFT\n");
                ProcessKeyboardMessage(&input->playerInputs->shift, isDown);
            } break;
            case VK_F4: {
                if (isDown) {
                    OutputDebugStringA("VK_F4\n");
                    const bool32 altPressed{ (message.lParam & (1 << 29)) != 0 };
                    if (altPressed) {
                        gIsGameRunning = false;
                    }
                }
            } break;
#if HANDMADE_INTERNAL
            case 'L': {
                if (isDown) {
                    HandleRecordButton(allState, input);
                }
            } break;
            case 'P': {
                if (isDown) {
                    if (gIsGamePaused) {
                        OutputDebugStringA("P: Game unpaused!\n");
                    } else {
                        OutputDebugStringA("P: Game paused!\n");
                    }
                    gIsGamePaused = !gIsGamePaused;
                }
            } break;
#endif
            case VK_UP: {
                OutputDebugStringA("VK_UP\n");
            } break;
            case VK_DOWN: {
                OutputDebugStringA("VK_DOWN\n");
            } break;
            case VK_LEFT: {
                OutputDebugStringA("VK_LEFT\n");
            } break;
            case VK_RIGHT: {
                OutputDebugStringA("VK_RIGHT\n");
            } break;
            case VK_ESCAPE: {
                OutputDebugStringA("VK_ESCAPE\n");
            } break;
            case VK_SPACE: {
                OutputDebugStringA("VK_SPACE\n");
            } break;
            default: {
                char buf[32];
                sprintf_s(buf, "vkCode: %u NOT HANDLED\n", vkCode);
                OutputDebugStringA(buf);
            } break;
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

    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(filename, GetFileExInfoStandard, &fileInfo)) {
        lastWriteTime = fileInfo.ftLastWriteTime;
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
        // log
        OutputDebugStringA("Failed to load handmade.dll!\n");
    }

    if (!gameCode.isValid) {
        OutputDebugStringA("gameCode is invalid, game functions are null!\n");
    }

    return gameCode;
}

INTERNAL void
UnloadGameCode(GameCode* gamecode) {
    if (gamecode->dll) {
        FreeLibrary(gamecode->dll);
        gamecode->dll = 0;
    }

    gamecode->updateAndRender = 0;
    gamecode->getSoundSamples = 0;
    gamecode->isValid = false;
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

    win32::AllState allState{};
    win32::GetExePathAndFilename(&allState);

    // NOTE: a little string processing cause we are handmade
    char srcDllPath[win32::allStateFileNameCount];
    win32::BuildGamePathFilename(&allState, "handmade.dll", srcDllPath, sizeof(srcDllPath));
    char tempDllPath[win32::allStateFileNameCount];
    win32::BuildGamePathFilename(&allState, "handmade_temp.dll", tempDllPath, sizeof(tempDllPath));

    constexpr i32 startingWidth{ 1280 };
    constexpr i32 startingHeight{ 720 };
    win32::ResizeDIBSection(&gScreenBuff, startingWidth, startingHeight);

    WNDCLASSA windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = win32::MainWindowCallback;
    windowClass.hInstance = hInstance;

    const char* name{ "Handmade Hero" };
    windowClass.lpszClassName = name;

    // NOTE: This will not be used if we recap episode 20 audio fixes
    // 3 seems to be enough for monitorHz of 60 (gameUpdateHz 30), 5 for 144
    // NOTE: audio is bugged when using the record and playback
    constexpr u32 framesOfAudioLatency{ 5 };

    constexpr u32 desiredSchedulerMS{ 1 };
    const bool32 isSleepGranular{ timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR };

    if (!RegisterClassA(&windowClass)) {
        // NOTE: Log?
        OutputDebugStringA("Failed to register windowClass!\n");
        return 0;
    }

    // NOTE: I don't need WS_EX_TOPMOST on dwExStyle as I have 2 monitors
    HWND windowHandle = CreateWindowExA(WS_EX_LAYERED, name, name, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        // Window size and position
                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, hInstance, 0);

    if (!windowHandle) {
        // NOTE: Log?
        OutputDebugStringA("Failed to create windowHandle!\n");
        return 0;
    }

    // NOTE: in the future make the game function with arbitrary frame rate?
    // Ideally yes, but in practice would need a lot of work
    const HDC deviceContextRefreshRate{ GetDC(windowHandle) }; // clang-tidy NOLINT
    u32 monitorHz{ 60 };
    const i32 win32MonitorHz{ GetDeviceCaps(deviceContextRefreshRate, VREFRESH) };
    if (win32MonitorHz > 1) {
        monitorHz = static_cast<u32>(win32MonitorHz);
    }

    // NOTE : Could this be the same as monitorHz?
    const f32 gameUpdateHz{ static_cast<f32>(monitorHz) / 2.0f };
    const f32 targetSecondsPerFrame{ 1.0f / gameUpdateHz };
    ReleaseDC(windowHandle, deviceContextRefreshRate);

    // DirectSound
    win32::SoundOutput soundOutput{};
    soundOutput.samplesPerSecond = 48000;
    soundOutput.bytesPerSample = sizeof(u16) * 2;
    soundOutput.buffSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
    // NOTE: gameUpdateHz can be a float here... will be fixed later!
    soundOutput.latencySampleCount =
        framesOfAudioLatency * soundOutput.samplesPerSecond / static_cast<u32>(gameUpdateHz);

    win32::InitDSound(windowHandle, soundOutput.samplesPerSecond, soundOutput.buffSize);
    win32::ClearSoundBuffer(&soundOutput);
    gSecondaryBuff->Play(0, 0, DSBPLAY_LOOPING);

    // TODO: pool with bitmap
    i16* soundBuffSamples{ static_cast<i16*>(
        VirtualAlloc(0, soundOutput.buffSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) };

#if HANDMADE_INTERNAL
    // Allocate at the same address if developer build
    LPVOID baseAddress{ reinterpret_cast<LPVOID>(TERABYTES(2)) };
#else
    LPVOID baseAddress{ 0 };
#endif

    game::GameMemory gameMemory{};
    gameMemory.permanentStorageSize = MEGABYTES(64);
    gameMemory.transientStorageSize =
        GIGABYTES(1); // NOTE: MEGABYTES(128); changed from GIGABYTES(1) to speed up recording

    // TODO: Variable memory allocation based on platform statistics
    const u64 totalSize{ gameMemory.permanentStorageSize + gameMemory.transientStorageSize };
    // TODO: check usage for MEM_LARGE_PAGES
    gameMemory.permanentStorage =
        VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    gameMemory.transientStorage =
        static_cast<u8*>(gameMemory.permanentStorage) + gameMemory.permanentStorageSize;
    if (!(soundBuffSamples && gameMemory.permanentStorage && gameMemory.transientStorage)) {
        // at least one of these failed, the game will not run correctly (or at all!)
        // NOTE: better assertion?
        ASSERT(!"One or more of the game memory allocations failed!");
    }

    // Platform exports
    gameMemory.DEBUGFreeFileMemory = platform_export::DEBUGFreeFileMemory;
    gameMemory.DEBUGReadFile = platform_export::DEBUGReadFile;
    gameMemory.DEBUGWriteFile = platform_export::DEBUGWriteFile;
    gameMemory.DEBUGPrintInt = platform_export::DEBUGPrintInt;
    gameMemory.DEBUGPrintFloat = platform_export::DEBUGPrintFloat;

    allState.gameMemory = gameMemory.permanentStorage;
    allState.memorySize = totalSize;

    // Saving game memory in memory, piggy time!
    for (u32 i{}; i < ARRAY_COUNT(allState.replayBuffers); ++i) {
        win32::ReplayBuffer* replayBuffer{ &allState.replayBuffers[i] };

        // Using memory mapping
        char filePath[win32::allStateFileNameCount];
        GetInputFilePath(&allState, false, i, filePath, sizeof(filePath));
        HANDLE fileHandle{ CreateFileA(filePath, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS,
                                       0, 0) };
        if (fileHandle != INVALID_HANDLE_VALUE) {
            replayBuffer->fileHandle = fileHandle;
        } else {
            OutputDebugStringA("Replaybuffer mapping CreateFileA writing to file failed!\n");
        }

        LARGE_INTEGER size;
        size.QuadPart = static_cast<i64>(allState.memorySize);
        replayBuffer->memoryMap = CreateFileMappingA(replayBuffer->fileHandle, 0, PAGE_READWRITE,
                                                     size.HighPart, size.LowPart, 0);
        replayBuffer->memoryBlock =
            MapViewOfFile(replayBuffer->memoryMap, FILE_MAP_ALL_ACCESS, 0, 0, allState.memorySize);

        // NOTE: assert here to avoid asserting later when using or switch to checking with if
        ASSERT(replayBuffer->memoryBlock);
    }

    // Performance statistics
    LARGE_INTEGER freqCounter;
    QueryPerformanceFrequency(&freqCounter);
    gPerfCounterFreq = freqCounter.QuadPart;

    LARGE_INTEGER lastCounter{ win32::GetWallClock() };
    // RDTSC
    u64 lastCycleCount{ __rdtsc() };

    DWORD lastPlayCursor{};
    bool32 isSoundValid{};

    // The game represented as a DLL which allows hot reloading and more fun stuff!
    win32::GameCode game{ win32::LoadGameCode(srcDllPath, tempDllPath) };
    game::Input gameInput{};

    // Main loop
    gIsGameRunning = true;

    while (gIsGameRunning) {
        const FILETIME newDllWriteTime{ win32::GetLastWriteTime(srcDllPath) };
        if (CompareFileTime(&game.lastWritetime, &newDllWriteTime)) {
            win32::UnloadGameCode(&game);
            game = win32::LoadGameCode(srcDllPath, tempDllPath);
        }

        for (u32 i{}; i < ARRAY_COUNT(gameInput.playerInputs[0].buttons); ++i) {
            gameInput.playerInputs[0].buttons[i].halfTransitionCount = 0;
        }

        win32::ProcessPendingMessages(&gameInput, &allState);

        // Mouse input

        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(windowHandle, &mousePos);
        gameInput.mouseX = mousePos.x;
        gameInput.mouseY = mousePos.y;
        gameInput.mouseZ = 0;

        for (u32 i{}; i < ARRAY_COUNT(gameInput.mouseButtons.buttons); ++i) {
            gameInput.mouseButtons.buttons[i].halfTransitionCount = 0;
        }

        // NOTE: query these in ProcessPendingMessages so everything is in one place?
        win32::ProcessKeyboardMessage(&gameInput.mouseButtons.left,
                                      GetKeyState(VK_LBUTTON) & (1 << 15));
        win32::ProcessKeyboardMessage(&gameInput.mouseButtons.middle,
                                      GetKeyState(VK_MBUTTON) & (1 << 15));
        win32::ProcessKeyboardMessage(&gameInput.mouseButtons.right,
                                      GetKeyState(VK_RBUTTON) & (1 << 15));
        win32::ProcessKeyboardMessage(&gameInput.mouseButtons.x1,
                                      GetKeyState(VK_XBUTTON1) & (1 << 15));
        win32::ProcessKeyboardMessage(&gameInput.mouseButtons.x2,
                                      GetKeyState(VK_XBUTTON2) & (1 << 15));

        if (gIsGamePaused) {
            continue;
        }

        // Directsound
        const win32::DSoundParams dSoundParams{ win32::ProcessDSoundParams(
            &soundOutput, lastPlayCursor, isSoundValid) };

        // Call game code

        game::OffScreenBuffer screenBuff{};
        screenBuff.memory = gScreenBuff.memory;
        screenBuff.width = gScreenBuff.width;
        screenBuff.height = gScreenBuff.height;
        screenBuff.bytesPerPixel = gScreenBuff.bytesPerPixel;
        screenBuff.pitch = gScreenBuff.pitch;

        if (allState.recordingIndex) {
            win32::RecordInput(&gameInput, &allState);
        }
        if (allState.playingIndex) {
            win32::PlaybackInput(&gameInput, &allState);
        }

        ThreadContext threadContext{};

        if (game.updateAndRender) {
            game.updateAndRender(&threadContext, &gameMemory, &screenBuff, &gameInput);
        }

        game::SoundOutputBuffer soundBuff{};
        soundBuff.samplesPerSecond = soundOutput.samplesPerSecond;
        soundBuff.sampleCount = dSoundParams.bytesToWrite / soundOutput.bytesPerSample;
        soundBuff.samples = soundBuffSamples;

        if (game.getSoundSamples) {
            game.getSoundSamples(&threadContext, &gameMemory, &soundBuff);
        }

        if (isSoundValid) {
            // soundBuff now contains game generated output
            win32::FillSoundBuffer(&soundOutput, dSoundParams.byteToLock, dSoundParams.bytesToWrite,
                                   &soundBuff);
            // TODO: look up episode 20 if we want to continue fixing the audio sync
            // For now we skip fixing the issues as it is very complicated and I think I
            // wouldn't get much out of it...
        }

        // Performance
        // We only query the performance once per frame so that we don't leave out the
        // time between the frame's end and start

        LARGE_INTEGER endCounter{ win32::GetWallClock() };
        const f64 secondsElapsed{ win32::GetSecondsElapsed(lastCounter, endCounter) };
        f64 secondsElapedForFrame{ secondsElapsed };

#if 1 // NOTE: if some macro?
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
            // log the missed frame!!!
        }
#endif

        endCounter = win32::GetWallClock();
        const f64 ms{ 1000 * win32::GetSecondsElapsed(lastCounter, endCounter) };
        const f64 FPS{ 1000 / ms };

        auto wndDimension{ win32::GetWindowDimensions(windowHandle) };
        const HDC deviceContext{ GetDC(windowHandle) }; // clang-tidy NOLINT
        win32::DisplayBufferWindow(deviceContext, &gScreenBuff, wndDimension.width,
                                   wndDimension.height);
        ReleaseDC(windowHandle, deviceContext);

        DWORD playCursor;
        DWORD writeCursor;
        if (SUCCEEDED(gSecondaryBuff->GetCurrentPosition(&playCursor, &writeCursor))) {
            lastPlayCursor = playCursor;
            isSoundValid = true;
        } else {
            isSoundValid = false;
        }

        const u64 endCycleCount{ __rdtsc() };
        const f64 cycleElapsedM{ static_cast<f64>((endCycleCount - lastCycleCount)) / 1000 * 1000 };

        // True timing before sleep
        //const f64 ms{ 1000.0 * secondsElapsed };
        //const f64 FPS{ 1000 / ms };
        char buf[64]; // yikes...
        sprintf_s(buf, "frame: %.5f ms | FPS: %.2f | cycles: %.4f M\n", ms, FPS, cycleElapsedM);
#if 0 // NOTE: if some macro?
        OutputDebugStringA(buf);
#endif

        lastCycleCount = endCycleCount;

        const LARGE_INTEGER resetCounter{ win32::GetWallClock() };
        lastCounter = resetCounter;
    }

    return 0;
}
