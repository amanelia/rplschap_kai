#ifndef _RPLS_PARSE_H_
#define _RPLS_PARSE_H_
typedef struct {
	unsigned char mark_type;
	unsigned char mark_name_length;
	unsigned short maker_id;
	unsigned short ref_to_playitem_id;
	unsigned long mark_time_stamp;
	unsigned short entry_es_pid;
	unsigned short ref_thumbnail_index;
} playlist_mark;

typedef struct {
	unsigned long playarea_address;
	int cpi;
	int num_list;
	int connection;
	int stc_id;
	char *clpi_filename;
	char *clpi_codec_identifier;
	unsigned long time_in;
	unsigned long time_out;
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long len;
	int num_playlist_marks;
	playlist_mark *pl_mark;
} rpls_t;

int read_rpls(const char *filename, rpls_t *rp);
void free_rpls(rpls_t *rp);
#endif
