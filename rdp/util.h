#ifndef RDP_UTIL_HEADER
#define RDP_UTIL_HEADER

#define loop while(1)

extern void rdp_debug();
extern void rdp_exit(const int status, const char *fmt, ...);
extern void rdp_log(const char *fmt, ...);
extern void rdp_log_hex(const char* src, int length);
extern void rdp_no_newlines(char* src);
extern void rdp_log_packet(
    const char* event,
    const char* source_ip,
    const char* source_port,
    const char* destination_ip,
    const char* destination_port,
    const char* packet_type,
    const int seq_ack_number,
    const int size
);
extern char* rdp_opt(char* opt);
extern int rdp_opt_cmp(char* value);
extern void rdp_zero(char* buffer, int buffer_size);
extern int rdp_streq(const char* a, const char* b);
extern int rdp_max(const int a, const int b);
extern int rdp_min(const int a, const int b);
extern int rdp_get_seq_number();

#endif