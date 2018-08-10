CC=			gcc
CFLAGS=		-g -Wall -Wno-unused-function -O2
WRAP_MALLOC=-DUSE_MALLOC_WRAPPERS
DFLAGS=		-DHAVE_PTHREAD $(WRAP_MALLOC)
OBJS=		main.o
PROG=		ksw
INCLUDES=	
LIBS=		-lm
SUBDIRS=	.

ifeq ($(shell uname -s),Linux)
	LIBS += -lrt
endif

.SUFFIXES:.c .o

.c.o:
	$(CC) -c $(CFLAGS) $(DFLAGS) $(INCLUDES) $< -o $@

all:$(PROG)

ksw:$(OBJS)

clean:
	rm -f gmon.out *.o a.out $(PROG) *~ *.a

depend:
	( LC_ALL=C ; export LC_ALL; makedepend -Y -- $(CFLAGS) $(DFLAGS) -- *.c )

# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: ksw.h githash.h

githash.h:
	printf '#ifndef GIT_HASH\n#define GIT_HASH "' > $@ && \
	(git describe --exact-match || git rev-parse HEAD || echo 'Unknown') | sed '/^\s*$$/d' | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@

.PHONY: test

test: ksw
	tests/test.sh || exit 1

