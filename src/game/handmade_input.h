#ifndef HANDMADE_INPUT_H
#define HANDMADE_INPUT_H

namespace hm_input {

/**
 * Used in the platform layer to register key presses
 */
INTERNAL void
ProcessInputEvent(Button* button, bool32 isDown) {
    if (button->endedDown != isDown) {
        button->endedDown = isDown;
        ++button->halfTransitionCount;
    }
}

/**
 * Used at the start of every frame in the platform layer to clear input
 */
INTERNAL void
ClearInputTransitionCounts(Input* gameInput) {
    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(gameInput->playerInputs);
         ++controllerIndex) {
        for (i32 i{}; i < ARRAY_COUNT(gameInput->playerInputs[0].buttons); ++i) {
            gameInput->playerInputs[controllerIndex].buttons[i].halfTransitionCount = 0;
        }
    }
}

/**
 * Returns true if the button was just pressed during the frame
 */
NODISCARD
INTERNAL bool32
ActionJustPressed(const Button* button) {
    const bool32 result{ button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

/**
 * Returns true if the button was pressed during the frame
 * As this returns true for the first frame as well, ActionJustPressed and ActionPressed both return
 * true for the first frame for the same button
 */
NODISCARD
INTERNAL bool32
ActionPressed(const Button* button) {
    const bool32 result{ button->endedDown };
    return result;
}

/**
 * Returns true if the button was just released during the frame
 */
NODISCARD
INTERNAL bool32
ActionReleased(const Button* button) {
    const bool32 result{ !button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

} //namespace hm_input

#endif // HANDMADE_INPUT_H
