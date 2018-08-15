CC=			  gcc
CFLAGS=		  -g -Wall -Wno-unused-function -O2
WRAP_MALLOC=  -DUSE_MALLOC_WRAPPERS
DFLAGS=		  -DHAVE_PTHREAD $(WRAP_MALLOC)
INCLUDES=	
LIBS=		  -lm -lz
LDFLAGS=


SRC_DIR=      src
OBJ_DIR=      obj
SRCS=         $(wildcard $(SRC_DIR)/*.c)
OBJS=         $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
KSW2_SRC_DIR= $(SRC_DIR)/ksw2
KSW2_OBJ_DIR= src/ksw2
KSW2_MAINS=   $(KSW2_OBJ_DIR)/main.c $(KSW2_OBJ_DIR)/cli.c
KSW2_SRCS=    $(filter-out $(KSW2_MAINS),$(wildcard $(KSW2_SRC_DIR)/*c))
KSW2_OBJS=    $(KSW2_SRCS:$(KSW2_SRC_DIR)/%.c=$(KSW2_OBJ_DIR)/%.o)

PROG=		  ksw
SUBDIRS=      $(SRC_DIR)/ksw2/

# Target installation directory
PREFIX:=      /usr/local/bin

ifeq ($(shell uname -s),Linux)
	LIBS += -lrt
endif

.SUFFIXES:.c .o


all:src/ksw2/Makefile $(PROG) versions/version.ksw2.txt

ksw: $(KSW2_OBJS) $(OBJS) 
	$(CC) $(LDFLAGS) $(DFLAGS) $^ $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/githash.h
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DFLAGS) $(INCLUDES) $< -o $@

src/ksw2/Makefile:
	@echo "To build ksw you must use git to also download its submodules."
	@echo "Do so by downloading ksw again using this command (note --recursive flag):"
	@echo "    git clone --recursive git://github.com/nh13/ksw.git"
	@error

clean:
	rm -f gmon.out a.out $(PROG) *~ *.a $(SRC_DIR)/githash.h $(OBJS) versions/version.ksw2.txt
	for dir in $(SUBDIRS); do if [ -d $$dir ]; then $(MAKE) -C $$dir -f Makefile $@; fi; done

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

versions/version.ksw2.txt:
	git ls-tree HEAD src/ksw2 > $@

.PHONY: test clean $(SUBDIRS)

test: ksw
	tests/test.sh || exit 1

install: $(PROG)
	install -m 755 $(PROG) "$(PREFIX)"
