#include <inttypes.h>

char* format_timestamp(int64_t seconds, int32_t microseconds, char* buffer, size_t buffer_size);
const char* format_transfer_type(uint8_t transfer_type);
const char* format_packet_direction(uint8_t type);
const char* format_in_out(uint8_t endpoint);
char* format_status(uint8_t type, uint32_t status, char* buffer, size_t buffer_length);
