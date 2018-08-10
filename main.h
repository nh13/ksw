#ifndef __MAIN_H
#define __MAIN_H

typedef struct {
	int32_t match_score;
	int32_t mismatch_score;
	int32_t gap_open;
	int32_t gap_extend;
	int32_t band_width;
	int32_t start_score;
	char *matrix_fn;
	int32_t alignment_mode;
	int32_t add_cigar;
	int32_t add_seq;
} main_opt_t;

#endif
