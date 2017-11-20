#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "usbmon.h"
#include "format.h"
#include "hexdump.h"

int open_capture_device(char* device) {
	int fd = open(device, O_RDONLY);

	if (fd < 0) {
		error(2, errno, "Failed to open device %s", device);
	}

	if (ioctl(fd, USBMON_IOCT_RING_SIZE, USBMON_RING_BUFFER_SIZE) < 0) {
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

	void* data = malloc(USBMON_RING_BUFFER_SIZE); // Overkill
	if (!data) {
		error(2, errno, "Failed to allocate memory for capturing USB data");
	}

	event->header = header;
	event->data = data;
	event->data_length = USBMON_RING_BUFFER_SIZE;

	if (ioctl(fd, USBMON_IOCX_GETX, event) < 0) {
		error(2, errno, "Failed to capture USB event");
	}

	uint32_t data_length = header->captured_length;

	if (data_length < USBMON_RING_BUFFER_SIZE) {
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

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: u /dev/usbmon<X>\n");
		exit(1);
	}

	char* device = argv[1];
	char timestamp[32];
	char status[12];

	int fd = open_capture_device(device);

	while (1) {
		UsbEvent* event = capture_event(fd);
		UsbEventHeader* header = event->header;

	
		printf("%s %s %03" PRIu16 ":%03" PRIu16 ".%" PRIu8 " [0x%016" PRIx64 "] %s %s (%" PRIu32 "/%" PRIu32 " bytes)%s\n",
			format_packet_direction(header->type),
			format_timestamp(header->timestamp, header->timestamp_microseconds, timestamp, sizeof(timestamp)),
			header->bus_number,
			header->device_number,
			header->endpoint & 0x7f,
			header->id,
			format_transfer_type(header->transfer_type),
			format_in_out(header->endpoint),
			header->captured_length,
			header->length,
			format_status(header->type, header->status, status, sizeof(status))
		);

		hexdump(event->data, event->data_length);

		free_event(event);
	}

	return 0;
}
