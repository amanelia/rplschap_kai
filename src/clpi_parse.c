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
	read(fd, buf, fsize);
	cl->sequence_info_addr = get32bit(buf, 8);
	cl->program_info_addr = get32bit(buf, 12);
	cl->cpi_addr = get32bit(buf, 16);
	cl->clipmark_addr = get32bit(buf, 24);
	cl->seq = malloc(sizeof(sequence_info));
	cl->prog = malloc(sizeof(program_info));
	if (!cl->seq || !cl->prog) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	cl->cpi = malloc(sizeof(cpi_t));
	if (!cl->cpi) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	sequence_info *seq = cl->seq;
	//SequenceInfo
	//unsigned int sequence_info_len = get32bit(buf, cl->sequence_info_addr);
	seq->atc_len = buf[cl->sequence_info_addr + 5];
	pos = cl->sequence_info_addr + 6;
	seq->atc = malloc(sizeof(sequence_info_atc) * seq->atc_len);
	for (i = 0; i < seq->atc_len; i++) {
		seq->atc[i].spn_atc_start = get32bit(buf, pos);
		seq->atc[i].stc_len = buf[pos + 4];
		seq->atc[i].offset_stc_id = buf[pos + 5];
		seq->atc[i].stc = malloc(sizeof(sequence_info_stc) * seq->atc[i].stc_len);
		int pos2 = pos + 6;
		for (j = 0; j < seq->atc[i].stc_len; j++) {
			seq->atc[i].stc[j].pcr_pid = get16bit(buf, pos2);
			seq->atc[i].stc[j].spn_stc_start = get32bit(buf, pos2 + 2);
			seq->atc[i].stc[j].start_time = get32bit(buf, pos2 + 6);
			seq->atc[i].stc[j].end_time = get32bit(buf, pos2 + 10);
			pos2 += 14;
			seq->atc[i].stc[j].time0 = 0;
			seq->atc[i].stc[j].time1 = (seq->atc[i].stc[j].end_time - seq->atc[i].stc[j].start_time) / 45000.0;
			//if (seq->atc[i].stc_len != 0) {
			//	seq->atc[i].stc[j].time0 = seq->atc[i].stc[seq->atc[i].stc_len - 1].time1;
			//}
		}
	}
	//ProgramInfo
	program_info *prog = cl->prog;
	//unsigned long program_info_len = get32bit(buf, cl->program_info_addr);
	prog->num_program = buf[cl->program_info_addr + 5];
	prog->program = malloc(sizeof(program_info_prog) * prog->num_program);
	pos = cl->program_info_addr + 6;
	for (i = 0; i < prog->num_program; i++) {
		double cur_fps = 0;
		prog->program[i].spn_program_sequence_start = get32bit(buf, pos);
		prog->program[i].program_map_pid = get16bit(buf, pos + 4);
		prog->program[i].num_stream = buf[pos + 6];
		prog->program[i].num_group = buf[pos + 7];
		prog->program[i].stream = malloc(sizeof(program_info_stream) * prog->program[i].num_stream);
		int pos2 = pos + 8;
		for(j = 0;j < prog->program[i].num_stream; j++) {
			unsigned short codec_type = buf[pos2 + 3];
			int len = buf[pos2 + 2];
			prog->program[i].stream[j].codec_type = codec_type;
			prog->program[i].stream[j].stream_pid = get16bit(buf, pos2);
			if (codec_type == 0x02 || codec_type == 0x1b) {
				int format = (buf[pos2 + 4] & 0xF0) >> 4;
				int frame_rate = buf[pos2 + 4] & 0x0F;
				prog->program[i].stream[j].format = format;
				prog->program[i].stream[j].frame_rate = frame_rate;
				cur_fps = fps_tables[frame_rate];
			}
			pos2 += 2 + len + 1;
		}
		for(j = 0; j < prog->program[i].num_group; j++) {
			int len = buf[pos2];
			pos2 += len + 1 + (len % 2 == 0) ? 1 : 0;
		}
		pos = pos2;
		if (cur_fps != 0) {
			seq->fps = cur_fps;
			break;
		}
	}
	//CPI
	cpi_t *cpi = cl->cpi;
	pos = cl->cpi_addr;
	cpi->cpi_len = get32bit(buf, pos);
	cpi->cpi_type = buf[pos + 5] & 0x0F;
	pos += 6;
	unsigned long ep_addr = pos;
	printf("EP addr:0x%lx\n", ep_addr);
	if (cpi->cpi_type == 1) {
		cpi->num_pid = buf[pos + 1];
		pos += 2;
		cpi->st = malloc(sizeof(cpi_stream) * cpi->num_pid);
		if (!cpi->st) {
			fprintf(stderr, "Unallocated memory.\n");
			return 0;
		}
		for(i = 0; i < cpi->num_pid; i++) {
			cpi->st[i].pid = get16bit(buf, pos);
			cpi->st[i].stream_type = (buf[pos + 3] >> 2) & 0x0F;
			cpi->st[i].num_coarse = ((buf[pos + 3]) & 0x3) << 14 | (buf[pos + 4] << 6) | ((buf[pos + 5] >> 2) & 0x3f);
			cpi->st[i].num_fine = ((buf[pos + 5] & 0x3) << 16) | (buf[pos + 6] << 8) | buf[pos + 7];
			cpi->st[i].ep_map_start_addr = get32bit(buf, pos + 8);
			pos += 12;
		}
		for (i = 0; i < cpi->num_pid; i++) {
			pos = ep_addr + cpi->st[i].ep_map_start_addr;
			unsigned long fine_addr = get32bit(buf, pos);
			pos += 4;
			cpi_ep_coarse *coarse = malloc(sizeof(cpi_ep_coarse) * cpi->st[i].num_coarse);
			if (!coarse) {
				fprintf(stderr, "Unallocated memory.\n");
				return 0;
			}
			cpi->st[i].coarse = coarse;
			for (j = 0; j < cpi->st[i].num_coarse; j++) {
				int id = (buf[pos] << 10) | (buf[pos + 1] << 2) | ((buf[pos + 2] >> 6) & 0x3);
				int pts = ((buf[pos + 2] & 0x3F) << 8) | buf[pos + 3];
				int spn = get32bit(buf, pos + 4);
				pos += 8;
				coarse[j].ref_ep_fine_id = id;
				coarse[j].pts_ep = pts;
				coarse[j].spn_ep = spn;
			}
			pos = ep_addr + cpi->st[i].ep_map_start_addr + fine_addr;
			cpi_ep_fine *fine = malloc(sizeof(cpi_ep_fine) * cpi->st[i].num_fine);
			if (!fine) {
				fprintf(stderr, "Unallocated memory.\n");
				return 0;
			}
			cpi->st[i].fine = fine;
			for (j = 0; j < cpi->st[i].num_fine; j++) {
				int type = (buf[pos] >> 7) & 0x1;
				int offset = (buf[pos] >> 4) & 0x7;
				unsigned long pts = ((buf[pos] & 0xF) << 7) | ((buf[pos + 1] >> 1) & 0x7F);
				unsigned long spn = ((buf[pos + 1] & 0x1) << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
				fine[j].is_angle_change_point = type;
				fine[j].i_end_position_offset = offset;
				fine[j].pts_ep = pts;
				fine[j].spn_ep = spn;
				pos += 4;
			}
			unsigned long pts;
			unsigned long spn;
			int k;
			for (j = 0; j < cpi->st[i].num_coarse; j++) {
				int start, end;
				printf("Coarse: %d\n", j);
				start = coarse[j].ref_ep_fine_id;
				if (j < cpi->st[i].num_coarse - 1) {
					end = coarse[j + 1].ref_ep_fine_id;
				} else {
					end = cpi->st[i].num_fine;
				}
				for (k = start; k < end; k++) {
					pts = ((coarse[j].pts_ep & ~0x01) << 19) + (fine[k].pts_ep << 9);
					spn = (coarse[j].spn_ep & ~0x1FFFF) + fine[k].spn_ep;
					printf("PTS:%ld/%ld SPN:%ld\n", pts, pts >> 1, spn);
				}
			}
		}
	}

	close(fd);
	free(buf);
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
