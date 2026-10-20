# The libwayne build. Run it from here (cd build; make) or from the top of
# libwayne, whose Makefile just hands every target to this one.
#
# Each variant of the library has its own directory here, e.g. build/libwayne-pg/,
# holding a cflags file (that variant's compiler flags) and a one-line Makefile.
# All the real compile rules are in Makefile.variant, shared by every variant.
# A directory with a cflags file IS a variant: adding one needs no edits here.
#
# Running make again only recompiles what changed, so there is no separate
# "incremental" build.

include Makefile.compiler

# Number of cores to use when invoking parallelism
ifndef CORES
    CORES := 2
endif

LIBWAYNE_HOME := $(abspath $(CURDIR)/..)

VARIANTS := $(patsubst %/cflags,%,$(wildcard */cflags))

# First target, default: just build the standard libwayne.a file
.DEFAULT_GOAL := libwayne

# Each variant target, e.g. 'make libwayne-pg', builds that one library.
$(VARIANTS): intsizes
	$(MAKE) -C $@

# Every variant. They share nothing, so they can all build at once.
libs: $(VARIANTS)

# Old names for three of the variants.
opt: libwayne
debug: libwayne-g
ndebug: libwayne-nd

# include/intSizes.h is only replaced if it actually changed; misc.h includes
# it, and almost everything includes misc.h, so a new timestamp means a full rebuild.
intsizes:
	@mkdir -p ../bin
	$(GCC) -o ../bin/intSizes ../intSizes.c && ../bin/intSizes > ../include/intSizes.h.new
	@cmp -s ../include/intSizes.h.new ../include/intSizes.h || mv ../include/intSizes.h.new ../include/intSizes.h
	@/bin/rm -f ../include/intSizes.h.new

../bin/parallel: ../parallel.c
	$(CC) $(STACKSIZE) -o $@ $<

../make-successful:
	touch ../make-started # create this file so it is OLDER than .a files
	$(MAKE) all
	mv ../make-started ../make-successful

make-successful: ../make-successful

all:
	@if [ -f ../do-not-make ]; then echo "not making; assuming they already exist"; ls -l ../libwayne*.a; else $(MAKE) -j$(CORES) libs ../bin/parallel; fi
	$(MAKE) testlib

# Run 'make watch' in a separate terminal for auto-recompilation
watch:
	ls ../src/*.c ../include/*.h | entr $(MAKE) -j$(CORES) libs

testlib:
	export LIBWAYNE_HOME=$(LIBWAYNE_HOME); for x in ebm covar stats hash raw_hashmap htree-test avltree-test bintree-test CI graph-sanity tinygraph-sanity graph-weighted graph-addedgelist-test circ_buf sim_anneal; do rm -f ../bin/$$x ../tests/$$x.o; ( cd ../tests; $(MAKE) $$x; mv $$x ../bin; IN=/dev/null; [ -f $$x.in ] && IN=$$x.in; cat $$IN | ../bin/$$x $$x.in > /tmp/$$x.test$$$$ 2>&1 || exit 1; cat /tmp/$$x.test$$$$ | if [ -f $$x.out ]; then cmp - $$x.out; else wc; fi; /bin/rm -f /tmp/$$x.test$$$$); done

# graph-addedgelist-errors-test deliberately Fatal()s (exit 1) on every valid invocation, since it
# demonstrates GraphAddEdgeList's input-validation failures--so it can't share testlib's generic
# "exit 0 == pass" loop above. Here, success means the program DID exit nonzero.
testlib-errors:
	export LIBWAYNE_HOME=$(LIBWAYNE_HOME); ( cd ../tests; $(MAKE) graph-addedgelist-errors-test; mv graph-addedgelist-errors-test ../bin; for c in 1 2; do ../bin/graph-addedgelist-errors-test $$c > /tmp/graph-addedgelist-errors-test.$$c 2>&1 && { echo "FAIL: case $$c of graph-addedgelist-errors-test unexpectedly succeeded" >&2; exit 1; }; echo "case $$c correctly failed as expected:"; cat /tmp/graph-addedgelist-errors-test.$$c; /bin/rm -f /tmp/graph-addedgelist-errors-test.$$c; done )

# Removes only build outputs. Never rm -rf: this directory holds tracked files.
clean:
	for v in $(VARIANTS); do $(MAKE) -C $$v clean || exit 1; done
	/bin/rm -f ../make-successful
	@# The following is meant to remove the non-Windows binary, ie stats but not stats.exe.
	/bin/rm -f ../bin/stats ../bin/raw_hashmap ../bin/hash
	cd ../C++ && $(MAKE) clean

.PHONY: $(VARIANTS) libs opt debug ndebug intsizes make-successful all watch testlib testlib-errors clean
