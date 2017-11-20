#include <inttypes.h>

// See: https://github.com/torvalds/linux/blob/master/Documentation/usb/usbmon.txt

#define USBMON_RING_BUFFER_SIZE (1200 * 1024)
#define USBMON_IOC_MAGIC 0x92

typedef struct {
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

typedef struct {
	UsbEventHeader *header;
	void *data;
	size_t data_length;
} UsbEvent;

#define USBMON_IOCT_RING_SIZE _IO(USBMON_IOC_MAGIC, 4)
#define USBMON_IOCX_GETX      _IOW(USBMON_IOC_MAGIC, 10, UsbEvent)

int open_capture_device(char* device);
UsbEvent* capture_event(int fd);
void free_event(UsbEvent* event);
