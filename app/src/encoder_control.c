#include "encoder_control.h"
#include "beat_player.h"
#include "hal/rotary_button.h"
#include "hal/rotary_encoder.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEMPO_STEP 5
#define WAIT_TIMEOUT_MS 100

static pthread_t turnThread;
static pthread_t pressThread;
static atomic_bool isStopping = false;
static bool isInitialized = false;

/**
 * @brief Changes the tempo by TEMPO_STEP for each knob click
 * @details ROTARY_STEP_CW = +1, ROTATRY_STEP_CCW = -1, CW speeds up, CCW speeds down
 * @param arg Parameter for Linux thread
 */
static void *turnThreadFn(void *arg)
{
    (void)arg;
    while (!atomic_load(&isStopping)) {
        int step = 0;
        int result = Rotary_waitStep(WAIT_TIMEOUT_MS, &step);
        if(result < 0) {
            fprintf(stderr, "Rotary encoder read failed\n");
            break;
        }
        if (result == 1) {
            BeatPlayer_setTempo(BeatPlayer_getTempo() + step * TEMPO_STEP);
        }
    }
    return NULL;
}

/**
 * @brief Starts a thread
 * @param thread Set to the new thread's handle
 * @param threadFn Function the thread runs
 * @param name Thread description
 */
static void startThread(pthread_t *thread, void *(*threadFn)(void *), const char *name)
{
    int err = pthread_create(thread, NULL, threadFn, NULL);
    if(err != 0) {
        fprintf(stderr, "Unable to create %s thread: %s\n", name, strerror(err));
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Moves to the next beat mode for each knob press until stopped
 * @param arg unusued
 */
static void *pressThreadFn(void *arg)
{
    (void)arg;
    while (!atomic_load(&isStopping)) {
        int result = RotaryButton_waitPress(WAIT_TIMEOUT_MS);
        if (result < 0) {
            fprintf(stderr, "Encoder button read failed; mode button disabled\n");
            break;
        }
        if (result == 1) {
            BeatPlayer_cycleMode();
        }
    }
    return NULL;
}

void EncoderControl_init(void)
{
    if (isInitialized) {
        return;
    }
    if (Rotary_init() != 0) {
        fprintf(stderr, "Unable to initialize rotary encoder\n");
        exit(EXIT_FAILURE);
    }
    if (RotaryButton_init() != 0) {
        fprintf(stderr, "Unable to initialize encoder button\n");
        Rotary_cleanup();
        exit(EXIT_FAILURE);
    }

    atomic_store(&isStopping, false);
    startThread(&turnThread, turnThreadFn, "encoder turn");
    startThread(&pressThread, pressThreadFn, "encoder button");
    isInitialized = true;
}

void EncoderControl_cleanup(void)
{
    if (!isInitialized) {
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(turnThread, NULL);
    pthread_join(pressThread, NULL);

    RotaryButton_cleanup();
    Rotary_cleanup();
    isInitialized = false;
}