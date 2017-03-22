#ifndef RDP_SENDER_HEADER
#define RDP_SENDER_HEADER

extern void rdp_sender(
    const char* sender_ip,
    const char* sender_port,
    const char* receiver_ip,
    const char* receiver_port
);
extern void rdp_sender_connect();
extern void rdp_sender_send();
extern void rdp_sender_disconnect();
extern void rdp_sender_stats();

#endif