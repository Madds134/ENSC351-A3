// Temporary test for the beat player: plays each mode and tempo in turn.

#include "beat_player.h"
#include "hal/timing.h"

#include <stdio.h>

#define SECTION_MS 6000
#define ONE_SECOND_MS 1000
#define FAST_TEMPO 200
#define SLOW_TEMPO 80
#define TOO_FAST_TEMPO 500
#define TOO_SLOW_TEMPO 10

static void printState(const char *label)
{
    printf("%s: mode %d, %d BPM\n", label, BeatPlayer_getMode(), BeatPlayer_getTempo());
}

int main(void)
{
    BeatPlayer_init();

    printState("Rock, default");
    Timing_sleepMs(SECTION_MS);

    BeatPlayer_setTempo(FAST_TEMPO);
    printState("Rock, fast");
    Timing_sleepMs(SECTION_MS);

    BeatPlayer_cycleMode();
    BeatPlayer_setTempo(SLOW_TEMPO);
    printState("Custom (boom-bap), slow");
    Timing_sleepMs(SECTION_MS);

    BeatPlayer_cycleMode();
    printState("None, with a snare every second");
    for (int i = 0; i < SECTION_MS / ONE_SECOND_MS; i++) {
        BeatPlayer_playSound(DRUM_SNARE);
        Timing_sleepMs(ONE_SECOND_MS);
    }

    BeatPlayer_setTempo(TOO_FAST_TEMPO);
    printState("Clamp test, asked for 500");
    BeatPlayer_setTempo(TOO_SLOW_TEMPO);
    printState("Clamp test, asked for 10");

    BeatPlayer_cleanup();
    printf("Done\n");
    return 0;
}
