/**
 * @file gpio_init.h
 * @brief Requests GPIO lines through libgpiod
 * @details Opens a GPIO chip and requests a group of lines as inputs (optionally with
 *          edge events and debouncing) or outputs. The returned handle is used
 *          with gpio_read.h and gpio_write.h.
 */
#ifndef GPIO_INIT_H
#define GPIO_INIT_H

#include <gpiod.h>
#include <stddef.h>

/**
 * @brief Initializes a group of GPIO pins
 *
 * @param line_offsets array of GPIO line numbers
 * @param num_lines    number of GPIO lines in the line_offsets array
 * @param direction    sets I/O direction for all pins in line_offsets array.
 *                     Possible values are : GPIOD_LINE_DIRECTION_AS_IS
 *                                           GPIOD_LINE_DIRECTION_INPUT
 *                                           GPIOD_LINE_DIRECTION_OUTPUT
 * @param edge         which edges generate events (read them with GPIO_WaitEdges).
 *                     Only valid with GPIOD_LINE_DIRECTION_INPUT.
 *                     Possible values are : GPIOD_LINE_EDGE_NONE (no events)
 *                                           GPIOD_LINE_EDGE_RISING
 *                                           GPIOD_LINE_EDGE_FALLING
 *                                           GPIOD_LINE_EDGE_BOTH
 * @param debounce_us  debounce period in microseconds for input pins
 *                     (0 disables debouncing)
 * @return struct gpiod_line_request* Handle for accessing GPIO pins,
 *         or NULL on failure
 */
struct gpiod_line_request *GPIO_Init(
    const char *chip_path,
    unsigned int *line_offsets,
    size_t num_lines,
    enum gpiod_line_direction direction,
    enum gpiod_line_edge edge,
    unsigned long debounce_us);

/**
 * @brief Finds which GPIO chip and line offset carry a named line
 *
 * Searches every /dev/gpiochipN for a line with the given name. On the
 * BeagleY-AI, header pins are named after their Raspberry Pi GPIO number
 * (e.g. "GPIO4" is header pin 7), the same names `gpiofind` uses.
 *
 * @param line_name     line name to look for, e.g. "GPIO4"
 * @param chip_path     filled in with the chip's path, e.g. "/dev/gpiochip2"
 * @param chip_path_len size of the chip_path buffer
 * @param line_offset   filled in with the line's offset on that chip
 * @return int 0 on success, -1 if no chip has a line with that name
 */
int GPIO_FindLine(
    const char *line_name,
    char *chip_path,
    size_t chip_path_len,
    unsigned int *line_offset);

#endif
