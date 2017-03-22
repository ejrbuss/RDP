/**
 * @author ejrbuss
 * @date 2017
 *
 *  RDP receiver file. Contains functions for managing the receiver state
 * machine.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "receiver.h"
#include "filestream.h"
#include "netconfig.h"
#include "util.h"

// Out of order packet buffer
static char payload_buffer[(WINDOW_SIZE + 1) * rdp_MAX_PACKET_SIZE];
// Out of order pafcket sequence numbers
static uint32_t payload_buffer_seq[WINDOW_SIZE];
// Indicates the number of received packets
static int received_packets;
// Indicates the number of resets
static int reset_count;
// Indicates the current number of timeouts
static int timeout_count;
// Indicates the current timoeut time
static int timeout;
// Indicates if the receiver is currently connected
static int connected;
// Inidcates if the receiver is finished
static int disconnect;
// Inidcates if the receiver has entered the disconnection process
static int disconnecting;
// Inidcates the current window size in # of packets
static uint16_t current_window_size;
// Indicates the current acknowldgement number
static uint32_t ack_number;
// Inidcates the last ack sent
static uint32_t last_ack;

/**
 * Creates a new RDP receiver. Prepares a source socket. After this function has
 * been called the RDP receiver can be asked to wait for an RDP sender.
 *
 * @param const char* receiver_ip   the IP to listen on
 * @param const char* receiver_port the port to listen on
 */
void rdp_receiver(const char* receiver_ip, const char* receiver_port) {

    // Init source socket and socket address
    rdp_open_source_socket(receiver_ip, receiver_port);

    current_window_size = WINDOW_SIZE;
    timeout             = TIMEOUT / 2;
    received_packets    = 0;
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
 * @param   uint32_t seq the sequence number
 * @returns int           1 if the sequence number is not in the buffer
 */
int not_in_queue(uint32_t seq) {
    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        if(payload_buffer_seq[i] == seq) return 0;
    }
    return 1;
}

/**
 * Helper function for sending an ACK.
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
 * State machine function. Recieved SYN packet.
 *
 * ACK the sequence number + 1.
 */
void received_SYN() {
    connected     = 1;
    timeout_count = 0;
    ack_number    = rdp_seq_ack_number() + 1;
    re_ack();
}

/**
 * State machine function. Recieved FIN packet.
 *
 * ACK the sequence number + 1;
 */
void received_FIN() {
    timeout_count = 0;
    disconnecting = 1;
    ack_number    = rdp_seq_ack_number() + 1;
    re_ack();
}

/**
 * State machine function. Recieved DAT packet.
 *
 * If the sequence number is the next packet write it to file and write any
 * out of packets that are now ready.
 *
 * If the window is full just indicate that a packet was recieved otherwise
 * add the packet to the out of order buffer.
 *
 * Finally recaclulate window size.
 */
void received_DAT() {

    int i;
    timeout       = TIMEOUT / 2;
    timeout_count = 0;

    // In order check
    if(rdp_seq_ack_number() == ack_number) {
        rdp_log("1");
        rdp_filestream_write(rdp_payload(), rdp_payload_size());
        ack_number += rdp_payload_size();
        received_packets++;

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
                    received_packets++;
                }
            }
        } while(dequeue);
    } else if(current_window_size == 0) {
        received_packets = 0;
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
 * State machine function. Recieved RST packet.
 *
 * Reset timeout count.
 */
void received_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
}

/**
 * State machine function. Timedout.
 *
 * When disconnecting prepare to close if sufficient timeouts have occured.
 *
 * WHen connected send a new ACK packet and increase the timeout count.
 */
void received_timeout() {
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
        received_packets = 0;
        timeout *= 1.5;
        if((timeout_count += (timeout / TIMEOUT)) > MAXIMUM_TIMEOUTS) {
            rdp_close_sockets();
            rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
        }
        re_ack();
    }
}

/**
 * State machine for the receive process.
 */
void rdp_receiver_receive() {
    while(!disconnect) {
        switch(rdp_listen(timeout)) {
            case event_SYN: received_SYN(); break;
            case event_FIN: received_FIN(); break;
            case event_DAT: received_DAT(); break;
            case event_RST: received_RST(); break;
            case event_ACK:
            case event_bad_packet:
                if(connected) {
                    re_ack();
                }
                break;
            case event_timeout: received_timeout(); break;
        }
    }
}

/**
 * Print the Receiver stats.
 */
void rdp_receiver_stats() {
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
        stats[stat_received_bytes],
        stats[stat_received_bytes_unique],
        stats[stat_received_DAT],
        stats[stat_received_DAT_unique],
        stats[stat_received_SYN],
        stats[stat_received_FIN],
        stats[stat_received_RST],
        stats[stat_sent_ACK],
        stats[stat_sent_RST],
        (stats[stat_end_time] - stats[stat_start_time])
    );
}