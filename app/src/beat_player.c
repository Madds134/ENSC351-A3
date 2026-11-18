#include "beat_player.h"
#include "audioMixer.h"
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Sounds
#define WAV_DIR "beatbox-wav-files/"

// Wave file for each drum sound
static const char *SOUND_FILES[NUM_DRUM_SOUNDS] = {
    [DRUM_BASE]       = WAV_DIR "100051__menegass__gui-drum-bd-hard.wav",
    [DRUM_HIHAT]      = WAV_DIR "100053__menegass__gui-drum-cc.wav",
    [DRUM_SNARE]      = WAV_DIR "100059__menegass__gui-drum-snare-soft.wav",
    [DRUM_HIHAT_OPEN] = WAV_DIR "100055__menegass__gui-drum-co.wav",
};

static wavedata_t sounds[NUM_DRUM_SOUNDS];

/**
 * @brief Reads every drum's wave file into memory
 */
static void loadSounds(void)
{
    for (int i = 0; i < NUM_DRUM_SOUNDS; i++) {
        AudioMixer_readWaveFileIntoMemory(SOUND_FILES[i], &sounds[i]);
    }
}

/**
 * @brief Frees every drum's wave data
 * @warning The mixer must be stopped first, or it will still be playing them
 */
static void freeSounds(void)
{
    for(int i = 0; i < NUM_DRUM_SOUNDS; i++) {
        AudioMixer_freeWaveFileData(&sounds[i]);
    }
}

// Beat patterns
#define HIT(drum) (1u << (drum)) // bit flag for one drum
#define NO_HITS 0u

// Steps per beat
#define EIGHTH_NOTES 2     
#define SIXTEENTH_NOTES 4

#define ARRAY_LENGTH(array) ((int)(sizeof(array) / sizeof((array)[0])))

/**
 * @brief One drum beat: the drums to hit at each step of a measure
 */
typedef struct {
    const unsigned int *steps;   // HIT() flags for each step
    int numSteps;                // steps in one measure
    int stepsPerBeat;            // EIGHTH_NOTES or SIXTEENTH_NOTES
} BeatPattern;

// A single silent step
static const unsigned int NONE_STEPS[] = {
    NO_HITS,
};

// Standard rock beat in eighth notes
static const unsigned int ROCK_STEPS[] = {
    HIT(DRUM_HIHAT) | HIT(DRUM_BASE),    // 1
    HIT(DRUM_HIHAT),                     // 1.5
    HIT(DRUM_HIHAT) | HIT(DRUM_SNARE),   // 2
    HIT(DRUM_HIHAT),                     // 2.5
    HIT(DRUM_HIHAT) | HIT(DRUM_BASE),    // 3
    HIT(DRUM_HIHAT),                     // 3.5
    HIT(DRUM_HIHAT) | HIT(DRUM_SNARE),   // 4
    HIT(DRUM_HIHAT),                     // 4.5
};

// Custom boom-bap (90s hip-hop) beat, in sixteenth notes: syncopated kicks
// (just before 3, and on the "and" of 3), snare on 2 and 4, and an open
// hi-hat on the "and" of 4
static const unsigned int BOOM_BAP_STEPS[] = {
    HIT(DRUM_HIHAT) | HIT(DRUM_BASE),     // 1
    NO_HITS,                              // 1 e
    HIT(DRUM_HIHAT),                      // 1 +
    NO_HITS,                              // 1 a
    HIT(DRUM_HIHAT) | HIT(DRUM_SNARE),    // 2
    NO_HITS,                              // 2 e
    HIT(DRUM_HIHAT),                      // 2 +
    HIT(DRUM_BASE),                       // 2 a
    HIT(DRUM_HIHAT),                      // 3
    NO_HITS,                              // 3 e
    HIT(DRUM_HIHAT) | HIT(DRUM_BASE),     // 3 +
    NO_HITS,                              // 3 a
    HIT(DRUM_HIHAT) | HIT(DRUM_SNARE),    // 4
    NO_HITS,                              // 4 e
    HIT(DRUM_HIHAT_OPEN),                 // 4 +
    NO_HITS,                              // 4 a
};

// Pattern for each mode 
static const BeatPattern PATTERNS[NUM_BEAT_MODES] = {
    [BEAT_MODE_NONE]   = {NONE_STEPS,     ARRAY_LENGTH(NONE_STEPS),     EIGHTH_NOTES},
    [BEAT_MODE_ROCK]   = {ROCK_STEPS,     ARRAY_LENGTH(ROCK_STEPS),     EIGHTH_NOTES},
    [BEAT_MODE_CUSTOM] = {BOOM_BAP_STEPS, ARRAY_LENGTH(BOOM_BAP_STEPS), SIXTEENTH_NOTES},
};

/* State */
static pthread_mutex_t stateLock = PTHREAD_MUTEX_INITIALIZER;
static BeatMode currentMode = BEAT_MODE_ROCK;
static int currentTempo = BEAT_TEMPO_DEFAULT;
static bool isModeChanged = false; // Restarts the beat thread

/**
 * @brief Limits a tempo to BEAT_TEMPO_MIN or BEAT_TEMPO_MAX
 */
static int clampTempo(int bpm)
{
    if (bpm < BEAT_TEMPO_MIN) {
        return BEAT_TEMPO_MIN;
    }
    if (bpm > BEAT_TEMPO_MAX) {
        return BEAT_TEMPO_MAX;
    }
    return bpm;
}

void BeatPlayer_playSound(DrumSound sound)
{
    int index = (int)sound;
    if (index < 0 || index >= NUM_DRUM_SOUNDS) {
        return;
    }
    AudioMixer_queueSound(&sounds[index]);
}

void BeatPlayer_setMode(BeatMode mode)
{
    int index = (int)mode;
    if (index < 0 || index >= NUM_BEAT_MODES) {
        return;
    }
    pthread_mutex_lock(&stateLock);
    currentMode = mode;
    isModeChanged = true;
    pthread_mutex_unlock(&stateLock);
}

void BeatPlayer_cycleMode(void)
{
    pthread_mutex_lock(&stateLock);
    currentMode = (currentMode + 1) % NUM_BEAT_MODES;
    isModeChanged = true;
    pthread_mutex_unlock(&stateLock);
}

BeatMode BeatPlayer_getMode(void)
{
    pthread_mutex_lock(&stateLock);
    BeatMode mode = currentMode;
    pthread_mutex_unlock(&stateLock);
    return mode;
}

void BeatPlayer_setTempo(int bpm)
{
    pthread_mutex_lock(&stateLock);
    currentTempo = clampTempo(bpm);
    pthread_mutex_unlock(&stateLock);
}

int BeatPlayer_getTempo(void)
{
    pthread_mutex_lock(&stateLock);
    int bpm = currentTempo;
    pthread_mutex_unlock(&stateLock);
    return bpm;
}

/* Beat thread */
#define NS_PER_SECOND 1000000000LL
#define NS_PER_MINUTE (60 * NS_PER_SECOND)

static pthread_t beatThread;
static atomic_bool isStopping = false;
static bool isInitialized = false;

/**
 * @brief Plays every drum marked in one step of a pattern
 * @param hits HIT() flags for the step
 */
static void playStep(unsigned int hits)
{
    for (int drum = 0; drum < NUM_DRUM_SOUNDS; drum++) {
        if (hits & HIT(drum)) {
            AudioMixer_queueSound(&sounds[drum]);
        }
    }
}

/**
 * @brief Gets the time between steps for a tempo
 * @param bpm Beats per minute
 * @param stepsPerBeat EIGHTH_NOTES or SIXTEENTH_NOTES
 * @return Step duration in nanoseconds
 */
static long long stepDurationNs(int bpm, int stepsPerBeat)
{
    return NS_PER_MINUTE / bpm / stepsPerBeat;
}

/**
 * @brief Moves a timestamp forward
 * @param time Timestamp to change
 * @param ns Nanoseconds to add
 */
static void addNs(struct timespec *time, long long ns)
{
    long long totalNs = time->tv_nsec + ns;
    time->tv_sec += totalNs / NS_PER_SECOND;
    time->tv_nsec = totalNs % NS_PER_SECOND;
}

/**
 * @brief Reads the mode and tempo for the next step
 * @details If the mode changed, restarts the pattern at its first step
 * @param mode Set to the current mode
 * @param bpm Set to the current tempo
 * @param stepIndex Set to 0 if the mode changed, otherwise unchanged
 */
static void readState(BeatMode *mode, int *bpm, int *stepIndex)
{
    pthread_mutex_lock(&stateLock);
    *mode = currentMode;
    *bpm = currentTempo;
    if (isModeChanged) {
        *stepIndex = 0;
        isModeChanged = false;
    }
    pthread_mutex_unlock(&stateLock);
}

/**
 * @brief Plays the current pattern one step at a time until stopped
 * @details Sleeps until an absolute time for each step, so time spent
 *          queueing sounds does not add up and make the beat drift.
 */
static void *beatThreadFn(void *arg)
{
    (void)arg;
    struct timespec nextStepTime;
    clock_gettime(CLOCK_MONOTONIC, &nextStepTime);
    int stepIndex = 0;

    while (!atomic_load(&isStopping)) {
        BeatMode mode;
        int bpm;
        readState(&mode, &bpm, &stepIndex);
        const BeatPattern *pattern = &PATTERNS[mode];

        playStep(pattern->steps[stepIndex]);
        stepIndex = (stepIndex + 1) % pattern->numSteps;

        addNs(&nextStepTime, stepDurationNs(bpm, pattern->stepsPerBeat));
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nextStepTime, NULL);
    }
    return NULL;
}

/* Init/cleanup */
void BeatPlayer_init(void)
{
    if (isInitialized) {
        return;
    }
    AudioMixer_init();
    loadSounds();

    atomic_store(&isStopping, false);
    int err = pthread_create(&beatThread, NULL, beatThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create beat thread: %s\n", strerror(err));
        AudioMixer_cleanup();
        freeSounds();
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void BeatPlayer_cleanup(void)
{
    if (!isInitialized) {
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(beatThread, NULL);

    AudioMixer_cleanup();   // stop playback before freeing what it plays
    freeSounds();
    isInitialized = false;
}


