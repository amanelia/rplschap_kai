#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifndef _UTIL_H_
#include "util.h"
#endif
#include "clpi_parse.h"


int read_clpi(const char *filename, clpi_t *cl) {
	int fd;
	int i,j,pos;
	unsigned long fsize;
	unsigned char *buf;
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "clpiファイルオープンに失敗しました\n");
		return 0;
	}
	lseek(fd, 0, SEEK_END);
	fsize = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);
	
	buf = (unsigned char *) malloc(fsize);
	if (!buf) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	memset(buf, 0, fsize);
	if (!read(fd, buf, fsize)) {
		fprintf(stderr, "read error.\n");
		return 0;
	}
	cl->sequence_info_addr = get32bit(buf, 8);
	cl->program_info_addr  = get32bit(buf, 12);
	cl->cpi_addr           = get32bit(buf, 16);
	cl->clipmark_addr      = get32bit(buf, 24);
	cl->seq  = malloc(sizeof(sequence_info));
	cl->prog = malloc(sizeof(program_info));
	cl->cpi  = malloc(sizeof(cpi_t));
	if (!cl->seq || !cl->prog || !cl->cpi) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	sequence_info *seq = cl->seq;

	//SequenceInfo
	//unsigned int sequence_info_len = get32bit(buf, cl->sequence_info_addr);
	seq->atc_len = buf[cl->sequence_info_addr + 5];
	pos = cl->sequence_info_addr + 6;
	seq->atc = malloc(sizeof(sequence_info_atc) * seq->atc_len);
	if (!seq->atc) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	for (i = 0; i < seq->atc_len; i++) {
		sequence_info_atc atc;
		atc.spn_atc_start = get32bit(buf, pos);
		atc.stc_len       = buf[pos + 4];
		atc.offset_stc_id = buf[pos + 5];
		int pos2 = pos + 6;
		atc.stc = malloc(sizeof(sequence_info_stc) * atc.stc_len);
		if (!atc.stc) {
			fprintf(stderr, "Unallocated memory.\n");
			return 0;
		}
		for (j = 0; j < atc.stc_len; j++) {
			sequence_info_stc stc;
			stc.pcr_pid       = get16bit(buf, pos2);
			stc.spn_stc_start = get32bit(buf, pos2 + 2);
			stc.start_time    = get32bit(buf, pos2 + 6);
			stc.end_time      = get32bit(buf, pos2 + 10);
			pos += 14;
			stc.time0 = 0;
			stc.time1 = (stc.end_time - stc.start_time) / 45000.0;
			memcpy(atc.stc + j, &stc, sizeof(sequence_info_stc));
		}
		memcpy(seq->atc + i, &atc, sizeof(sequence_info_atc));
	}

	//ProgramInfo
	program_info *prog = cl->prog;
	//unsigned long program_info_len = get32bit(buf, cl->program_info_addr);
	prog->num_program = buf[cl->program_info_addr + 5];
	prog->program = malloc(sizeof(program_info_prog) * prog->num_program);
	if (!prog->program) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	pos = cl->program_info_addr + 6;
	for (i = 0; i < prog->num_program; i++) {
		double cur_fps = 0.0;
		program_info_prog program;
		program.spn_program_sequence_start = get32bit(buf, pos);
		program.program_map_pid = get16bit(buf, pos + 4);
		program.num_stream = buf[pos + 6];
		program.num_group  = buf[pos + 7];
		program.stream     = malloc(sizeof(program_info_stream) * program.num_stream);
		if (!program.stream) {
			fprintf(stderr, "Unallocated memory.\n");
			return 0;
		}
		int pos2 = pos + 8;
		for (j = 0; j < program.num_stream; j++) {
			program_info_stream stream;
			int len = buf[pos2 + 2];
			stream.stream_pid = get16bit(buf, pos2);
			stream.codec_type = buf[pos2 + 3];
			if (stream.codec_type == 0x02 || stream.codec_type == 0x1b) {
				stream.format     = (buf[pos2 + 4] & 0xF0) >> 4;
				stream.frame_rate = buf[pos2 + 4] & 0x0F;
				cur_fps           = fps_tables[stream.frame_rate];
			} else if (stream.codec_type == 0x0f) {
				stream.format     = (buf[pos2 + 4] & 0xF0) >> 4;
				stream.frame_rate = buf[pos2 + 4] & 0x0F;
			}
			pos2 += 2 + len + 1;
			memcpy(program.stream + j, &stream, sizeof(program_info_stream));
		}
		memcpy(prog->program + i, &program, sizeof(program_info_prog));
		for (j = 0; j < program.num_group; j++) {
			int len = buf[pos2];
			pos2 += len + 1 + (len % 2 == 0) ? 1 : 0;
		}
		pos = pos2;
		if (cur_fps != 0.0) {
			seq->fps = cur_fps;
			break;
		}
	}
	//CPI
	cpi_t *cpi = cl->cpi;
	pos = cl->cpi_addr;
	cpi->cpi_len  = get32bit(buf, pos);
	cpi->cpi_type = buf[pos + 5] & 0x0F;
	pos += 6;
	unsigned long ep_addr = pos;
	//printf("EP addr:0x%lx\n", ep_addr);
	if (cpi->cpi_type == 1) {
		cpi->num_pid = buf[pos + 1];
		pos += 2;
		cpi->st = malloc(sizeof(cpi_stream) * cpi->num_pid);
		if (!cpi->st) {
			fprintf(stderr, "Unallocated memory.\n");
			return 0;
		}
		for (i = 0; i < cpi->num_pid; i++) {
			cpi_stream stream;
			stream.pid         = get16bit(buf, pos);
			stream.stream_type = (buf[pos + 3] >> 2) & 0x0F;
			stream.num_coarse  = ((buf[pos + 3]) & 0x3) << 14 | (buf[pos + 4] << 6) | ((buf[pos + 5] >> 2) & 0x3F);
			stream.num_fine    = ((buf[pos + 5] & 0x3) << 16) | (buf[pos + 6] << 8) | buf[pos + 7];
			stream.ep_map_start_addr = get32bit(buf, pos + 8);
			stream.coarse = malloc(sizeof(cpi_ep_coarse) * stream.num_coarse);
			stream.fine   = malloc(sizeof(cpi_ep_fine) * stream.num_fine);
			if (!stream.coarse || !stream.fine) {
				fprintf(stderr, "Unallocated memory.\n");
				return 0;
			}
			memcpy(cpi->st + i, &stream, sizeof(cpi_stream));
			pos += 12;
		}
		for (i = 0; i < cpi->num_pid; i++) {
			cpi_stream stream = cpi->st[i];
			pos = ep_addr + stream.ep_map_start_addr;
			unsigned long fine_addr = get32bit(buf, pos);
			pos += 4;
			for (j = 0; j < stream.num_coarse; j++) {
				cpi_ep_coarse coarse;
				coarse.ref_ep_fine_id = (buf[pos] << 10) | (buf[pos + 1] << 2) | ((buf[pos + 2] >> 6) & 0x3);
				coarse.pts_ep         = ((buf[pos + 2] & 0x3F) << 8) | buf[pos + 3];
				coarse.spn_ep         = get32bit(buf, pos + 4);
				memcpy(stream.coarse + j, &coarse, sizeof(cpi_ep_coarse));
				pos += 8;
			}
			pos = ep_addr + stream.ep_map_start_addr + fine_addr;
			for (j = 0; j < stream.num_fine; j++) {
				cpi_ep_fine fine;
				fine.is_angle_change_point = (buf[pos] >> 7) & 0x1;
				fine.i_end_position_offset = (buf[pos] >> 4) & 0x7;
				fine.pts_ep = ((buf[pos] & 0xF) << 7) | ((buf[pos + 1] >> 1) & 0x7F);
				fine.spn_ep = ((buf[pos + 1] & 0x1) << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
				memcpy(stream.fine + j, &fine, sizeof(cpi_ep_fine));
				pos += 4;
			}
		}
	}
	close(fd);
	free(buf);
	return 1;
}

int gen_pts_spn(const clpi_t *cl, const int stream_id, ep_map *ep) {
	int i,j;
	if (cl->cpi->cpi_len - 1 < stream_id) {
		fprintf(stderr, "Cannot find streamID.\n");
		return 0;
	}
	unsigned long num_coarse = cl->cpi->st[stream_id].num_coarse;
	unsigned long num_fine   = cl->cpi->st[stream_id].num_fine;
	for (i = 0; i < num_coarse; i++) {
		int start, end;
		cpi_ep_coarse coarse = cl->cpi->st[stream_id].coarse[i];
		start = coarse.ref_ep_fine_id;
		if (i < num_coarse - 1) {
			end = cl->cpi->st[stream_id].coarse[i + 1].ref_ep_fine_id;
		} else {
			end = num_fine;
		}
		for (j = start; j < end; j++) {
			cpi_ep_fine fine = cl->cpi->st[stream_id].fine[j];
			unsigned long pts = ((coarse.pts_ep & ~0x01) << 19) + (fine.pts_ep << 9);
			unsigned long spn = (coarse.spn_ep & ~0x1FFFF) + fine.spn_ep;
			printf("PTS:%ld/%ld SPN:%ld\n", pts, pts >> 1, spn);
		}
	}
	return 1;
}

void free_clpi(clpi_t *cl) {
	int i;
	for (i = 0; i < cl->prog->num_program; i++) {		
		free(cl->prog->program[i].stream);
	}
	free(cl->prog->program);
	for (i = 0; i < cl->seq->atc_len; i++) {
		free(cl->seq->atc[i].stc);
	}
	free(cl->seq->atc);
	free(cl->seq);
	free(cl->prog);
	for (i = 0; i < cl->cpi->num_pid; i++) {
		free(cl->cpi->st[i].coarse);
		free(cl->cpi->st[i].fine);
	}
	free(cl->cpi->st);
	free(cl->cpi);
}
