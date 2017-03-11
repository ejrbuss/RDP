#include <stdio.h>
#include <stdlib.h>
#include "netconfig.h"
#include "protocol.h"
#include "util.h"

int maximum_rst_attempts = 5;

/* Stat tracking */
int stat_sent_total_bytes             = 0;
int stat_sent_unique_bytes            = 0;
int stat_sent_dat_packets             = 0;
int stat_sent_dat_unique_packets      = 0;
int stat_sent_syn_packets             = 0;
int stat_sent_ack_packets             = 0;
int stat_sent_fin_packets             = 0;
int stat_sent_rst_packets             = 0;
int stat_recieved_total_bytes         = 0;
int stat_recieved_unique_bytes        = 0;
int stat_recieved_dat_packets         = 0;
int stat_recieved_dat_unique_packets  = 0;
int stat_recieved_syn_packets         = 0;
int stat_recieved_ack_packets         = 0;
int stat_recieved_fin_packets         = 0;
int stat_recieved_rst_packets         = 0;
int stat_start_time                   = 0;
int stat_end_time                     = 0;

/*  */

/* Listen variables */

enum { event_recieved, event_timeout };

char recieve_buffer[2000];

int listen() {

}

void rdp_sender(
    const char* sender_ip,
    const char* sender_port,
    const char* reciever_ip,
    const char* reciever_port
) {

}

void rdp_sender_connect() {

    // SEND SYN packet

    while(1) {
        int event = listen();
        if(event == event_recieved) {
            if(rdp_flags() & rdp_ACK) {
                stat_recieved_ack_packets++;
                // check sequence number
                // return
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                // try again
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer);
            }
        } else {
            // timeout
            // resend
            // extend timer
        }
    }
}

void rdp_send() {

    // SEND DAT packet

    while(1) {
        int event = listen();
        if(event == event_recieved) {
            if(rdp_flags() & rdp_ACK) {
                stat_recieved_ack_packets++;
                // check sequence number
                // send next set of data
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                // try again
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer);
            }
        } else {
            // timeout
            // resend
            // extend timer
        }
    }
}

void rdp_sender_disconnect() {

    // SEND FIN packet

    while(1) {
        int event = listen();
        if(event == event_recieved) {
            if(rdp_flags() & rdp_ACK) {
                stat_recieved_ack_packets++;
                // check ack number
                    // return
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                // try again
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer);
            }
        } else {
            // timeout
            // resend
            // extend timer
        }
    }
}

void rdp_sender_stats() {
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
        "total time duration (second): %d",
        stat_sent_total_bytes,
        stat_sent_unique_bytes,
        stat_sent_dat_packets,
        stat_sent_dat_unique_packets,
        stat_sent_syn_packets,
        stat_sent_fin_packets,
        stat_sent_rst_packets,
        stat_recieved_ack_packets,
        stat_recieved_rst_packets,
        stat_end_time - stat_start_time
    );
}

// Reciever
void rdp_reciever(
    const char* reciever_ip,
    const char* reciever_port
) {

}

void rdp_recieve() {
    while(1) {
        int event = listen();
        if(event == event_recieved) {
            if(rdp_flags() & rdp_SYN) {
                stat_recieved_syn_packets++;
                // send ack
            } else if(rdp_flags() & rdp_FIN) {
                stat_recieved_fin_packets++;
                // send ack
                // return
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                // try again
            } else if(rdp_flags() & rdp_DAT) {
                stat_recieved_dat_packets++;
                // read data
                // check if we should send ack
                    // send ack
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer);
            }
        } else {
            // timeout
            // if connected
                // send ack
        }
    }
}

void rdp_reciever_stats() {
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
        stat_recieved_total_bytes,
        stat_recieved_unique_bytes,
        stat_recieved_dat_packets,
        stat_recieved_dat_unique_packets,
        stat_recieved_syn_packets,
        stat_recieved_fin_packets,
        stat_recieved_rst_packets,
        stat_sent_ack_packets,
        stat_sent_rst_packets,
        stat_end_time - stat_start_time
    );
}