#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "filestream.h"
#include "protocol.h"
#include "netconfig.h"
#include "util.h"

int connected;
int reset_count;
int timeout_count;
int timeout;

unint16_t seq_number;

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
    open_destination_socket(reciever_ip, reciever_port);
    // Init source socket and socket address
    open_source_socket(sender_ip, sender_port);

    connected     = 0;
    reset_count   = 0;
    timeout_count = 0;
    timeout       = 0;
    seq_number    = (unint16_t) rdp_get_seq_number();
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
        send_rdp(rdp_SYN | rdp_RES, seq_number, 0, "");
    }
}

/**
 *
 */
void connect_recieved_reset() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    send_rdp(rdp_SYN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void connect_recieved_timeout() {
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision fauked as the connection timed out too many times.");
    }
    rdp_log("Request timed out. Retrying...");
    send_rdp(rdp_SYN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void rdp_sender_connect() {

    rdp_log("Establishing connection...");
    send_rdp(rdp_SYN, seq_number, 0, "");

    while(!connected) {
        switch(listen_rdp(timeout)) {
            case event_ACK: connect_recieved_ACK(); break;
            case event_RST: connect_recieved_reset(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                send_rdp(rdp_SYN | rdp_RES, seq_number, 0, "");
                break;
            case event_timeout: connect_recieved_timeout(); break;
        }
    }
}

void rdp_sender_send() {

    // SEND DAT packet
    int size = rdp_filestream_size();
    int l;
    int i;

    for(i = 0; i < size; i += 700) {
        char payload[701];
        rdp_filestream_read(payload, 700, i);
        send_rdp(rdp_DAT, seq_number, 700, payload);
        seq_number += 700;
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
    }
    send_rdp(rdp_FIN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void disconnect_recieved_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    send_rdp(rdp_FIN | rdp_RES, seq_number, 0, "");
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
    send_rdp(rdp_FIN | rdp_RES, seq_number, 0, "");
}

/**
 *
 */
void rdp_sender_disconnect() {

    rdp_log("Disconnecting...");
    send_rdp(rdp_FIN, seq_number, 0, "");

    while(connected) {
        switch(listen_rdp(timeout)) {
            case event_ACK: disconnect_recieved_ACK(); break;
            case event_RST: disconnect_recieved_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                send_rdp(rdp_FIN | rdp_RES, seq_number, 0, "");
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