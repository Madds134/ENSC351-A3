#include "input.h"
#include "audioMixer.h"
#include "hal/adc.h"
#include "hal/joystick.h"
#include "hal/timing.h"
#include "beat_player.h"
#include "periodTimer.h"
#include "hal/accelerometer.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POLL_INTERVAL_MS 10
#define VOLUME_STEP 5
#define VOLUME_MIN 0
#define HOLD_DELAY_MS 500 // how long to hold before repeat
#define REPEAT_INTERVAL_MS 250 // time between repeats
#define BASELINE_SAMPLES 50 // 500 ms of readings
#define DRUM_HIT_THRESHOLD 400 // distance from rest for a detected "hit"
#define REARM_THRESHOLD 200 // Hysteresis: must return this close to rest before the next hit
#define DRUM_HIT_LOCKOUT_MS 250 // ignore axis after it plays

static pthread_t inputThread;
static atomic_bool isStopping = false;
static bool isInitialized = false;

// Joystick state 
static JoystickDirection heldDirection = JOYSTICK_CENTRE;
static long long nextRepeatMs = 0;

/**
 * @brief One air drum axis
 */
typedef struct {
    XL_Axis axis;
    DrumSound sound;
    int threshold; // distance from baseline for detecting a hit
    int baseline; // reading at rest
    bool isArmed; // hysteresis flag: false after a hit until back near rest
    long long lastHitMs; // time of the last hit
} AirDrum;

// Air-drum axes 
static AirDrum airDrums[NUM_XL_AXES] = {
    {.axis = XL_AXIS_X, .sound = DRUM_SNARE, .threshold = DRUM_HIT_THRESHOLD, .isArmed = true},
    {.axis = XL_AXIS_Y, .sound = DRUM_HIHAT, .threshold = DRUM_HIT_THRESHOLD, .isArmed = true},
    {.axis = XL_AXIS_Z, .sound = DRUM_BASE,  .threshold = DRUM_HIT_THRESHOLD, .isArmed = true},
};

/**
 * @brief Measure each axis's resting reading
 */
static void calibrateAirDrums(void)
{
    long long sums[NUM_XL_AXES] = {0};
    int counts[NUM_XL_AXES] = {0};

    for(int sample = 0; sample < BASELINE_SAMPLES; sample++) {
        for(int i = 0; i < NUM_XL_AXES; i++) {
            int reading = Accelerometer_readAxis(airDrums[i].axis);
            if(reading != ADC_READ_ERROR) {
                sums[i] += reading;
                counts[i]++;
            }
        }
        Timing_sleepMs(POLL_INTERVAL_MS);
    }
    for(int i = 0; i < NUM_XL_AXES; i++) {
        if (counts[i] > 0) {
            airDrums[i].baseline = (int)(sums[i] / counts[i]);
        }
    }
}

/**
 * @brief Plays an axis's sound when it moves past threshold from rest
 * @details Dobounced by how often can make drum hit and the axis must return near rest after one hit
 * @param drum Axis to check
 * @param nowMs Current time in ms
 */
static void handleAirDrum(AirDrum *drum, long long nowMs)
{
    int reading = Accelerometer_readAxis(drum->axis);
    if(reading == ADC_READ_ERROR) {
        return;
    }
    
    int distance = abs(reading - drum->baseline);
    if(!drum->isArmed && distance < REARM_THRESHOLD) {
        drum->isArmed = true;
    }

    bool isLockedOut = (nowMs - drum->lastHitMs) < DRUM_HIT_LOCKOUT_MS;
    if(drum->isArmed && !isLockedOut && distance > drum->threshold){
        BeatPlayer_playSound(drum->sound);
        drum->isArmed = false;
        drum->lastHitMs = nowMs;
    }
}

/**
 * @brief Check every axis for a drum hit and records time
 * @param nowMs Current time in ms
 */
static void handleAccelerometer(long long nowMs)
{
    for (int i = 0; i < NUM_XL_AXES; i++) {
        handleAirDrum(&airDrums[i], nowMs);
    }
    Period_markEvent(PERIOD_EVENT_XL_SAMPLE);
}

/**
 * @brief Raises or lowers the volume by one step, limited to [VOLUME_MIN, AUDIOMIXER_MAX_VOLUME]
 * @param direction JOYSTICK_UP to raise, JOYSTICK_DOWN to lower
 */
static void stepVolume(JoystickDirection direction)
{
    int step = (direction == JOYSTICK_UP) ? VOLUME_STEP : -VOLUME_STEP;
    int volume = AudioMixer_getVolume() + step;
    if (volume > AUDIOMIXER_MAX_VOLUME) {
        volume = AUDIOMIXER_MAX_VOLUME;
    }
    if (volume < VOLUME_MIN) {
        volume = VOLUME_MIN;
    }
    AudioMixer_setVolume(volume);
}

/**
 * @brief Changes the volume once per press, repeatedly while held
 * @param nowMs Current time in milliseconds
 */
static void handleJoystick(long long nowMs)
{
    JoystickDirection direction = Joystick_getDirection();
    if (direction == JOYSTICK_CENTRE) { // released the hold
        heldDirection = JOYSTICK_CENTRE; 
        return;
    }

    if (direction != heldDirection) { // new press/hold
        stepVolume(direction);
        heldDirection = direction;
        nextRepeatMs = nowMs + HOLD_DELAY_MS;
    } else if (nowMs >= nextRepeatMs) {     // held long enough to repeat
        stepVolume(direction);
        nextRepeatMs = nowMs + REPEAT_INTERVAL_MS;
    }
}

/**
 * @brief Polls the inputs every POLL_INTERVAL_MS until stopped
 */
static void *inputThreadFn(void *arg)
{
    (void)arg;
    printf("Calibrating accelerometer\n");
    calibrateAirDrums();

    while (!atomic_load(&isStopping)) {
        long long nowMs = Timing_getTimeMs();
        handleJoystick(nowMs);
        handleAccelerometer(nowMs);
        Timing_sleepMs(POLL_INTERVAL_MS);
    }
    return NULL;
}

void Input_init(void)
{
    if (isInitialized) {
        return;
    }
    ADC_init();

    atomic_store(&isStopping, false);
    int err = pthread_create(&inputThread, NULL, inputThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create input thread: %s\n", strerror(err));
        ADC_cleanup();
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void Input_cleanup(void)
{
    if (!isInitialized) {
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(inputThread, NULL);
    ADC_cleanup();
    isInitialized = false;
}