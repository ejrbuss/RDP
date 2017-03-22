/**
 * @author ejrbuss
 * @date 2017
 *
 * RDP Sender file. Contains functions for managing the sender state machine.
 */
#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "filestream.h"
#include "netconfig.h"
#include "util.h"

// Payload size is the packet size minus the header
#define PAYLOAD_SIZE (rdp_MAX_PACKET_SIZE - rdp_HEADER_SIZE)

// Indicates that the connection has closed
static int finished;
// Indicates that the connection is open
static int connected;
// Indicates the number of RSTs sent or receiveed
static int reset_count;
// Indicates the current number of timeouts
static int timeout_count;
// Indicates the current timeout time
static int timeout;
// Inidcates the sending file size
static int file_size;
// Indicates the current sequence number
static uint32_t seq_number;
// Inidcates the previously ACKed sequence number
static uint32_t last_seq;
// Indicates the offset of the sequence number from the file position
static uint16_t offset;

/**
 * Creates a new RDP sender. Prepares a source socket. After this function has
 * been called the RDP sender can be asked to connect to a RDP receiver.
 *
 * @param const char* sender_ip     IP address to listen on
 * @param const char* sender_port   Port to listen on
 * @param const char* receiver_ip   IP address to send to
 * @param const char* receiver-port Port to send to
 */
void rdp_sender(
    const char* sender_ip,
    const char* sender_port,
    const char* receiver_ip,
    const char* receiver_port
) {
    // Init destination socket and socket address
    rdp_open_destination_socket(receiver_ip, receiver_port);
    // Init source socket and socket address
    rdp_open_source_socket(sender_ip, sender_port);

    // Prepare initial state
    finished      = 0;
    connected     = 0;
    reset_count   = 0;
    timeout_count = 0;
    file_size     = rdp_filestream_size();
    timeout       = TIMEOUT * 2;
    seq_number    = rdp_get_seq_number();
    offset        = seq_number + 1;
}

/**
 * Connect state machine function. received ACK packet.
 *
 * If the received ACK packet contains the expected sequence number indicate
 * the connection as opend. Otherwise resend the SYN packet.
 */
void connect_received_ACK() {
    timeout_count  = 0;
    if(rdp_seq_ack_number() == seq_number + 1) {
        rdp_log("Connected.");
        seq_number++;
        connected = 1;
    } else {
        rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
    }
}

/**
 * Connect state machine function. received RST packet.
 *
 * Reset timeout count and resend SYN packet.
 */
void connect_received_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
}

/**
 * Connect state machine function. Timedout.
 *
 * Resend SYN packet.
 */
void connect_received_timeout() {
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
    }
    rdp_log("Request timed out. Retrying...");
    rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
}

/**
 * State machine for connection process.
 */
void rdp_sender_connect() {

    rdp_log("Establishing connection...");
    rdp_send(rdp_SYN, seq_number, 0, "");

    while(!connected) {
        switch(rdp_listen(timeout)) {
            case event_ACK: connect_received_ACK(); break;
            case event_RST: connect_received_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                rdp_send(rdp_SYN | rdp_RES, seq_number, 0, "");
                break;
            case event_timeout: connect_received_timeout(); break;
        }
    }
}

/**
 * Helper function for sending data packets. Sends the number of packets equal
 * to the last received window size. Sends a minimum of one packet. Will not
 * send any more packets than are needed to finish sending the file.
 */
void send_packets() {

    int i;
    int window_size = rdp_window_size() > 0
        ? rdp_window_size()
        : 1;

    rdp_log("Sending %d packets...\n", window_size);

    // Send DAT packets
    for(i = 0; i < window_size && seq_number - offset < file_size; i++) {
        char payload[PAYLOAD_SIZE + 1];
        int idx  = seq_number - offset;
        int len  = rdp_filestream_read(payload, PAYLOAD_SIZE, idx);
        int size = PAYLOAD_SIZE > len ? len : PAYLOAD_SIZE;
        rdp_send(
            seq_number < last_seq
                ? rdp_DAT | rdp_RES
                : rdp_DAT,
            seq_number,
            size,
            payload
        );
        seq_number += size;
    }
}

/**
 * Send state machine function. received ACK packet.
 *
 * Reset teimout. Shift sequence number. Check if finished, if not send more DAT
 * packets.
 */
void send_received_ACK() {
    timeout_count  = 0;
    last_seq       = seq_number;
    seq_number     = rdp_seq_ack_number();
    if(seq_number - offset >= file_size) {
        finished = 1;
    } else {
        send_packets();
    }
}

/**
 * Send state machine function. received ACK packet.
 *
 * Reset timeout count and restart data transfer.
 */
void send_received_RST() {
    timeout_count = 0;
    seq_number    = offset;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    send_packets();
}

/**
 * Send state machine function. Timedout.
 *
 * Do nothing until timeout_count is hit.
 */
void send_received_timeout() {
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
    }
}

/**
 * State machine for send process.
 */
void rdp_sender_send() {

    send_packets();
    // SEND DAT packet
    while(!finished) {
        switch(rdp_listen(timeout)) {
            case event_ACK: send_received_ACK(); break;
            case event_RST: send_received_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                send_packets();
                break;
            case event_timeout: send_received_timeout(); break;
        }
    }
}

/**
 * Disconnect state machine function. received ACK packet.
 *
 * If the received ACK packet contains the expected sequence number, disconnects
 * otherwise resend the FIN packet.
 */
void disconnect_received_ACK() {
    if(rdp_seq_ack_number() == seq_number + 1) {
        rdp_close_sockets();
        connected = 0;
        rdp_log("Disconnected.");
    } else {
        rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
    }
}

/**
 * Disconnect state machine function. received RST packet.
 *
 * Reset the timeout count. Resend the FIN packet.
 */
void disconnect_received_RST() {
    timeout_count = 0;
    if(reset_count++ > MAXIMUM_RESETS) {
        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
    }
    rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
}

/**
 * Disconnect state machine function. Timedout.
 *
 * Resend the FIN packet.
 */
void disconnect_received_timeout() {
    if(timeout_count++ > MAXIMUM_TIMEOUTS) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "RDP transimmision failed as the connection timed out too many times.");
    }
    rdp_log("Request timed out. Retrying...");
    rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
}

/**
 * State machine for the disconnect process.
 */
void rdp_sender_disconnect() {

    rdp_log("Disconnecting...");
    rdp_send(rdp_FIN, seq_number, 0, "");

    while(connected) {
        switch(rdp_listen(timeout)) {
            case event_ACK: disconnect_received_ACK(); break;
            case event_RST: disconnect_received_RST(); break;
            case event_SYN:
            case event_FIN:
            case event_DAT:
            case event_bad_packet:
                rdp_send(rdp_FIN | rdp_RES, seq_number, 0, "");
                break;
            case event_timeout: disconnect_received_timeout(); break;
        }
    }
}

/**
 * Print the sender stats.
 */
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
        stats[stat_received_ACK],
        stats[stat_received_RST],
        (stats[stat_end_time] - stats[stat_start_time])
    );
}