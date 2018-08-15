#ifndef __MAIN_H
#define __MAIN_H

typedef struct {
	int32_t match_score;
	int32_t mismatch_score;
	int32_t gap_open;
	int32_t gap_extend;
	int32_t band_width;
	char *matrix_fn;
	int32_t alignment_mode;
	int32_t add_cigar;
	int32_t add_seq;
	int32_t add_header;
	int32_t right_align_gaps;
	int32_t offset_and_length;
} main_opt_t;

#endif
