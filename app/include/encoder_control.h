/**
 * @file encoder_control.h
 * @brief Lets the rotary encoder control the beat using two threads
 * @details Turning the knob chnages the tempo by 5 BPM. Pressing the knob cycles beat mode
 */
#ifndef ENCODER_CONTROL_H
#define ENCODER_CONTROL_H

/**
 * @brief Initializes the encoder and its button, & respective threads
 * @warning BeatPlayer_init() must be called first
 */
void EncoderControl_init(void);

/**
 * @brief Cleanups the encoder threads and releases GPIOs
 */
void EncoderControl_cleanup(void);
#endif