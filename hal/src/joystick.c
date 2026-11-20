/**
 * @file joystick.c
 * @author Madden Wynans
 * @copyright Copyright (c) 2026
 */

#include "hal/joystick.h"
#include "hal/adc.h"

#define JOYSTICK_Y_CHANNEL 3
#define JOYSTICK_UP_THRESHOLD 3000
#define JOYSTICK_DOWN_THRESHOLD 1000

JoystickDirection Joystick_getDirection(void)
{
    int y = ADC_read_channel(JOYSTICK_Y_CHANNEL);
    if (y == ADC_READ_ERROR) {
        return JOYSTICK_CENTRE;
    }
    if (y > JOYSTICK_UP_THRESHOLD) {
        return JOYSTICK_UP;
    }
    if (y < JOYSTICK_DOWN_THRESHOLD) {
        return JOYSTICK_DOWN;
    }
    return JOYSTICK_CENTRE;
}
