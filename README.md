# HandmadeHero

My implementation of the code as I followed Casey Muratori's [Handmade Hero](https://guide.handmadehero.org/) playlist

## General architecture

Unity build, game as a service to the platform

Header files mostly contain definitions for structs, everything else lives in .cpp files

...

## Code style remarks

Global variables which are meant to be modified are prefixed with g

_ is used to prefix a variable, a function or a macro meant to be accessed or called with extra caution (somewhat meaning they are private in the object-oriented sense). This is to inform the caller that these are usually internal to the data structure, implementation etc...


...

## Hot reloading
Build the game after modifying the code to load the DLL automatically and see the changes instantly

Supported automatically when modifying game code only! All other files except platform code (prefixed e.g. win32 AND handmade_platform.h). Basically any other files not linked to the actual executable

## Extra features

- Custom amount of replay buffers and can switch between them. Press 1, 2, 3 or 4 while not recording to switch between a buffer to playback input from that buffer. If there was no input recorded yet the buffer is now selected for recording to. Press shift + number to start recording in the numbered buffer overwriting possible previous recording. Press L to start recording in the selected buffer also overwriting. Shift + L to enable/disable replay looping.
- Idea: a button (shift + P?) to advance game by 1 frame at a time when paused

- Input checking with a simple API:
    - is action just pressed (for the first frame the key was pressed)
    - is action pressed (continuous, also for the first frame)
    - is action just released (last frame the key was held)

## Building

### Windows

Visual Studio 2022+ (or build tools). Also earlier versions might work. Have to test GCC and Clang

- Clone the repository if you already haven't
- Open x64 Native Tools Command Prompt for VS <*some version*> (or similar named)
- Navigate to the project root
- run [scripts/win32_build.bat](scripts/win32_build.bat) FROM THE ROOT

```
.\scripts\win32_build.bat
```

which puts the executable into the created build folder

Run it:

```
.\build\win32_handmade.exe
```

If using other shells: modify the [scripts/setup_env.bat](scripts/setup_env.bat) script to have vcvarsall.bat path to your Visual Studio installation before running [scripts/win32_build.bat](scripts/win32_build.bat). This varies between Visual Studio installations
- run [scripts/setup_env.bat](scripts/setup_env.bat) to initialize x64 environment for MSVC and then build according to the instructions above

### Linux

Using David Gow's [Handmade Penguin](https://davidgow.net/handmadepenguin/) SDL 2 port of the platform layer. A repository which I used as a reference [SDL Handmade](https://github.com/KimJorgensen/sdl_handmade)

Heavily modified to match the code structure and style of the project. As the logic is very similar to the Windows platform layer I didn't bother to keep the same comments on the source files

---

#### Requirements

LLVM compiler (clang++) supporting C++20 (almost any version not older than ~3 years, so 16.0+)

SDL 2, installing this should be as simple as running

```
sudo apt install libsdl2-dev
```

In other cases more information can be found here: https://wiki.libsdl.org/SDL2/Installation

- Clone the repository if you already haven't
- Navigate to the project root
- run [scripts/linux_build.sh](scripts/linux_build.sh) FROM THE ROOT

```
./scripts/linux_build.sh
```

which puts the executable into the created build folder

Run it:

```
./build/linux_handmade
```
