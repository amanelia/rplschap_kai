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
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	memset(buf, 0, length);
	read(fd, buf, length);
	rp->playarea_address = get32bit(buf, 0x08);
	rp->cpi              = buf[rp->playarea_address + 5] & 0x01;
	rp->num_list         = get16bit(buf, rp->playarea_address + 6);
	rp->clpi_filename         = malloc(sizeof(char) * (5 + 1));
	rp->clpi_codec_identifier = malloc(sizeof(char) * (4 + 1));
	if (!rp->clpi_filename || !rp->clpi_codec_identifier) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	memset(rp->clpi_filename, 0, sizeof(char) * (5 + 1));
	memset(rp->clpi_codec_identifier, 0, sizeof(char) * (4 + 1));
	memcpy(rp->clpi_filename, buf + rp->playarea_address + 12, 5);
	memcpy(rp->clpi_codec_identifier, buf + rp->playarea_address + 17, 4);
	pos = rp->playarea_address + 8 + 2;
	for (i = 0; i < rp->num_list; i++) {
		rp->connection = buf[pos + 12];
		rp->stc_id     = buf[pos + 13];
		rp->time_in    = get32bit(buf, pos + 14);
		rp->time_out   = get32bit(buf, pos + 18);
		pos += 22;
	}
	rp->start_addr   = get32bit(buf, 0x0C);
	rp->end_addr     = get32bit(buf, 0x10);
	rp->len          = get32bit(buf, rp->start_addr);
	rp->num_playlist_marks = buf[rp->start_addr + 4] << 8 | buf[rp->start_addr + 5];
	rp->pl_mark = malloc(sizeof(playlist_mark) * rp->num_playlist_marks);
	if (!rp->pl_mark) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	pos = rp->start_addr + 6;
	for (i = 0; i < rp->num_playlist_marks; i++) {
		playlist_mark pl;
		pl.mark_type           = buf[pos];
		pl.mark_name_length    = buf[pos + 1];
		pl.maker_id            = get16bit(buf, pos + 2);
		pl.ref_to_playitem_id  = get16bit(buf,pos + 4);
		pl.mark_time_stamp     = get32bit(buf, pos + 6);
		pl.entry_es_pid        = get16bit(buf, pos + 10);
		pl.ref_thumbnail_index = get16bit(buf, pos + 12);
		memcpy(rp->pl_mark + i, &pl, sizeof(playlist_mark));
		pos += 46;
	}
	free(buf);
	close(fd);
	return 1;
}

void free_rpls(rpls_t *rp) {
	free(rp->clpi_filename);
	free(rp->clpi_codec_identifier);
	free(rp->pl_mark);
	free(rp);
}
