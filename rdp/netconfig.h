#ifndef RDP_NOTCONFIG_HEADER
#define RDP_NETCONFIG_HEADER

#define SOCK_BUFFER_SIZE 2000
#define ADDR_SIZE        200
#define TIMEOUT          500
#define MAXIMUM_TIMEOUTS 5
#define MAXIMUM_RESETS   5
#define WINDOW_SIZE      10

// Event names
enum { event_SYN, event_FIN, event_DAT, event_RST, event_ACK, event_bad_packet, event_timeout };

// Stat names
enum {
    stat_start_time,   stat_end_time,
    stat_sent_bytes,   stat_sent_bytes_unique,   stat_recieved_bytes, stat_recieved_bytes_unique,
    stat_sent_DAT,     stat_sent_DAT_unique,     stat_recieved_DAT,   stat_recieved_DAT_unique,
    stat_sent_SYN,     stat_sent_FIN,            stat_sent_RST,       stat_sent_ACK,
    stat_recieved_SYN, stat_recieved_FIN,        stat_recieved_RST,   stat_recieved_ACK,
    stat_length
};

extern void rdp_open_source_socket(const char* ip, const char* port);
extern void rdp_open_destination_socket(const char* ip, const char* port);
extern void rdp_close_sockets();
extern void rdp_send();
extern int rdp_listen();
extern int* rdp_stats();

#endif