/*
 * rplsChap kai
 * author amanelia
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "rpls_parse.h"
#include "clpi_parse.h"
#include "util.h"

static const unsigned int fps_num[] ={0, 24000, 24, 25, 30000, 30, 50, 60000, 60, 0, 0, 0, 0, 0, 0};
static const unsigned int fps_den[] ={1, 1001, 1, 1, 1001, 1, 1, 1001, 1, 1, 1, 1, 1, 1, 1};

rpls_t *rp;
clpi_t *cl;

int frame_rate_type = -1;
//Chapters
int num_chapter;
unsigned long *chapter_timecode;
//keyframe
int num_keyframe;
unsigned int *keyframe;

void print_hex(const unsigned char *b, const unsigned long len) {
	int i;
	for (i = 0; i < len; i++) {
		if (i % 16 == 0) printf("\n");
		printf("%02X ", b[i]);
	}
	printf("\n");
}

//get framerate type from clpi file.
int get_framerate_type() {
	int i,j;
	if (!cl) {
		fprintf(stderr, "uninitialized clpi struct.\n");
		return 0;
	}
	for (i = 0; i < cl->prog->num_program; i++) {
		program_info_prog pr = cl->prog->program[i];
		for (j = 0; j < pr.num_stream; j++) {
			program_info_stream st = pr.stream[j];
			if (st.codec_type == 0x02 || st.codec_type == 0x1b) {
				frame_rate_type = st.frame_rate;
				return 1;
			}
		}
	}
	return 0;
}

//PTS -> Timecode
int _conv_timecode_to_chapter(const unsigned long timecode, double *time) {
	int i,j;
	for (i = 0; i < cl->seq->atc_len; i++) {
		sequence_info_atc atc = cl->seq->atc[i];
		for (j = 0; j < atc.stc_len; j++) {
			sequence_info_stc stc = atc.stc[j];
			if ((stc.start_time <= timecode) && (timecode <= stc.end_time)) {
				unsigned long t = timecode - stc.start_time;
				if (rp->time_in != 0) {
					t = timecode - rp->time_in;
				}
				*time = t / 45000.0;
				return 1;
			}
		}
	}
	return 0;
}

//Timecode -> PTS
int _conv_chapter_to_timecode(const double time, unsigned long *timecode) {
	int i, j;
	for (i = 0; i < cl->seq->atc_len; i++) {
		sequence_info_atc atc = cl->seq->atc[i];
		for (j = 0; j < atc.stc_len; j++) {
			sequence_info_stc stc = atc.stc[j];
			if ((stc.time0 <= time) && (time <= stc.time1)) {
				*timecode = stc.start_time + (time - stc.time0) * 45000.0;
				return 1;
			}
		}
	}
	return 0;
}

int _conv_keyframe_to_timecode(const unsigned int frame, unsigned long *timecode) {
	if (frame_rate_type < 0) {
		fprintf(stderr, "no framerate type!\n");
		return 0;
	}
	//一応丸め処理？最大1ミリ秒のズレあり。
	int time = (frame / ((double)fps_num[frame_rate_type] / (double)fps_den[frame_rate_type]) * 1000.0);
	double t_time = ((double) time / 1000.0);
	_conv_chapter_to_timecode(t_time, timecode);
	return 1;
}

int _conv_timecode_to_keyframe(const unsigned long timecode, unsigned int *frame) {
	double time = 0.0;
	if (frame_rate_type < 0) {
		fprintf(stderr, "no framerate type!\n");
		return 0;
	}
	if (!_conv_timecode_to_chapter(timecode, &time)) {
		fprintf(stderr, "cannot convert to chapter.\n");
		return 0;
	}
	*frame = (time + 0.0005 / 1000.0) * ((double)fps_num[frame_rate_type] / (double)fps_den[frame_rate_type]) + 0.5;
	return 1;
}

int _write_rpls(const char *infile, const char *outfile, rpls_t *rp, clpi_t *cl) {
	FILE *fp;
	unsigned char *inbuf;
	int in_size, out_size;
	unsigned char *outbuf;
	int i,pos;
	fp = fopen(infile, "rb");
	if (!fp) {
		fprintf(stderr, "入力ファイル読み込みエラー.\n");
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	in_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	inbuf = malloc(sizeof(unsigned char) * in_size);
	if (!inbuf) {
		fprintf(stderr, "Unallocated memory\n");
		return 0;
	}
	if (!fread(inbuf, sizeof(unsigned char), sizeof(unsigned char) * in_size, fp)) {
		fprintf(stderr, "cannot read.\n");
		return 0;
	}
	fclose(fp);
	unsigned long start_addr = get32bit(inbuf, 0x0C);
	unsigned long private_addr = get32bit(inbuf, 0x10);
	printf("inbuf_size:0x%x\n", in_size);
	printf("StartAddr:0x%lx\n", start_addr);
	printf("PrivAddr:0x%lx\n", private_addr);
	int size = 46;
	unsigned long playitem_len = 6 + size * num_chapter;
	printf("should write:0x%lx\n", playitem_len);
	unsigned char *playitem = malloc(sizeof(unsigned char) * playitem_len);
	memset(playitem, 0, sizeof(unsigned char) * playitem_len);
	if (!playitem) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	unsigned long priv_data_addr = start_addr + playitem_len;
	printf("outbuf_size:0x%lx\n", in_size + playitem_len);
	out_size = in_size + playitem_len;
	outbuf = malloc(sizeof(unsigned char) * out_size);
	if (!outbuf) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	memset(outbuf, 0, sizeof(unsigned char) * out_size);
	//バイト複写
	for (i = 0; i < in_size; i++) {
		outbuf[i] = inbuf[i];
	}

	//MarkerPrivateData address (pana)
	//outbuf[0x10] = (priv_data_addr & 0xFF000000) >> 24;
	//outbuf[0x11] = (priv_data_addr & 0x00FF0000) >> 16;
	//outbuf[0x12] = (priv_data_addr & 0x0000FF00) >> 8;
	//outbuf[0x13] = (priv_data_addr & 0x000000FF);
	//PlayListMark
	pos = 0;
	//PlayListMark length
	playitem[pos]     = ((playitem_len - 4) & 0xFF000000) >> 24;
	playitem[pos + 1] = ((playitem_len - 4) & 0x00FF0000) >> 16;
	playitem[pos + 2] = ((playitem_len - 4) & 0x0000FF00) >> 8;
	playitem[pos + 3] = ((playitem_len - 4) & 0x000000FF);
	//number_of_PlayList_marks
	playitem[pos + 4] = (num_chapter & 0xFF00) >> 8;
	playitem[pos + 5] = (num_chapter & 0x00FF);
	pos += 6;
	int pos2 = pos;
	for (i = 0; i < num_chapter; i++) {
		pos2 = pos + 0x2E * i;
		//mark_type
		//pana == 0x5 sony == 0x4(original)
		playitem[pos2    ] = 0x05;
		//mark name length
		playitem[pos2 + 1] = 0x00;
		//Maker ID 
		//pana == 0x3 sony == 0x8(original)
		playitem[pos2 + 2] = 0x01;
		playitem[pos2 + 3] = 0x03;
		//ref to PlayItemID
		playitem[pos2 + 4] = 0x00;
		playitem[pos2 + 5] = 0x00;
		//mark_time_stamp
		playitem[pos2 + 6] = ((chapter_timecode[i] & 0xFF000000) >> 24);
		playitem[pos2 + 7] = ((chapter_timecode[i] & 0x00FF0000) >> 16);
		playitem[pos2 + 8] = ((chapter_timecode[i] & 0x0000FF00) >> 8 );
		playitem[pos2 + 9] = (chapter_timecode[i] & 0x000000FF);
		//Entry ES_PID (must be 0xFFFF)
		playitem[pos2 + 10] = 0xFF;
		playitem[pos2 + 11] = 0xFF;
		//ref_thumbnail_index (must be 0xFFFF)
		playitem[pos2 + 12] = 0xFF;
		playitem[pos2 + 13] = 0xFF;
	}
	//debug
	//print_hex(playitem, playitem_len);
	memcpy(outbuf + start_addr, playitem, playitem_len);
	pos = priv_data_addr;
	//MarkerPrivateData (pana)
	unsigned long priv_data_len = out_size - pos - 6;
	outbuf[priv_data_addr    ] = (priv_data_len & 0xFF000000) >> 24;
	outbuf[priv_data_addr + 1] = (priv_data_len & 0x00FF0000) >> 16;
	outbuf[priv_data_addr + 2] = (priv_data_len & 0x0000FF00) >> 8;
	outbuf[priv_data_addr + 3] = (priv_data_len & 0x000000FF);

	fp = fopen(outfile, "wb");
	if (!fp) {
		fprintf(stderr,"書き込みrplsファイルが開けませんでした。\n");
		return 0;
	}
	if (!fwrite(outbuf, sizeof(unsigned char), sizeof(unsigned char) * out_size, fp)) {
		fprintf(stderr, "書き込みエラー\n");
		return 0;
	}
	fclose(fp);

	free(playitem);
	free(outbuf);
	free(inbuf);
	return 1;
}

int _read_keyframe(const char *filename) {
	FILE *fp;
	int i, line_count;
	char buf[512];
	char *e;
	fp = fopen(filename, "rt");
	if (!fp) {
		fprintf(stderr, "keyframeファイルオープンエラー.\n");
		return 0;
	}
	line_count = 0;
	while (fgets(buf, sizeof(buf), fp)) {
		line_count++;
	}
	if (ferror(fp)) {
		fprintf(stderr, "keyframeファイル読み込みエラー.\n");
		return 0;
	}
	keyframe = malloc(sizeof(unsigned int) * line_count);
	if (!keyframe) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	chapter_timecode = malloc(sizeof(unsigned long) * line_count);
	if (!chapter_timecode) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	rewind(fp);
	num_keyframe = line_count;
	num_chapter = line_count;
	i = 0;
	while (fgets(buf, sizeof(buf), fp)) {
		unsigned int key = strtoul(buf, &e, 10);
		unsigned long timecode = 0L;
		if (_conv_keyframe_to_timecode(key, &timecode)) {
			printf("keyframe:%d time:%ld\n", key, timecode);
			memcpy(chapter_timecode + i, &timecode, sizeof(unsigned long));
		}
		i++;
	}
	fclose(fp);
	return 1;
}

int _read_chapter(const char *filename) {
	FILE *fp;
	int count, i;
	char buf[512];
	fp = fopen(filename, "rt");
	if (!fp) {
		fprintf(stderr, "チャプターファイルオープンエラー.\n");
		return 0;
	}
	count = 0;
	while (fgets(buf, sizeof(buf), fp)) {
		if (strlen(buf) < 12) break;
		count++;
	}
	if (ferror(fp)) {
		fprintf(stderr, "チャプターファイル読み込みエラー.\n");
		return 0;
	}
	num_chapter = count;
	if (num_chapter < 1) {
		fprintf(stderr, "チャプターファイルに記述がありません。\n");
		return 0;
	}
	chapter_timecode = malloc(sizeof(unsigned long) * num_chapter);
	if (!chapter_timecode) {
		fprintf(stderr, "unallocated memory.\n");
		return 0;
	}
	rewind(fp);
	i = 0;
	while (fgets(buf, sizeof(buf), fp)) {
		if (strlen(buf) < 12) break;
		int h, m, s, ms;
		buf[12] = '\0'; ms = atoi(&buf[9]);
		buf[ 8] = '\0'; s  = atoi(&buf[6]);
		buf[ 5] = '\0'; m  = atoi(&buf[3]);
		buf[ 3] = '\0'; h  = atoi(&buf[0]);
		double time = ms * 0.001 + s + m * 60 + h * 60 * 60;
		unsigned long timecode = 0L;
		if (_conv_chapter_to_timecode(time, &timecode)) {
			printf("time:%f PTS:%ld(0x%lx)\n", time, timecode, timecode);
			memcpy(chapter_timecode + i, &timecode, sizeof(unsigned long));
			i++;
		}
	}
	if (ferror(fp)) {
		fprintf(stderr, "チャプターファイル読み込みエラー.\n");
		return 0;		
	}
	fclose(fp);
	return 1;
}

int _write_keyframe(const char *filename) {
	FILE *fp;
	int i;
	fp = fopen(filename, "wt");
	if (!fp) {
		fprintf(stderr, "keyframeファイルオープンエラー.\n");
		return 0;
	}
	for (i = 0; i < rp->num_timecode; i++) {
		unsigned int keyframe = 0;
		if (!_conv_timecode_to_keyframe(rp->timecode[i], &keyframe)) {
			continue;
		}
		fprintf(fp, "%d\n", keyframe);
	}
	fclose(fp);
	return 1;
}

int _write_chapter(const char *filename) {
	FILE *fp;
	char buf[512];
	int i;
	fp = fopen(filename, "wt");
	if (!fp) {
		fprintf(stderr, "チャプターファイルオープンエラー.\n");
		return 0;
	}
	for (i = 0; i < rp->num_timecode; i++) {
		double time;
		if (!_conv_timecode_to_chapter(rp->timecode[i], &time)) {
			continue;
		}
		int hour = time / 3600;
		time -= hour * 3600;
		int min = time / 60;
		time -= min * 60;
		int sec = time;
		time -= sec;
		int msec = time * 1000;
		sprintf(buf, "%02d:%02d:%02d.%03d", hour, min, sec, msec);
		fprintf(fp, "%s\n", buf);
	}
	fclose(fp);
	return 1;
}

void show_usage(char *path) {
	printf("チャプター情報を追加。\n");
	printf("usage: %s rplsfile dstrplsfile -k file.keyframe\n", path);
	printf("usage: %s rplsfile dstrplsfile -t file.time\n", path);
	printf("チャプター情報を抽出。\n");
	printf("usage: %s rplsfile -ok file.keyframe\n", path);
	printf("usage: %s rplsfile -ot file.time\n", path);
}

int main(int argc, char **argv) {
	int i;
	//mode 1:extract.
	//mode 2:insert.
	int mode = 0;
	//type 1:keyframe
	//type 2:timecode
	int type = 0;
	char *rplsfile     = '\0';
	char *outrplsfile  = '\0';
	char *parent_dir   = '\0';
	char *clpifile     = '\0';
	char *m2tsfile     = '\0';
	char *chapterfile  = '\0';
	char *keyframefile = '\0';
	if (argc < 2) {
		printf("rplsChap kai ver. 1.0\n");
		printf("author: amanelia ");
		printf("respected by rplsChap.\n");
		show_usage(argv[0]);
		return 0;
	}
	for (i = 1; i < argc; i++) {
		int next = 0;
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'o':
				if (i == (argc - 1)) {
					fprintf(stderr, "Invalid argument.\n");
					return 0;
				}
				printf("ExtractOption:");
				mode = 1;
				if (argv[i][2] == 'k') {
					printf("keyframe:%s\n", argv[i + 1]);
					if (!keyframefile) {
						keyframefile = argv[i + 1];
					}
					type = 1;
				} else if (argv[i][2] == 't') {
					printf("timecode:%s\n", argv[i + 1]);
					if (!chapterfile) {
						chapterfile = argv[i + 1];
					}
					type = 2;
				} else {
					printf("unknown\n");
				}
				next = 1;
				break;
			case 't':
				if (i == (argc - 1)) {
					fprintf(stderr, "Invalid argument.\n");
					return 0;
				}
				printf("InsertOption:timecode:%s\n", argv[i + 1]);
				if (!chapterfile) {
					chapterfile = argv[i + 1];
				}
				mode = 2;
				type = 2;
				next = 1;
				break;
			case 'k':
				if (i == (argc - 1)) {
					fprintf(stderr, "Invalid argument.\n");
					return 0;
				}
				printf("InsertOption:keyframe:%s\n", argv[i + 1]);
				if (!keyframefile) {
					keyframefile = argv[i + 1];
				}
				mode = 2;
				type = 1;
				next = 1;
				break;
			default:
				fprintf(stderr, "Invalid switch.\n");
				return 0;
			}
		} else {
			printf("file %s\n", argv[i]);
			if (rplsfile && !outrplsfile) {
				outrplsfile = argv[i];
			}
			if (!rplsfile) {
				rplsfile = argv[i];
			}
		}
		if (next) i++;
	}
	if (!rplsfile) {
		fprintf(stderr, "rplsファイルが指定されていません。\n");
		return 0;
	}
	rp = (rpls_t *) malloc(sizeof(rpls_t));
	if (!rp) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	if (!read_rpls(rplsfile, rp)) {
		fprintf(stderr, "rplsファイルの読み込みに失敗しました。\n");
		return 0;
	}
	printf("clpiFilename:%s\n", rp->clpi_filename);
	if (!strstr(rplsfile, "/BDAV/")) {
		fprintf(stderr,"正常なBDAVディレクトリではありません。\n");
		fprintf(stderr, "正しいディレクトリ構成にしてください。\n");
		return 0;
	}
	int parent_dir_len = strlen(rplsfile) - strlen(strstr(rplsfile, "/BDAV/"));
	parent_dir = malloc(parent_dir_len + 1);
	if (!parent_dir) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	memset(parent_dir, 0, parent_dir_len + 1);
	parent_dir = strncpy(parent_dir, rplsfile, strlen(rplsfile) - strlen(strstr(rplsfile, "/BDAV/")));
	clpifile = malloc(parent_dir_len + strlen("/BDAV/CLIPINF/") + 5 + strlen(".clpi") + 1);
	m2tsfile = malloc(parent_dir_len + strlen("/BDAV/STREAM/") + 5 + strlen(".m2ts"));

	sprintf(clpifile, "%s/%s/%s.%s", parent_dir, "BDAV/CLIPINF", rp->clpi_filename, "clpi");
	sprintf(m2tsfile, "%s/%s/%s.%s", parent_dir, "BDAV/STREAM", rp->clpi_filename, "m2ts");
	cl = malloc(sizeof(clpi_t));
	if (!cl) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	printf("parentdir:%s\n", parent_dir);
	printf("clpifile:%s\n", clpifile);
	if (!read_clpi(clpifile, cl)) {
		fprintf(stderr, "clpiファイルの読み込みに失敗しました。\n");
		return 0;
	}
//get clpi framerate_type.
	if (!get_framerate_type()) {
		fprintf(stderr, "フレームレート取得に失敗しました。\n");
		return 0;
	}
	if (mode == 1) {
		if (type == 1 && !_write_keyframe(keyframefile)) {
			fprintf(stderr, "keyframeファイルの書き込みに失敗しました。\n");
			return 0;
		}
		if (type == 2 && !_write_chapter(chapterfile)) {
			fprintf(stderr, "チャプターファイルの書き込みに失敗しました。\n");
			return 0;
		}
		printf("チャプターファイルを書き出しました。\n");
	} else if (mode == 2) {
		if (type == 1 && !_read_keyframe(keyframefile)) {
			fprintf(stderr, "keyframeファイルの読み込みに失敗しました。\n");
			return 0;
		}
		if (type == 2 && !_read_chapter(chapterfile)) {
			fprintf(stderr, "チャプターファイルの読み込みに失敗しました。\n");
			return 0;
		}
		if (!_write_rpls(rplsfile, outrplsfile, rp, cl)) {
			fprintf(stderr, "書き込み失敗。\n");
			return 0;
		}
		printf("rplsファイルを書き出しました。\n");
	}
	if (chapter_timecode) free(chapter_timecode);
	if (keyframe) free(keyframe);
	free(m2tsfile);
	free(clpifile);
	free(parent_dir);
	free_rpls(rp); 
	return 0;
}
