/**
 * @author ejrbuss
 * @date 2017
 *
 *  RDP Reciever file. Contains functions for managing the reciever state machine.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reciever.h"
#include "filestream.h"
#include "netconfig.h"
#include "util.h"

// Out of order packet buffer
static char payload_buffer[(WINDOW_SIZE + 1) * rdp_MAX_PACKET_SIZE];
// Out of order pafcket sequence numbers
static unint32_t payload_buffer_seq[WINDOW_SIZE];
// Indicates the number of recieved packets
static int recieved_packets;
// Indicates the number of resets
static int reset_count;
// Indicates the current number of timeouts
static int timeout_count;
// Indicates the current timoeut time
static int timeout;
// Indicates if the reciever is currently connected
static int connected;
// Inidcates if the reciever is finished
static int disconnect;
// Inidcates if the reciever has entered the disconnection process
static int disconnecting;
// Inidcates the current window size in # of packets
static unint16_t current_window_size;
// Indicates the current acknowldgement number
static unint32_t ack_number;
// Inidcates the last ack sent
static unint32_t last_ack;

/**
 * Creates a new RDP reciever. Prepares a source socket. After this function has
 * been called the RDP reciever can be asked to wait for an RDP sender.
 *
 * @param const char* reciever_ip   the IP to listen on
 * @param const char* reciever_port the port to listen on
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
 * Helper function to determine if a sequence number is in the current out of
 * order buffer.
 *
 * @param   unint32_t seq the sequence number
 * @returns int           1 if the sequence number is not in the buffer
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
        if((timeout_count += (timeout / TIMEOUT)) > MAXIMUM_TIMEOUTS) {
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