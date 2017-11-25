# This version only builds each version once.

# Configuration - will automatically remove any version you don't have.

GCC_VERSIONS :=
GCC_VERSIONS += 4.5
GCC_VERSIONS += 4.6
GCC_VERSIONS += 4.7
GCC_VERSIONS += 4.8
GCC_VERSIONS += 4.9
GCC_VERSIONS += 5
GCC_VERSIONS += 6
GCC_VERSIONS += 7
GCC_VERSIONS += 8
GCC_VERSIONS += 9 # doesn't exist yet, as of when this is written

override GCC_VERSIONS := $(foreach v,${GCC_VERSIONS}, $(patsubst g++-%,%,$(notdir $(shell which g++-$v))))

space = ${empty} ${empty}
comma = ,
$(info Found GCC versions: $(subst ${space},${comma}${space},$(strip ${GCC_VERSIONS})))

# Generic "forward everything to another makefile" logic

SUBDIRS := $(foreach v,${GCC_VERSIONS},$v-$v)

.DEFAULT_GOAL = .default_goal
.PHONY: ${MAKECMDGOALS}
${MAKECMDGOALS} .default_goal: $(addprefix .forward-all-,${SUBDIRS})
	@:

builds/%/Makefile:
	mkdir -p ${@D}
	@rm -f $@
	@echo DEFAULT_CC=gcc-$(word 2,$(subst -,${space},$*)) >> $@
	@echo DEFAULT_CXX=g++-$(word 2,$(subst -,${space},$*)) >> $@
	@echo PLUGIN_CC=gcc-$(word 1,$(subst -,${space},$*)) >> $@
	echo include ../../make/one.make >> $@

.forward-all-%: builds/%/Makefile
	${MAKE} -R -r -C builds/$* ${MAKECMDGOALS}

${MAKEFILE_LIST}:
	@:
.SUFFIXES:
.SECONDARY:
.DELETE_ON_ERROR:
