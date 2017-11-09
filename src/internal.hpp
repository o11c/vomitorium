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

#if 1
# ifndef IN_GCC
#  define IN_GCC
# endif
# include "config.h"
# ifndef ENABLE_TREE_CHECKING
#  define ENABLE_TREE_CHECKING
# endif
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
#include <langhooks.h>

#include <tm.h>

// also needs to be defined whenever you use LANG_IDENTIFIER_CAST
#define lang_identifier cxx_lang_identifier
#include <cp/cp-tree.h>
#undef lang_identifier

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
#include <predict.h>
#include <tree-flow.h>

// Internal to c-decl.c, but included in tree_size() so we need to dump it.
// It hasn't changed since 4.5 so we're probably good.
struct GTY((chain_next ("%h.prev"))) c_binding {
  union GTY(()) {              /* first so GTY desc can use decl */
    tree GTY((tag ("0"))) type; /* the type in this scope */
    struct c_label_vars * GTY((tag ("1"))) label; /* for warnings */
  } GTY((desc ("TREE_CODE (%0.decl) == LABEL_DECL"))) u;
  tree decl;                   /* the decl bound */
  tree id;                     /* the identifier it's bound to */
  struct c_binding *prev;      /* the previous decl in this scope */
  struct c_binding *shadowed;  /* the innermost decl shadowed by this one */
  unsigned int depth : 28;      /* depth of this scope */
  BOOL_BITFIELD invisible : 1;  /* normal lookup should ignore this binding */
  BOOL_BITFIELD nested : 1;     /* do not set DECL_CONTEXT when popping */
  BOOL_BITFIELD inner_comp : 1; /* incomplete array completed in inner scope */
  BOOL_BITFIELD in_struct : 1; /* currently defined as struct field */
  location_t locus;            /* location for nested bindings */
};
#define lang_identifier c_lang_identifier
struct GTY(()) lang_identifier {
  struct c_common_identifier common_id;
  struct c_binding *symbol_binding; /* vars, funcs, constants, typedefs */
  struct c_binding *tag_binding;    /* struct/union/enum tags */
  struct c_binding *label_binding;  /* labels */
};
#undef lang_identifier

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

// Note: it would be a *really* good idea to require GCC 4.7,
// so that we can use `template using` to prettify the old VEC stuff.
//
// This requires passing GCCPLUGIN_VERSION from the Makefile, since we
// don't know how to safely include headers without knowing it first.
// (This is useful for old versions that don't define it anyway).

void debug_events();
void enable_dump();
void enable_dump_v1();
