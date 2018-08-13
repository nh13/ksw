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
	fprintf(stderr, "       -0 INT      The start score (extend only) [%d]\n", opt->start_score);
	fprintf(stderr, "       -m FILE     Path to the scoring matrix (4x4 or 5x5) [%s]\n", opt->matrix_fn == NULL ? "None" : opt->matrix_fn);
	fprintf(stderr, "       -c          Append the cigar to the output [%s]\n", opt->add_cigar == 0 ? "false" : "true");
	fprintf(stderr, "       -s          Append the query and target to the output [%s]\n", opt->add_seq == 0 ? "false" : "true");
	fprintf(stderr, "       -H          Add a header line to the output [%s]\n", opt->add_header == 0 ? "false" : "true");
}

main_opt_t *main_opt_init()
{
	main_opt_t *opt = calloc(1, sizeof(main_opt_t));

	opt->match_score = 1;
	opt->mismatch_score = 3;
	opt->gap_open = 5;
	opt->gap_extend = 2;
	opt->band_width = INT_MAX / 4; // divide by four since in some places we multiply by two
	opt->start_score = 0;
	opt->matrix_fn = NULL;
	opt->alignment_mode = 0;
	opt->add_cigar = 0;
	opt->add_seq = 0;
	opt->add_header = 0;

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
		int *n_cigar, 
		uint32_t **cigar)
{
	int ql = qle - qlb;
	int tl = tle - tlb;
	int s = ksw_global(ql, (uint8_t*)(query + qlb), tl, (uint8_t*)(target + tlb), 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, n_cigar, cigar);
	if (s != prev_score) {
		int i;
		fprintf(stderr, "Bug: prev_score: %d new_score: %d\n", prev_score, s);
		fprintf(stderr, "\tqlb: %d qle: %d tlb: %d tle: %d\n", qlb, qle, tlb, tle);
		fputc('\t', stderr);
		fprintf(stderr, "query: ");
		for (i = 0; i < ql; ++i) fputc("ACGTN"[(uint8_t)query[i]], stderr);
		fputc('\n', stderr);
		fputc('\t', stderr);
		fprintf(stderr, "target: ");
		for (i = 0; i < tl; ++i) fputc("ACGTN"[(uint8_t)target[i]], stderr);
		fputc('\n', stderr);
		exit(1);
	}
}

void align(char *query, char *target, main_opt_t *opt, int8_t *mat)
{
	int i;
	int qlb = 0, qle = 0, tlb = 0, tle = 0; // query/target x alignment start/end 
	int ql = strlen(query); // query length
	int tl = strlen(target); // target length
	int xtra = KSW_XSTART | KSW_XSUBO; // for ksw_align
	kswr_t r; // for ksw_align
	int n_cigar = 0;
	uint32_t *cigar = NULL;

	// remove ending newlines
	if (query[ql-1] == '\n') { query[ql-1] = '\0'; ql--; }
	if (target[tl-1] == '\n') { target[tl-1] = '\0'; tl--; }

	// convert to bases in integer format
	for (i = 0; i < ql; ++i) query[i] = seq_nt4_table[(int)query[i]];
	for (i = 0; i < tl; ++i) target[i] = seq_nt4_table[(int)target[i]];

	int *n_cigar_ = opt->add_cigar ? &n_cigar : NULL;
	uint32_t **cigar_ = opt->add_cigar ? &cigar : NULL;

	int s = 0; // the score
	kswq_t *q = 0;
	switch (opt->alignment_mode) {
		case 0: // local
			r = ksw_align(ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, xtra, &q);
			s = r.score;
			// set return values
			qlb = r.qb;
			qle = r.qe+1;
			tlb = r.tb;
			tle = r.te+1;
			free(q);
			if (opt->add_cigar) get_cigar(query, target, opt, mat, qlb, qle, tlb, tle, s, n_cigar_, cigar_); 
			break;
		case 1: // glocal: full query, local target
			s = ksw_glocal(ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, &tlb, &tle, n_cigar_, cigar_);
			qlb = 0; // start from the beginning
			qle = ql;
			break;
		case 2: // extend
			s = ksw_extend(ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, opt->start_score, &qle, &tle);
			qlb = tlb = 0; // start from the beginning
			if (opt->add_cigar) get_cigar(query, target, opt, mat, qlb, qle, tlb, tle, s, n_cigar_, cigar_); 
			break;
		case 3: // global
			s = ksw_global(ql, (uint8_t*)query, tl, (uint8_t*)target, 5, mat, opt->gap_open, opt->gap_extend, opt->band_width, n_cigar_, cigar_);
			qlb = tle = 0; // start from the beginning
			// use all of query and target
			qle = ql;
			tle = tl;
			break;
		default:
			fprintf(stderr, "Unknown mode: %d\n", opt->alignment_mode); 
			exit(1);
	}

	fprintf(stdout, "%d\t%d\t%d\t%d\t%d", s, qlb, qle, tlb, tle);
	if (opt->add_cigar == 1) {
		fputc('\t', stdout);
		for (i = 0; i < n_cigar; ++i) {
			int len = cigar[i] >> 4;
			int op = cigar[i] & 0xf; 
			fprintf(stdout, "%d%c", len, "MID"[op]);
		}
		free(cigar);
	}
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

	opt = main_opt_init();

	while ((c = getopt(argc, argv, "M:a:b:q:r:w:0:m:csHh")) >= 0) {
		switch (c) {
			case 'M': opt->alignment_mode = atoi(optarg); break;
			case 'a': opt->match_score = atoi(optarg); break;
			case 'b': opt->mismatch_score = atoi(optarg); break;
			case 'q': opt->gap_open = atoi(optarg); break;
			case 'r': opt->gap_extend = atoi(optarg); break;
			case 'w': opt->band_width = atoi(optarg); break;
			case '0': opt->start_score = atoi(optarg); break;
			case 'm': opt->matrix_fn = optarg; break; 
			case 'c': opt->add_cigar = 1; break;
			case 's': opt->add_seq = 1; break;
			case 'H': opt->add_header = 1; break;
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

	if (opt->add_header) {
		fprintf(stdout, "score\tquery_start\tquery_end\ttarget_start\ttarget_end");
		if (opt->add_cigar == 1) fprintf(stdout, "\tcigar");
		if (opt->add_seq) fprintf(stdout, "\tquery\ttarget");
		fputc('\n', stdout);
	}

	// read a query and target at a time
	while (NULL != fgets(query, buffer, stdin) && NULL != fgets(target, buffer, stdin)) {
		align(query, target, opt, mat);
	}
	free(opt);

	return 0;
}

