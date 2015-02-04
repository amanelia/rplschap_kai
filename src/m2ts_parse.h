#ifndef _M2TS_PARSE_H_
#define _M2TS_PARSE_H_

#define PACKET_SIZE 192

typedef struct {
	unsigned long spn;
	unsigned char stream_id;
	unsigned char pts_dts_flag;
	unsigned long pts;
	unsigned long dts;
} pes_timestamp;

typedef struct {
	int pes_prefix;
	unsigned char stream_id;
	unsigned char scramble_control;
	unsigned char priority;
	unsigned char data_align_indicator;
	unsigned char copyright;
	unsigned char original_or_copy;
	unsigned char pts_dts_flag;
	unsigned char escr_flag;
	unsigned char es_rate_flag;
	unsigned long pts;
	unsigned long dts;
} pes_t;

typedef struct {
	unsigned char sync;
	unsigned char tp_error_indicator;
	unsigned char pu_start_indicator;
	unsigned char tp_pri;
	int pid;
	unsigned char tp_scranble_control;
	unsigned char af_control;
	unsigned char cc;
} packet_header;

int read_m2ts_packet(const char *filename, const unsigned long spn, pes_timestamp *pt);

#endif
