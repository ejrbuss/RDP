#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "netconfig.h"
#include "protocol.h"
#include "util.h"

#define SOCK_BUFFER_SIZE 2000
#define ADDR_SIZE        200
#define TIMEOUT          500

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

/* listen_rdp variables */

enum { event_recieved, event_timeout };

char recieve_buffer[SOCK_BUFFER_SIZE];
char send_buffer[SOCK_BUFFER_SIZE];

int source_socket;
struct sockaddr_in source_address;
int destination_socket;
struct sockaddr_in destination_address;

int seq_number;
int ack_number;

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
        (const void *) &option,
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
    switch (select_result) {
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
    rdp_parse(recieve_buffer);

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
        rdp_seq_number(),
        rdp_ack_number(),
        rdp_window_size() | rdp_payload_size()
    );

    rdp_log_hex(recieve_buffer, rdp_packaged_size(rdp_payload_size()));

    return event_recieved;
}

void send_rdp(
    const char* event,
    const unint16_t flags,
    const unint16_t seq_number,
    const unint16_t ack_number,
    const unint16_t size,
    const char* payload
) {
    rdp_package(
        send_buffer,
        flags,
        seq_number,
        ack_number,
        size,
        payload
    );
    if(sendto(
        source_socket,
        send_buffer,
        rdp_packaged_size(flags & rdp_DAT ? size : 0),
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
            seq_number,
            ack_number,
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

    // SEND SYN packet

    // init seq number
    int seq = rdp_get_seq_number();

    send_rdp("s", rdp_SYN, seq, 0, 0, "");

    while(1) {
        int event = listen_rdp(TIMEOUT);
        if(event == event_recieved) {
            if(rdp_flags() & rdp_ACK) {
                stat_recieved_ack_packets++;
                if(rdp_ack_number() == seq + 1) { // Check sequence number
                    seq++;
                    return;
                }
                send_rdp("S", rdp_SYN, seq, 0, 0, "");
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                send_rdp("S", rdp_SYN, seq, 0, 0, "");
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer, rdp_packaged_size(rdp_payload_size()));
            }
        } else {
            // timeout
            send_rdp("S", rdp_SYN, seq, 0, 0, send_buffer);
            // resend
            // extend timer
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
        send_rdp("s", rdp_DAT, 0, 0, 700, payload);
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

    // SEND FIN packet
    send_rdp("s", rdp_FIN, ++seq_number, 0, 0, "");

    while(1) {
        int event = listen_rdp(TIMEOUT);
        if(event == event_recieved) {
            if(rdp_flags() & rdp_ACK) {
                stat_recieved_ack_packets++;
                if(rdp_ack_number() == seq_number + 1) {
                    return;
                }
                send_rdp("S", rdp_FIN, ++seq_number, 0, 0, "");
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                send_rdp("S", rdp_FIN, ++seq_number, 0, 0, "");
                // try again
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer, rdp_packaged_size(rdp_payload_size()));
            }
        } else {
            // timeout
            send_rdp("S", rdp_FIN, ++seq_number, 0, 0, "");
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
    while(1) {
        int event = listen_rdp(TIMEOUT);
        if(event == event_recieved) {
            if(rdp_flags() & rdp_SYN) {
                stat_recieved_syn_packets++;
                send_rdp("s", rdp_ACK, 0, rdp_seq_number() + 1, 0, "");
            } else if(rdp_flags() & rdp_FIN) {
                stat_recieved_fin_packets++;
                send_rdp("s", rdp_ACK, 0, rdp_seq_number() + 1, 0, "");
                return;
                // send ack
                // return
            } else if(rdp_flags() & rdp_RST) {
                stat_recieved_rst_packets++;
                // try again
            } else if(rdp_flags() & rdp_DAT) {
                stat_recieved_dat_packets++;
                char buffer[SOCK_BUFFER_SIZE];
                rdp_payload(buffer);
                rdp_filestream_write(buffer, 700);
                // read data
                // check if we should send ack
                    // send ack
            } else {
                rdp_log("Unkown packet:");
                rdp_log_hex(recieve_buffer, rdp_packaged_size(rdp_payload_size()));
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