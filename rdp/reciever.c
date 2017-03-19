#include <stdio.h>
#include <stdlib.h>
#include "reciever.h"
#include "filestream.h"
#include "protocol.c"
#include "netconfig.h"
#include "util.h"

char payload_buffer[(WINDOW_SIZE + 1) * rdp_MAX_PACKET_SIZE];
unint16_t payload_buffer_seq[WINDOW_SIZE];

int recieved_packets;
int reset_count;
int timeout_count;
int timeout;
int connected;
int disconnecting;

unint16_t window_size;
unint16_t ack_number;
unint16_t last_ack;

/**
 * @param const char* reciever_ip
 * @param const char* reciever_port
 */
void rdp_reciever(const char* reciever_ip, const char* reciever_port) {

    // Init source socket and socket address
    rdp_open_source_socket(reciever_ip, reciever_port);

    window_size      = WINDOW_SIZE;
    timeout          = TIMEOUT;
    recieved_packets = 0;
    reset_count      = 0;
    timeout_count    = 0;
    connected        = 0;
    disconnecting    = 0;
    last_ack         = 0;

    // Mark all of the payload buffer as available
    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        payload_buffer_seq[i] = -1;
    }
}

/**
 *
 */
void re_ack() {
    if(ack_number == last_ack) {
        send_rdp(rdp_ACK | rdp_RES, ack_number, window_size, "");
    } else {
        last_ack = ack_number;
        send_rdp(rdp_ACK, ack_number, window_size, "");
    }
}

/**
 *
 */
void recieve_SYN() {
    connected  = 1;
    ack_number = rdp_seq_ack_number() + 1;
    re_ack();
}

/**
 *
 */
void recieve_FIN() {
    disconnecting = 1;
    ack_number    = rdp_seq_ack_number() + 1;
    re_ack();
}

/**
 *
 */
void recieve_DAT() {
    int i;
    // In order check
    if(rdp_seq_ack_number() == ack_number) {
        rdp_filestream_write(rdp_payload(), rdp_payload_size());
        ack_number += rdp_payload_size();

        // Dequeue data
        if(window_size != WINDOW_SIZE) {
            int dequeue = 0;
            do {
                for(i = 0; i < WINDOW_SIZE; i++) {
                    if(payload_buffer_seq[i] == ack_number) {
                        char* payload = payload_buffer + (i * rdp_MAX_PACKET_SIZE);
                        rdp_filestream_write(payload, strlen(payload));
                        ack_number         += strlen(payload);
                        payload_buffer_seq[i] = -1;
                        dequeue             = 1;
                        window_size++;
                    }
                }
            } while(dequeue);
        }
    } else if(window_size == 0) {
        re_ack();
    } else {
        // Queue data
        for(i = 0; i < WINDOW_SIZE; i++) {
            if(payload_buffer_seq[i] == -1) {
                char* payload = rdp_payload();
                payload_buffer_seq[i] = rdp_seq_ack_number();
                rdp_zero(payload_buffer + (i * rdp_MAX_PACKET_SIZE), rdp_MAX_PACKET_SIZE);
                memcpy(payload_buffer + (i * rdp_MAX_PACKET_SIZE), &payload, rdp_payload_size());
                break;
            }
        }
        if(--window_size == 0 || ++recieved_packets > WINDOW_SIZE) {
            recieved_packets = 0;
            re_ack();
        }
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
    if(connected) {
        rdp_log("Timed out.\n"
                "    Connected:     %d\n"
                "    Disconnecting: %d\n",
            connected, disconnecting
        );
        recieved_packets = 0;
        if(timeout_count++ > MAXIMUM_TIMEOUTS) {
            rdp_close_sockets();
            rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
        }
        re_ack();

    }
    if(disconnecting) {
        rdp_close_sockets();
        return;
    }
}

/**
 *
 */
void rdp_reciever_recieve() {
    loop {
        switch(listen_rdp(timeout)) {
            case event_SYN: recieve_SYN(); break;
            case event_FIN: recieve_FIN(); break;
            case event_DAT: recieve_DAT(); break;
            case event_RST: recieve_RST(); break;
            case event_ACK:
            case event_bad_packet:
                if(connected) {
                    re_ack();
                }
                break;
            case event_timeout: recieve_timeout(); break;
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