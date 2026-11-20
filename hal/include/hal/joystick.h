/**
 * @file joystick.h
 * @author Madden Wynans
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

typedef enum {
    JOYSTICK_CENTRE,
    JOYSTICK_UP,
    JOYSTICK_DOWN
} JoystickDirection;

/**
 * @brief Gets the current joystick direction (Y direction on)
 */
JoystickDirection Joystick_getDirection(void);

#endif