#include "reporter.h"
#include "periodTimer.h"
#include "beat_player.h"
#include "audioMixer.h"
#include "hal/timing.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPORT_INTERVAL_MS 1000

static pthread_t reporterThread;
static atomic_bool isStopping = false;
static bool isInitialized = false;

/**
 * @brief Prints the beat-box status and the timing of the last second's events
 * @details One printf, so the line can't be split by another thread's output
 */
static void report_one_second(void)
{
    Period_statistics_t audio;
    Period_statistics_t accel;
    Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO_REFILL, &audio);
    Period_getStatisticsAndClear(PERIOD_EVENT_XL_SAMPLE, &accel);

    printf("M%d %dbpm vol:%d   Audio[%.3f, %.3f] avg %.3f/%d   Accel[%.3f, %.3f] avg %.3f/%d\n",
           BeatPlayer_getMode(), BeatPlayer_getTempo(), AudioMixer_getVolume(),
           audio.minPeriodInMs, audio.maxPeriodInMs, audio.avgPeriodInMs, audio.numSamples,
           accel.minPeriodInMs, accel.maxPeriodInMs, accel.avgPeriodInMs, accel.numSamples);
}

/* Reporter thread */
static void *reporterThreadFn(void *arg)
{
    (void)arg;
    while (!atomic_load(&isStopping)) {
        Timing_sleepMs(REPORT_INTERVAL_MS);
        report_one_second();
    }
    return NULL;
}

/* Init/cleanup */
void Reporter_init(void)
{
    if(isInitialized) {
        return;
    }
    atomic_store(&isStopping, false);
    int err = pthread_create(&reporterThread, NULL, reporterThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create reporter thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void Reporter_cleanup(void)
{
    if(!isInitialized){
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(reporterThread, NULL);
    isInitialized = false;
}
