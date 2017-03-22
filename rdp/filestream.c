/**
 * @author ejrbuss
 * @date 2017
 */
#include <stdio.h>
#include <stdlib.h>
#include "filestream.h"
#include "util.h"

static FILE* file_read  = NULL;
static FILE* file_write = NULL;
static int file_size;

/**
 * Open a filestream for reading or writing. Two file streams can be open at a
 * time, a read stream and a write stream. Exits if the file cannot be found or
 * if a flag other than "r" or "w" is passed.
 *
 * @param const char* file path to file to open
 * @param const char* flag open flags
 */
void rdp_filestream_open(const char* file, const char* flag) {
    rdp_log("Opening %s...", file);
    if(rdp_streq(flag, "w")) {
        file_write = fopen(file, flag);
        if(file_write == NULL) {
            rdp_exit(EXIT_FAILURE, "Cannot open file: %s", file);
        }
        rdp_log("...File opened for writing.\n");
    } else if(rdp_streq(flag, "r")) {
        file_read = fopen(file, flag);
        if(file_read == NULL) {
            rdp_exit(EXIT_FAILURE, "Cannot open file: %s", file);
        }
        rdp_log("...File opened for reading.\n");
    } else {
         rdp_exit(EXIT_FAILURE, "Unrecognized flag: %s", flag);
    }
}

/**
 * Returns the length of the read stream file.
 *
 * @returns int length of the file
 */
int rdp_filestream_size() {
    if(file_read == NULL) {
        rdp_exit(EXIT_FAILURE, "No file opened!");
    }
    fseek(file_read, 0, SEEK_END);
    return (file_size = ftell(file_read));
}

/**
 * Reads content from the read file stream to a buffer from any given point in
 * the file based off the given sequence number.
 *
 * @param   char*     buffer          a buffer to write to
 * @param   const int buffer_size     the size of the buffer
 * @param   const int sequence_number the sequence number to read at
 * @returns int                       the number of bytes read
 */
int rdp_filestream_read(char* buffer, const int buffer_size, const int sequence_number) {
    if(file_read == NULL) {
        rdp_exit(EXIT_FAILURE, "No file opened!");
    }
    if(sequence_number > file_size) {
        rdp_exit(EXIT_FAILURE, "Cannot read byte sequence larger than file size: %d > %d", sequence_number, file_size);
    }
    rdp_zero(buffer, buffer_size);
    fseek(file_read, sequence_number, SEEK_SET);
    return fread(buffer, 1, buffer_size, file_read);
}

/**
 * Append a buffer's content to the write file stream.'
 *
 * @param char*    buffer      a buffer containing content to write
 * @param cont int buffer_size the buffer size
 */
void rdp_filestream_write(char* buffer, const int buffer_size) {
    if(file_write == NULL) {
        rdp_exit(EXIT_FAILURE, "No file opened!");
    }
    fwrite(buffer, 1, buffer_size, file_write);
}

/**
 * Close any open filestreams.
 */
void rdp_filestream_close() {
    if(file_read != NULL) {
        fclose(file_read);
    }
    if(file_write != NULL) {
        fclose(file_write);
    }

}