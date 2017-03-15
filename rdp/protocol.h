#ifndef RDP_PROTOCOL_HEADER
#define RDP_PROTOCOL_HEADER

#include <cstdint>

#define rdp_ACK ((unint8_t) 0b00000001)
#define rdp_SYN ((unint8_t) 0b00000010)
#define rdp_FIN ((unint8_t) 0b00000100)
#define rdp_RST ((unint8_t) 0b00001000)
#define rdp_DAT ((unint8_t) 0b00010000)

#define rdp_MAX_PACKET_SIZE 1023 // Always 0 pad just in case

extern const char* rdp_flag_names[];

extern unint16_t  rdp_packaged_size(const unint16_t payload_size);
extern void rdp_package(
    char* buffer,
    const unint16_t flags,
    const unint16_t seq_number,
    const unint16_t ack_number,
    const unint16_t window_size,
    const unint16_t payload_size,
    const char* payload
);
extern int rdp_parse(char* buffer);
extern unint16_t  rdp_flags();
extern unint16_t  rdp_seq_number();
extern unint16_t  rdp_ack_number();
extern unint16_t  rdp_window_size();
extern unint16_t  rdp_payload_size();
extern void rdp_payload(char* buffer);

#endif