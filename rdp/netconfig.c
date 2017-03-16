#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "filestream.h"
#include "netconfig.h"
#include "protocol.h"
#include "util.h"

#define SOCK_BUFFER_SIZE 2000
#define ADDR_SIZE        200
#define TIMEOUT          500
#define MAXIMUM_TIMEOUTS 5
#define MAXIMUM_RESETS   5
#define WINDOW_SIZE      10

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

/* listen_rdp variables */

enum { event_recieved, event_timeout, event_bad_packet };

char recieve_buffer[SOCK_BUFFER_SIZE];
char send_buffer[SOCK_BUFFER_SIZE];

int source_socket;
struct sockaddr_in source_address;
int destination_socket;
struct sockaddr_in destination_address;

unint16_t sequence_number;

char source_ip[ADDR_SIZE];
char source_port[ADDR_SIZE];
char destination_ip[ADDR_SIZE];
char destination_port[ADDR_SIZE];

void open_source_socket() {
    rdp_log("source_ip: %s\nsource_port: %s\n", source_ip, source_port);
    source_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (source_socket == -1) {
        rdp_exit(EXIT_FAILURE, "Unable to create sender socket");
    }
    // Let go of socket on exit/crash
    int option = 1;
    setsockopt(
        source_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const void*) &option,
        sizeof(int)
    );
    memset(&source_address, 0, sizeof(source_address));
    source_address.sin_family      = AF_INET;
    source_address.sin_addr.s_addr = inet_addr(source_ip);
    source_address.sin_port        = htons(atoi(source_port));
    if (-1 == bind(
        source_socket,
        (struct sockaddr*) &source_address,
        sizeof(source_address)
    )) {
        close(source_socket);
        rdp_exit(EXIT_FAILURE, "Failed to bind socket: %s\n", strerror(errno));
    }
}

void open_destination_socket() {
    rdp_log("destination_ip: %s\ndestination_port %s\n", destination_ip, destination_port);
    destination_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (destination_socket == -1) {
        rdp_exit(EXIT_FAILURE, "Unable to create reciever socket");
    }
    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family      = AF_INET;
    destination_address.sin_addr.s_addr = inet_addr(destination_ip);
    destination_address.sin_port        = htons(atoi(destination_port));
}

int listen_rdp(int timeout_milli) {

    int select_result;
    ssize_t recsize;
    struct timeval timeout;
    fd_set read_fds;
    socklen_t socket_length = sizeof(destination_address);

    rdp_log("listening...");

    FD_ZERO(&read_fds);
    FD_SET(source_socket, &read_fds);

    timeout.tv_sec  = 0;
    timeout.tv_usec = timeout_milli * 1000;
    // Wait on select
    select_result = select(source_socket + 1, &read_fds, NULL, NULL, &timeout);

    // Check result
    switch(select_result) {
        case -1:
            close(source_socket);
            rdp_exit(EXIT_FAILURE, "Error in select: %s\n", strerror(errno));
        case 0: return event_timeout;
        default: break;
    }
    // Recieve and parse the packet
    rdp_zero(recieve_buffer, rdp_MAX_PACKET_SIZE + 1);
    recsize = recvfrom(
        source_socket,
        (void*) recieve_buffer,
        SOCK_BUFFER_SIZE,
        0,
        (struct sockaddr*) &destination_address,
        &socket_length
    );

    int parse = rdp_parse(recieve_buffer);


    // We don't yet know our destination so read it in
    if(destination_ip[0] == 0 || destination_port[0] == 0) {
        strcpy(destination_ip, inet_ntoa(destination_address.sin_addr));
        sprintf(destination_port, "%d", ntohs(destination_address.sin_port));
        if (destination_ip == NULL) {
            close(source_socket);
            rdp_exit(EXIT_FAILURE, "Failed to read destination IP:port from recieved packet.");
        }
        open_destination_socket();
    }

    // Log the packet
    rdp_log_packet(
        "r",
        destination_ip,
        destination_port,
        source_ip,
        source_port,
        rdp_flag_names[rdp_flags()],
        rdp_seq_ack_number(),
        rdp_payload_size() | rdp_window_size()
    );
    rdp_log_hex(recieve_buffer, rdp_size());

    // Compute statistics
    stat_recieved_syn_packets += !!(rdp_flags() & rdp_SYN);
    stat_recieved_fin_packets += !!(rdp_flags() & rdp_FIN);
    stat_recieved_dat_packets += !!(rdp_flags() & rdp_DAT);
    stat_recieved_ack_packets += !!(rdp_flags() & rdp_ACK);
    stat_recieved_rst_packets += !!(rdp_flags() & rdp_RST);

    if(parse == 0) {
        rdp_log("Bad packet!");
        return event_bad_packet;
    }

    return event_recieved;
}

void send_rdp(
    const char* event,
    const unint16_t flags,
    const unint16_t seq_ack_number,
    const unint16_t size,
    const char* payload
) {
    // Compute statistics
    stat_sent_syn_packets += !!(flags & rdp_SYN);
    stat_sent_fin_packets += !!(flags & rdp_FIN);
    stat_sent_dat_packets += !!(flags & rdp_DAT);
    stat_sent_ack_packets += !!(flags & rdp_ACK);
    stat_sent_ack_packets += !!(flags & rdp_RST);

    rdp_pack(
        send_buffer,
        flags,
        seq_ack_number,
        size,
        payload
    );
    if(sendto(
        source_socket,
        send_buffer,
        rdp_packed_size(flags & rdp_DAT ? size : 0),
        0,
        (struct sockaddr*) &destination_address,
        sizeof(destination_address)
    ) < 0) {
        close(source_socket);
        rdp_exit(EXIT_FAILURE, "Error sending packet: %s\n", strerror(errno));
    } else {
        rdp_log_packet(
            event,
            source_ip,
            source_port,
            destination_ip,
            destination_port,
            rdp_flag_names[flags],
            seq_ack_number,
            size
        );
    }
}

void rdp_sender(
    const char* sender_ip,
    const char* sender_port,
    const char* reciever_ip,
    const char* reciever_port
) {
    // Sent internal variables
    strcpy(source_ip, sender_ip);
    strcpy(source_port, sender_port);
    strcpy(destination_ip, reciever_ip);
    strcpy(destination_port, reciever_port);

    // Init destination socket and socket address
    open_destination_socket();
    // Init source socket and socket address
    open_source_socket();
}

void rdp_sender_connect() {

    sequence_number   = rdp_get_seq_number();
    int reset_count   = 0;
    int timeout_count = 0;
    int timeout       = TIMEOUT;

    rdp_log("Establishing connection...");
    send_rdp("s", rdp_SYN, sequence_number, 0, "");

    while(1) {
        switch(listen_rdp(timeout)) {
            case event_recieved: {
                if(rdp_flags() & rdp_ACK) {
                    if(rdp_seq_ack_number() == sequence_number + 1) {
                        sequence_number++;
                        return;
                    }
                }
                if(rdp_flags() & rdp_RST) {
                    timeout_count = 0;
                    if(reset_count++ > MAXIMUM_RESETS) {
                        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
                    }
                } else {
                    rdp_log("Unexpected packet!");
                }
                send_rdp("S", rdp_SYN, sequence_number, 0, "");
                break;
            }
            case event_bad_packet: {
                send_rdp("S", rdp_SYN, sequence_number, 0, "");
                break;
            }
            case event_timeout: {
                timeout = (int) timeout * 1.1;
                timeout_count++;
                if(timeout_count > MAXIMUM_TIMEOUTS) {
                    rdp_exit(EXIT_FAILURE, "RDP transimmision as the connection timed out too many times.");
                }
                rdp_log("Request timed out. Retrying...");
                send_rdp("S", rdp_SYN, sequence_number, 0, "");
                break;
            }
        }
    }
}

void rdp_send() {

    // SEND DAT packet
    int size = rdp_filestream_size();
    int l;
    int i;

    for(i = 0; i < size; i += 700) {
        char payload[701];
        rdp_filestream_read(payload, 700, i);
        send_rdp("s", rdp_DAT, sequence_number, 700, payload);
        sequence_number += 700;
    }
    /*
    while(1) {
        int event = listen_rdp(TIMEOUT);
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
                rdp_log_hex(recieve_buffer, rdp_packaged_size(rdp_payload_size()));
            }
        } else {
            // timeout
            // resend
            // extend timer
        }
    }
    */
}

void rdp_sender_disconnect() {

    int reset_count   = 0;
    int timeout_count = 0;
    int timeout = TIMEOUT / 2;

    send_rdp("s", rdp_FIN, sequence_number, 0, "");

    while(1) {
        switch(listen_rdp(timeout)) {
            case event_recieved: {
                if(rdp_flags() & rdp_ACK) {
                    if(rdp_seq_ack_number() == sequence_number + 1) {
                        return;
                    }
                } else if(rdp_flags() & rdp_RST) {
                    timeout_count = 0;
                    if(reset_count++ > MAXIMUM_RESETS) {
                        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
                    }
                } else {
                    rdp_log("Unexpected packet!");
                }
                send_rdp("S", rdp_FIN, sequence_number, 0, "");
                break;
            }
            case event_bad_packet: {
                send_rdp("S", rdp_FIN, sequence_number, 0, "");
                break;
            }
            case event_timeout: {
                timeout = (int) timeout * 1.1;
                timeout_count++;
                if(timeout_count > MAXIMUM_TIMEOUTS) {
                    rdp_exit(EXIT_FAILURE, "RDP transimmision as the connection timed out too many times.");
                }
                send_rdp("S", rdp_FIN, sequence_number, 0, "");
                break;
            }
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
        "total time duration (second): %d\n",
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
    strcpy(source_ip, reciever_ip);
    strcpy(source_port, reciever_port);
    rdp_zero(destination_ip, ADDR_SIZE);
    rdp_zero(destination_port, ADDR_SIZE);

    // Init source socket and socket address
    open_source_socket();
}

void rdp_recieve() {
    char payload_buffer[(WINDOW_SIZE + 1) * rdp_MAX_PACKET_SIZE];
    int sequence_numbers[WINDOW_SIZE];

    int recieved_packets = 0;
    int reset_count      = 0;
    int timeout_count    = 0;
    int timeout          = TIMEOUT;
    int connected        = 0;
    int disconnecting    = 0;

    unint16_t window_size = WINDOW_SIZE;
    unint16_t ack_number  = 0;

    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        sequence_numbers[i] = -1; // Mark as available
    }

    while(1) {
        switch(listen_rdp(timeout)) {
            case event_recieved: {
                if(rdp_flags() & rdp_SYN) {
                    connected  = 1;
                    ack_number = rdp_seq_ack_number() + 1;
                    send_rdp("s", rdp_ACK, ack_number, window_size, "");
                } else if(rdp_flags() & rdp_FIN) {
                    disconnecting = 1;
                    ack_number    = rdp_seq_ack_number() + 1;
                    send_rdp("s", rdp_ACK, ack_number, window_size, "");
                } else if(rdp_flags() & rdp_DAT) {

                    // In order check
                    if(rdp_seq_ack_number() == ack_number) {

                        rdp_filestream_write(rdp_payload(), rdp_payload_size());
                        ack_number += rdp_payload_size();

                        // Dequeue data
                        if(window_size != WINDOW_SIZE) {
                            int dequeue = 0;
                            do {
                                for(i = 0; i < WINDOW_SIZE; i++) {
                                    if(sequence_numbers[i] == ack_number) {

                                        char* payload = payload_buffer + (i * rdp_MAX_PACKET_SIZE);
                                        rdp_filestream_write(payload, strlen(payload));
                                        ack_number         += strlen(payload);
                                        sequence_numbers[i] = -1;
                                        dequeue             = 1;
                                        window_size++;
                                    }
                                }
                            } while(dequeue);
                        }
                    } else if(window_size == 0) {
                        send_rdp("s", rdp_ACK, ack_number, window_size, "");
                        break;
                    } else {
                        // Queue data
                        for(i = 0; i < WINDOW_SIZE; i++) {
                            if(sequence_numbers[i] == -1) {
                                char* payload = rdp_payload();
                                sequence_numbers[i] = rdp_seq_ack_number();
                                rdp_zero(payload_buffer + (i * rdp_MAX_PACKET_SIZE), rdp_MAX_PACKET_SIZE);
                                memcpy(payload_buffer + (i * rdp_MAX_PACKET_SIZE), &payload, rdp_payload_size());
                                break;
                            }
                        }
                        if(--window_size == 0 || ++recieved_packets > WINDOW_SIZE) {
                            recieved_packets = 0;
                            send_rdp("s", rdp_ACK, ack_number, window_size, "");
                        }
                    }
                } else if(rdp_flags() & rdp_RST) {
                    timeout_count = 0;
                    if(reset_count++ > MAXIMUM_RESETS) {
                        rdp_exit(EXIT_FAILURE, "RDP transmission failed as the connection was reset too many times.");
                    }
                } else {
                    rdp_log("Unexpected packet!");
                }
                break;
            }
            case event_bad_packet: {
                if(connected) {
                    send_rdp("s", rdp_ACK, ack_number, window_size, "");
                }
                break;
            }
            case event_timeout: {
                rdp_log("Timed out! %d", disconnecting);
                if(connected) {
                    recieved_packets = 0;
                    send_rdp("s", rdp_ACK, ack_number, window_size, "");
                }
                if(disconnecting) {
                    return;
                }
                break;
            }
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