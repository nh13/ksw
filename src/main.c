/* The MIT License

   Copyright (c) 201* by Nils Homer

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <emmintrin.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
//#include "ksw2/kalloc.h"
#include "ksw2/kseq.h"
#include "ksw2/ksw2.h"
#include "parasail/parasail.h"
#include "githash.h"
#include "main.h"

KSEQ_INIT(int, read)


enum Library {
	LibraryStart = 0,
	AutoLibrary  = 0,
	Ksw2         = 1,
	Parasail     = 2,
	LibraryEnd   = 2,
};

enum AlignmentMode {
	AlignmentModeStart = 0,
	Local              = 0,
	Glocal             = 1,
	Extension          = 2,
	Global             = 3,
	AlignmentModeEnd   = 3,
};

// converts ascii DNA bases to their integer format (only [ACGTacgt], the rest go to N)
unsigned char seq_nt4_table[256] = {
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

void fill_matrix(int8_t *matrix, char *fn) {
	FILE *fp = fopen(fn, "r");
	char buffer[256];
	char *pch;
	int i = 0;

	while (0 < fgets(buffer, 256, fp)) {
		pch = strtok(buffer, ",\t");
		while (pch != NULL) {
			if (25 <= i) {
				fprintf(stderr, "Too many values in %s\n", fn);
				exit(1);
			}
			matrix[i] = atoi(pch);
			i++;
		}
	}
	if (i != 16 && i != 25) {
		fprintf(stderr, "Incorrect # of values (found %d, want 16 or 25) in %s\n", i, fn);
		exit(1);
	}

	fclose(fp);
}

char *alignment_mode_to_str(int mode)
{
	switch (mode) {
		case Local: return "local";
		case Glocal: return "glocal";
		case Extension: return "extend";
		case Global: return "global";
		default: return "unknown";
	}
}

char *library_to_str(int mode)
{
	switch (mode) {
		case AutoLibrary: return "auto";
		case Ksw2: return "ksw2";
		case Parasail: return "parasail";
		default: return "unknown";
	}
}

/*****************/
/** ksw2_data_t **/
/*****************/

ksw2_data_t *ksw2_data_init(main_opt_t *opt, const int8_t *matrix)
{
	ksw2_data_t *data = calloc(1, sizeof(ksw2_data_t));

	data->ksw2_flags = (opt->right_align_gaps == 1) ? KSW_EZ_RIGHT : 0;
	if (opt->add_cigar != 1) data->ksw2_flags |= KSW_EZ_SCORE_ONLY;
	memset(&data->ez, 0, sizeof(ksw_extz_t));
	
	//data->km = km_init(); FIXME

	data->matrix = calloc(1, sizeof(int8_t)*25);
	memcpy(data->matrix, matrix, sizeof(int8_t)*25);
	
	return data;

}

void ksw2_data_destroy(ksw2_data_t *data)
{
	free(data->matrix);
	kfree(data->km, data->ez.cigar);
	//km_destroy(data->km); FIXME
	free(data);
}

/*******************/
/* parasail_data_t */
/*******************/

// See: https://github.com/jeffdaily/parasail#standard-function-naming-convention
void parasail_to_func_name(char *parasail_func_name, int alignment_mode, int add_cigar, int vec_strategy)
{
	parasail_func_name[0] = '\0';
	strcat(parasail_func_name, "parasail");


	// Vectorized
	switch (alignment_mode) {
		case Local: strcat(parasail_func_name, "_sw"); break; // local
		case Glocal: strcat(parasail_func_name, "_sg_dx"); break; // glocal
		case Extension: fprintf(stderr, "Parasail does not support extension\n"); exit(1);
		case Global: strcat(parasail_func_name, "_nw"); break; // global
		default: fprintf(stderr, "Unknown alignment mode: %d\n", alignment_mode); exit(1);
	}
	if (add_cigar == 1) strcat(parasail_func_name, "_trace");
	switch (vec_strategy) {
		case 0: strcat(parasail_func_name, "_striped"); break;
		case 1: strcat(parasail_func_name, "_scan"); break;
		case 2: strcat(parasail_func_name, "_diag"); break;
		default: fprintf(stderr, "Unknown parasail vectorization strategy: %d\n", vec_strategy); exit(1);
	}
	strcat(parasail_func_name, "_32"); // TODO: add this as a command line option?
}

parasail_data_t *parasail_data_init(main_opt_t *opt, const int8_t *matrix)
{
	int i, j, l;
	parasail_data_t *data = calloc(1, sizeof(parasail_data_t));
	char parasail_func_name[128];
	
	// create a matrix, don't care about the valukes
	data->matrix = parasail_matrix_create("ACGTN", matrix[0], matrix[1]); 
	// update the matrix with the correct values
	for (i = l = 0; i < 5; ++i) {
		for (j = 0; j < 5; ++j, ++l) {
			parasail_matrix_set_value(data->matrix, i, j, matrix[l]);
		}
	}

	parasail_to_func_name(parasail_func_name, opt->alignment_mode, opt->add_cigar, opt->parasail_vec_strat);
	data->func = parasail_lookup_function(parasail_func_name);

	// the global alignment function is needed for glocal when we don't align the full query
	if (opt->alignment_mode == Glocal) {
		data->func_global = data->func;
	}
	else {
		parasail_to_func_name(parasail_func_name, Global, opt->add_cigar, opt->parasail_vec_strat);
		data->func_global = parasail_lookup_function(parasail_func_name);
	}

	return data;
}

void parasail_data_destroy(parasail_data_t *data)
{
	parasail_matrix_free(data->matrix);
	free(data);
}

/****************/
/** main_opt_t **/
/****************/

main_opt_t *main_opt_init()
{
	main_opt_t *opt = calloc(1, sizeof(main_opt_t));

	opt->match_score = 1;
	opt->mismatch_score = 3;
	opt->gap_open = 5;
	opt->gap_extend = 2;
	opt->band_width = INT_MAX / 4; // divide by four since in some places we multiply by two
	opt->matrix_fn = NULL;
	opt->alignment_mode = Local;
	opt->add_cigar = 0;
	opt->add_seq = 0;
	opt->add_header = 0;
	opt->right_align_gaps = 0;
	opt->offset_and_length = 0;
	opt->parasail_vec_strat = 0; // TODO: set on the command line
	opt->zdrop = -1;
	opt->library = AutoLibrary;

	return opt;
}

void main_opt_init_library(main_opt_t *opt)
{
	int i, j, k;
	int8_t matrix[25];

	// initialize scoring matrix
	for (i = k = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j) {
			matrix[k++] = i == j? opt->match_score: -opt->mismatch_score;
		}
		matrix[k++] = 0; // ambiguous base
	}
	for (j = 0; j < 5; ++j) matrix[k++] = 0;
	// overwrite if a matrix file was given
	if (opt->matrix_fn != NULL) fill_matrix(matrix, opt->matrix_fn);

	// adjust the library mode if it is set on auto
	if (opt->library == AutoLibrary) {
		switch (opt->alignment_mode) {
			case Local: 
			case Glocal: 
			case Global: opt->library = Parasail; break;
			case Extension: opt->library = Ksw2; break;
			default:
				fprintf(stderr, "Unknown alignment mode in %s: %d\n", __func__, opt->alignment_mode); 
				exit(1);
		}
	}

	switch (opt->library) {
		case Ksw2: 
			opt->_library_func = align_with_ksw2;
			opt->_library_data = (void*)ksw2_data_init(opt, matrix); 
			break;
		case Parasail: 
			opt->_library_func = align_with_parasail;
			opt->_library_data = (void*)parasail_data_init(opt, matrix); 
			break;
		default:
			fprintf(stderr, "Unknown library in %s: %d", __func__, opt->library);
			exit(1);
	}
}

void main_opt_destroy(main_opt_t *opt)
{
	switch (opt->library) {
		case Ksw2: ksw2_data_destroy((ksw2_data_t*)opt->_library_data); break;
		case Parasail: parasail_data_destroy((parasail_data_t*)opt->_library_data); break;
		default:
			fprintf(stderr, "Unknown library in %s: %d", __func__, opt->library);
			exit(1);
	}
	free(opt);
}

void assert_or_exit(int condition, const char *fmt, ...)
{
	if (condition == 0) {
		fprintf(stderr, "Error: ");
		va_list args;
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		fprintf(stderr, "\n");
		exit(1);
	}
}

void main_opt_validate(main_opt_t *opt)
{
	assert_or_exit(AlignmentModeStart <= opt->alignment_mode && opt->alignment_mode <= AlignmentModeEnd, "Alignment mode (-M) was not valid ([%d-%d]), found %d.", AlignmentModeStart, AlignmentModeEnd, opt->alignment_mode);
	assert_or_exit(opt->mismatch_score > 0, "Mismatch penalty (-b) must be greater than zero, found %d.", opt->mismatch_score);
	assert_or_exit(opt->match_score > 0, "Match score (-a) must be greater than zero, found %d.", opt->match_score);
	assert_or_exit(opt->gap_open >= 0, "Gap open penalty (-q) must be greater than or equal to zero, found %d.", opt->gap_open);
	assert_or_exit(opt->gap_extend > 0, "Gap extend penalty (-r) must be greater than zero, found %d.", opt->gap_extend);
	assert_or_exit(0 <= opt->band_width, "Band width (-w) must be greater than or equal zero, found %d.", opt->band_width);
	assert_or_exit(LibraryStart <= opt->library && opt->library <= LibraryEnd, "Library (-l) was not valid ([%d-%d]), found %d.", LibraryStart, LibraryEnd, opt->library);

	// verify library type with alignment_mode
	int found_mismatch = 0;
	switch (opt->library) {
		case Ksw2: 
			if (opt->alignment_mode != Extension && opt->alignment_mode != Global) found_mismatch = 1; 
			break;
		case Parasail: 
			assert_or_exit(opt->right_align_gaps == 0, "Cannot right adjust gaps (-R) with parasail (-l)");
			if (opt->alignment_mode != Local && opt->alignment_mode != Glocal && opt->alignment_mode != Global) found_mismatch = 1; 
			break;
		default: break;
	}
	assert_or_exit(found_mismatch == 0, "Cannot use alignment mode (-M) %d-%s with library (-l) %d-%s.", 
			opt->alignment_mode, library_to_str(opt->alignment_mode),
			opt->library, library_to_str(opt->library));
}

/***************/
/* alignment_t */
/***************/

alignment_t *alignment_init() 
{
	alignment_t *alignment = calloc(1, sizeof(alignment_t));
	return alignment;
}
void alignment_reset(alignment_t *a)
{
	a->qlb = a->tlb = a->qle = a->tle = 0;
	a->n_cigar = 0;
}

void alignment_print(FILE *fp, const char *query, const char *target, const main_opt_t *opt, const alignment_t *a) 
{
	int i;
	// output the score and start/end or start/length
	if (opt->offset_and_length == 1) {
		int qll = a->qle - a->qlb + 1;
		int tll = a->tle - a->tlb + 1;
		fprintf(fp, "%d\t%d\t%d\t%d\t%d", a->score, a->qlb, qll, a->tlb, tll);
	}
	else {
		fprintf(fp, "%d\t%d\t%d\t%d\t%d", a->score, a->qlb, a->qle, a->tlb, a->tle);
	}
	// output the cigar
	if (opt->add_cigar == 1) {
		fputc('\t', fp);
		if (a->n_cigar == 0) fputc('*', fp);
		for (i = 0; i < a->n_cigar; ++i) {
			fprintf(fp, "%d%c", a->cigar[i]>>4, "MID"[a->cigar[i]&0xf]);
		}
	}
	// output the query and target
	if (opt->add_seq) fprintf(fp, "\t%s\t%s", query, target);
	// flush the output so no one is waiting on buffering.
	fputc('\n', fp);
	fflush(fp);
}

void alignment_destroy(alignment_t *alignment)
{
	free(alignment->cigar);
	free(alignment);
}

/*************************************/
/* Library-specific aligment methods */
/*************************************/

void align_with_ksw2(char *query, int query_length, char *target, int target_length, main_opt_t *opt, void* library_data, alignment_t *alignment) {
	int i;
	ksw2_data_t *ksw2_data = (ksw2_data_t*)library_data;

	// convert to bases in integer format
	for (i = 0; i < query_length; ++i) query[i] = seq_nt4_table[(int)query[i]];
	for (i = 0; i < target_length; ++i) target[i] = seq_nt4_table[(int)target[i]];

	switch (opt->alignment_mode) {
		case Local: fprintf(stderr, "KSW2 does not support local\n"); exit(1);
		case Glocal: fprintf(stderr, "KSW2 does not support glocal\n"); exit(1);
		case Extension: // extend
			ksw_extz2_sse(0, query_length, (uint8_t*)query, target_length, (uint8_t*)target, 5, ksw2_data->matrix, opt->gap_open, opt->gap_extend, opt->band_width, opt->zdrop, 0, ksw2_data->ksw2_flags | KSW_EZ_EXTZ_ONLY, &ksw2_data->ez);
			alignment->score   = ksw2_data->ez.mqe; // maximum score when we reach the end of the query
			if (ksw2_data->ez.max_q < 0) {
				alignment->qlb = -1;
				alignment->qle = -1;
				alignment->tlb = -1;
				alignment->tle = -1;
			}
			else {
				alignment->qlb = 0;
				alignment->tlb = 0;
				alignment->qle = ksw2_data->ez.max_q;
				alignment->tle = ksw2_data->ez.max_t;
			}
			break;
		case Global: // global
			ksw_extz2_sse(0, query_length, (uint8_t*)query, target_length, (uint8_t*)target, 5, ksw2_data->matrix, opt->gap_open, opt->gap_extend, opt->band_width, opt->zdrop, 0, ksw2_data->ksw2_flags, &ksw2_data->ez);
			alignment->score = ksw2_data->ez.score;
			alignment->qlb = 0;
			alignment->tlb = 0;
			alignment->qle   = query_length-1;
			alignment->tle   = target_length-1;
			break;
		default:
			fprintf(stderr, "Unknown alignment mode in %s: %d\n", __func__, opt->alignment_mode); 
			exit(1);
	}

	// copy cigar
	if (opt->add_cigar == 1) {
		if (alignment->m_cigar < ksw2_data->ez.m_cigar) {
			alignment->m_cigar = ksw2_data->ez.m_cigar;
			alignment->cigar = (uint32_t*)realloc(alignment->cigar, alignment->m_cigar*sizeof(uint32_t));
		}
		for (i = 0; i < ksw2_data->ez.n_cigar; i++) {
			alignment->cigar[i] = ksw2_data->ez.cigar[i];
		}
		alignment->n_cigar = ksw2_data->ez.n_cigar;
	}

	// convert back to bases
	for (i = 0; i < query_length; ++i) query[i] = "ACGTN"[(int)query[i]];
	for (i = 0; i < target_length; ++i) target[i] = "ACGTN"[(int)target[i]];
}


void align_with_parasail(char *query, int query_length, char *target, int target_length, main_opt_t *opt, void *library_data, alignment_t *alignment)
{
	parasail_data_t *parasail_data = (parasail_data_t*)library_data;
	int i;
	parasail_result_t *parasail_result;
	parasail_cigar_t *parasail_cigar;
	parasail_result = parasail_data->func(query, query_length, target, target_length, opt->gap_open + opt->gap_extend, opt->gap_extend, parasail_data->matrix);

	// set the score
	alignment->score = parasail_result->score;

	// set end of alignmnet
	alignment->qle = parasail_result->end_query;
	alignment->tle = parasail_result->end_ref;

	// add the cigar, and if so, set the alignment beginning
	if (opt->add_cigar == 1) {
		parasail_cigar = parasail_result_get_cigar(parasail_result, query, query_length, target, target_length, parasail_data->matrix);
		alignment->qlb = parasail_cigar->beg_query;
		alignment->tlb = parasail_cigar->beg_ref;
		// NB: recompute beg_ref using leading deletions for glocal alignment
		// beg_ref can be wrong for glocal.  See: https://github.com/jeffdaily/parasail/issues/97
		if (opt->alignment_mode == Glocal) {
			alignment->tlb = 0;
			for (i = 0; i < parasail_cigar->len; ++i) {
				char op = parasail_cigar_decode_op(parasail_cigar->seq[i]);
				if (op != 'D') break;
				uint32_t len = parasail_cigar_decode_len(parasail_cigar->seq[i]);
				alignment->tlb += len;
			}
		}
		int prev_op_int = -1;
		for (i = 0; i < parasail_cigar->len; ++i) {
			char op = parasail_cigar_decode_op(parasail_cigar->seq[i]);
			int op_int;
			// "MIDNSHP=XB"
			switch (op) {
				case 'M':
				case '=':
				case 'X':
					op_int = 0; break;
				case 'I':
					op_int = 1; break;
				case 'D':
					op_int = 2; break;
				default:
					fprintf(stderr, "Unknown cigar type: %c\n", op);
					exit(1);
			}
			uint32_t len = parasail_cigar_decode_len(parasail_cigar->seq[i]);
			if (prev_op_int == op_int) { // add to the previous
				len += alignment->cigar[alignment->n_cigar-1] >> 4; 
				alignment->cigar[alignment->n_cigar-1] = len<<4 | op_int;
			}
			else { // new cigar element
				if (alignment->n_cigar == alignment->m_cigar) {
					alignment->m_cigar = alignment->m_cigar ? (alignment->m_cigar)<<1 : 4;
					alignment->cigar = (uint32_t*)realloc(alignment->cigar, alignment->m_cigar*sizeof(uint32_t));
				}
				alignment->cigar[alignment->n_cigar] = len<<4 | op_int;
				alignment->n_cigar++;
			}
			prev_op_int = op_int;
		}
		parasail_cigar_free(parasail_cigar);
	}
	else {
		alignment->qlb = -1;
		alignment->tlb = -1;
	}
	parasail_result_free(parasail_result);
}

void align(char *query, char *target, main_opt_t *opt, alignment_t *alignment) 
{
	int ql = strlen(query); // query length
	int tl = strlen(target); // target length

	// remove ending newlines
	if (query[ql-1] == '\n') { query[ql-1] = '\0'; ql--; }
	if (target[tl-1] == '\n') { target[tl-1] = '\0'; tl--; }

	// reset the alignment
	alignment_reset(alignment);

	// do the alignment
	opt->_library_func(query, ql, target, tl, opt, opt->_library_data, alignment);

	// print it
	alignment_print(stdout, query, target, opt, alignment);
}

/*********/
/** main */
/*********/

void usage(main_opt_t *opt)
{
	int i;
	fprintf(stderr, "\n");
	fprintf(stderr, "Program: ksw (klib smith-waterman)\n");
	fprintf(stderr, "Version: %s\n", GIT_HASH);
	fprintf(stderr, "Usage: ksw [options]\n\n");
	fprintf(stderr, "Algorithm options:\n\n");
	fprintf(stderr, "       -M INT      The alignment mode:");
	for (i = AlignmentModeStart; i <= AlignmentModeEnd; ++i) {
		fprintf(stderr, " %d - %s", i, alignment_mode_to_str(i));
		if (i < AlignmentModeEnd) fputc(',', stderr);
	}
	fprintf(stderr, " [%d - %s]\n", opt->alignment_mode, alignment_mode_to_str(opt->alignment_mode));
	fprintf(stderr, "       -a INT      The match score (>0) [%d]\n", opt->match_score);
	fprintf(stderr, "       -b INT      The mismatch penalty (>0) [%d]\n", opt->mismatch_score);
	fprintf(stderr, "       -q INT      The gap open penalty (>=0) [%d]\n", opt->gap_open);
	fprintf(stderr, "       -r INT      The gap extend penalty (>0) [%d]\n", opt->gap_extend);
	fprintf(stderr, "       -w INT      The band width (ksw only) [%d]\n", opt->band_width);
	fprintf(stderr, "       -m FILE     Path to the scoring matrix (4x4 or 5x5) [%s]\n", opt->matrix_fn == NULL ? "None" : opt->matrix_fn);
	fprintf(stderr, "       -c          Append the cigar to the output [%s]\n", opt->add_cigar == 0 ? "false" : "true");
	fprintf(stderr, "       -s          Append the query and target to the output [%s]\n", opt->add_seq == 0 ? "false" : "true");
	fprintf(stderr, "       -H          Add a header line to the output [%s]\n", opt->add_header == 0 ? "false" : "true");
	fprintf(stderr, "       -R          Right-align gaps (ksw only)[%s]\n", opt->right_align_gaps == 0 ? "false" : "true");
	fprintf(stderr, "       -O          Output offset-and-length, otherwise start-and-end (all zero-based)[%s]\n", opt->offset_and_length == 0 ? "false" : "true");
	fprintf(stderr, "       -l INT      The library type:");
	for (i = LibraryStart; i <= LibraryEnd; ++i) {
		fprintf(stderr, " %d - %s", i, library_to_str(i));
		if (i < LibraryEnd) fputc(',', stderr);
	}
	fprintf(stderr, " [%d - %s]\n", opt->library, library_to_str(opt->library));
	fprintf(stderr, "       -z INT      Z-drop (for KSW) [%d]\n", opt->zdrop);
	fprintf(stderr,"\nNote: when any of the algorithms open a gap, the gap open plus the gap extension penalty is applied.\n");
}

int main(int argc, char *argv[])
{
	main_opt_t * opt = NULL;
	int c;
	ksw_extz_t ez;
	alignment_t *alignment = alignment_init();

	memset(&ez, 0, sizeof(ksw_extz_t));

	opt = main_opt_init();

	// FIXME: for local or glocal we don't the query/target starts unless we output the cigar
	while ((c = getopt(argc, argv, "M:a:b:q:r:w:m:csHROz:l:h")) >= 0) {
		switch (c) {
			case 'M': opt->alignment_mode = atoi(optarg); break;
			case 'a': opt->match_score = atoi(optarg); break;
			case 'b': opt->mismatch_score = atoi(optarg); break;
			case 'q': opt->gap_open = atoi(optarg); break;
			case 'r': opt->gap_extend = atoi(optarg); break;
			case 'w': opt->band_width = atoi(optarg); break;
			case 'm': opt->matrix_fn = optarg; break; 
			case 'c': opt->add_cigar = 1; break;
			case 's': opt->add_seq = 1; break;
			case 'H': opt->add_header = 1; break;
			case 'R': opt->right_align_gaps = 1; break;
			case 'O': opt->offset_and_length = 1; break;
			case 'z': opt->zdrop = atoi(optarg); break;
			case 'l': opt->library = atoi(optarg); break;
			case 'h': usage(opt); return 1;
			default: usage(opt); return 1;
		}
	}
	if (optind > argc) {
		usage(opt);
		return 1;
	}

	// validate args
	main_opt_validate(opt);
	
	// set the library data **after** setting the scoring matrix
	main_opt_init_library(opt);

	// output the header
	if (opt->add_header) {
		// based output format
		if (opt->offset_and_length == 1) fprintf(stdout, "score\tquery_offset\tquery_length\ttarget_offset\ttarget_length");
		else fprintf(stdout, "score\tquery_start\tquery_end\ttarget_start\ttarget_end");
		// append the cigar
		if (opt->add_cigar == 1) fprintf(stdout, "\tcigar");
		// append the query and target sequence
		if (opt->add_seq) fprintf(stdout, "\tquery\ttarget");
		fputc('\n', stdout);
	}

	// read a query and target at a time
	kstream_t *fp     = ks_init(fileno(stdin));
	kstring_t *query  = (kstring_t*)calloc(1, sizeof(kstring_t));
	kstring_t *target = (kstring_t*)calloc(1, sizeof(kstring_t));
	int retval = 0;
	while (ks_getuntil(fp, 0, query, &retval) > 0 && ks_getuntil(fp, 0, target, &retval) > 0) {
		align(query->s, target->s, opt, alignment);
	}
	free(query->s);
	free(query);
	free(target->s);
	free(target);
	ks_destroy(fp);

	// clean up
	alignment_destroy(alignment);
	main_opt_destroy(opt);

	return 0;
}

