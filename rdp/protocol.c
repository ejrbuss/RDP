#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "protocol.h"
#include "netconfig.h"
#include "util.h"

#define header_size 13

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

unint16_t  flags;
unint16_t  seq_number;
unint16_t  ack_number;
unint16_t  window_size;
unint16_t  payload_size;
char* payload;

unint16_t rdp_packaged_size(const unint16_t  payload_size) {
    return payload_size + header_size;
}

void rdp_package(
    char* buffer,
    const unint16_t  flags,
    const unint16_t  seq_number,
    const unint16_t  ack_number,
    const unint16_t  size,
    const char* payload
) {
    // Get total packet size
    unint16_t  size = rdp_packaged_size(payload_size);
    char* _magic_ = "CSC361";
    // Zero the buffer
    rdp_zero(buffer, size + 1);
    // Serialize
    memcpy(buffer + 0, _magic_,      6);
    memcpy(buffer + 6,  &flags,      1);
    memcpy(buffer + 7,  &seq_number, 2);
    memcpy(buffer + 9,  &ack_number, 2);
    memcpy(buffer + 11, size,        2);
    if(flags & rdp_DAT) {
        memcpy(buffer + header_size, payload, size);
    }
}

int rdp_parse(char* buffer) {

    char _magic_[7];
    rdp_zero(_magic_, 7);
    flags        = 0;
    seq_number   = 0;
    ack_number   = 0;
    window_size  = 0;
    payload_size = 0;

    // Deserialize
    memcpy(_magic_,       buffer + 0,  6);
    memcpy(&flags,        buffer + 6,  1);
    memcpy(&seq_number,   buffer + 7,  2);
    memcpy(&ack_number,   buffer + 9,  2);
    if(flags & rdp_DAT) {
        memcpy(&payload_size, buffer + 11, 2);
        window_size = 0;
    } else {
        memcpy(&window_size,  buffer + 11, 2);
        payload_size = 0;
    }
    payload = buffer + header_size;
    return rdp_streq(_magic_, "CSC361");
}

unint16_t  rdp_flags() {
    return flags;
}

unint16_t  rdp_seq_number() {
    return seq_number;
}

unint16_t  rdp_ack_number() {
    return ack_number;
}

unint16_t  rdp_window_size() {
    return window_size;
}

unint16_t  rdp_payload_size() {
    return payload_size;
}

void rdp_payload(char* buffer) {
    rdp_zero(buffer, payload_size + 2);
    memcpy(buffer, payload, payload_size);
}