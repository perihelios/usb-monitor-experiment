#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "hexdump.h"

static const char HEX[] = "0123456789abcdef";

// Line layout:
//
// BlockStart     HSep         HSep         HSep      BlockSep CSep CSep CSep  BlockEnd
//      |            |            |            |             |    |    |    |     |
//  Ind |        Hex |        Hex |        Hex |        Hex  | Chr| Chr| Chr| Chr |Null
//    | |          | |          | |          | |          |  |   ||   ||   ||   | ||
// <--|<|<---------|<|<---------|<|<---------|<|<---------|<-|<--||<--||<--||<--|<||
//     | 30 00 00 00  31 00 00 00  32 00 00 00  33 00 00 00 | 0... 1... 2... 3... |N
// 0.........1.........2.........3.........4.........5.........6.........7.........8.........9
//           0         0         0         0         0         0         0         0         0
//
static const char TEMPLATE[] = "    |                                                    |                     |";

#define HEX_DUMP_LINE_LENGTH (sizeof(TEMPLATE))
#define MAJOR_GROUPING 16 // Always power of 2
#define MINOR_GROUPING 4 // Always power of 2
#define HSTART 6
#define CSTART 59
#define HGAP 1
#define HSEP 2
#define CSEP 1

typedef struct {
	char buffer[HEX_DUMP_LINE_LENGTH];
	char* x;
	char* c;
} HexDumpLine;

static void prep_line(HexDumpLine* line) {
	char* buffer = line->buffer;
	memcpy(buffer, TEMPLATE, HEX_DUMP_LINE_LENGTH);

	line->x = buffer + HSTART;
	line->c = buffer + CSTART;
}

void hexdump(uint8_t* data, uint32_t length) {
	HexDumpLine line;
	prep_line(&line);

	uint32_t pos = 0;

	while (pos < length) {
		uint8_t byte = *data;

		*line.x++ = HEX[byte >> 4];
		*line.x++ = HEX[byte & 0xf];

		if (byte <= 32 || byte >= 127) {
			*line.c++ = '.';
		} else {
			*line.c++ = (char) byte;
		}

		data++;
		pos++;

		if (!(pos & (MAJOR_GROUPING - 1))) {
			puts(line.buffer);

			prep_line(&line);
		} else if (!(pos & (MINOR_GROUPING - 1))) {
			line.x += HSEP;
			line.c += CSEP;
		} else {
			line.x += HGAP;
		}
	}

	if (pos & (MAJOR_GROUPING - 1)) {
		puts(line.buffer);
	}
}
