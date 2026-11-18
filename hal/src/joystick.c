/**
 * @file joystick.c
 * @author Madden Wynans
 * @copyright Copyright (c) 2026
 */

#include "hal/joystick.h"
#include "hal/adc.h"
#include <stdlib.h>

#define JOYSTICK_X_DIRECTION 0
#define JOYSTICK_Y_DIRECTION 1

void Joystick_init(void) {
    ADC_init();
}

void Joystick_cleanup(void) {
    ADC_cleanup();
}

JoystickDirection Joystick_get_direction(void) {
    uint16_t x = ADC_read_channel(JOYSTICK_X_DIRECTION);
    uint16_t y = ADC_read_channel(JOYSTICK_Y_DIRECTION);

    // UP
    if (x < 2200 && y > 3000) {
        return UP;
    }
    // RIGHT
    else if(x > 3500 && y < 2200 ) {
        return RIGHT;
    }
    // LEFT
    else if(x < 500 && y < 2200 && y > 1000) {
        return LEFT;
    }
    // DOWN
    else if(x > 500 && y < 500) {
        return DOWN;
    }
    // CENTRE
    return CENTRE;
}
