/*
 * rplsChap kai
 * author amanelia
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpls_parse.h"
#include "clpi_parse.h"

rpls_t *rp;
clpi_t *cl;

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
	char *rplsfile;
	char *parent_dir;
	char *clpifile;
	char *m2tsfile;
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
					type = 1;
				} else if (argv[i][2] == 't') {
					printf("timecode:%s\n", argv[i + 1]);
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
			rplsfile = argv[i];
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
	parent_dir = malloc(parent_dir_len);
	if (!parent_dir) {
		fprintf(stderr, "Unallocated memory.\n");
		return 0;
	}
	memset(parent_dir, 0, parent_dir_len);
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
	if (!read_clpi(clpifile, cl)) {
		fprintf(stderr, "clpiファイルの読み込みに失敗しました。\n");
		return 0;
	}
	free(m2tsfile);
	free(clpifile);
	free(parent_dir);
	free_rpls(rp); 
	return 0;
}
