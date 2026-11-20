#include "hal/rotary_button.h"
#include "hal/gpio_init.h"
#include "hal/gpio_read.h"

#include <gpiod.h>
#include <limits.h>
#include <stdio.h>

#define BUTTON_NAME "GPIO4"   // header pin 7; pressed pulls it low
#define LEVEL_PRESSED  0
#define LEVEL_RELEASED 1

// Kernel debounce drops real edges on this board (see rotary_encoder.c), so
// debouncing is done here instead
#define KERNEL_DEBOUNCE_US 0
#define DEBOUNCE_MS 20        // level must hold this long to count as a change
#define EVENT_BATCH_SIZE 16

static struct gpiod_line_request *request = NULL;
static unsigned int line = 0;
static int stableLevel = LEVEL_RELEASED;  // last debounced level

/**
 * @brief Throws away queued edge events without blocking
 * @return 0 if none were queued (pin was quiet), 1 if some were, -1 on error
 */
static int drainEdges(int timeoutMs)
{
    GPIO_EdgeEvent events[EVENT_BATCH_SIZE];
    int numEvents = GPIO_WaitEdges(request, timeoutMs, events, EVENT_BATCH_SIZE);
    if (numEvents < 0) {
        return -1;
    }
    return (numEvents > 0) ? 1 : 0;
}

/**
 * @brief After an edge, waits for the pin to stop bouncing, then reads it
 * @return the settled level (0 or 1), or -1 on error
 */
static int waitForSettledLevel(void)
{
    // Keep waiting until a full debounce period passes with no edges
    while (true) {
        int result = drainEdges(DEBOUNCE_MS);
        if (result < 0) {
            return -1;
        }
        if (result == 0) {
            return GPIO_Read(request, line);
        }
    }
}

int RotaryButton_init(void)
{
    if (request != NULL) {
        fprintf(stderr, "RotaryButton_init: already initialized\n");
        return -1;
    }

    char chip[PATH_MAX];
    if (GPIO_FindLine(BUTTON_NAME, chip, sizeof(chip), &line) != 0) {
        return -1;
    }

    request = GPIO_Init(chip, &line, 1, GPIOD_LINE_DIRECTION_INPUT,
                        GPIOD_LINE_EDGE_BOTH, KERNEL_DEBOUNCE_US);
    if (request == NULL) {
        return -1;
    }

    int level = GPIO_Read(request, line);
    if (level < 0) {
        RotaryButton_cleanup();
        return -1;
    }
    // If held down at start-up, the press is not reported until it is released
    // and pressed again
    stableLevel = level;
    return 0;
}

void RotaryButton_cleanup(void)
{
    if (request != NULL) {
        gpiod_line_request_release(request);
        request = NULL;
    }
}

int RotaryButton_waitPress(int timeoutMs)
{
    if (request == NULL) {
        fprintf(stderr, "RotaryButton_waitPress: not initialized\n");
        return -1;
    }

    while (true) {
        int result = drainEdges(timeoutMs);
        if (result <= 0) {
            return result;  // 0 = timeout, -1 = error
        }

        int level = waitForSettledLevel();
        if (level < 0) {
            return -1;
        }
        if (level == stableLevel) {
            continue;  // just a glitch; nothing changed
        }

        stableLevel = level;
        if (level == LEVEL_PRESSED) {
            return 1;
        }
        // A release: keep waiting for the next press
    }
}

bool RotaryButton_isPressed(void)
{
    if (request == NULL) {
        return false;
    }
    return GPIO_Read(request, line) == LEVEL_PRESSED;
}
