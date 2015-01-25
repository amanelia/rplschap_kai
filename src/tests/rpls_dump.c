/**
 * BDAV rpls parser (tests)
 * author: amanelia
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _UTIL_H_
#include "../util.h"
#endif
#include "../rpls_parse.h"

int main(int argc, char **argv) {
	int i;
	rpls_t *rpls;
	rpls = malloc(sizeof(rpls_t));
	memset(rpls, 0, sizeof(rpls_t));
	if (argc < 2) {
		printf("usage:%s XXXXX.rpls\n", argv[0]);
		return 0;
	}
	read_rpls(argv[1], rpls);
	printf("PlayListMarkAddress:0x%lx\n", rpls->playarea_address);
	printf("CPIType  :%d\n", rpls->cpi);
	printf("numofList:%d\n", rpls->num_list);
	printf("filename :%s.clpi\n", rpls->clpi_filename);
	printf("m2ts     :%s.%s\n", rpls->clpi_filename, rpls->clpi_codec_identifier);
	printf("Connection:%d\n", rpls->connection);
	printf("STC_ID   :%d\n", rpls->stc_id);
	printf("TimeIn   :%ld (0x%08lx)\n", rpls->time_in, rpls->time_in);
	printf("TimeOut  :%ld (0x%08lx)\n", rpls->time_out, rpls->time_out);
	printf("StartAddr:0x%lx\n", rpls->start_addr);
	printf("EndAddr  :0x%lx\n", rpls->end_addr);
	printf("len      :0x%lx\n", rpls->len);
	printf("numOfChap:%d\n", rpls->num_timecode);
	int pos = 0;
	for (i = 0; i < rpls->num_timecode; i++) {
		printf("PTS %d    :%ld (0x%08lx)\n", i + 1, rpls->timecode[i], rpls->timecode[i]);
		pos += 4;
	}
	free_rpls(rpls);
	return 1;
}
