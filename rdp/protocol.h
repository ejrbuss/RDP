#ifndef RDP_PROTOCOL_HEADER
#define RDP_PROTOCOL_HEADER

// #include <stdint.h>
// Manually define unint8_t and unint16_t due to compilation erros when
// including headers. This assumes char is 1 bytes and short is 2, however
// this assumption does not need to hold for the code to function. What matters
// is that the values are encoded as unsigned valeus so that runcated integers
// are equivalent regardless of size.
typedef unsigned char  unint8_t;
typedef unsigned short unint16_t;

// Bit masks for packet flags
#define rdp_ACK ((unint8_t) 0b00000001)
#define rdp_SYN ((unint8_t) 0b00000010)
#define rdp_FIN ((unint8_t) 0b00000100)
#define rdp_RST ((unint8_t) 0b00001000)
#define rdp_DAT ((unint8_t) 0b00010000)

// Always 0 pad just in case
#define rdp_MAX_PACKET_SIZE 1023

// Defines names based of packet flags
extern const char* rdp_flag_names[];

// Exported funtions
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