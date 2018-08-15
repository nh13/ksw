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
#include "ksw.h"
#include "ksw2/ksw2.h"
#include "githash.h"
#include "main.h"

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

void fill_matrix(int8_t *mat, char *fn) {
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
			mat[i] = atoi(pch);
			i++;
		}
	}
	if (i != 16 || i != 25) {
		fprintf(stderr, "Incorrect # of values (found %d, want 16 or 25) in %s\n", i, fn);
		exit(1);
	}

	fclose(fp);
}

char *mode_to_str(mode)
{
	switch (mode) {
		case 0: return "local";
		case 1: return "glocal";
		case 2: return "extend";
		case 3: return "global";
		default:
				fprintf(stderr, "Unknown mode: %d\n", mode); 
				exit(1);
	}
}

void usage(main_opt_t *opt)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Program: ksw (klib smith-waterman)\n");
	fprintf(stderr, "Version: %s\n", GIT_HASH);
	fprintf(stderr, "Usage: ksw [options]\n\n");
	fprintf(stderr, "Algorithm options:\n\n");
	fprintf(stderr, "       -M INT      The alignment mode: 0 - local, 1 - glocal, 2 - extend, 3 - global [%d/%s]\n", opt->alignment_mode, mode_to_str(opt->alignment_mode));
	fprintf(stderr, "       -a INT      The match score (>0) [%d]\n", opt->match_score);
	fprintf(stderr, "       -b INT      The mismatch penalty (>0) [%d]\n", opt->mismatch_score);
	fprintf(stderr, "       -q INT      The gap open penalty (>0) [%d]\n", opt->gap_open);
	fprintf(stderr, "       -r INT      The gap extend penalty (>0) [%d]\n", opt->gap_extend);
	fprintf(stderr, "       -w INT      The band width [%d]\n", opt->band_width);
	fprintf(stderr, "       -m FILE     Path to the scoring matrix (4x4 or 5x5) [%s]\n", opt->matrix_fn == NULL ? "None" : opt->matrix_fn);
	fprintf(stderr, "       -c          Append the cigar to the output [%s]\n", opt->add_cigar == 0 ? "false" : "true");
	fprintf(stderr, "       -s          Append the query and target to the output [%s]\n", opt->add_seq == 0 ? "false" : "true");
	fprintf(stderr, "       -H          Add a header line to the output [%s]\n", opt->add_header == 0 ? "false" : "true");
	fprintf(stderr, "       -R          Right-align gaps [%s]\n", opt->right_align_gaps == 0 ? "false" : "true");
	fprintf(stderr, "       -o          Output offset-and-length, otherwise start-and-end (all zero-based)[%s]\n", opt->offset_and_length == 0 ? "false" : "true");
}

main_opt_t *main_opt_init()
{
	main_opt_t *opt = calloc(1, sizeof(main_opt_t));

	opt->match_score = 1;
	opt->mismatch_score = 3;
	opt->gap_open = 5;
	opt->gap_extend = 2;
	opt->band_width = INT_MAX / 4; // divide by four since in some places we multiply by two
	opt->matrix_fn = NULL;
	opt->alignment_mode = 0;
	opt->add_cigar = 0;
	opt->add_seq = 0;
	opt->add_header = 0;
	opt->right_align_gaps = 0;
	opt->offset_and_length = 0;

	return opt;
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
	assert_or_exit(0 <= opt->alignment_mode && opt->alignment_mode <= 3, "Alignment mode (-M) was not valid ([0-3]), found %d.", opt->alignment_mode);
	assert_or_exit(opt->mismatch_score > 0, "Mismatch score (-a) must be greater than zero, found %d.", opt->mismatch_score);
	assert_or_exit(opt->match_score > 0, "Mismatch score (-b) must be greater than zero, found %d.", opt->mismatch_score);
	assert_or_exit(opt->gap_open > 0, "Gap open penalty (-q) must be greater than zero, found %d.", opt->gap_open);
	assert_or_exit(opt->gap_extend > 0, "Gap extend penalty (-r) must be greater than zero, found %d.", opt->gap_extend);
	assert_or_exit(0 <= opt->band_width, "Band width (-w) must be greater than or equal zero, found %d.", opt->band_width);
}


void get_cigar(
		char *query, 
		char *target, 
		main_opt_t *opt, 
		int8_t *mat, 
		int qlb,
		int qle,
		int tlb,
		int tle,
		int prev_score, 
		void *km,
		ksw_extz_t *ez)
{
	int ql = qle - qlb + 1;
	int tl = tle - tlb + 1;
	int ksw2_flags = (opt->right_align_gaps == 1) ? KSW_EZ_RIGHT : 0;
	ksw_extz2_sse(km, ql, (uint8_t*)(query+qlb), tl, (uint8_t*)(target+tlb), 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, -1, ksw2_flags, ez);
	int new_score = ez->score;
	if (new_score != prev_score) {
		int i;
		fprintf(stderr, "Bug: prev_score: %d new_score: %d\n", prev_score, new_score);
		fprintf(stderr, "\tqlb: %d qle: %d tlb: %d tle: %d\n", qlb, qle, tlb, tle);
		fputc('\t', stderr);
		fprintf(stderr, "query: ");
		for (i = 0; i < ql; ++i) fputc("ACGTN"[(uint8_t)query[i+qlb]], stderr);
		fputc('\n', stderr);
		fputc('\t', stderr);
		fprintf(stderr, "target: ");
		for (i = 0; i < tl; ++i) fputc("ACGTN"[(uint8_t)target[i+tlb]], stderr);
		fputc('\n', stderr);
		exit(1);
	}
}

void align(char *query, char *target, main_opt_t *opt, int8_t *mat, void *km, ksw_extz_t *ez)
{
	int i, j, score;
	int qlb = 0, qle = 0, tlb = 0, tle = 0; // query/target x alignment start/end, zero-based
	int ql = strlen(query); // query length
	int tl = strlen(target); // target length
	int xtra = KSW_XSTART | KSW_XSUBO; // for ksw_align
	kswr_t r; // for ksw_align
	int ksw2_flags = (opt->right_align_gaps == 1) ? KSW_EZ_RIGHT : 0;
	if (opt->add_cigar != 1) ksw2_flags |= KSW_EZ_SCORE_ONLY;

	// remove ending newlines
	if (query[ql-1] == '\n') { query[ql-1] = '\0'; ql--; }
	if (target[tl-1] == '\n') { target[tl-1] = '\0'; tl--; }

	// convert to bases in integer format
	for (i = 0; i < ql; ++i) query[i] = seq_nt4_table[(int)query[i]];
	for (i = 0; i < tl; ++i) target[i] = seq_nt4_table[(int)target[i]];

	// TODO: store a function and use it
	kswq_t *q = 0;
	switch (opt->alignment_mode) {
		case 0: // local
			r = ksw_align(ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, xtra, &q);
			score = r.score;
			// set return values
			qlb = r.qb;
			qle = r.qe;
			tlb = r.tb;
			tle = r.te;
			free(q);
			if (opt->add_cigar) get_cigar(query, target, opt, mat, qlb, qle, tlb, tle, score, km, ez);
			break;
		case 1: // glocal: full query, local target
			ksw_extz2_sse(km, ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, -1, ksw2_flags, ez);
			score = ez->score;
			qlb = tlb = 0;
			qle = ql-1;
			tle = tl-1;
			// Convert from global to glocal by removing leading and trailing deletions
			// remove leading deletions
			for (j = 0; j < ez->n_cigar && (ez->cigar[j]&0xf) == 2; ++j) { 
				int len = ez->cigar[j]>>4;
				tlb += len;
				score += opt->gap_open + (len * opt->gap_extend);
			}
			if (j > 0) {
				for (i = 0; j < ez->n_cigar; ++i, ++j) ez->cigar[i] = ez->cigar[j];
				ez->n_cigar = i;
			}
			// remove trailing deletions
			for (i = ez->n_cigar-1, j = 0;0 <= i && (ez->cigar[i]&0xf) == 2; --i, ++j) { 
				int len = ez->cigar[i]>>4;
				tle -= len;
				score += opt->gap_open + (len * opt->gap_extend);
			}
			if (0 < j) ez->n_cigar -= j;
			break;
		case 2: // extend
			ksw_extz2_sse(km, ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, -1, ksw2_flags | KSW_EZ_EXTZ_ONLY, ez);
			score   = ez->mqe; // maximum score when we reach the end of the query
			if (ez->max_q < 0) {
				qlb = tlb = qle = tle = -1;
			}
			else {
				qlb = tlb = 0;
				qle = ez->max_q;
				tle = ez->max_t;
			}
			break;
		case 3: // global
			ksw_extz2_sse(km, ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, -1, ksw2_flags, ez);
			score = ez->score;
			qlb   = tlb = 0;
			qle   = ql-1;
			tle   = tl-1;
			break;
		default:
			fprintf(stderr, "Unknown mode: %d\n", opt->alignment_mode); 
			exit(1);
	}
	// output the score and start/end or start/length
	if (opt->offset_and_length == 1) {
		int qll = qle - qlb + 1;
		int tll = tle - tlb + 1;
		fprintf(stdout, "%d\t%d\t%d\t%d\t%d", score, qlb, qll, tlb, tll);
	}
	else {
		fprintf(stdout, "%d\t%d\t%d\t%d\t%d", score, qlb, qle, tlb, tle);
	}
	// output the cigar
	if (opt->add_cigar == 1) {
		fputc('\t', stdout);
		if (ez->n_cigar == 0) fputc('*', stdout);
		for (i = 0; i < ez->n_cigar; ++i) {
			fprintf(stdout, "%d%c", ez->cigar[i]>>4, "MID"[ez->cigar[i]&0xf]);
		}
	}
	// output the query and target
	if (opt->add_seq) {
		fputc('\t', stdout);
		for (i = 0; i < ql; ++i) fputc("ACGTN"[(uint8_t)query[i]], stdout);
		fputc('\t', stdout);
		for (i = 0; i < tl; ++i) fputc("ACGTN"[(uint8_t)target[i]], stdout);
	}
	fputc('\n', stdout);
	fflush(stdout);
}


int main(int argc, char *argv[])
{
	main_opt_t * opt = NULL;
	int c, i, j, k;
	int8_t mat[25];
	int buffer=1024; 
	char query[buffer], target[buffer];
	void *km = 0;
	ksw_extz_t ez;

#ifdef HAVE_KALLOC
	km = no_kalloc? 0 : km_init();
#endif
	memset(&ez, 0, sizeof(ksw_extz_t));

	opt = main_opt_init();

	while ((c = getopt(argc, argv, "M:a:b:q:r:w:m:csHROh")) >= 0) {
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

	// initialize scoring matrix
	for (i = k = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j) {
			mat[k++] = i == j? opt->match_score: -opt->mismatch_score;
		}
		mat[k++] = 0; // ambiguous base
	}
	for (j = 0; j < 5; ++j) mat[k++] = 0;

	// overwrite if a matrix file was given
	if (opt->matrix_fn != NULL) fill_matrix(mat, opt->matrix_fn);

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
	while (NULL != fgets(query, buffer, stdin) && NULL != fgets(target, buffer, stdin)) {
		align(query, target, opt, mat, km, &ez);
	}
	free(opt);

	kfree(km, ez.cigar);
#ifdef HAVE_KALLOC
	km_destroy(km);
#endif

	return 0;
}

