// DO NOT INCLUDE ANY GCC HEADERS

#include <sys/mman.h>


// smallest unit that mmap/mprotect operate on
#define PAGESIZE 4096

__attribute__((aligned(PAGESIZE), section("weak_blackhole")))
char initial_padding[PAGESIZE];

#define JOIN(a, b) a##b

// define 2 symbols, so that one is always available to ease debugging
// (the same reason they aren't all aliases of the same padding)
#define WEAK(name) __attribute__ ((section("weak_blackhole"))) long long JOIN(name, _weak_blackhole); __attribute__ ((weak, alias(#name "_weak_blackhole"))) extern long long name

#pragma GCC visibility push(default)
// Define various weak symbols
WEAK(c_global_trees);
WEAK(c_language);
WEAK(cp_global_trees);
WEAK(deferred_mark_used_calls);
WEAK(global_namespace);
WEAK(global_scope_name);
WEAK(global_type_node);
WEAK(integer_three_node);
WEAK(integer_two_node);
WEAK(keyed_classes);
WEAK(local_classes);
WEAK(pragma_extern_prefix);
WEAK(registered_builtin_types);
WEAK(ridpointers);
WEAK(static_aggregates);
WEAK(static_decls);
WEAK(tls_aggregates);
WEAK(unemitted_tinfo_decls);
#pragma GCC visibility pop

__attribute__((aligned(PAGESIZE), section("weak_blackhole")))
char final_padding[64*1024];

__attribute__((constructor))
static void make_unreadable()
{
    char *start = initial_padding;
    char *end = final_padding + sizeof(final_padding);

    // accessing a symbol from the wrong frontend should cause a SEGFAULT
    mprotect(start, end - start, PROT_NONE);
}
