#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reciever.h"
#include "filestream.h"
#include "netconfig.h"
#include "util.h"

static char payload_buffer[(WINDOW_SIZE + 1) * rdp_MAX_PACKET_SIZE];
static unint32_t payload_buffer_seq[WINDOW_SIZE];

static int recieved_packets;
static int reset_count;
static int timeout_count;
static int timeout;
static int connected;
static int disconnect;
static int disconnecting;

static unint16_t current_window_size;
static unint32_t ack_number;
static unint32_t last_ack;

/**
 * @param const char* reciever_ip
 * @param const char* reciever_port
 */
void rdp_reciever(const char* reciever_ip, const char* reciever_port) {

    // Init source socket and socket address
    rdp_open_source_socket(reciever_ip, reciever_port);

    current_window_size = WINDOW_SIZE;
    timeout             = TIMEOUT / 2;
    recieved_packets    = 0;
    reset_count         = 0;
    timeout_count       = 0;
    connected           = 0;
    disconnect          = 0;
    disconnecting       = 0;
    last_ack            = 0;

    // Mark all of the payload buffer as available
    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        payload_buffer_seq[i] = 0;
    }
}
/**
 *
 */
int not_in_queue(unint32_t seq) {
    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        if(payload_buffer_seq[i] == seq) return 0;
    }
    return 1;
}

/**
 *
 */
void re_ack() {
    if(ack_number == last_ack) {
        rdp_send(rdp_ACK | rdp_RES, ack_number, current_window_size, "");
    } else {
        last_ack = ack_number;
        rdp_send(rdp_ACK, ack_number, current_window_size, "");
    }
}

/**
 *
 */
void recieved_SYN() {
    connected  = 1;
    ack_number = rdp_seq_ack_number() + 1;
    re_ack();
}

/**
 *
 */
void recieved_FIN() {
    timeout_count = 0;
    disconnecting = 1;
    ack_number    = rdp_seq_ack_number() + 1;
    re_ack();
}

/**
 *
 */
void recieved_DAT() {

    int i;
    timeout       = TIMEOUT;
    timeout_count = 0;

    // In order check
    if(rdp_seq_ack_number() == ack_number) {
        rdp_log("1");
        rdp_filestream_write(rdp_payload(), rdp_payload_size());
        ack_number += rdp_payload_size();
        recieved_packets++;

        int dequeue;
        do {
            dequeue = 0;
            for(i = 0; i < WINDOW_SIZE; i++) {
                if(payload_buffer_seq[i] < ack_number) {
                    payload_buffer_seq[i] = 0;
                } else if(payload_buffer_seq[i] == ack_number) {
                    char* payload = payload_buffer + (i * rdp_MAX_PACKET_SIZE);
                    rdp_filestream_write(payload, strlen(payload));
                    ack_number           += strlen(payload);
                    payload_buffer_seq[i] = 0;
                    dequeue               = 1;
                    recieved_packets++;
                }
            }
        } while(dequeue);
    } else if(current_window_size == 0) {
        recieved_packets = 0;
    } else {
        // Queue data
        if(rdp_seq_ack_number() > ack_number && not_in_queue(rdp_seq_ack_number())) {
            for(i = 0; i < WINDOW_SIZE; i++) {
                if(payload_buffer_seq[i] == 0) {
                    char* payload = rdp_payload();
                    payload_buffer_seq[i] = rdp_seq_ack_number();
                    rdp_zero(payload_buffer + (i * rdp_MAX_PACKET_SIZE), rdp_MAX_PACKET_SIZE);
                    memcpy(payload_buffer + (i * rdp_MAX_PACKET_SIZE), &payload, rdp_payload_size());
                    break;
                }
            }
        }
    }
    current_window_size = 0;
    for(i = 0; i < WINDOW_SIZE; i++) {
        current_window_size += (payload_buffer_seq[i] == 0);
    }
}

/**
 *
 */
void recieved_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
}

/**
 *
 */
void recieved_timeout() {
    if(disconnecting)  {
        if(timeout_count++ > MAXIMUM_TIMEOUTS) {
            rdp_close_sockets();
            disconnect = 1;
        }
    } else if(connected) {
        rdp_log("Timed out.\n"
                "    Connected:     %d\n"
                "    Disconnecting: %d\n",
            connected, disconnecting
        );
        recieved_packets = 0;
        timeout *= 1.5;
        if(timeout > 128) {
            timeout = 128;
        }
        if(timeout_count++ > MAXIMUM_TIMEOUTS) {
            rdp_close_sockets();
            rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
        }
        re_ack();
    }
}

/**
 *
 */
void rdp_reciever_recieve() {
    while(!disconnect) {
        switch(rdp_listen(timeout)) {
            case event_SYN: recieved_SYN(); break;
            case event_FIN: recieved_FIN(); break;
            case event_DAT: recieved_DAT(); break;
            case event_RST: recieved_RST(); break;
            case event_ACK:
            case event_bad_packet:
                if(connected) {
                    re_ack();
                }
                break;
            case event_timeout: recieved_timeout(); break;
        }
    }
}

/**
 *
 */
void rdp_reciever_stats() {
    int* stats = rdp_stats();
    printf("\n\n"
        "total data bytes received: %d\n"
        "unique data bytes received: %d\n"
        "total data packets received: %d\n"
        "unique data packets received: %d\n"
        "SYN packets received: %d\n"
        "FIN packets received: %d\n"
        "RST packets received: %d\n"
        "ACK packets sent: %d\n"
        "RST packets sent: %d\n"
        "total time duration (second): %d\n",
        stats[stat_recieved_bytes],
        stats[stat_recieved_bytes_unique],
        stats[stat_recieved_DAT],
        stats[stat_recieved_DAT_unique],
        stats[stat_recieved_SYN],
        stats[stat_recieved_FIN],
        stats[stat_recieved_RST],
        stats[stat_sent_ACK],
        stats[stat_sent_RST],
        (stats[stat_end_time] - stats[stat_start_time])
    );
}