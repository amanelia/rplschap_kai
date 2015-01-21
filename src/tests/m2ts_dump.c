#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _UTIL_H_
#include "../util.h"
#endif
#include "../m2ts_parse.h"

int main(int argc, char **argv) {
	char *e;
	pes_timestamp *pt;
	if (argc < 3) {
		printf("usage:m2ts_dump m2tsfile SPN(SourcePacketNumber)");
		return 0;
	}
	if (argv[1]) {
		unsigned long spn = strtoul(argv[2], &e, 10);
		if (!spn) {
			fprintf(stderr, "Cannot convert SPN.\n");
			return 0;
		}
		pt = malloc(sizeof(pes_timestamp));
		if (!pt) {
			fprintf(stderr, "Unallocated memory.\n");
			return 0;
		}
		printf("SourcePacketNumber(=SPN):%ld\n", spn);
		if (!read_m2ts_packet(argv[1], spn, pt)) {
			fprintf(stderr, "Cannot read m2ts PES.\n");
			return 0;
		}
		if (pt->stream_id == 0xE0 || pt->stream_id == 0xC0) {
			printf("ElementaryStream: ");
			if (pt->stream_id == 0xE0)
				printf("VideoPES\n");
			if (pt->stream_id == 0xC0)
				printf("AudioPES\n");
			if (pt->pts_dts_flag == 0x2) {
				printf("PTS: %ld\n", pt->pts);
			} else if (pt->pts_dts_flag == 0x3) {
				printf("PTS: %ld\n", pt->pts);
				printf("DTS: %ld\n", pt->dts);
			}
		} else if (pt->stream_id == 0xBF) {
			printf("PrivateStream2 PES\n");
		} else {
			printf("Other PES\n");
		}
		free(pt);
	}
	return 0;
}
