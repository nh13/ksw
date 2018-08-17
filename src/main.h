#ifndef __MAIN_H
#define __MAIN_H

typedef struct {
	int8_t *matrix;
	int ksw2_flags;
	ksw_extz_t ez;
	void *km;
} ksw2_data_t;

typedef struct {
	parasail_matrix_t *matrix;
	parasail_function_t *func;
	parasail_function_t *func_global; // needed for glocal
} parasail_data_t;

typedef struct main_opt_t main_opt_t;

typedef struct {
	int score;
	int qlb;
	int tlb;
	int qle;
	int tle;
	uint32_t *cigar;
	int n_cigar;
	int m_cigar;
} alignment_t;

typedef void alignment_function_t(
		char *query, 
		int query_length, 
		char *target, 
		int target_length, 
		main_opt_t *opt, 
		void *library_data, 
		alignment_t *alignment);

struct main_opt_t {
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
	int32_t library;

	int32_t parasail_vec_strat;

	// hidden
	alignment_function_t *_library_func;
	void *_library_data;
};

void align_with_ksw2(char *query, int query_length, char *target, int target_length, main_opt_t *opt, void* library_data, alignment_t *alignment);
void align_with_parasail(char *query, int query_length, char *target, int target_length, main_opt_t *opt, void *library_data, alignment_t *alignment);

#endif
