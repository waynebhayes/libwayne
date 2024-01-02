# Attempt to figure out what kind of machine we're on.
UNAME=$(shell uname -a | awk '{if(/CYGWIN/){V="CYGWIN"}else if(/Darwin/){if(/arm64/)V="arm64";else V="Darwin"}else if(/Linux/){V="Linux"}}END{if(V){print V;exit}else{print "unknown OS" > "/dev/stderr"; exit 1}}')

# Number of cores to use when invoking parallelism
ifndef CORES
    CORES := 2
endif

# Waywe needs gcc-6 on MacOS:
GCC_VER=$(shell echo $(UNAME) $(HOME) | awk '/Darwin/&&/Users.wayne/{V="-6"}END{if(V)print V;else{printf "using default gcc: " > "/dev/null"; exit 1}}')
GCC=gcc$(GCC_VER) # gcc gcc-4.2 gcc-6 gcc-7 gcc-8 gcc-9 # Possibilities on Darwin
STACKSIZE=$(shell ($(GCC) -v 2>&1; uname -a) | awk '/CYGWIN/{print "-Wl,--stack,83886080"}/gcc-/{actualGCC=1}/Darwin/&&actualGCC{print "-Wl,-stack_size -Wl,0x5000000"}')
CC=$(GCC) $(OPT) $(GDB) $(DEBUG) -Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wshadow $(PG) $(STACKSIZE)
LIBWAYNE_HOME:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

default: gcc-ver
	if ls -rtlL *.a | tail -1 | grep .-g; then $(MAKE) debug; else $(MAKE) opt; fi

gcc-ver:
	$(GCC) -v

all:
	@if [ -f do-not-make ]; then echo "not making; assuming they already exist"; ls -l libwayne*.a; else /bin/rm -f *.a; $(MAKE) libwayne_all; fi
	$(MAKE) testlib

libwayne_all:
	/bin/rm -f *.a
	# Make the pg versions (for profiling)
	$(MAKE) debug_clean
	$(MAKE) -j$(CORES) 'PG=-pg' debug && mv libwayne-g.a src/libwayne-pg-g/libwayne-pg-g.a
	$(MAKE) opt_clean
	$(MAKE) -j$(CORES) 'PG=-pg' opt && mv libwayne.a src/libwayne-pg/libwayne-pg.a
	# Make the non-pg versions (for profiling)
	$(MAKE) debug_clean
	$(MAKE) -j$(CORES) debug && mv libwayne-g.a src/libwayne-g/libwayne-g.a
	$(MAKE) opt_clean
	$(MAKE) -j$(CORES) opt && mv libwayne.a src/libwayne/libwayne.a
	cp src/libwayne*/*.a src && cp src/*.a .

testlib:
	export LIBWAYNE_HOME=$(LIBWAYNE_HOME); for x in ebm covar stats hash raw_hashmap htree-test bintree-test CI graph-sanity graph-weighted; do rm -f bin/$$x tests/$$x.o; (cd tests; $(MAKE) $$x; mv $$x ../bin; [ -f $$x.in ] && cat $$x.in | ../bin/$$x $$x.in | if [ -f $$x.out ]; then cmp - $$x.out; else wc; fi); done

opt:
	$(MAKE) 'OPT=-O2' 'LIBOUT=libwayne.a' libwayne

debug:
	$(MAKE) 'GDB=-ggdb' 'DEBUG=-DDEBUG=1' 'LIBOUT=libwayne-g.a' libwayne

libwayne:
	$(MAKE) $(LIBOUT)
	# add misc.o below since adding nothing fails on Mac M1, and misc.o is pretty much necessary for libwayne to work.
	[ "$(UNAME)" = Darwin ] || ar r $(LIBOUT) src/misc.o

debug_clean:
	@$(MAKE) 'GDB=-ggdb' 'DEBUG=-DDEBUG=1' 'LIBOUT=libwayne-g.a' raw_clean

opt_clean:
	@$(MAKE) 'OPT=-O2' 'LIBOUT=libwayne.a' raw_clean

raw_clean:
	@/bin/rm -f src/*.[oa] $(LIBOUT)
	@cd MT19937; $(MAKE) clean

clean:
	@# The following is meant to remove the non-Windows binary, ie stats but not stats.exe.
	@/bin/rm -f bin/stats bin/raw_hashmap bin/hash
	@/bin/rm -f *.a
	@$(MAKE) debug_clean
	@$(MAKE) opt_clean

$(LIBOUT): src/$(LIBOUT)
	if ranlib src/$(LIBOUT); then :; else echo "ranlib failed but it's not crucial" >&2; fi
	mv src/$(LIBOUT) .

src/$(LIBOUT):
	cd src; $(MAKE) 'CC=$(CC)' 'LIBOUT=$(LIBOUT)' 'GDB=$(GDB)' '$(LIBOUT)'
