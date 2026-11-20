/**
 * @file input.h
 * @author Madden Wynans
 * @brief Reads the joystick value with a thread
 * @details Joyticks changes the volume by 5 in the y direction. Press and hold repeats
 * @date 2026-11-20
 * @copyright Copyright (c) 2026
 */
#ifndef INPUT_H
#define INPUT_H

/**
 * @brief Opens the ADC and starts the input thread
 * @warning BeatPlayer_init() must be called first
 */
void Input_init(void);

/**
 * @brief Stops the input thread and closes ADC conncetion
 */
void Input_cleanup(void);

#endif