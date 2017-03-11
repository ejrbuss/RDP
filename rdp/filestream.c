#include <stdio.h>
#include <stdlib.h>
#include "filestream.h"
#include "util.h"

FILE* file_read  = NULL;
FILE* file_write = NULL;
int size         = 0;

void rdp_filestream_open(const char* file, const char* flag) {
    if(rdp_streq(flag, "w")) {
        file_write = fopen(file, flag);
        if(file_write == NULL) {
            rdp_exit(EXIT_FAILURE, "Cannot open file: %s", file);
        }
    } else if(rdp_streq(flag, "r")) {
        file_read = fopen(file, flag);
        if(file_read == NULL) {
            rdp_exit(EXIT_FAILURE, "Cannot open file: %s", file);
        }
    } else {
         rdp_exit(EXIT_FAILURE, "Unrecognized flag: %s", flag);
    }
}

int rdp_filestream_size() {
    if(file_read == NULL) {
        rdp_exit(EXIT_FAILURE, "No file opened!");
    }
    fseek(file_read, 0, SEEK_END);
    return (size = ftell(file_read));
}

int rdp_filestream_read(char* buffer, const int buffer_size, const int sequence_number) {
    if(file_read == NULL) {
        rdp_exit(EXIT_FAILURE, "No file opened!");
    }
    if(sequence_number > size) {
        rdp_exit(EXIT_FAILURE, "Cannot read byte sequence larger than file size: %d > %d", sequence_number, size);
    }
    rdp_zero(buffer, buffer_size);
    fseek(file_read, sequence_number, SEEK_SET);
    return fread(buffer, 1, buffer_size, file_read);
}

void rdp_filestream_write(char* buffer, const int buffer_size) {
    if(file_write == NULL) {
        rdp_exit(EXIT_FAILURE, "No file opened!");
    }
    fwrite(buffer, 1, buffer_size, file_write);
}

void rdp_filestream_close() {
    if(file_read != NULL) {
        fclose(file_read);
    }
    if(file_write != NULL) {
        fclose(file_write);
    }

}