#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rdp/rdp.h"

void rdps_test();

static const char* usage =
"Sender Demo usage:\n\n"
"  rdps [options] <IP> <port> <reciever IP> <reciever port> <file>\n\n";

int main(int argc, char** argv) {

    enum {sender_ip, sender_port, receiver_ip, receiver_port, sender_file_name};
    char* args[4];
    int arg = 0;
    int i;

    if(argc == 1) {
        printf("%s%s", rdp_logo, usage);
    }
    for(i = 0; i < argc; i++) {
        rdp_opt(argv[i]);
        if(rdp_opt_cmp("-h") || rdp_opt_cmp("--help")) {
            rdp_exit(EXIT_SUCCESS,
                "%s%s"
                "Sends a file to rdpr.\n"
                "Options:\n"
                "  -h --help    display this message\n"
                "  -v --version display version\n"
                "  -d --debug   enable debug printing\n"
                "  -t --test    run tests\n\n",
                rdp_logo, usage
            );
        }
        if(rdp_opt_cmp("-v") || rdp_opt_cmp("--version")) {
            rdp_exit(EXIT_SUCCESS, "%s", rdp_version);
        }
        if(rdp_opt_cmp("-d") || rdp_opt_cmp("--debug")) {
            rdp_debug(); continue;
        }
        if(rdp_opt_cmp("-t") || rdp_opt_cmp("--test")) {
            rdps_test(); exit(EXIT_SUCCESS);
        }
        args[arg++] = argv[i];
    }

    // Test arguments
    switch(arg) {
        case 0: rdp_exit(EXIT_FAILURE, "Missing required argument IP\n%s", usage);
        case 1: rdp_exit(EXIT_FAILURE, "Missing required argument port\n%s", usage);
        case 2: rdp_exit(EXIT_FAILURE, "Missing required argument reciever IP\n%s", usage);
        case 3: rdp_exit(EXIT_FAILURE, "Missing required argument reciever port\n%s", usage);
        case 4: rdp_exit(EXIT_FAILURE, "Missing required argument file\n%s", usage);
    }

    // Configure network
    rdp_sender(
        args[sender_ip],
        args[sender_port],
        args[receiver_ip],
        args[receiver_port]
    );

    // Open filestream
    rdp_filestream_open(args[sender_file_name], "r");
    // Open connection
    rdp_sender_connect();
    // Send file
    rdp_send();
    // Close connection
    rdp_sender_disconnect();
    // Close filestream
    rdp_filestream_close();

    return EXIT_SUCCESS;
}

void rdps_test() {

}