#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rdp/rdp.h"

void rdps_test();

static const char* usage =
"Reciever Demo usage:\n\n"
"  rdpr [options] <IP> <port> <file>\n\n";

int main(int argc, char** argv) {

    enum {receiver_ip, receiver_port, receiver_file_name};
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
                "Recieves a file from rdps.\n"
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
        case 3: rdp_exit(EXIT_FAILURE, "Missing required argument file\n%s", usage);
    }

    // Configure network
    rdp_reciever(
        args[receiver_ip],
        args[receiver_port]
    );

    // Open filestream
    rdp_filestream_open(args[receiver_file_name], "w");
    // Recieve file
    rdp_recieve();
    // Close filestream
    rdp_filestream_close();
    // Print stats
    rdp_reciever_stats();

    return EXIT_SUCCESS;
}

void rdps_test() {

    rdp_debug();

    // Testing file streaming
    {
        rdp_filestream_open("./send/sample.txt", "r");
        rdp_filestream_open("./recieve/sample.txt", "w");

        int size = rdp_filestream_size();
        int l;
        int i;

        rdp_log("%d", size);

        for(i = 0; i < size; i += 700) {
            char buffer[701];
            rdp_filestream_write(buffer, rdp_min(700, rdp_filestream_read(buffer, 700, i)));
        }

        rdp_filestream_close();
    }
    // Testing protocol header
    {
        char* payload = "Hello World!!!";
        char read_buffer[1000];
        char write_buffer[1000];

        rdp_package(read_buffer, rdp_DAT, 12, 15, 0, strlen(payload), payload);

        int success = rdp_parse(read_buffer);

        rdp_log("Parsing: %d", success);

        rdp_log("Expected : Acutal");
        rdp_log("Flags - %d : %d", rdp_DAT, rdp_flags());
        rdp_log("Seq # - %d : %d", 12, rdp_seq_number());
        rdp_log("Ack # - %d : %d", 15, rdp_ack_number());
        rdp_log("Wndsz - %d : %d", 0, rdp_window_size());
        rdp_log("Paysz - %d : %d", strlen(payload), rdp_payload_size());

        rdp_payload(write_buffer);
        rdp_log("Payload - %s", write_buffer);

        rdp_log_packet(
            "s",
            "127.0.0.1",
            "3000",
            "10.0.0.1",
            "3001",
            rdp_flag_names[rdp_flags()],
            rdp_seq_number(),
            rdp_ack_number(),
            rdp_window_size() == -1
                ? rdp_payload_size()
                : rdp_window_size()
        );
    }
    // Test printing stats
    {
        rdp_sender_stats();
        rdp_reciever_stats();
    }
    // Test log hex
    {
        rdp_log_hex("Hello World!!! How are you doing today :)");
    }
}