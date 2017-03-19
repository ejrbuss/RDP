#ifndef RDP_NOTCONFIG_HEADER
#define RDP_NETCONFIG_HEADER

#define SOCK_BUFFER_SIZE 2000
#define ADDR_SIZE        200
#define TIMEOUT          500
#define MAXIMUM_TIMEOUTS 5
#define MAXIMUM_RESETS   5
#define WINDOW_SIZE      10

// Event names
extern enum event_t;

// Stat names
extern enum stat_t;

extern void rdp_open_source_socket(const char* ip, const char* port);
extern void rdp_open_destination_socket(const char* ip, const char* port);
extern void rdp_close_sockets();
extern void rdp_send();
extern int rdp_listen();
extern int* rdp_stats();

#endif