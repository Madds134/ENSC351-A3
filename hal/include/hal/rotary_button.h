/**
 * @file rotary_button.h
 * @brief HAL for the rotary encoder's push button on GPIO4 (header pin 7)
 * @details Pressing the knob pulls GPIO4 low (an internal pull-up holds it
 *          high otherwise). Presses are debounced in software: a change in
 *          level only counts once the pin has stayed at the new level for
 *          the debounce period.
 */
#ifndef ROTARY_BUTTON_H
#define ROTARY_BUTTON_H

#include <stdbool.h>

/**
 * @brief Requests the button's GPIO line and starts watching for edges
 * @return 0 on success, -1 on failure
 */
int RotaryButton_init(void);

/**
 * @brief Releases the button's GPIO line
 */
void RotaryButton_cleanup(void);

/**
 * @brief Blocks until the button is pressed (debounced), or the timeout expires
 *
 * A press is reported once, when the button goes down. Holding it does not
 * repeat, and releasing it is not reported.
 *
 * @param timeoutMs How long to wait for an edge (negative waits forever)
 * @return 1 if a press was detected, 0 on timeout, -1 on error
 * @warning RotaryButton_init() must be called first. Only one thread should
 *          call this function.
 */
int RotaryButton_waitPress(int timeoutMs);

/**
 * @brief Reads the raw (not debounced) button level right now
 * @return true if the button is currently held down
 * @note Safe to call from another thread while RotaryButton_waitPress() is blocked
 */
bool RotaryButton_isPressed(void);

#endif
