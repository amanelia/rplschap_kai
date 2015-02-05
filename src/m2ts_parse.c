#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifndef _UTIL_H_
#include "util.h"
#endif
#include "m2ts_parse.h"

packet_header *header;
pes_t *pes;

unsigned long _parse_timestamp(const unsigned char *buf) {
	unsigned long result;
	result = ((unsigned long)(buf[0] & 0x0E)) << 29;
	result |=  buf[1]         << 22;
	result |= (buf[2] & 0xFE) << 14;
	result |=  buf[3]         << 7;
	result |= (buf[4] & 0xFE) >> 1;
	return result;
}

int _parse_pes(const unsigned char *buf, const unsigned long spn, pes_timestamp *pt) {
	unsigned char pts_dts_flag = 0x0;
	unsigned long pts = 0L;
	unsigned long dts = 0L;
	unsigned int pes_prefix = buf[0] << 16 | buf[1] << 8 | buf[2];	
	if (pes_prefix != 1) {
		fprintf(stderr, "invalid PES header (should be 00 00 01).\n");
		return 0;
	}
	unsigned char stream_id = buf[3];
	//unsigned int pes_length = buf[4] << 8 | buf[5];
	if (stream_id == 0xE0 || stream_id == 0xC0) {
		pts_dts_flag = ((buf[7] >> 6) & 0x3);
		if (pts_dts_flag == 0x2) {
			pts = _parse_timestamp(buf + 9);
		} else if (pts_dts_flag == 0x3) {
			pts = _parse_timestamp(buf + 9);
			dts = _parse_timestamp(buf +14);
		}
	} else if (stream_id == 0xBF) {
		printf("type:PrivateStream PES\n");
	} else {
		printf("type:Other PES\n");
	}
	pt->spn = spn;
	pt->stream_id = stream_id;
	pt->pts_dts_flag = pts_dts_flag;
	pt->pts = pts;
	pt->dts = dts;
	return 1;
}

void _free_m2ts() {
	free(pes);
	free(header);
}

int read_m2ts_packet(const char *filename, const unsigned long spn, pes_timestamp *pt) {
	int fd;
	unsigned long fsize;
	unsigned char *buf;
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "cannot open file.\n");
		return 0;
	}
	lseek(fd, 0, SEEK_END);
	fsize = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);
	//printf("size:%ld\n", fsize);
	if (fsize % PACKET_SIZE != 0) {
		fprintf(stderr, "invalid m2ts packet. it file may raw format.\n");
		return 0;
	}
	lseek(fd, spn * PACKET_SIZE, SEEK_SET);
	buf = (unsigned char *) malloc(PACKET_SIZE);
	if (!buf) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	read(fd, buf, PACKET_SIZE);
	header = malloc(sizeof(packet_header));
	memset(header, 0, sizeof(packet_header));
	header->sync = buf[4];
	if (header->sync != 0x47) {
		fprintf(stderr, "Sync word failed.\n");
		return 0;
	}
	header->tp_error_indicator = ((buf[5] >> 7) & 0x1);
	header->pu_start_indicator = ((buf[5] >> 6) & 0x1);
	header->pid = ((buf[5] & 0x1F) << 8) | buf[6];
	if (!header->pu_start_indicator) {
		fprintf(stderr, "no PayloadUnit start Indicator.\n");
		return 0;
	}
	printf("PayloadUnit exists.\n");
	_parse_pes(buf + 8, spn, pt);

	_free_m2ts();
	close(fd);
	return 1;
}
