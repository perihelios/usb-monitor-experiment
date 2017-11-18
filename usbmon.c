#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

typedef struct _UsbEventHeader {
	uint64_t id;
	uint8_t type;
	uint8_t transfer_type;
	uint8_t endpoint;
	uint8_t device_number;
	uint16_t bus_number;
	int8_t flag_setup;
	int8_t flag_data;
	int64_t timestamp;
	int32_t timestamp_microseconds;
	int32_t status;
	uint32_t length;
	uint32_t captured_length;

	union {
		uint8_t setup[8];
		struct iso_record {
			int32_t error_count;
			int32_t descriptor_count;
		} iso;
	};

	int32_t interval;
	int32_t start_frame;
	uint32_t transfer_flags;
	uint32_t captured_iso_descriptor_count;
} UsbEventHeader;

typedef struct _UsbEvent {
	UsbEventHeader *header;
	void *data;
	size_t data_length;
} UsbEvent;

char* TRANSFER_TYPE[] = {
	"ISOC", "INTR", "CTRL", "BULK"
};

char HEX[] = "0123456789abcdef";

#define MON_IOC_MAGIC      0x92
#define MON_IOCT_RING_SIZE _IO(MON_IOC_MAGIC, 4)
#define MON_IOCX_GETX      _IOW(MON_IOC_MAGIC, 10, UsbEvent)

#define RING_BUFFER_SIZE (1200 * 1024)

int open_capture_device(char* device) {
	int fd = open(device, O_RDONLY);

	if (fd < 0) {
		error(2, errno, "Failed to open device %s", device);
	}

	if (ioctl(fd, MON_IOCT_RING_SIZE, RING_BUFFER_SIZE) < 0) {
		error(2, errno, "Failed to set ring buffer size for capture on device %s", device);
	}

	return fd;
}

UsbEvent* capture_event(int fd) {
	UsbEventHeader* header = malloc(sizeof(UsbEventHeader));
	if (!header) {
		error(2, errno, "Failed to allocate memory for capturing USB event header");
	}

	UsbEvent* event = malloc(sizeof(UsbEvent));
	if (!event) {
		error(2, errno, "Failed to allocate memory for capturing USB event");
	}

	void* data = malloc(RING_BUFFER_SIZE); // Overkill
	if (!data) {
		error(2, errno, "Failed to allocate memory for capturing USB data");
	}

	event->header = header;
	event->data = data;
	event->data_length = RING_BUFFER_SIZE;

	if (ioctl(fd, MON_IOCX_GETX, event) < 0) {
		error(2, errno, "Failed to capture USB event");
	}

	uint32_t data_length = header->captured_length;

	if (data_length < RING_BUFFER_SIZE) {
		data = realloc(data, data_length); // Free unneeded memory

		if (data_length && !data) {
			error(2, errno, "Failed to reallocate memory for captured USB data");
		}

		event->data = data;
		event->data_length = data_length;
	}

	return event;
}

void free_event(UsbEvent* event) {
	free(event->header);
	free(event->data);
	free(event);
}

char* format_timestamp(int64_t seconds, int32_t microseconds, char* buffer, size_t buffer_size) {
	struct tm time;
	localtime_r(&seconds, &time);

	strftime(buffer, buffer_size, "%FT%H:%M:%S.      %z", &time);

	char micro_str[7];
	snprintf(micro_str, sizeof(micro_str), "%06" PRIu32, microseconds);

	char *s;
	char *d;
	for (s = micro_str, d = &buffer[20]; s < &micro_str[6]; s++, d++) {
		*d = *s;
	}

	return buffer;
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

// TODO: This is quite messy
void hexdump(uint8_t* data, uint32_t length) {
	char line[80];
	char* x = line;
	char* c = line + 52;
	memset(line, ' ', sizeof(line));
	*c++ = '|';
	*c++ = ' ';
	*x++ = '\t';
	*(line + 74) = '|';
	*(line + 75) = '\0';

	uint32_t pos = 0;

	while (pos < length) {
		uint8_t byte = *data;

		*x++ = HEX[byte >> 4];
		*x++ = HEX[byte & 0xf];
		*x++ = ' ';

		if (byte <= 32 || byte > 127) {
			*c++ = '.';
		} else {
			*c++ = (char) byte;
		}

		data++;
		pos++;

		if (!(pos & 0xf)) {
			puts(line);

			x = line;
			c = line + 52;
			memset(line, ' ', sizeof(line));
			*c++ = '|';
			*c++ = ' ';
			*x++ = '\t';
			*(line + 74) = '|';
			*(line + 75) = '\0';
		} else if (!(pos & 0x3)) {
			*x++ = ' ';
			*c++ = ' ';
		}
	}

	if (pos & 0xf) {
		puts(line);
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: u /dev/usbmon<X>\n");
		exit(1);
	}

	char* device = argv[1];
	char timestamp[32];
	char status[16];

	int fd = open_capture_device(device);

	while (1) {
		UsbEvent* event = capture_event(fd);
		UsbEventHeader* header = event->header;

	
		printf("%s %s Dev:%03" PRIu16 ":%03" PRIu16 " [0x%016" PRIx64 "] %s %s (EndP:%" PRIu8 ") (%" PRIu32 "/%" PRIu32 " bytes)%s\n",
			format_packet_direction(header->type),
			format_timestamp(header->timestamp, header->timestamp_microseconds, timestamp, sizeof(timestamp)),
			header->bus_number,
			header->device_number,
			header->id,
			TRANSFER_TYPE[header->transfer_type],
			format_in_out(header->endpoint),
			header->endpoint & 0x7f,
			header->captured_length,
			header->length,
			format_status(header->type, header->status, status, sizeof(status))
		);

		hexdump(event->data, event->data_length);

		free_event(event);
	}

	return 0;
}
