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


all:src/ksw2/Makefile $(PROG)

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
	rm -f gmon.out a.out $(PROG) *~ *.a $(SRC_DIR)/githash.h $(OBJS)
	for dir in $(SUBDIRS); do if [ -d $$dir ]; then $(MAKE) -C $$dir -f Makefile $@; fi; done

$(SRC_DIR)/githash.h: version.ksw.txt
ifndef PKG_VERSION
	$(eval PKG_VERSION := $(shell (git describe --tags --exact-match 2> /dev/null || git rev-parse HEAD || cat version.ksw.txt) | tr -d "\n"))
endif
	printf '#ifndef GIT_HASH\n#define GIT_HASH "' > $@ && \
	printf $(PKG_VERSION) >> $@ && \
	printf '"\n#endif\n' >> $@
	@echo $(PKG_VERSION) > version.ksw.txt

# Developer Note:
# Both version.ksw.txt and $(SRC_DIR)/githash.h are untracked by git. `make clean` will
# only remove the githash.h file, and leave the version.ksw.txt file, so that the next time
# githash.h is generated from scratch, it can default to the last known version, which happens
# if we are building from a non-git-directory (i.e. from the tarball).  This file **is** removed
# using the post-commit, if installed, so it is generated correctly for the next commit.

version.ksw.txt:
	@mkdir -p $(dir $@)
	(if [ ! -f $(SRC_DIR)/githash.h ]; then echo Unknown; else sed -n '2p;3q' $(SRC_DIR)/githash.h | cut -f3 -d ' ' | sed -e 's_"__g'; fi) >> version.ksw.txt

tarball: $(SRC_DIR)/githash.h
ifneq ("$(shell git diff --quiet)", "")
	@echo "Cannot create a tarball: the current working directory is dirty (see git diff)"
	@error
else
ifndef TARBALL_REF
	@echo Setting version
	$(eval TARBALL_REF := $(shell sed -n '2p;3q' $(SRC_DIR)/githash.h | cut -f3 -d ' ' | sed -e 's_"__g'))
endif
	@echo TARBALL_REF $(TARBALL_REF)
	@echo Running git archive...
	git archive --format=tar -o ksw-tmp.tar --prefix ksw-$(TARBALL_REF)/ master
	tar xvf ksw-tmp.tar && rm ksw-tmp.tar
	cp version.ksw.txt ksw-$(TARBALL_REF)/version.ksw.txt
	cp $(SRC_DIR)/githash.h ksw-$(TARBALL_REF)/$(SRC_DIR)/githash.h
	@echo Running git archive on submodules...
	p=`pwd` && git submodule foreach | while read entering path; do \
		echo Adding $$path; \
		temp="$${path%\'}"; \
		temp="$${temp#\'}"; \
		path=$$temp; \
		[ "$$path" = "" ] && continue; \
		pushd $$path; \
		git archive --prefix=ksw-$(TARBALL_REF)/$$path/ HEAD > $$p/tmp.tar; \
		popd; \
		tar xvf tmp.tar; \
		rm tmp.tar; \
	done
	@echo Creating taball
	tar -zcf $(TARBALL_REF).tar.gz ksw-$(TARBALL_REF)
	rm -r ksw-$(TARBALL_REF)
endif


.PHONY: test clean tarball $(SUBDIRS)

test: ksw
	tests/test.sh || exit 1

install: $(PROG)
	install -m 755 $(PROG) "$(PREFIX)"
