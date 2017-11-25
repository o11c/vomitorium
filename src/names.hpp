#pragma once

#include "internal.hpp"

extern const char *ts_enum_names[];
extern const char *rid_names[];
extern const char *cti_names[];
extern const char *ti_names[];
extern const char *itk_names[];
extern const char *stk_names[];
extern const char *cpti_names[];
extern const char *cilk_ti_names[];

extern const char *omp_clause_schedule_names[];
extern const char *omp_clause_default_names[];
extern const char *omp_clause_depend_names[];
extern const char *omp_clause_map_names[];
extern const char *gomp_map_names[];
extern const char *omp_clause_proc_bind_names[];
extern const char *omp_clause_linear_names[];

// provided since GCC 5, so our declaration has to match
extern const char *const tls_model_names[];

extern const char *symbol_visibility_names[];
extern const char *cpp_node_type_names[];
extern const char *cpp_builtin_type_names[];
