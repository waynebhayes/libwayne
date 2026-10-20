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

.PHONY: default FORCE
