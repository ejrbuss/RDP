#ifndef RDP_PROTOCOL_HEADER
#define RDP_PROTOCOL_HEADER

#include <stdint.h>
// Manually define uint8_t and uint16_t due to compilation erros when
// including headers. This assumes char is 1 bytes and short is 2, however
// this assumption does not need to hold for the code to function. What matters
// is that the values are encoded as unsigned valeus so that runcated integers
// are equivalent regardless of size.
//typedef unsigned char  uint8_t;
//typedef unsigned short uint16_t;
//typedef unsigned int   uint32_t;

// Defines the full size of the header
#define rdp_HEADER_SIZE 15

// Bit masks for packet flags
#define rdp_ACK ((uint8_t) 0b00000001) // Acknowledgment flag
#define rdp_SYN ((uint8_t) 0b00000010) // Synchrnoize flag
#define rdp_FIN ((uint8_t) 0b00000100) // Finish flag
#define rdp_RST ((uint8_t) 0b00001000) // Reset flag
#define rdp_DAT ((uint8_t) 0b00010000) // Data flag
#define rdp_RES ((uint8_t) 0b00100000) // Resend flag
#define rdp_UN1 ((uint8_t) 0b01000000) // Undefined 1
#define rdp_UN2 ((uint8_t) 0b10000000) // Undefiend 2

// Always 0 pad just in case
#define rdp_MAX_PACKET_SIZE 1023

// Defines names based of packet flags
extern const char* rdp_flag_names[];

// Exported funtions
extern uint16_t  rdp_packed_size(const uint16_t payload_size);
extern char* rdp_pack(
    char* buffer,
    const uint8_t flags,
    const uint32_t seq_ack_number,
    const uint16_t size,
    const char* payload
);
extern int rdp_parse(char* buffer);
extern uint16_t rdp_checksum(
    const uint8_t flags,
    const uint32_t seq_ack_number,
    const uint16_t size,
    const char* payload
);
extern uint16_t rdp_flags();
extern uint32_t rdp_seq_ack_number();
extern uint16_t rdp_payload_size();
extern uint16_t rdp_window_size();
extern uint16_t rdp_size();
extern char* rdp_payload();

#endif