#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include "format.h"

static const char* TRANSFER_TYPE[] = {
	"ISOC", "INTR", "CTRL", "BULK"
};

char* format_timestamp(int64_t seconds, int32_t microseconds, char* buffer, size_t buffer_size) {
	struct tm time;
	localtime_r(&seconds, &time);

	strftime(buffer, buffer_size, "%FT%H:%M:%S.      %z", &time);

	char micro_str[7];
	snprintf(micro_str, sizeof(micro_str), "%06" PRIu32, microseconds);

	char *s;
	char *d;
	for (s = micro_str, d = buffer + 20; s < micro_str + 6; s++, d++) {
		*d = *s;
	}

	return buffer;
}

const char* format_transfer_type(uint8_t transfer_type) {
	return TRANSFER_TYPE[transfer_type];
}

const char* format_packet_direction(uint8_t type) {
	switch (type) {
		case 'C':
			return "<";
		case 'S':
			return ">";
		case 'E':
			return "E";
		default:
			return "?";
	}
}

const char* format_in_out(uint8_t endpoint) {
	return (endpoint & 0x80) ? "IN" : "OUT";
}

char* format_status(uint8_t type, uint32_t status, char* buffer, size_t buffer_length) {
	if (buffer_length == 0) {
		return NULL;
	}

	if (type == 'S') {
		*buffer = '\0';
		return buffer;
	}

	snprintf(buffer, buffer_length, " = %" PRIi32, status);

	return buffer;
}
