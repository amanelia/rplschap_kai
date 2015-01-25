/*
 * BDAV rpls parser
 * author: amanelia
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifndef _UTIL_H_
#include "util.h"
#endif
#include "rpls_parse.h"

int read_rpls(const char *filename, rpls_t *rp) {
	int fd;
	unsigned long length;
	unsigned char *buf;
	int pos, i;
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "rplsファイルオープンに失敗しました。\n");
		return 0;
	}
	lseek(fd, 0, SEEK_END);
	length = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);

	buf = (unsigned char *) malloc(length);
	if (!buf) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	memset(buf, 0, length);
	read(fd, buf, length);
	rp->playarea_address = get32bit(buf, 0x08);
	rp->cpi = buf[rp->playarea_address + 5] & 0x01;
	rp->num_list = get16bit(buf, rp->playarea_address + 6);
	memcpy(rp->clpi_filename, &buf[0] + rp->playarea_address + 12, 5);
	rp->clpi_filename[6] = '\0';
	memcpy(rp->clpi_codec_identifier, &buf[0] + rp->playarea_address + 17, 4);
	pos = rp->playarea_address + 8 + 2;
	for (i = 0; i < rp->num_list; i++) {
		rp->connection = buf[pos + 12];
		rp->stc_id = buf[pos + 13];
		rp->time_in = get32bit(buf, pos + 14);
		rp->time_out = get32bit(buf, pos + 18);
		pos += 22;
	}
	rp->start_addr = get32bit(buf, 0x0C);
	rp->end_addr = get32bit(buf, 0x10);
	rp->len = get32bit(buf, rp->start_addr);
	rp->num_timecode = buf[rp->start_addr + 4] << 8 | buf[rp->start_addr + 5];
	rp->timecode = (unsigned long *) malloc(sizeof(unsigned long) * rp->num_timecode);
	if (!rp->timecode) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	memset(rp->timecode, 0, sizeof(unsigned long) * rp->num_timecode);
	pos = rp->start_addr + 6;
	for (i = 0; i < rp->num_timecode; i++) {
		rp->timecode[i] = get32bit(buf, pos + 6);
		pos += 46;
	}
	free(buf);
	close(fd);
	return 1;
}

void free_rpls(rpls_t *rp) {
	free(rp->timecode);
	free(rp);
}
