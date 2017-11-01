#pragma once

#include "vomitorium.h"

#include <cstdlib>

#define MY_GCC_VERSION (1000 * __GNUC__ + __GNUC_MINOR__)

////////////////////////////////////////////////////////////
// IMPORTANT
// This must be the only place that any GCC headers are
// included, or weird shit will happen.
// (Exception: including for macros only.)
////////////////////////////////////////////////////////////
#pragma GCC visibility push(default)
#if MY_GCC_VERSION < 4007
#include "gmp.h"
extern "C"
{
#endif

#include <gcc-plugin.h>
#include <plugin-version.h>

#if MY_GCC_VERSION != GCC_VERSION
# error "???"
#endif

// Yes, this needs to go in the middle of the #include block
#ifndef GCCPLUGIN_VERSION
# if GCC_VERSION >= 4007
#  error "Plugin version not defined, and obviously built with newer GCC."
# endif
# define GCCPLUGIN_VERSION GCC_VERSION
#endif

#undef abort
// safe-ctype tries to kill these, but libstdc++ needs them
#undef isalpha
#undef isalnum
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
#undef toupper
#undef tolower

#include <tree.h>

#include <tm.h>
#include <cp/cp-tree.h>
#if GCCPLUGIN_VERSION < 4006
# include <c-common.h>
# include <c-pragma.h>
#else
# include <c-family/c-common.h>
# include <c-family/c-pragma.h>
#endif
#if GCCPLUGIN_VERSION >= 4009
# include <internal-fn.h>
#endif
#include <rtl.h>
#if GCCPLUGIN_VERSION >= 5000
# include <cilk.h>
# include <expr.h>
# if GCCPLUGIN_VERSION < 7000
#  include <omp-low.h>
# else
#  include <omp-offload.h>
# endif
# include <gimple-expr.h>
# include <tree-chrec.h>
#endif
#if GCCPLUGIN_VERSION >= 6000
# include <vtable-verify.h>
#endif
#include <langhooks.h>


#if MY_GCC_VERSION < 4007
}
#endif
#pragma GCC visibility pop
////////////////////////////////////////////////////////////
// End of GCC includes.
////////////////////////////////////////////////////////////


__attribute__((unused))
static void _unused_gcc_statics()
{
    (void)gcc_version;
}

#if GCCPLUGIN_VERSION >= 4008
# define VEC(T, A) vec<T, va_##A>
#endif

#if GCCPLUGIN_VERSION != GCC_VERSION
# if GCCPLUGIN_VERSION < 4008
#  error "Plugins must be built with the same GCC they are run with."
# else
#  warning "Plugins should be built with the same GCC they are run with."
# endif
#endif

#if GCC_VERSION < 4006
# define nullptr NULL
#endif

#if GCC_VERSION < 4007
# define override /*nothing*/
#endif

void debug_events();
void enable_dump();
void enable_dump_v1();
