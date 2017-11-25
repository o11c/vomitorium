make := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
. := $(patsubst %/,%,$(dir ${make}))
src := $(patsubst ./%,%,$./src)
include := $(patsubst ./%,%,$./include)
scripts := $(patsubst ./%,%,$./scripts)
#obj = obj
#lib = lib
#bin = bin
#gen = gen
#gen_src = gen/src
#gen_include = gen/include
#gen_make = gen/make

.DEFAULT_GOAL = default

include $(sort $(wildcard ${make}/[0-9]*.make))
