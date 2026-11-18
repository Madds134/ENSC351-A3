/**
 * @file joystick.h
 * @author Madden Wynans
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

typedef enum {
    CENTRE,
    UP,
    DOWN,
    LEFT,
    RIGHT
} JoystickDirection;

/**
 * @brief Initializes the joystick interface
 */
void Joystick_init(void);

/**
 * @brief Cleans up the joystick interface 
 */
void Joystick_cleanup(void);

/**
 * @brief Gets the current joystick direction
 */
JoystickDirection Joystick_get_direction(void);

#endif