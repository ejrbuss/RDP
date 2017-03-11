#ifndef RDP_PROTOCOL_HEADER
#define RDP_PROTOCOL_HEADER

#define rdp_ACK ((unsigned char) 0b00000001)
#define rdp_SYN ((unsigned char) 0b00000010)
#define rdp_FIN ((unsigned char) 0b00000100)
#define rdp_RST ((unsigned char) 0b00001000)
#define rdp_DAT ((unsigned char) 0b00010000)

#define rdp_MAX_PACKET_SIZE 1023 // Always 0 pad just in case

extern const char* rdp_flag_names[];

extern int rdp_packaged_size(const int payload_size);
extern void rdp_package(
    char* buffer,
    const int flags,
    const int seq_number,
    const int ack_number,
    const int window_size,
    const int payload_size,
    const char* payload
);
extern int rdp_parse(char* buffer);
extern int rdp_flags();
extern int rdp_seq_number();
extern int rdp_ack_number();
extern int rdp_window_size();
extern int rdp_payload_size();
extern void rdp_payload(char* buffer);

#endif