#ifndef RDP_FILESTREAM_HEADER
#define RDP_FILESTREAM_HEADER

// Exported functions
extern int rdp_filestream_size();
extern void rdp_filestream_open(const char* file, const char* flag);
extern int rdp_filestream_read(char* buffer, const int buffer_size, const int sequence_number);
extern void rdp_filestream_write(char* buffer, const int buffer_size);
extern void rdp_filestream_close();

#endif