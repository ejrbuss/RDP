/**
 * @author ejrbuss
 * @date 2017
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "protocol.h"
#include "netconfig.h"
#include "util.h"

// Local variables for maintaining parse states
static uint16_t flags;
static uint32_t seq_ack_number;
static uint16_t payload_size;
static uint16_t window_size;
static uint16_t size;
static char payload[rdp_MAX_PACKET_SIZE];

// Packet names based off flags
const char* rdp_flag_names[] = {
    "NO-TYPE",    "ACK",             "SYN",         "ACK/SYN",
    "FIN",        "ACK/FIN",         "SYN/FIN",     "ACK/SYN/FIN",
    "RST",        "ACK/RST",         "SYN/RST",     "ACK/SYN/RST",
    "FIN/RST",    "ACK/FIN/RST",     "SYN/FIN/RST", "SYN/ACK/FIN/RST",
    "DAT",        "ACK/DAT",         "SYN/DAT",     "ACK/SYN/DAT",
    "FIN/DAT",    "ACK/FIN/DAT",     "SYN/FIN/DAT", "ACK/SYN/FIN/DAT",
    "RST/DAT",    "ACK/RST/DAT",     "SYN/RST/DAT", "ACK/SYN/RST/DAT",
    "FIN/RST/DAT","ACK/FIN/RST/DAT", "SYN/FIN/RST", "SYN/ACK/FIN/RST/DAT"
};

/**
 * Calculates the total packet size for a given data payload.
 *
 * @param   const uint16_t payload_size the size of the paylaod in bytes (no padding)
 * @returns uint16_t                    the packed packet size in bytes
 */
uint16_t rdp_packed_size(const uint16_t payload_size) {
    return payload_size + rdp_HEADER_SIZE;
}

/**
 * Packs a buffer (expected to be at least rdp_MAX_PACKET_SIZE in length) for
 * RDP transmission.
 *
 * @param   char*           buffer         a buffer to pack
 * @param   const uint16_t flags          flag byte
 * @param   const uint32_t seq_ack_number sequence or acknowledgement number
 * @param   const uint16_t size           paylod or window size number
 * @param   const char*     payload        a buffer containing the payload
 * @returns char*                          a pointer to the packed buffer
 */
char* rdp_pack(
    char* buffer,
    const uint8_t flags,
    const uint32_t seq_ack_number,
    const uint16_t size,
    const char* payload
) {
    // Get total packet size
    uint16_t checksum = rdp_checksum(flags, seq_ack_number, size, payload);
    char*      _magic_ = "CSC361";

    // Zero the buffer
    rdp_zero(buffer, rdp_MAX_PACKET_SIZE + 1);
    // Serialize
    memcpy(buffer + 0,  _magic_,         6);
    memcpy(buffer + 6,  &flags,          1);
    memcpy(buffer + 7,  &seq_ack_number, 4);
    memcpy(buffer + 11, &size,           2);
    memcpy(buffer + 13, &checksum,       2);
    if(flags & rdp_DAT) {
        memcpy(buffer + rdp_HEADER_SIZE, payload, size);
    }
    // Return packed buffer
    return buffer;
}

/**
 * Reads a received buffer and parses the header and payload. Expects the
 * given buffer to be at least rdp_MAX_PACKET_SIZE in length. Returns a flag
 * indicating if the packet had a valid header and had a matching checksum.
 *
 * @param   char* buffer a buffer containing received bytes
 * @returns int          a flag indicating if the header was valid
 */
int rdp_parse(char* buffer) {

    uint16_t checksum = 0;
    char _magic_[7];
    rdp_zero(payload, rdp_MAX_PACKET_SIZE);
    rdp_zero(_magic_, 7);

    flags          = 0;
    seq_ack_number = 0;
    payload_size   = 0;
    window_size    = 0;

    // Deserialize
    memcpy(_magic_,         buffer + 0,  6);
    memcpy(&flags,          buffer + 6,  1);
    memcpy(&seq_ack_number, buffer + 7,  4);
    memcpy(&size,           buffer + 11, 2);
    memcpy(&checksum,       buffer + 13, 2);
    if(flags & rdp_DAT) {
        memcpy(&payload_size, buffer + 11, 2);
        memcpy(&payload, buffer + rdp_HEADER_SIZE, size);
    } else {
        memcpy(&window_size, buffer + 11, 2);
    }
    size = rdp_packed_size(payload_size);

    // Valdiate header
    return
        rdp_streq(_magic_, "CSC361") &&
        rdp_checksum(flags, seq_ack_number, payload_size | window_size, payload) == checksum;
}

/**
 * Returns a checksum for a packet.
 *
 * @param   const uint8_t  flags          flag byte
 * @param   const uint16_t seq_ack_number sequence or acknowledgement number
 * @param   const uint16_t size           payload or window size number
 * @param   const char*     payload        a buffer containing the payload
 * @returns uint16_t                      checksum of the buffer
 */
uint16_t rdp_checksum(
    const uint8_t flags,
    const uint32_t seq_ack_number,
    const uint16_t size,
    const char* payload
) {

    char buffer[rdp_MAX_PACKET_SIZE];
    char* _magic_    = "CSC361";
    int payload_size = 0;

    memcpy(buffer + 0,  _magic_,         6);
    memcpy(buffer + 6,  &flags,          1);
    memcpy(buffer + 7,  &seq_ack_number, 4);
    memcpy(buffer + 11, &size,           2);
    if(flags & rdp_DAT) {
        memcpy(buffer + rdp_HEADER_SIZE - 2, payload, size);
    };

    uint16_t sum  = 0;
    uint16_t word = 0;

    int i;
    int len = flags & rdp_DAT ? size : 0;
    for(i = 0; i < rdp_packed_size(len - 2) / 2; i++) {
        memcpy(&word, buffer + (i * 2), 2);
        sum += word;
    }
    return ~sum;
}

// Getters
uint16_t rdp_flags() { return flags; }
uint32_t rdp_seq_ack_number() { return seq_ack_number; }
uint16_t rdp_payload_size() { return payload_size; }
uint16_t rdp_window_size() { return window_size; }
uint16_t rdp_size() { return size; }
char* rdp_payload() { return payload; }