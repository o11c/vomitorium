#pragma once

#include "vomitorium.h"

#include "compat.hpp"

#if 1
# ifndef IN_GCC
#  define IN_GCC
# endif
# include "config.h"
// for weak-check.cpp
# ifndef ENABLE_TREE_CHECKING
#  define ENABLE_TREE_CHECKING
# endif
#endif

// Just for convenience, since anyone including us probably needs it.
#include "vgcc/plugin-version.h"

// Note: it would be a *really* good idea to require GCC 4.7,
// so that we can use `template using` to prettify the old VEC stuff.
//
// This requires passing GCCPLUGIN_VERSION from the Makefile, since we
// don't know how to safely include headers without knowing it first.
// (This is useful for old versions that don't define it anyway).

void debug_events();
void enable_dump();
void enable_dump_v1();
