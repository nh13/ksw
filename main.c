#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <emmintrin.h>
#include <unistd.h>
#include <stdio.h>
#include <zlib.h>
#include "ksw.h"

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


int main(int argc, char *argv[])
{
	int c, sa = 1, sb = 3, i, j, k, forward_only = 0, max_rseq = 0;
	int8_t mat[25];
	int gapo = 5, gape = 2, minsc = 0;
	uint8_t *rseq = 0;
	int m = 5, w = 10;
	int h0 = 0;
	int usage = 0;
	int mode = 0; // 0 local, 1 glocal, 2 extend, 3 global
	char *mat_fn = 0;
	kswr_t r;
	int buffer=1024; 
	char query[buffer], target[buffer];
	uint8_t query_nt4[buffer], target_nt4[buffer];
	int xtra = KSW_XSTART | KSW_XSUBO;
	int with_seq = 0;

	// parse command line
	while ((c = getopt(argc, argv, "a:b:q:r:t:w:0:m:M:sh")) >= 0) {
		switch (c) {
			case 'a': sa = atoi(optarg); break;
			case 'b': sb = atoi(optarg); break;
			case 'q': gapo = atoi(optarg); break;
			case 'r': gape = atoi(optarg); break;
			case 't': minsc = atoi(optarg); break;
			case 'w': w = atoi(optarg); break;
			case '0': h0 = atoi(optarg); break;
			case 'm': mat_fn = optarg; break; 
			case 'M': mode = atoi(optarg); break;
			case 's': with_seq = 1; break; 
			case 'h': usage = 1; break; 
		}
	}
	if (usage || optind > argc) {
		// TODO: proper usage message
		fprintf(stderr, "Usage: ksw [-a <match> %d] [-b <mismatch> %d] [-q <gap-open> %d] [-r <gap-extend> %d] [-t <min-score> %d] [-w <band-width> %d] [-0 <start-score> %d]\n", sa, sb, gapo, gape, minsc, w, h0);
		return 1;
	}
	if (minsc > 0xffff) minsc = 0xffff;
	xtra |= minsc;
	// initialize scoring matrix
	for (i = k = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j) {
			mat[k++] = i == j? sa : -sb;
		}
		mat[k++] = 0; // ambiguous base
	}
	for (j = 0; j < 5; ++j) mat[k++] = 0;
	if (mat_fn != NULL) fill_matrix(mat, mat_fn);


	while (NULL != fgets(query, buffer, stdin) && NULL != fgets(target, buffer, stdin)) {
		int ql = strlen(query);
		int tl = strlen(target);

		if (query[ql-1] == '\n') { query[ql-1] = '\0'; ql--; }
		if (target[tl-1] == '\n') { target[tl-1] = '\0'; tl--; }
		
		for (i = 0; i < ql; ++i) query_nt4[i] = seq_nt4_table[(int)query[i]];
		for (i = 0; i < tl; ++i) target_nt4[i] = seq_nt4_table[(int)target[i]];

		int qlb = 0;
		int qle = 0;
		int tlb = 0;
		int tle = 0;
		

		//int mode = 0; // 0 local, 1 glocal, 2 extend, 3 global
		int s = 0;
		kswq_t *q = 0;
		switch (mode) {
			case 0: // local
				r = ksw_align(ql, (uint8_t*)query_nt4, tl, (uint8_t*)target_nt4, 5, mat, gapo, gape, xtra, &q);
				s = r.score;
				// set return values
				qlb = r.qb;
				qle = r.qe+1;
				tlb = r.tb;
				tle = r.te+1;
				free(q);
				break;
			case 1: // glocal: full query, local target
				s = ksw_glocal(ql, (uint8_t*)query_nt4, tl, (uint8_t*)target_nt4, 5, mat, gapo, gape, 1000, &tlb, &tle, NULL, NULL);
				qlb = 0; // start from the beginning
				qle = ql;
				break;
			case 2: // extend
				s = ksw_extend(ql, (uint8_t*)query_nt4, tl, (uint8_t*)target_nt4, 5, mat, gapo, gape, 1000, h0, &qle, &tle);
				qlb = tle = 0; // start from the beginning
				break;
			case 3: // global
				s = ksw_global(ql, (uint8_t*)query_nt4, tl, (uint8_t*)target_nt4, 5, mat, gapo, gape, 1000, NULL, NULL);
				qlb = tle = 0; // start from the beginning
				// use all of query and target
				qle = ql;
				tle = tl;
				break;
		}

		//fprintf(stdout, "%d %d query: %s target: %s qle: %d tle: %d s: %d\n", ql, tl, query, target, qle, tle, s);
		if (with_seq) {
			fprintf(stdout, "%d\t%s\t%d\t%d\t%s\t%d\t%d\n", s, query, qlb, qle, target, tlb, tle);
		}
		else {
			fprintf(stdout, "%d\t%d\t%d\t%d\t%d\n", s, qlb, qle, tlb, tle);
		}
		fflush(stdout);
	}
}

