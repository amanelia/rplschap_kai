#ifndef _CLPI_PARSE_H_
#define _CLPI_PARSE_H_
//SequenceInfo
typedef struct {
	unsigned short pcr_pid;
	unsigned long spn_stc_start;
	unsigned long start_time;
	unsigned long end_time;
	double time0, time1;
} sequence_info_stc;

typedef struct {
	double time;
	unsigned long PTS;
	unsigned long STN;
	unsigned long frame;
} ep_map;

typedef struct {
	unsigned long spn_atc_start;
	unsigned short offset_stc_id;
	unsigned int stc_len;
	sequence_info_stc *stc;
} sequence_info_atc;

typedef struct {
	double fps;
	int m_offset_frame;
	unsigned long max_stn;
	unsigned long time0;
	unsigned long m_time0;
	unsigned int atc_len;
	unsigned int epmap_len;
	sequence_info_atc *atc;
	ep_map *ep_map;
} sequence_info;

//ProgramInfo
typedef struct {
	unsigned short codec_type;
	unsigned short stream_pid;
	int format;
	int frame_rate;
} program_info_stream;

typedef struct {
	unsigned long spn_program_sequence_start;
	unsigned short program_map_pid;
	int num_stream;
	int num_group;
	program_info_stream *stream;
} program_info_prog;

typedef struct {
	int num_program;
	program_info_prog *program;
} program_info;

//CPI
typedef struct {
	int is_angle_change_point;
	int i_end_position_offset;
	unsigned long pts_ep;
	unsigned long spn_ep;
} cpi_ep_fine;

typedef struct {
	int ref_ep_fine_id;
	int pts_ep;
	unsigned long spn_ep;
} cpi_ep_coarse;

typedef struct {
	int pid;
	int stream_type;
	unsigned long num_coarse;
	unsigned long num_fine;
	unsigned long ep_map_start_addr;
	cpi_ep_coarse *coarse;
	cpi_ep_fine   *fine;
} cpi_stream;

typedef struct {
	unsigned long cpi_len;
	int cpi_type;
	int num_pid;
	cpi_stream *st;
} cpi_t;

typedef struct {
	unsigned long sequence_info_addr;
	unsigned long program_info_addr;
	unsigned long cpi_addr;
	unsigned long clipmark_addr;
	sequence_info *seq;
	program_info *prog;
	cpi_t *cpi;
} clpi_t;

static const double fps_tables[] = {0, 24000.0/1001, 24, 25, 30000.0/1001, 30, 50, 60000.0/1001, 60, 0, 0, 0, 0, 0, 0};

int read_clpi(const char *filename, clpi_t *seq);
void free_clpi(clpi_t *clpi);

#endif
