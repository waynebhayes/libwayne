# The real libwayne build lives in build/Makefile. This file only hands every
# target to it, so 'make', 'make all', 'make clean', 'make libwayne-g', etc.
# keep working from here (other projects such as BLANT rely on that).

default:
	@$(MAKE) -C build

# Without this, make would try to rebuild this Makefile itself via the rule below.
Makefile: ;

%: FORCE
	@$(MAKE) -C build $@

FORCE:

# Hand over one target at a time, so 'make -j4 clean all' cannot run clean during
# all. Each target still builds in parallel inside build/.
.NOTPARALLEL:

.PHONY: default FORCE
