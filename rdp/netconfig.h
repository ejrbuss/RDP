#ifndef RDP_NOTCONFIG_HEADER
#define RDP_NETCONFIG_HEADER

// Sender
extern void rdp_sender(
    const char* sender_ip,
    const char* sender_port,
    const char* reciever_ip,
    const char* reciever_port
);
ex10tern void rdp_sender_connect();
extern void rdp_send();
extern void rdp_sender_disconnect();
extern void rdp_sender_stats();

// Reciever
extern void rdp_reciever(
    const char* reciever_ip,
    const char* reciever_port
);
extern void rdp_recieve();
extern void rdp_reciever_stats();

#endif