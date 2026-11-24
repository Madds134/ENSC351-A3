/**
 * @file beat_player.h
 * @brief Plays drum beats in the background (uses a thread)
 * @details Loads the drum sounds, and plays the selected beat at the selected
 *          tempo. Other modules can also play any single drum sound.
 */
#ifndef BEAT_PLAYER_H
#define BEAT_PLAYER_H

// Beat modes
typedef enum {
    BEAT_MODE_NONE = 0,
    BEAT_MODE_ROCK,
    BEAT_MODE_CUSTOM,
    NUM_BEAT_MODES
} BeatMode;

// Drum sounds
typedef enum {
    DRUM_BASE = 0,
    DRUM_HIHAT,
    DRUM_SNARE,
    DRUM_HIHAT_OPEN,
    NUM_DRUM_SOUNDS
} DrumSound;

#define BEAT_TEMPO_MIN 40
#define BEAT_TEMPO_MAX 300
#define BEAT_TEMPO_DEFAULT 120

/**
 * @brief Starts the audio mixer, loads the drum sounds, and starts the beat thread
 */
void BeatPlayer_init(void);

/**
 * @brief Plays one drum sound
 * @param sound Drum to play
 */
void BeatPlayer_playSound(DrumSound sound);

/**
 * @brief Selects the beat to play
 * @param mode Beat mode
 */
void BeatPlayer_setMode(BeatMode mode);

/**
 * @brief Moves to the next mode
 */
void BeatPlayer_cycleMode(void);

/**
 * @brief Gets the current beat mode
 * @return The current mode
 */
BeatMode BeatPlayer_getMode(void);

/**
 * @brief Sets the tempo
 * @param bpm Beats per minute
 */
void BeatPlayer_setTempo(int bpm);

/**
 * @brief Gets the current tempo
 * @return Beats per minute
 */
int BeatPlayer_getTempo(void);

/**
 * @brief Stops the beat thread and the audio mixer
 */
void BeatPlayer_cleanup(void);

#endif
