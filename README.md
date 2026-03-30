# HandmadeHero

My implementation of the code as I followed Casey Muratori's [Handmade Hero](https://guide.handmadehero.org/) playlist

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

Visual Studio 2022+ (or build tools) Also earlier versions might work

- Open x64 Native Tools Command Prompt for Visual Studio 2022
- Navigate to the project root
- run build_cli.bat which puts the executable into build folder

If using other shells: modify the setup_env.bat script to have vcvarsall.bat path to your Visual Studio installation before running build_cli.bat. This varies between Visual Studio installations
- run setup_env.bat to initialize x64 environment for MSVC and then build according to the instructions above
