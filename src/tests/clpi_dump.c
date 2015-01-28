#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../clpi_parse.h"

int main(int argc, char **argv) {
	int i, j;
	int show_coarse = 0;
	int show_fine = 0;
	char *filename;
	if (argc < 2) {
		fprintf(stdout, "usage: clpi_dump:XXXXX.clpi\n");
		fprintf(stdout, "options:\n");
		fprintf(stdout, "\t-c :Show coarse list.\n");
		fprintf(stdout, "\t-f :Show fine list.\n");
		return 0;
	}
	for (i = 1; i < argc; i++) {
		int next = 0;
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'c':
				printf("Show Coarse list.\n");
				show_coarse = 1;
				break;
			case 'f':
				printf("Show Fine list.\n");
				show_fine = 1;
				break;
			}
		} else {
			filename = argv[i];
		}
		if (next) i++;
	}
	if (!filename) {
		fprintf(stderr, "Invalid argument.\n");
		return 0;
	}
	clpi_t *clpi;
	clpi = malloc(sizeof(clpi_t));
	read_clpi(filename, clpi);
	printf("-----%s-----\n", filename);
	printf("SequenceInfoAddr:0x%lx\n", clpi->sequence_info_addr);
	printf("ProgramInfoAddr :0x%lx\n", clpi->program_info_addr);
	printf("CPIAddr         :0x%lx\n", clpi->cpi_addr);
	printf("ClipMarkAddr    :0x%lx\n", clpi->clipmark_addr);
	printf("\t-----SequenceInfo-----\n");
	sequence_info *seq = clpi->seq;
	printf("\tATC length     :0x%x\n", seq->atc_len);
	for (i = 0; i < seq->atc_len; i++) {
		sequence_info_atc atc = clpi->seq->atc[i];
		printf("\t\t-----ATC-----\n");
		printf("\t\tSPN ATC start:0x%lx\n", atc.spn_atc_start);
		printf("\t\tOffset STC ID:0x%x\n", atc.offset_stc_id);
		printf("\t\tSTC len      :0x%x\n", atc.stc_len);
		for (j = 0; j < atc.stc_len; j++) {
			sequence_info_stc stc = atc.stc[j];
			printf("\t\t\t-----STC-----\n");
			printf("\t\t\tPCR PID      :0x%x\n", stc.pcr_pid);
			printf("\t\t\tSPN STC start:0x%lx\n", stc.spn_stc_start);
			printf("\t\t\tstart time   :0x%ld\n", stc.start_time);
			printf("\t\t\tend time     :0x%ld\n", stc.end_time);
			printf("\t\t\ttime0        :%f\n", stc.time0);
			printf("\t\t\ttime1        :%f\n", stc.time1);
		}
	}
	printf("\t-----ProgramInfo-----\n");
	program_info *prog = clpi->prog;
	printf("\tNum Program:%d\n", prog->num_program);
	for (i = 0; i < prog->num_program; i++) {
		program_info_prog pr = prog->program[i];
		printf("\t\t-----Program %d -----\n", i);
		printf("\t\tSPN Program Sequence start:%ld\n", pr.spn_program_sequence_start);
		printf("\t\tProgramMap PID            :0x%x\n", pr.program_map_pid);
		printf("\t\tNumber of Stream          :%d\n", pr.num_stream);
		printf("\t\tNumber of Group           :%d\n", pr.num_group);
		for (j = 0; j < pr.num_stream; j++) {
			program_info_stream st = pr.stream[j];
			printf("\t\t\t-----Stream %d -----\n", j);
			printf("\t\t\tCodecType:0x%02x\n", st.codec_type);
			printf("\t\t\tStreamPID:0x%02x\n", st.stream_pid);
			if (st.codec_type == 0x02 || st.codec_type == 0x1b) {
				printf("\t\t\tFormat   :0x%02x\n", st.format);
				printf("\t\t\tFramerate:%f(%d)\n", fps_tables[st.frame_rate], st.frame_rate);
			}
		}
	}
	printf("\t-----CPI-----\n");
	cpi_t *cpi = clpi->cpi;
	printf("\tCPI len :0x%lx\n", cpi->cpi_len);
	printf("\tCPI type:%d\n", cpi->cpi_type);
	printf("\tnum PID :%d\n", cpi->num_pid);
	for (i = 0; i < cpi->num_pid; i++) {
		cpi_stream st = cpi->st[i];
		printf("\t\t-----Stream: %d -----\n", i);
		printf("\t\tPID             :0x%x\n", st.pid);
		printf("\t\tStreamType      :%d\n", st.stream_type);
		printf("\t\tnum Coarse      :%ld\n", st.num_coarse);
		printf("\t\tnum Fine        :%ld\n", st.num_fine);
		printf("\t\tEPMap start addr:0x%lx\n", st.ep_map_start_addr);
		if (show_coarse) {
			printf("\t\t\t-----Coarse-----\n");
			for (j = 0; j < st.num_coarse; j++) {
				cpi_ep_coarse coarse = st.coarse[j];
				printf("\t\t\tCoarse:%d Ref EP Fine ID:%d PTS EP:%d SPN EP:%ld\n", j, coarse.ref_ep_fine_id, coarse.pts_ep, coarse.spn_ep);
			}
		}
		if (show_fine) {
			printf("\t\t\t-----Fine-----\n");
			for (j = 0; j < st.num_fine; j++) {
				cpi_ep_fine fine = st.fine[j];
				printf("\t\t\tFine  :%d IOffset:%d PTS EP:%ld SPN EP:%ld\n", j, fine.i_end_position_offset, fine.pts_ep, fine.spn_ep);
			}
		}
	}
	free_clpi(clpi);
	free(clpi);
	return 0;
}
