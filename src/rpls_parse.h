#ifndef _RPLS_PARSE_H_
#define _RPLS_PARSE_H_
typedef struct {
	unsigned long playarea_address;
	int cpi;
	int num_list;
	int connection;
	int stc_id;
	char clpi_filename[6];
	char clpi_codec_identifier[5];
	unsigned long time_in;
	unsigned long time_out;
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long len;
	int num_timecode;
	unsigned long *timecode;
} rpls_t;

int read_rpls(const char *filename, rpls_t *rp);
void free_rpls(rpls_t *rp);
#endif
