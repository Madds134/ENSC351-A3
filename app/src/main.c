// Beat-box: plays drum beats, controlled from the web page over UDP.
// Initializes all modules, waits for the "stop" command, then cleans up
// in reverse order.

#include "stop.h"
#include "beat_player.h"
#include "encoder_control.h"
#include "UDP.h"

#include <stdio.h>

int main(void)
{
    printf("Beat-box starting...\n");

    Stop_init();
    BeatPlayer_init();
    EncoderControl_init();
    UDP_init();

    Stop_waitForRequest();

    UDP_cleanup();
    EncoderControl_cleanup();
    BeatPlayer_cleanup();
    Stop_cleanup();

    printf("Beat-box done.\n");
    return 0;
}
