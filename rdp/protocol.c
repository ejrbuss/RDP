#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "protocol.h"
#include "netconfig.h"
#include "util.h"

// Defines the full size of the header
#define HEADER_SIZE 13

// Local variables for maintaining parse states
unint16_t flags;
unint16_t seq_ack_number;
unint16_t payload_size;
unint16_t window_size;
unint16_t size;
char payload[rdp_MAX_PACKET_SIZE];

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
 * @param   const unint16_t payload_size the size of the paylaod in bytes (no padding)
 * @returns unint16_t                    the packed packet size in bytes
 */
unint16_t rdp_packed_size(const unint16_t payload_size) {
    return payload_size + HEADER_SIZE;
}

/**
 * Packs a buffer (expected to be at least rdp_MAX_PACKET_SIZE in length) for
 * RDP transmission.
 *
 * @param   char*           buffer         a buffer to pack
 * @param   const unint16_t flags          flag byte
 * @param   const unint16_t seq_ack_number sequence or acknowledgement number
 * @param   const unint16_t size           paylod or window size number
 * @param   const char*     payload        a buffer containing the payload
 * @returns char*                          a pointer to the packed buffer
 */
char* rdp_pack(
    char* buffer,
    const unint8_t flags,
    const unint16_t seq_ack_number,
    const unint16_t size,
    const char* payload
) {
    // Get total packet size
    unint16_t checksum = rdp_checksum(flags, seq_ack_number, size, payload);
    char*      _magic_ = "CSC361";
    // Zero the buffer
    rdp_zero(buffer, size + 1);
    // Serialize
    memcpy(buffer + 0, _magic_,         6);
    memcpy(buffer + 6, &flags,          1);
    memcpy(buffer + 7, &seq_ack_number, 2);
    memcpy(buffer + 9, &size,           2);
    memcpy(buffer + 11, &checksum, 2);
    if(flags & rdp_DAT) {
        memcpy(buffer + HEADER_SIZE, payload, size);
    }
    // Return packed buffer
    return buffer;
}
/**
 * Reads a recieved buffer and parses the header and payload. Expects the
 * given buffer to be at least rdp_MAX_PACKET_SIZE in length. Returns a flag
 * indicating if the packet had a valid header and had a matching checksum.
 *
 * @param   char* buffer a buffer containing recieved bytes
 * @returns int          a flag indicating if the header was valid
 */
int rdp_parse(char* buffer) {

    unint16_t checksum = 0;
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
    memcpy(&seq_ack_number, buffer + 7,  2);
    memcpy(&size,           buffer + 9,  2);
    memcpy(&checksum,       buffer + 11, 2);
    if(flags & rdp_DAT) {
        memcpy(&payload_size, buffer + 9, 2);
        memcpy(&payload, buffer + HEADER_SIZE, size);
    } else {
        memcpy(&window_size, buffer + 9, 2);
    }
    size = rdp_packed_size(payload_size);

    // Valdiate header
    return
        rdp_streq(_magic_, "CSC361") &&
        rdp_checksum(flags, seq_ack_number, payload_size, payload) == checksum;
}

/**
 * Returns a checksum for a packet.
 *
 * @param   const unint8_t  flags          flag byte
 * @param   const unint16_t seq_ack_number sequence or acknowledgement number
 * @param   const unint16_t size           payload or window size number
 * @param   const char*     payload        a buffer containing the payload
 * @returns unint16_t                      checksum of the buffer
 */
unint16_t rdp_checksum(
    const unint8_t flags,
    const unint16_t seq_ack_number,
    const unint16_t size,
    const char* payload
) {

    rdp_log("flag: %d\n", flags);
    rdp_log("seqa: %d\n", seq_ack_number);
    rdp_log("size: %d\n", size);
    rdp_log("payl: %s\n", payload);

    char buffer[rdp_MAX_PACKET_SIZE];
    char* _magic_ = "CSC361";

    memcpy(buffer + 0, _magic_,         6);
    memcpy(buffer + 6, &flags,          1);
    memcpy(buffer + 7, &seq_ack_number, 2);
    memcpy(buffer + 9, &size,           2);
    if(flags & rdp_DAT) {
        memcpy(buffer + HEADER_SIZE - 2, payload, size);
    };

    unint16_t sum  = 0;
    unint16_t word = 0;

    int i;
    for(i = 0; i < rdp_packed_size(size - 2) / 2; i++) {
        memcpy(&word, buffer + (i * 2), 2);
        sum += word;
        rdp_log("%d", sum);
    }
    rdp_log("final: %d", ~sum);
    return ~sum;
}

// Getters
unint16_t rdp_flags() { return flags; }
unint16_t rdp_seq_ack_number() { return seq_ack_number; }
unint16_t rdp_payload_size() { return payload_size; }
unint16_t rdp_window_size() { return window_size; }
unint16_t rdp_size() { return size; }
char* rdp_payload() { return payload; }