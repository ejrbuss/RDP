#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "filestream.h"
#include "netconfig.h"
#include "util.h"

/* Stat tracking */
static int stats[stat_length];

/* Buffers */
static char recieve_buffer[SOCK_BUFFER_SIZE];
static char send_buffer[SOCK_BUFFER_SIZE];
static char source_ip[ADDR_SIZE];
static char destination_ip[ADDR_SIZE];
static char source_port[ADDR_SIZE];
static char destination_port[ADDR_SIZE];

/* Sockets and addresses */
static int source_socket;
static int destination_socket;
static struct sockaddr_in source_address;
static struct sockaddr_in destination_address;

/* Flags */
static int source_bound      = 0;
static int destination_bound = 0;
static int timedout          = 0;

/**
 * Binds the source socket for rdp_listen. Packets will become recievable after
 * this function has been called.
 *
 * @param const char* ip   the source IP address
 * @param const char* port the source port number
 */
void rdp_open_source_socket(const char* ip, const char* port) {

    stats[stat_start_time] = time(NULL);
    rdp_log("STAT START TIME: %d\n", stats[stat_start_time]);

    // Copy source IP and port
    strcpy(source_ip, ip);
    strcpy(source_port, port);

    // Log socket creation
    rdp_log("Opening source socket...\n"
            "    IP:   %s\n"
            "    Port: %s\n",
        ip, port
    );

    // Create socket
    source_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (source_socket == -1) {
        rdp_exit(EXIT_FAILURE, "Unable to create sender socket: %s\n", strerror(errno));
    }

    // Let go of socket on exit/crash
    int option = 1;
    if(-1 == setsockopt(
        source_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const void*) &option,
        sizeof(int)
    )) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "Failed to set socket option: %s\n", strerror(errno));
    }

    // Create and bind address to socket
    memset(&source_address, 0, sizeof(source_address));
    source_address.sin_family      = AF_INET;
    source_address.sin_addr.s_addr = inet_addr(source_ip);
    source_address.sin_port        = htons(atoi(source_port));
    if (-1 == bind(
        source_socket,
        (struct sockaddr*) &source_address,
        sizeof(source_address)
    )) {
        rdp_close_sockets();        rdp_exit(EXIT_FAILURE, "Failed to bind socket: %s\n", strerror(errno));
    }

    // Prepare stats
    int i;
    for(i = 0; i < stat_length; i++) {
        stats[i] = 0;
    }

    source_bound = 1;
}

/**
 * Creates the socket for rdp_send. After this function has been called packets
 * can be sent.
 *
 * @param const char* ip   the destiantion IP address
 * @param const char* port the destination port number
 */
void rdp_open_destination_socket(const char* ip, const char* port) {

    // Copy destination IP and port
    strcpy(destination_ip, ip);
    strcpy(destination_port, port);

    // Log socket creation
    rdp_log("Opening destination socket...\n"
            "    IP:   %s\n"
            "    Port: %s\n",
        ip, port
    );

    //Create socket
    destination_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (destination_socket == -1) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "Unable to create reciever socket");
    }

    // Create address
    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family      = AF_INET;
    destination_address.sin_addr.s_addr = inet_addr(destination_ip);
    destination_address.sin_port        = htons(atoi(destination_port));

    destination_bound = 1;
}

/**
 * Closes any and all open sockets.
 */
void rdp_close_sockets() {
    if(source_bound) {
        close(source_socket);
    }
    stats[stat_end_time] = time(NULL);
}

/**
 * Listens for a set number of milliseconds for a packet ont the source socket.
 * Returns an event indicating what occured.
 *
 * @param   const int timeout_milli
 * @returns int       the event
 */
int rdp_listen(const int timeout_milli) {

    int select_result;
    ssize_t recsize;
    struct timeval timeout;
    socklen_t socket_length = sizeof(destination_address);
    fd_set read_fds;

    if(!timedout) {
        rdp_log("Listening...");
    } else {
        timedout = 0;
    }

    FD_ZERO(&read_fds);
    FD_SET(source_socket, &read_fds);

    // Set timeout
    timeout.tv_sec  = 0;
    timeout.tv_usec = timeout_milli * 1000;

    // Wait on select
    select_result = select(source_socket + 1, &read_fds, NULL, NULL, &timeout);

    // Check result
    switch(select_result) {
        case -1:
            rdp_close_sockets();
            rdp_exit(EXIT_FAILURE, "Error in select: %s\n", strerror(errno));
        case 0:
            timedout = 1;
            return event_timeout;
        default: break;
    }

    // Recieve and parse the packet
    rdp_zero(recieve_buffer, SOCK_BUFFER_SIZE);
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
    if(!destination_bound) {
        char ip[ADDR_SIZE];
        char port[ADDR_SIZE];
        strcpy(ip, inet_ntoa(destination_address.sin_addr));
        sprintf(port, "%d", ntohs(destination_address.sin_port));
        if (ip == NULL) {
            rdp_close_sockets();
            rdp_exit(EXIT_FAILURE, "Failed to read destination IP:port from recieved packet: %s\n", strerror(errno));
        }
        rdp_open_destination_socket(ip, port);
    }

    // Log the packet
    rdp_log_packet(
        rdp_flags() & rdp_RES ? "R" : "r",
        destination_ip,
        destination_port,
        source_ip,
        source_port,
        rdp_flag_names[rdp_flags() & 0b11111],
        rdp_seq_ack_number(),
        rdp_payload_size() | rdp_window_size()
    );
    // For debuggin
    // rdp_log_hex(rdp_payload(), rdp_payload_size());

    // Compute statistics
    stats[stat_recieved_SYN]          += !!(rdp_flags() & rdp_SYN);
    stats[stat_recieved_FIN]          += !!(rdp_flags() & rdp_FIN);
    stats[stat_recieved_DAT]          += !!(rdp_flags() & rdp_DAT);
    stats[stat_recieved_ACK]          += !!(rdp_flags() & rdp_ACK);
    stats[stat_recieved_RST]          += !!(rdp_flags() & rdp_RST);
    stats[stat_recieved_bytes_unique] += !(rdp_flags() & rdp_RES) * rdp_size();
    stats[stat_recieved_bytes]        += rdp_size();

    // Determine event
    if(parse == 0) {
        return event_bad_packet;
    }
    if(rdp_flags() & rdp_SYN) return event_SYN;
    if(rdp_flags() & rdp_FIN) return event_FIN;
    if(rdp_flags() & rdp_DAT) return event_DAT;
    if(rdp_flags() & rdp_RST) return event_RST;
    if(rdp_flags() & rdp_ACK) return event_ACK;
    return event_bad_packet;
}

/**
 * Send a packet.
 *
 * @param const unint16_t flags
 * @param const unint16_t seq_ack_number
 * @param const unint16_t size
 * @param const char*     payload
 */
void rdp_send(
    const unint16_t flags,
    const unint16_t seq_ack_number,
    const unint16_t size,
    const char* payload
) {
    int packed_size = rdp_packed_size(flags & rdp_DAT ? size : 0);

    // Compute statistics
    stats[stat_sent_SYN]          += !!(flags & rdp_SYN);
    stats[stat_sent_FIN]          += !!(flags & rdp_FIN);
    stats[stat_sent_DAT]          += !!(flags & rdp_DAT);
    stats[stat_sent_RST]          += !!(flags & rdp_RST);
    stats[stat_sent_ACK]          += !!(flags & rdp_ACK);
    stats[stat_sent_bytes_unique] += !(flags & rdp_RES) * packed_size;
    stats[stat_sent_bytes]        += packed_size;

    if(sendto(
        source_socket,
        rdp_pack(send_buffer, flags, seq_ack_number, size, payload),
        packed_size,
        0,
        (struct sockaddr*) &destination_address,
        sizeof(destination_address)
    ) < 0) {
        rdp_close_sockets();
        rdp_exit(EXIT_FAILURE, "Error sending packet: %s\n", strerror(errno));
    } else {
        rdp_log_packet(
            flags & rdp_RES ? "S" : "s",
            source_ip,
            source_port,
            destination_ip,
            destination_port,
            rdp_flag_names[flags & 0b11111],
            seq_ack_number,
            size
        );
    }
}

/**
 * Retrieve statistics.
 *
 * @returns int* the array of stats
 */
int* rdp_stats() {
    return stats;
}