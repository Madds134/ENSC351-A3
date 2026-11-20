// Live test for the rotary encoder and its push button.
//
// One thread decodes knob steps, another detects button presses, and the
// main thread redraws a status line 20 times a second showing:
//   - position (net detents turned: CW adds 1, CCW subtracts 1)
//   - raw levels of channels A and B and of the button
//   - number of (debounced) button presses
// Press Ctrl+C to quit.

#include "hal/rotary_button.h"
#include "hal/rotary_encoder.h"
#include "hal/timing.h"

#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#define WAIT_TIMEOUT_MS 100   // lets the threads notice Ctrl+C
#define DISPLAY_PERIOD_MS 50

static atomic_bool running = true;
static atomic_int position = 0;
static atomic_int stepsCw = 0;
static atomic_int stepsCcw = 0;
static atomic_int presses = 0;

static void onSigint(int signal)
{
    (void)signal;
    running = false;
}

static void *encoderThread(void *arg)
{
    (void)arg;
    while (running) {
        int step = 0;
        int result = Rotary_waitStep(WAIT_TIMEOUT_MS, &step);
        if (result < 0) {
            fprintf(stderr, "\nEncoder read error\n");
            running = false;
        } else if (result > 0) {
            position += step;
            if (step == ROTARY_STEP_CW) {
                stepsCw++;
            } else {
                stepsCcw++;
            }
        }
    }
    return NULL;
}

static void *buttonThread(void *arg)
{
    (void)arg;
    while (running) {
        int result = RotaryButton_waitPress(WAIT_TIMEOUT_MS);
        if (result < 0) {
            fprintf(stderr, "\nButton read error\n");
            running = false;
        } else if (result > 0) {
            presses++;
        }
    }
    return NULL;
}

static void printStatus(void)
{
    int a = -1;
    int b = -1;
    Rotary_readLevels(&a, &b);
    bool down = RotaryButton_isPressed();

    // \r and the trailing spaces redraw the same line in place
    printf("\rPosition: %+5d  (CW %4d, CCW %4d) | A=%d B=%d | Button: %-4s | Presses: %4d   ",
           position, stepsCw, stepsCcw, a, b, down ? "DOWN" : "up", presses);
    fflush(stdout);
}

int main(void)
{
    if (Rotary_init() != 0) {
        fprintf(stderr, "Failed to initialize the rotary encoder (GPIO23/GPIO24)\n");
        return 1;
    }
    if (RotaryButton_init() != 0) {
        fprintf(stderr, "Failed to initialize the encoder button (GPIO4)\n");
        Rotary_cleanup();
        return 1;
    }

    struct sigaction action = {0};
    action.sa_handler = onSigint;  // no SA_RESTART, so blocked waits wake up
    sigaction(SIGINT, &action, NULL);

    printf("Encoder test: turn the knob and press it. Ctrl+C to quit.\n");

    pthread_t encoder;
    pthread_t button;
    pthread_create(&encoder, NULL, encoderThread, NULL);
    pthread_create(&button, NULL, buttonThread, NULL);

    while (running) {
        printStatus();
        Timing_sleepMs(DISPLAY_PERIOD_MS);
    }

    pthread_join(encoder, NULL);
    pthread_join(button, NULL);
    printStatus();
    printf("\nDone\n");

    RotaryButton_cleanup();
    Rotary_cleanup();
    return 0;
}
