#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "filestream.h"
#include "netconfig.h"
#include "util.h"

#define PAYLOAD_SIZE (rdp_MAX_PACKET_SIZE - rdp_HEADER_SIZE)

static int finished;
static int connected;
static int reset_count;
static int timeout_count;
static int timeout;
static int start;
static int file_size;

static int file_pointer;
static unint16_t seq_number;

/**
 *
 */
void rdp_sender(
    const char* sender_ip,
    const char* sender_port,
    const char* reciever_ip,
    const char* reciever_port
) {
    // Init destination socket and socket address
    rdp_open_destination_socket(reciever_ip, reciever_port);
    // Init source socket and socket address
    rdp_open_source_socket(sender_ip, sender_port);

    finished      = 0;
    connected     = 0;
    reset_count   = 0;
    timeout_count = 0;
    file_pointer  = 0;
    file_size     = rdp_filestream_size();

    timeout       = TIMEOUT * 1.1;
    seq_number    = (unint16_t) rdp_get_seq_number();
    start         = seq_number;
}

/**
 *
 */
void connect_recieved_ACK() {
    if(rdp_seq_ack_number() == seq_number + 1) {
        rdp_log("Connected.");
        seq_number++;
        connected = 1;
    } else {
        rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
    }
}

/**
 *
 */
void connect_recieved_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void connect_recieved_timeout() {
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
    }
    rdp_log("Request timed out. Retrying...");
    rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void rdp_sender_connect() {

    rdp_log("Establishing connection...");
    rdp_send(rdp_SYN, seq_number, 0, "");

    while(!connected) {
        switch(rdp_listen(timeout)) {
            case event_ACK: connect_recieved_ACK(); break;
            case event_RST: connect_recieved_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
                break;
            case event_timeout: connect_recieved_timeout(); break;
        }
    }
}

/**
 *
 */
unint16_t seq_diff() {
    if(seq_number < rdp_seq_ack_number()) {
        return (0xFFFF - seq_number + rdp_seq_ack_number());
    } else {
        return rdp_seq_ack_number() - seq_number;
    }
}

/**
 *
 */
void send_packets() {

    int i;
    int window_size = rdp_window_size();

    rdp_log("Sending %d packets...\n", window_size);

    // Send DAT packets
    for(i = 0; i < window_size && file_pointer < file_size; i++) {
        char payload[PAYLOAD_SIZE + 1];
        int len  = rdp_filestream_read(payload, PAYLOAD_SIZE, file_pointer);
        int size = PAYLOAD_SIZE > len ? len : PAYLOAD_SIZE;
        rdp_send(rdp_DAT, seq_number, size, payload);
        file_pointer += size;
        seq_number   += size;
    }
}

/**
 *
 */
void send_recieved_ACK() {
    unint16_t diff = seq_diff();
    file_pointer += diff;
    seq_number   += diff;
    if(file_pointer >= file_size) {
        finished = 1;
    } else {
        send_packets();
    }
}

/**
 *
 */
void send_recieved_RST() {
    file_pointer  = 0;
    timeout_count = 0;
    seq_number    = start;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    send_packets();
}

/**
 *
 */
void send_recieved_timeout() {
    //send_packets();
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
    }
}

/**
 *
 */
void rdp_sender_send() {

    send_packets();
    // SEND DAT packet
    while(!finished) {
        switch(rdp_listen(timeout)) {
            case event_ACK: send_recieved_ACK(); break;
            case event_RST: send_recieved_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                send_packets();
                break;
            case event_timeout: send_recieved_timeout(); break;
        }
    }
}

/**
 *
 */
void disconnect_recieved_ACK() {
    if(rdp_seq_ack_number() == seq_number + 1) {
        rdp_close_sockets();
        connected = 0;
        rdp_log("Disconnected.");
    } else {
        rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
    }
}

/**
 *
 */
void disconnect_recieved_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void disconnect_recieved_timeout() {
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
    }
    rdp_log("Request timed out. Retrying...");
    rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void rdp_sender_disconnect() {

    rdp_log("Disconnecting...");
    rdp_send(rdp_FIN, seq_number, 0, "");

    while(connected) {
        switch(rdp_listen(timeout)) {
            case event_ACK: disconnect_recieved_ACK(); break;
            case event_RST: disconnect_recieved_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
                break;
            case event_timeout: disconnect_recieved_timeout(); break;
        }
    }
}

void rdp_sender_stats() {
    int* stats = rdp_stats();
    printf("\n\n"
        "total data bytes sent: %d\n"
        "unique data bytes sent: %d\n"
        "total data packets sent: %d\n"
        "unique data packets sent: %d\n"
        "SYN packets sent: %d\n"
        "FIN packets sent: %d\n"
        "RST packets sent: %d\n"
        "ACK packets received: %d\n"
        "RST packets received: %d\n"
        "total time duration (second): %d\n",
        stats[stat_sent_bytes],
        stats[stat_sent_bytes_unique],
        stats[stat_sent_DAT],
        stats[stat_sent_DAT_unique],
        stats[stat_sent_SYN],
        stats[stat_sent_FIN],
        stats[stat_sent_RST],
        stats[stat_recieved_ACK],
        stats[stat_recieved_RST],
        (stats[stat_end_time] - stats[stat_start_time])
    );
}