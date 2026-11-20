/**
 * @file reporter.h
 * @author Madden Wynans
 * @brief Prints the beat-box status once per second (uses a thread)
 * @details Mode, tempo, volume, and the timing of audio buffer refills and
 *          accelerometer samples over the previous second
 * @version 0.1
 * @date 2026-10-28
 *  * @copyright Copyright (c) 2026
 */
#ifndef REPORTER_H
#define REPORTER_H

/**
 * @brief Starts the reporting thread
 * @warning Period_init() and BeatPlayer_init() must be called first
 */
void Reporter_init(void);

/**
 * @brief Stops the reporting thread
 */
void Reporter_cleanup(void);

#endif