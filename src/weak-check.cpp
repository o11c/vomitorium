#include "internal.hpp"

// Used only if ENABLE_TREE_CHECKING was *not* originally enabled.
// TODO copy the originals out of GCC source

#pragma GCC visibility push(default)
__attribute__((weak))
void tree_contains_struct_check_failed(const_tree, const enum tree_node_structure_enum, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void tree_check_failed(const_tree, const char *, int, const char *, ...)
{
    abort();
}

__attribute__((weak))
void tree_not_check_failed(const_tree, const char *, int, const char *, ...)
{
    abort();
}

__attribute__((weak))
void tree_class_check_failed(const_tree, const enum tree_code_class, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void tree_range_check_failed(const_tree, const char *, int, const char *, enum tree_code, enum tree_code)
{
    abort();
}

__attribute__((weak))
void tree_not_class_check_failed(const_tree, const enum tree_code_class, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void tree_vec_elt_check_failed(int, int, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void phi_node_elt_check_failed(int, int, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void tree_operand_check_failed(int, const_tree, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void omp_clause_check_failed(const_tree, const char *, int, const char *, enum omp_clause_code)
{
    abort();
}

__attribute__((weak))
void omp_clause_operand_check_failed(int, const_tree, const char *, int, const char *)
{
    abort();
}

__attribute__((weak))
void omp_clause_range_check_failed(const_tree, const char *, int, const char *, enum omp_clause_code, enum omp_clause_code)
{
    abort();
}
#pragma GCC visibility pop
