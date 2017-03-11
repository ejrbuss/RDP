#include <stdio.h>
#include <string.h>
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

int flags;
int seq_number;
int ack_number;
int window_size;
int payload_size;
char* payload;

int rdp_packaged_size(const int payload_size) {
    return payload_size + header_size;
}

void rdp_package(
    char* buffer,
    const int flags,
    const int seq_number,
    const int ack_number,
    const int window_size,
    const int payload_size,
    const char* payload
) {
    // Get total packet size
    int size = rdp_packaged_size(payload_size);
    char* _magic_ = "CSC361";
    // Zero the buffer
    rdp_zero(buffer, size + 1);
    // Serialize
    memcpy(buffer + 0, _magic_,        6);
    memcpy(buffer + 6,  &flags,        1);
    memcpy(buffer + 7,  &seq_number,   2);
    memcpy(buffer + 9,  &ack_number,   2);
    if(flags & rdp_DAT) {
        memcpy(buffer + 11, &payload_size, 2);
        memcpy(buffer + header_size, payload, payload_size);
    } else {
        memcpy(buffer + 11, &window_size,  2);
    }
}

int rdp_parse(char* buffer) {

    char _magic_[7];
    rdp_zero(_magic_, 7);

    // Deserialize
    memcpy(_magic_,       buffer + 0,  6);
    memcpy(&flags,        buffer + 6,  1);
    memcpy(&seq_number,   buffer + 7,  2);
    memcpy(&ack_number,   buffer + 9,  2);
    if(flags & rdp_DAT) {
        memcpy(&payload_size, buffer + 11, 2);
        window_size = -1;
    } else {
        memcpy(&window_size,  buffer + 11, 2);
        payload_size = -1;
    }
    payload = buffer + header_size;
    return rdp_streq(_magic_, "CSC361");
}

int rdp_flags() {
    return flags;
}

int rdp_seq_number() {
    return seq_number;
}

int rdp_ack_number() {
    return ack_number;
}

int rdp_window_size() {
    return window_size;
}

int rdp_payload_size() {
    return payload_size;
}

void rdp_payload(char* buffer) {
    rdp_zero(buffer, payload_size + 2);
    memcpy(buffer, payload, payload_size);
}