# Attempt to figure out what kind of machine we're on.
ARCH=$(shell uname -a | awk '{if(/CYGWIN/){V="CYGWIN"}else if(/Darwin/){V="Darwin"}else if(/Linux/){V="Linux"}}END{if(V){print V;exit}}')

# Number of cores to use when invoking parallelism
ifndef CORES
    CORES := 2
endif

# Waywe needs gcc-6 on MacOS:
GCC_VER=$(shell echo $(ARCH) $(HOME) | awk '/Darwin/&&/Users.wayne/{V="-6"}END{if(V)print V;else{printf "using default gcc: " > "/dev/null"; exit 1}}')
GCC=gcc$(GCC_VER) # gcc gcc-4.2 gcc-6 gcc-7 gcc-8 gcc-9 # Possibilities on Darwin
STACKSIZE=$(shell ($(GCC) -v 2>&1; uname -a) | awk '/CYGWIN/{print "-Wl,--stack,83886080"}/gcc-/{actualGCC=1}/Darwin/&&actualGCC{print "-Wl,-stack_size -Wl,0x5000000"}')
CC=$(GCC) $(OPT) $(DEBUG) -Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wshadow $(PG) $(STACKSIZE)

default: gcc-ver
	# Make the same thing we made most recently
	if ls -rtlL *.a | tail -1 | grep .-g; then $(MAKE) debug; else $(MAKE) opt; fi

gcc-ver:
	$(GCC) -v

all:
	/bin/rm -f *.a
	$(MAKE) tests
	touch made

libwayne_all:
	# Make the pg versions (for profiling)
	$(MAKE) debug_clean
	$(MAKE) -j$(CORES) 'PG=-pg' debug
	$(MAKE) opt_clean
	$(MAKE) -j$(CORES) 'PG=-pg' opt
	for i in *.a; do b=`basename $$i .a`; mv $$i $$b-pg.a; done
	# Make the non-pg versions (for profiling)
	$(MAKE) debug_clean
	$(MAKE) -j$(CORES) debug
	$(MAKE) opt_clean
	$(MAKE) -j$(CORES) opt

tests: libwayne_all
	for x in ebm covar stats hashtest htree-test bintree-test CI graph-sanity; do (cd tests; $(MAKE) $$x; mv $$x ../bin; [ -f $$x.in ] && cat $$x.in | ../bin/$$x $$x.in | if [ -f $$x.out ]; then cmp - $$x.out; else wc; fi); done

opt:
	$(MAKE) 'OPT=-O2' 'LIBOUT=libwayne.a' libwayne

debug:
	$(MAKE) 'DEBUG=-ggdb' 'LIBOUT=libwayne-g.a' libwayne

libwayne:
	$(MAKE) $(LIBOUT)
	[ "$(ARCH)" = Darwin ] || ar r $(LIBOUT)

debug_clean:
	@$(MAKE) 'DEBUG=-ggdb' 'LIBOUT=libwayne-g.a' raw_clean

opt_clean:
	@$(MAKE) 'OPT=-O2' 'LIBOUT=libwayne.a' raw_clean

raw_clean:
	@/bin/rm -f src/*.[oa] $(LIBOUT) made
	@cd MT19937; $(MAKE) clean

clean:
	@# The following is meant to remove the non-Windows binary, ie stats but not stats.exe.
	@/bin/rm -f bin/stats bin/hashtest
	@/bin/rm -f *.a
	@$(MAKE) debug_clean
	@$(MAKE) opt_clean
	@/bin/rm -f made

$(LIBOUT): src/$(LIBOUT)
	ranlib src/$(LIBOUT)
	mv src/$(LIBOUT) .

src/$(LIBOUT):
	cd src; $(MAKE) 'CC=$(CC)' 'LIBOUT=$(LIBOUT)' 'DEBUG=$(DEBUG)'
