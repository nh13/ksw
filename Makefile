CC=			  gcc
CFLAGS=		  -g -Wall -Wno-unused-function -O2
WRAP_MALLOC=  -DUSE_MALLOC_WRAPPERS
DFLAGS=		  -DHAVE_PTHREAD $(WRAP_MALLOC)
INCLUDES=	
LIBS=		  -lm
LDFLAGS=


SRC_DIR=      src
OBJ_DIR=      obj
SRCS=         $(wildcard $(SRC_DIR)/*.c)
OBJS=         $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

PROG=		  ksw
SUBDIRS=	  .

# Target installation directory
PREFIX:=      /usr/local/bin

ifeq ($(shell uname -s),Linux)
	LIBS += -lrt
endif

.SUFFIXES:.c .o


all:$(PROG)

ksw: $(OBJS) 
	$(CC) $(LDFLAGS) $(DFLAGS) $^ $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/githash.h
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f gmon.out a.out $(PROG) *~ *.a $(SRC_DIR)/githash.h $(OBJS)

$(SRC_DIR)/githash.h:
ifndef PKG_VERSION
	printf '#ifndef GIT_HASH\n#define GIT_HASH "' > $@ && \
	(git describe --tags --exact-match 2> /dev/null || git rev-parse HEAD || printf Unknown) | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@
else
	printf '#ifndef GIT_HASH\n#define GIT_HASH "' > $@ && \
    printf $(PKG_VERSION) >> $@ && \
	printf '"\n#endif\n' >> $@
endif

.PHONY: test clean

test: ksw
	tests/test.sh || exit 1

install: $(PROG)
	install -m 755 $(PROG) "$(PREFIX)"
