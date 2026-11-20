/**
 * @file UDP.h
 * @author Madden Wynans
 * @brief UDP server on port 12345 for the beat-box web page
 * @details Commands: "mode N", "volume N", "tempo N", "play N", "stop".
 *          If N is a number it is applied; the reply is always the current
 *          value, so "volume undefined" is a query.
 * @version 0.1
 * @date 2026-10-28
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef UDP_H
#define UDP_H

/**
 * @brief Opens the UDP socket and starts the server
 */
void UDP_init(void);

/**
 * @brief Closes the UDP socket after the server thread finishes
 */
void UDP_cleanup(void);



#endif