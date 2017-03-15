#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "util.h"

int DEBUG = 0;

void rdp_debug() {
    DEBUG = 1;
}

void rdp_exit(const int status, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(status);
}

void rdp_log(const char *fmt, ...) {
    if(DEBUG) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
    }
}

void rdp_log_hex(const char* src, int length) {
    if(DEBUG) {
        int i;
        int j;
        for(i = 0, j = 1; i < length; i++, j++) {
            printf(" %02x", (unsigned short) src[i]);
            if(j % 4 == 0) {
                char tmp[5];
                rdp_zero(tmp, 5);
                memcpy(tmp, src + i - 3, 4);
                rdp_no_newlines(tmp);
                printf(" | %s\n", tmp);
            }
        }
        if(j % 4 != 0) {
            char tmp[5];
            rdp_zero(tmp, 5);
            memcpy(tmp, src + i - 3, 4);
            rdp_no_newlines(tmp);
            for(i = 4; i >= j % 4; i--) {
                printf("   ");
            }
            printf(" | %s\n", tmp);
        }
    }
}

void rdp_no_newlines(const char* src) {
    int i;
    for(i = 0; i < strlen(src); i++) {
        if(src[i] == '\n' || src[i] == ' ') {
            src[i] = ' ';
        }
    }
}

void rdp_log_packet(
    const char* event,
    const char* source_ip,
    const char* source_port,
    const char* destination_ip,
    const char* destination_port,
    const char* packet_type,
    const int seq_number,
    const int ack_number,
    const int size
) {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%H:%M:%S", tm_info);

    int micro = ((int) (clock() / (CLOCKS_PER_SEC / 1000000.0)))
             - (((int) (clock() / CLOCKS_PER_SEC)) * 1000000.0);

    printf("%s.%06d %s %s:%s %s:%s %s %d/%d %d\n",
        buffer,
        micro,
        event,
        source_ip,
        source_port,
        destination_ip,
        destination_port,
        packet_type,
        seq_number,
        ack_number,
        size
    );
}

char* option;

char* rdp_opt(char* opt) {
    option = opt;
    return opt;
}
int rdp_opt_cmp(char* value) {
    return strcmp(option, value) == 0;
}

void rdp_zero(char* buffer, int buffer_size) {
    memset(buffer, 0, buffer_size);
}

int rdp_streq(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

int rdp_max(const int a, const int b) {
    return a > b ? a : b;
}

int rdp_min(const int a, const int b) {
    return a < b ? a : b;
}

int rdp_get_seq_number() {
    return rand() & 0xffff;
}