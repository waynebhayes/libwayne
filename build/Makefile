# Each variant of the library has its own directory here, e.g. build/libwayne-pg/,
# holding a cflags file (that variant's compiler flags).
# All the real compile rules are in Makefile.variant, shared by every variant.
# A directory with a cflags file IS a variant: adding one needs no edits here.

include Makefile.compiler

# Number of cores to use when invoking parallelism
ifeq ($(strip $(CORES)),)
    override CORES := 2
endif

VARIANTS := $(patsubst %/cflags,%,$(wildcard */cflags))

# Every target in this file that is not a variant.
TARGETS := libs opt debug ndebug intsizes make-successful all watch testlib testlib-errors need-debug-lib clean

# A variant named like a target replaces that target. A directory named all/
# even makes 'make all' call itself forever, so refuse these before doing anything.
BAD_NAMES := $(filter $(TARGETS),$(VARIANTS))
ifneq ($(BAD_NAMES),)
    $(error these build/ directories have a cflags file but use a reserved name: $(BAD_NAMES))
endif

# make splits names on spaces, so a variant name cannot have one.
SPACE_NAMES := $(shell ls -d */cflags 2>/dev/null | grep " ")
ifneq ($(SPACE_NAMES),)
    $(error a variant directory name cannot contain a space: $(SPACE_NAMES))
endif

# clean must finish before anything is built, even under -j.
ifneq ($(filter clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif

# First target, default: just build the standard libwayne.a file
.DEFAULT_GOAL := libwayne

# Each variant target, e.g. 'make libwayne-pg', builds that one library.
# Naming Makefile.variant here means a cflags file alone is enough to make a variant.
$(VARIANTS): intsizes
	$(MAKE) -C $@ -f ../Makefile.variant

# Every variant. They share nothing, so they can all build at once.
libs: $(VARIANTS)

# Old names for three of the variants.
opt: libwayne
debug: libwayne-g
ndebug: libwayne-nd

# include/intSizes.h is only replaced if it actually changed; misc.h includes
# it, and almost everything includes misc.h, so a new timestamp means a full rebuild.
# Temp names hold the process id, so two makes at once cannot trip over each other.
intsizes:
	@mkdir -p ../bin
	$(GCC) -o ../bin/intSizes.$$$$ ../intSizes.c \
	    && ../bin/intSizes.$$$$ > ../include/intSizes.h.new.$$$$ \
	    && { cmp -s ../include/intSizes.h.new.$$$$ ../include/intSizes.h || mv ../include/intSizes.h.new.$$$$ ../include/intSizes.h; } \
	    && mv ../bin/intSizes.$$$$ ../bin/intSizes; \
	status=$$?; /bin/rm -f ../bin/intSizes.$$$$ ../include/intSizes.h.new.$$$$; exit $$status

../bin/parallel: ../parallel.c
	@mkdir -p ../bin
	$(CC) $(STACKSIZE) -o $@ $<

../make-successful:
	touch ../make-started # create this file so it is OLDER than .a files
	$(MAKE) all
	mv ../make-started ../make-successful

make-successful: ../make-successful

all:
	@if [ -f ../do-not-make ]; then echo "not making; assuming they already exist"; ls -l ../libwayne*.a; else $(MAKE) -j$(CORES) libs ../bin/parallel; fi
	@# BLANT runs 'make all', so failed tests warn here but do not stop it. 'make testlib' does fail.
	@$(MAKE) testlib || echo "WARNING: some libwayne tests failed; see the FAILED line above" >&2

# Run 'make watch' in a separate terminal for auto-recompilation
watch:
	ls ../src/*.c ../include/*.h | entr $(MAKE) -j$(CORES) libs

# The tests link with bin/wgcc, which uses libwayne-g.a. Not a dependency, so
# that do-not-make can still run the tests without compiling anything.
need-debug-lib:
	@[ -f ../libwayne-g.a ] || { echo "the tests need libwayne-g.a; run 'make debug' first" >&2; exit 1; }

# Runs every test, then fails if any test failed, naming each one.
testlib: need-debug-lib
	@export LIBWAYNE_HOME="$$(cd .. && pwd)"; failed=; \
	for x in ebm covar stats hash raw_hashmap htree-test avltree-test bintree-test CI graph-sanity tinygraph-sanity graph-weighted graph-addedgelist-test circ_buf sim_anneal; do \
	    rm -f ../bin/$$x ../tests/$$x.o; \
	    ( cd ../tests && $(MAKE) $$x && mv $$x ../bin || exit 1; \
	      IN=/dev/null; ARG=; [ -f $$x.in ] && { IN=$$x.in; ARG=$$x.in; }; \
	      out=/tmp/$$x.test$$$$; \
	      cat $$IN | ../bin/$$x $$ARG > $$out 2>&1; status=$$?; \
	      if [ $$status = 0 ]; then if [ -f $$x.out ]; then cmp $$out $$x.out; status=$$?; else wc < $$out; fi; fi; \
	      /bin/rm -f $$out; exit $$status ) || failed="$$failed $$x"; \
	done; \
	if [ -n "$$failed" ]; then echo "testlib FAILED:$$failed" >&2; exit 1; fi; \
	echo "testlib: all tests passed"

testlib-errors: need-debug-lib
	export LIBWAYNE_HOME="$$(cd .. && pwd)"; ( cd ../tests && $(MAKE) graph-addedgelist-errors-test && mv graph-addedgelist-errors-test ../bin || exit 1; for c in 1 2; do ../bin/graph-addedgelist-errors-test $$c > /tmp/graph-addedgelist-errors-test.$$c 2>&1 && { echo "FAIL: case $$c of graph-addedgelist-errors-test unexpectedly succeeded" >&2; exit 1; }; echo "case $$c correctly failed as expected:"; cat /tmp/graph-addedgelist-errors-test.$$c; /bin/rm -f /tmp/graph-addedgelist-errors-test.$$c; done )

clean:
	/bin/rm -f ../make-successful ../make-started
	@# The following is meant to remove the non-Windows binary, ie stats but not stats.exe.
	/bin/rm -f ../bin/stats ../bin/raw_hashmap ../bin/hash
	@# One part failing must not stop the others from being cleaned.
	@status=0; \
	for v in $(VARIANTS); do $(MAKE) -C $$v -f ../Makefile.variant clean || status=1; done; \
	( cd ../C++ && $(MAKE) clean ) || status=1; \
	exit $$status

.PHONY: $(VARIANTS) $(TARGETS)
