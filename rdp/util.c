/**
 * @author ejrbuss
 * @date 2017
 *
 * This file contains extraneous utility functions.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "util.h"

static int DEBUG = 0;

/**
 * Enable debug mode.
 */
void rdp_debug() {
    DEBUG = 1;
}

/**
 * Close the program and print a message. The first argument is the exit code.
 * The following arguments can be formatted like printf.
 *
 * @param const int status
 * @param const char* fmt,
 * @param ...
 */
void rdp_exit(const int status, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(status);
}

/**
 * Prints a log if the DEBUG flag is set. Accepts arguments formatted like
 * printf.
 *
 * @param const char* fmt,
 * @param ...
 */
void rdp_log(const char *fmt, ...) {
    if(DEBUG) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
    }
}

/**
 * Logs the contents of a buffer in hex and string format.
 *
 * @param const char* src    buffer to log
 * @param cont int    length the length of the buffer
 */
void rdp_log_hex(const char* src, const int length) {
    if(DEBUG) {
        int i;
        int j;
        for(i = 0, j = 1; i < length; i++, j++) {
            printf(" %02x", (unsigned short) src[i]);
            if(j % 4 == 0) {
                char tmp[5];
                rdp_zero(tmp, 5);
                memcpy(tmp, src + i - 3, 4);
                rdp_no_newlines(tmp, 'N');
                printf(" | %s\n", tmp);
            }
        }
        if(j % 4 != 0) {
            char tmp[5];
            rdp_zero(tmp, 5);
            memcpy(tmp, src + i - 3, 4);
            rdp_no_newlines(tmp, 'N');
            for(i = 4; i >= j % 4; i--) {
                printf("   ");
            }
            printf(" | %s\n", tmp);
        }
    }
}

/**
 * Modifies a string to remove all newlines and replace them with the specified
 * character.
 */
void rdp_no_newlines(char* src, const char replacement) {
    int i;
    for(i = 0; i < strlen(src); i++) {
        if(src[i] == '\n') {
            src[i] = replacement;
        }
    }
}

/**
 * Logs a packet with the appropriate format.
 *
 * @param const char* event
 * @param const char* source_ip
 * @param const char* source_port
 * @param const char* destination_ip
 * @param const char* destination_port
 * @param const char* packet_type
 * @param const int   seq_ack_number
 * @param const int   size
 */
void rdp_log_packet(
    const char* event,
    const char* source_ip,
    const char* source_port,
    const char* destination_ip,
    const char* destination_port,
    const char* packet_type,
    const int seq_ack_number,
    const int size
) {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%H:%M:%S", tm_info);

    int clocks = clock() * 1.0;
    int micro = ((int) (clocks / (CLOCKS_PER_SEC / 1000000)))
             - (((int) (clocks / CLOCKS_PER_SEC)) * 1000000);

    printf("%s.%06d %s %s:%s %s:%s %s %d %d\n",
        buffer,
        micro,
        event,
        source_ip,
        source_port,
        destination_ip,
        destination_port,
        packet_type,
        seq_ack_number,
        size
    );
}

char* option;
/**
 * Set option.
 *
 * @param char* option
 */
char* rdp_opt(char* opt) {
    option = opt;
    return opt;
}

/**
 * Compare option.
 *
 * @param char* value
 */
int rdp_opt_cmp(char* value) {
    return strcmp(option, value) == 0;
}

/**
 * Zero a buffer.
 *
 * @param char* buffer
 * @param int   buffer_size
 */
void rdp_zero(char* buffer, int buffer_size) {
    memset(buffer, 0, buffer_size);
}

/**
 * Returns true if the two strings are equal.
 *
 * @param   const char* a
 * @param   const char* b
 * @returns int         1 if a is equal to b
 */
int rdp_streq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

/**
 * Returns the max of two integers.
 *
 * @param   const int a
 * @param   const int b
 * @returns int       the greater integer
 */
int rdp_max(const int a, const int b) {
    return a > b ? a : b;
}

/**
 * Returns the min of two integers.
 *
 * @param   const int a
 * @param   const int b
 * @returns int       the lesser integer
 */
int rdp_min(const int a, const int b) {
    return a < b ? a : b;
}

/**
 * Returns a random 16 bit integer.
 */
int rdp_get_seq_number() {
    // Mask to the first 16 bits.
    return rand() & 0xffff;
}