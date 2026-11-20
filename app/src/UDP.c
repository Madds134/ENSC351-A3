#include "UDP.h"
#include "stop.h"
#include "beat_player.h"
#include "audioMixer.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

#define UDP_PORT 12345
#define DEFAULT_PROTOCOL 0
#define MAX_PACKET_SIZE 1500
#define NO_FLAGS 0
#define REPLY_BUFFER_SIZE 256
#define WORD_SEPARATORS " \t\r\n"
#define UNKNOWN_COMMAND "Unknown command.\n"
#define DECIMAL_BASE 10
#define PLAY_OUTPUT "played"
static int socketFd = -1;
static pthread_t udpThread;
static bool isInitialized = false;

/**
 * @brief Creates the UDP socket and binds it to UDP_PORT
 * @note Claude and lecture slices were used to help with syntax needed to open socket
 */
static void openSocket(void)
{
    socketFd = socket(AF_INET, SOCK_DGRAM, DEFAULT_PROTOCOL);
    if (socketFd < 0) {
        perror("Unable to create UDP socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(UDP_PORT);

    if (bind(socketFd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        perror("Unable to bind UDP port");
        close(socketFd);
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Sends a '\0' terminated text reply back to client
 * 
 * @param message text to send
 * @param sender address of the client that sent the command
 * @note Claude was used to assist in the syntax needed to make this
 */
static void sendReply(const char *message, const struct sockaddr_in *sender)
{
    sendto(socketFd, message, strlen(message), NO_FLAGS, 
            (const struct sockaddr *)sender, sizeof(*sender));
}

/**
 * @brief Sends a number as the reply
 * @details The web page shows the reply directly, so it must be just the number
 * @param value Number to send
 * @param sender Address of the client that sent the command
 */
static void sendNumber(int value, const struct sockaddr_in *sender) {
    char reply[REPLY_BUFFER_SIZE];
    snprintf(reply, sizeof(reply), "%d", value);
    sendReply(reply, sender);
}

/**
 * @brief Reads a whole number from a command's argument
 * @details Leading spaces are skipped. The web page polls with "undefined",
 *          which is not a number, so it returns false for a pure query.
 * @param arg Text after the command word (may be empty or "undefined")
 * @param value Set to the number, only if one was found
 * @return true if arg starts with a number, false otherwise
 */
static bool parseNumber(const char *arg, int *value)
{
    char *end;
    long number = strtol(arg, &end, DECIMAL_BASE);
    if(end == arg) {
        return false;
    }
    *value = (int)number;
    return true;
}

/* Command handlers for replies */
/**
 * @brief Function signature shared by every UDP command handler
 * @details Each handler applies its command (if any) and sends one reply
 * @param arg Text after the command word
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running, false to terminate it
 */
typedef bool (*CommandHandler)(const char *arg, const struct sockaddr_in *sender); // Function pointer to specific command handler

/**
 * @brief Pair command with the handler
 */
typedef struct {
    const char *name;
    CommandHandler handler;
} Command;

/**
 * @brief "mode N": sets the beat mode if N is given, then replies with the current mode
 * @param arg Text after the command word (mode number, or "undefined" to query)
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running
 */
static bool handleModeCommand(const char *arg, const struct sockaddr_in *sender)
{
    int mode;
    if(parseNumber(arg, &mode)) {
        BeatPlayer_setMode((BeatMode)mode);
    }
    sendNumber(BeatPlayer_getMode(), sender);
    return true;
}

/**
 * @brief "volume N": sets the volume if N is given, then replies with the current volume
 * @param arg Text after the command word (volume 0-100, or "undefined" to query)
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running
 */
static bool handleVolumeCommand(const char *arg, const struct sockaddr_in *sender)
{
    int volume;
    if (parseNumber(arg, &volume)) {
        AudioMixer_setVolume(volume);
    }
    sendNumber(AudioMixer_getVolume(), sender);
    return true;
}

/**
 * @brief "tempo N": sets the tempo if N is given, then replies with the current tempo
 * @param arg Text after the command word (BPM, or "undefined" to query)
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running
 */
static bool handleTempoCommand(const char *arg, const struct sockaddr_in *sender)
{
    int bpm;
    if (parseNumber(arg, &bpm)) {
        BeatPlayer_setTempo(bpm);
    }
    sendNumber(BeatPlayer_getTempo(), sender);
    return true;
}

/**
 * @brief "play N": plays drum sound N (0 = base, 1 = hi-hat, 2 = snare), then replies
 * @param arg Text after the command word (drum sound number)
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running
 */
static bool handlePlayCommand(const char *arg, const struct sockaddr_in *sender)
{
    int sound;
    if (parseNumber(arg, &sound)) {
        BeatPlayer_playSound((DrumSound)sound);
    }
    sendReply(PLAY_OUTPUT, sender);
    return true;
}

/**
 * @brief "stop": replies "Program terminating." and requests the program to stop
 * @param arg Unused
 * @param sender address of the client that sent the command
 * @return false to end receive loop
 */
static bool handleStopCommand(const char *arg, const struct sockaddr_in *sender)
{
    (void)arg;
    sendReply("Program terminating.\n", sender);
    Stop_request();
    return false;
}

/* Command LUT */
static const Command Commands[] = {
    {"mode", handleModeCommand},
    {"volume", handleVolumeCommand},
    {"tempo", handleTempoCommand},
    {"play", handlePlayCommand},
    {"stop", handleStopCommand}
};

#define NUM_COMMANDS (sizeof(Commands) / sizeof(Commands[0]))

/**
 * @brief Looks up command in LUT for first word of packet
 * 
 * @param word Start of the received command
 * @param wordLength Number of characters in first word
 * @note Claude was used to explain how the strncasecmp() function works
 * @return const Command* Pointer to macthing LUT entry, or NULL if unknown
 */
static const Command *findCommand(const char *word, size_t wordLength)
{
    for(size_t i = 0; i < NUM_COMMANDS; i++)
    {
        const char *name = Commands[i].name;

        // Check if word is correct
        bool sameLength = (strlen(name) == wordLength);

        // Case insentitivity for first word of command
        bool sameCharacters = (strncasecmp(word, name, wordLength) == 0);

        if (sameLength && sameCharacters) {
            return &Commands[i]; // Return row in LUT
        }
    }
    return NULL;
}

/**
 * @brief Runs the command in one received packet and replies to the sender
 * 
 * @param command Terminated packet text
 * @param sender address of the client that sent the command
 * @return false if server should stop, true otherwise
 */
static bool handleIncomingCommand(const char *command, const struct sockaddr_in *sender)
{
    size_t wordLength = strcspn(command, WORD_SEPARATORS);
    const Command *toRun = findCommand(command, wordLength);
    if (toRun == NULL) {
        sendReply(UNKNOWN_COMMAND, sender);
        return true;
    }
    const char *arg = command + wordLength; // Text after the command word
    return toRun->handler(arg, sender);
}

/**
 * @brief UDP server thread, receives one command per packet and replies
 * 
 * @param arg Unsued
 * @return void* 
 * @note Claude was used in syntax for creating UDP thread
 */
static void *udpThreadFn(void *arg)
{
    (void)arg;
    char command[MAX_PACKET_SIZE + 1];
    bool keepRunning = true;

    while(keepRunning) {
        struct sockaddr_in sender;
        socklen_t senderLength = sizeof(sender);
        ssize_t numBytes = recvfrom(socketFd, command, MAX_PACKET_SIZE, NO_FLAGS,
                                    (struct sockaddr *)&sender, &senderLength);
        
        if (numBytes < 0) {
            perror("UDP receive failed");
            continue;
        }
        command[numBytes] = '\0';
        keepRunning = handleIncomingCommand(command, &sender);
    }
    return NULL;
}

void UDP_init(void)
{
    if (isInitialized) {
        return;
    }
    openSocket();
    int err = pthread_create(&udpThread, NULL, udpThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create UDP thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void UDP_cleanup(void)
{
    if(!isInitialized) {
        return;
    }
    pthread_join(udpThread, NULL);
    close(socketFd);
    socketFd = -1;
    isInitialized = false;

}