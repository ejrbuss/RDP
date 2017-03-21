#ifndef RDP_NOTCONFIG_HEADER
#define RDP_NETCONFIG_HEADER

#include "protocol.h"

// Net Constants
#define SOCK_BUFFER_SIZE 2048
#define ADDR_SIZE        256
#define TIMEOUT          4
#define MAXIMUM_TIMEOUTS 128
#define MAXIMUM_RESETS   16
#define WINDOW_SIZE      64

// Events
#define event_SYN        0
#define event_FIN        1
#define event_DAT        2
#define event_RST        3
#define event_ACK        4
#define event_bad_packet 5
#define event_timeout    6

// Stat indexes
#define stat_start_time            0
#define stat_end_time              1
#define stat_sent_bytes            2
#define stat_sent_bytes_unique     3
#define stat_recieved_bytes        4
#define stat_recieved_bytes_unique 5
#define stat_sent_DAT              6
#define stat_sent_DAT_unique       7
#define stat_recieved_DAT          8
#define stat_recieved_DAT_unique   9
#define stat_sent_SYN              10
#define stat_sent_FIN              11
#define stat_sent_RST              12
#define stat_sent_ACK              14
#define stat_recieved_SYN          15
#define stat_recieved_FIN          16
#define stat_recieved_RST          17
#define stat_recieved_ACK          18
#define stat_length                19

extern void rdp_open_source_socket(const char* ip, const char* port);
extern void rdp_open_destination_socket(const char* ip, const char* port);
extern void rdp_close_sockets();
extern int rdp_listen(const int timeout_milli);
extern void rdp_send(
    const unint16_t flags,
    const unint32_t seq_ack_number,
    const unint16_t size,
    const char* payload
);
extern int* rdp_stats();

#endif