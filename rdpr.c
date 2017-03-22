/**
 * @author ejrbuss
 * @date 2017
 *
 * RDPR
 *
 * Code entry point for the rdp receiver demo. This file is responsible for
 * parsing command line options and calling the appropriate functions from the
 * rdp library.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rdp/rdp.h"

void rdpr_test();

static const char* usage =
"receiver Demo usage:\n\n"
"  rdpr [options] <IP> <port> <file>\n\n";

int main(int argc, char** argv) {

    enum { receiver_ip, receiver_port, receiver_file_name };
    char* args[4];
    int arg = 0;
    int i;

    if(argc == 1) {
        rdp_exit(EXIT_FAILURE, "%s", usage);
    }
    for(i = 1; i < argc; i++) {
        rdp_opt(argv[i]);
        if(rdp_opt_cmp("-h") || rdp_opt_cmp("--help")) {
            rdp_exit(EXIT_SUCCESS,
                "%s%s"
                "receives a file from rdps.\n"
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
            rdpr_test(); exit(EXIT_SUCCESS);
        }
        rdp_log("found arg: %s", argv[i]);
        args[arg++] = argv[i];
    }

    // Test arguments
    switch(arg) {
        case 0: rdp_exit(EXIT_FAILURE, "Missing required argument IP\n%s", usage);
        case 1: rdp_exit(EXIT_FAILURE, "Missing required argument port\n%s", usage);
        case 2: rdp_exit(EXIT_FAILURE, "Missing required argument file\n%s", usage);
        default: break;
    }

    // Open filestream
    rdp_filestream_open(args[receiver_file_name], "w");
    // Configure network
    rdp_receiver(args[receiver_ip], args[receiver_port]);
    // receive file
    rdp_receiver_receive();
    // Close filestream
    rdp_filestream_close();
    // Print stats
    rdp_receiver_stats();

    return EXIT_SUCCESS;
}

/**
 * Run a default configuration
 */
void rdpr_test() {
    rdp_filestream_open("receive/test.dat", "w");
    rdp_receiver("10.10.1.100", "9001");
    rdp_receiver_receive();
    rdp_filestream_close();
    rdp_receiver_stats();
}