/**
 * @author ejrbuss
 * @date 2017
 */
#ifndef RDP_NOTCONFIG_HEADER
#define RDP_NETCONFIG_HEADER

#include "protocol.h"

// Net Constants (Change these for performance conerns)
#define SOCK_BUFFER_SIZE 2048
#define ADDR_SIZE        256
#define TIMEOUT          8   // Timeout time in milliseconds
#define MAXIMUM_TIMEOUTS 256 // Maximum number of timeouts
#define MAXIMUM_RESETS   16  // Maximum number of resets
#define WINDOW_SIZE      16  // Maximum number of out of order packets

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
#define stat_received_bytes        4
#define stat_received_bytes_unique 5
#define stat_sent_DAT              6
#define stat_sent_DAT_unique       7
#define stat_received_DAT          8
#define stat_received_DAT_unique   9
#define stat_sent_SYN              10
#define stat_sent_FIN              11
#define stat_sent_RST              12
#define stat_sent_ACK              14
#define stat_received_SYN          15
#define stat_received_FIN          16
#define stat_received_RST          17
#define stat_received_ACK          18
#define stat_length                19

extern void rdp_open_source_socket(const char* ip, const char* port);
extern void rdp_open_destination_socket(const char* ip, const char* port);
extern void rdp_close_sockets();
extern int rdp_listen(const int timeout_milli);
extern void rdp_send(
    const uint16_t flags,
    const uint32_t seq_ack_number,
    const uint16_t size,
    const char* payload
);
extern int* rdp_stats();

#endif