# HandmadeHero

My implementation of the code as I followed Casey Muratori's [Handmade Hero](https://guide.handmadehero.org/) playlist

## Extra features

- Custom amount of replay buffers and can switch between them. Press 1, 2, 3 or 4 while not recording to switch between a buffer to playback input from that buffer. If there was no input recorded yet the buffer is now selected for recording to. Press shift + number to start recording in the numbered buffer overwriting possible previous recording. Press L to start recording in the selected buffer also overwriting. Shift + L to enable/disable replay looping.
- Idea: a button (shift + P?) to advance game by 1 frame at a time when paused

- Input checking with a simple API:
    - is action just pressed (for the first frame the key was pressed)
    - is action pressed (continuous, also for the first frame)
    - is action just released (last frame the key was held)
