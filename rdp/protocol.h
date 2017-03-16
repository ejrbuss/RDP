#ifndef RDP_PROTOCOL_HEADER
#define RDP_PROTOCOL_HEADER

// #include <stdint.h>
typedef unsigned char  unint8_t;
typedef unsigned short unint16_t;

#define rdp_ACK ((unint8_t) 0b00000001)
#define rdp_SYN ((unint8_t) 0b00000010)
#define rdp_FIN ((unint8_t) 0b00000100)
#define rdp_RST ((unint8_t) 0b00001000)
#define rdp_DAT ((unint8_t) 0b00010000)

#define rdp_MAX_PACKET_SIZE 1023 // Always 0 pad just in case

extern const char* rdp_flag_names[];

extern unint16_t  rdp_packed_size(const unint16_t payload_size);
extern char* rdp_pack(
    char* buffer,
    const unint8_t flags,
    const unint16_t seq_ack_number,
    const unint16_t size,
    const char* payload
);
extern int rdp_parse(char* buffer);
extern unint16_t rdp_checksum(
    const unint8_t flags,
    const unint16_t seq_ack_number,
    const unint16_t size,
    const char* payload
);
extern unint16_t rdp_flags();
extern unint16_t rdp_seq_ack_number();
extern unint16_t rdp_payload_size();
extern unint16_t rdp_window_size();
extern unint16_t rdp_size();
extern char* rdp_payload();

#endif