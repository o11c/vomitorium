#include "names.hpp"

#include <cassert>

#include "internal.hpp"


const char *rid_names[RID_MAX];
const char *cti_names[CTI_MAX];
const char *ti_names[TI_MAX];
const char *itk_names[itk_none];
#if GCCPLUGIN_VERSION >= 4008
# define TYPE_KIND_LAST stk_type_kind_last
#endif
const char *stk_names[TYPE_KIND_LAST];
const char *cpti_names[CPTI_MAX];
#if GCCPLUGIN_VERSION >= 5000
const char *cilk_ti_names[CILK_TI_MAX];
#endif

// no _MAX element for these!
// guess; the assert will probably catch it
const char *omp_clause_schedule_names[5];
const char *omp_clause_default_names[5];

const char *const tls_model_names[] =
{
    "none",
    "emulated",
    "global_dynamic", // aka "real",
    "local_dynamic",
    "initial_exec",
    "local_exec",
};

const char *symbol_visibility_names[4];
const char *cpp_node_type_names[3];
const char *cpp_builtin_type_names[
    10
#if GCCPLUGIN_VERSION >= 5000
    + 1 // BT_HAS_ATTRIBUTE
#endif
#if GCCPLUGIN_VERSION >= 4006
    + (BT_LAST_USER+1 - BT_FIRST_USER)
#endif
];


// can't use decltype in an expression until 4.7
template<size_t n>
struct array_size_helper_class
{
    char sizer[n];
};
template<class T, size_t n>
array_size_helper_class<n> array_sizeof_helper(T (&)[n]);
// constexpr even in gcc 4.5 which doesn't support the keyword
#define ARRAY_SIZEOF(a) (sizeof(array_sizeof_helper(a)))
// Boo, no designated-initializers in C++.
#define NAME(a, x) do { assert (x >= 0 && x < ARRAY_SIZEOF(a)); assert(a[x] == nullptr); a[x] = #x; } while (0)
#define CHECK(a) check_array(#a, a)

__attribute__((unused))
static int constexpr_check[ARRAY_SIZEOF(rid_names)];

template<class T, size_t N>
static void check_array(const char *name, T (&arr)[N])
{
    for (size_t i = 0; i < N; ++i)
    {
        if (arr[i] == nullptr)
        {
            printf("Warning: incomplete array %s\n", name);
            printf("         missing element #%zu\n", i);
            if (i)
                printf("         previous was %s\n", arr[i-1]);
            return;
        }
    }
}

__attribute__((constructor))
static void check()
{
    // from <c-family/c-common.h>
    NAME(rid_names, RID_ACCUM);
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_ADDRESSOF);
#endif
    NAME(rid_names, RID_ADDR_SPACE_0);
    NAME(rid_names, RID_ADDR_SPACE_1);
    NAME(rid_names, RID_ADDR_SPACE_10);
    NAME(rid_names, RID_ADDR_SPACE_11);
    NAME(rid_names, RID_ADDR_SPACE_12);
    NAME(rid_names, RID_ADDR_SPACE_13);
    NAME(rid_names, RID_ADDR_SPACE_14);
    NAME(rid_names, RID_ADDR_SPACE_15);
    NAME(rid_names, RID_ADDR_SPACE_2);
    NAME(rid_names, RID_ADDR_SPACE_3);
    NAME(rid_names, RID_ADDR_SPACE_4);
    NAME(rid_names, RID_ADDR_SPACE_5);
    NAME(rid_names, RID_ADDR_SPACE_6);
    NAME(rid_names, RID_ADDR_SPACE_7);
    NAME(rid_names, RID_ADDR_SPACE_8);
    NAME(rid_names, RID_ADDR_SPACE_9);
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_ALIGNAS);
#endif
    NAME(rid_names, RID_ALIGNOF);
    NAME(rid_names, RID_ASM);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_ASSIGN);
#endif
#if GCCPLUGIN_VERSION >= 4009
    NAME(rid_names, RID_ATOMIC);
#endif
#if GCCPLUGIN_VERSION >= 6000
    NAME(rid_names, RID_ATOMIC_CANCEL);
    NAME(rid_names, RID_ATOMIC_NOEXCEPT);
#endif
    NAME(rid_names, RID_ATTRIBUTE);
    NAME(rid_names, RID_AT_ALIAS);
    NAME(rid_names, RID_AT_CATCH);
    NAME(rid_names, RID_AT_CLASS);
    NAME(rid_names, RID_AT_DEFS);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_AT_DYNAMIC);
#endif
    NAME(rid_names, RID_AT_ENCODE);
    NAME(rid_names, RID_AT_END);
    NAME(rid_names, RID_AT_FINALLY);
    NAME(rid_names, RID_AT_IMPLEMENTATION);
    NAME(rid_names, RID_AT_INTERFACE);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_AT_OPTIONAL);
    NAME(rid_names, RID_AT_PACKAGE);
#endif
    NAME(rid_names, RID_AT_PRIVATE);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_AT_PROPERTY);
#endif
    NAME(rid_names, RID_AT_PROTECTED);
    NAME(rid_names, RID_AT_PROTOCOL);
    NAME(rid_names, RID_AT_PUBLIC);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_AT_REQUIRED);
#endif
    NAME(rid_names, RID_AT_SELECTOR);
    NAME(rid_names, RID_AT_SYNCHRONIZED);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_AT_SYNTHESIZE);
#endif
    NAME(rid_names, RID_AT_THROW);
    NAME(rid_names, RID_AT_TRY);
    NAME(rid_names, RID_AUTO);
#if GCCPLUGIN_VERSION >= 4009
    NAME(rid_names, RID_AUTO_TYPE);
#endif
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_BASES);
#endif
    NAME(rid_names, RID_BOOL);
    NAME(rid_names, RID_BREAK);
#if GCCPLUGIN_VERSION >= 5000
    NAME(rid_names, RID_BUILTIN_CALL_WITH_STATIC_CHAIN);
#endif
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_BUILTIN_COMPLEX);
#endif
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_BUILTIN_LAUNDER);
#endif
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_BUILTIN_SHUFFLE);
#endif
    NAME(rid_names, RID_BYCOPY);
    NAME(rid_names, RID_BYREF);
    NAME(rid_names, RID_C99_FUNCTION_NAME);
    NAME(rid_names, RID_CASE);
    NAME(rid_names, RID_CATCH);
    NAME(rid_names, RID_CHAR);
    NAME(rid_names, RID_CHAR16);
    NAME(rid_names, RID_CHAR32);
    NAME(rid_names, RID_CHOOSE_EXPR);
#if GCCPLUGIN_VERSION >= 5000
    NAME(rid_names, RID_CILK_FOR);
#endif
#if GCCPLUGIN_VERSION >= 4009
    NAME(rid_names, RID_CILK_SPAWN);
    NAME(rid_names, RID_CILK_SYNC);
#endif
    NAME(rid_names, RID_CLASS);
    NAME(rid_names, RID_COMPLEX);
#if GCCPLUGIN_VERSION >= 6000
    NAME(rid_names, RID_CONCEPT);
#endif
    NAME(rid_names, RID_CONST);
    NAME(rid_names, RID_CONSTCAST);
    NAME(rid_names, RID_CONSTEXPR);
    NAME(rid_names, RID_CONTINUE);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_COPY);
#endif
    NAME(rid_names, RID_CXX_COMPAT_WARN);
    NAME(rid_names, RID_DECLTYPE);
    NAME(rid_names, RID_DEFAULT);
    NAME(rid_names, RID_DELETE);
    NAME(rid_names, RID_DFLOAT128);
    NAME(rid_names, RID_DFLOAT32);
    NAME(rid_names, RID_DFLOAT64);
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_DIRECT_BASES);
#endif
    NAME(rid_names, RID_DO);
    NAME(rid_names, RID_DOUBLE);
    NAME(rid_names, RID_DYNCAST);
    NAME(rid_names, RID_ELSE);
    NAME(rid_names, RID_ENUM);
    NAME(rid_names, RID_EXPLICIT);
    NAME(rid_names, RID_EXPORT);
    NAME(rid_names, RID_EXTENSION);
    NAME(rid_names, RID_EXTERN);
    NAME(rid_names, RID_FALSE);
    NAME(rid_names, RID_FLOAT);
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_FLOAT16);
    NAME(rid_names, RID_FLOAT32);
    NAME(rid_names, RID_FLOAT64);
    NAME(rid_names, RID_FLOAT128);
    NAME(rid_names, RID_FLOAT32X);
    NAME(rid_names, RID_FLOAT64X);
    NAME(rid_names, RID_FLOAT128X);
#endif
    NAME(rid_names, RID_FOR);
    NAME(rid_names, RID_FRACT);
    NAME(rid_names, RID_FRIEND);
    NAME(rid_names, RID_FUNCTION_NAME);
#if GCCPLUGIN_VERSION >= 4009
    NAME(rid_names, RID_GENERIC);
#endif
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_GETTER);
#endif
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_GIMPLE);
#endif
    NAME(rid_names, RID_GOTO);
    NAME(rid_names, RID_HAS_NOTHROW_ASSIGN);
    NAME(rid_names, RID_HAS_NOTHROW_CONSTRUCTOR);
    NAME(rid_names, RID_HAS_NOTHROW_COPY);
    NAME(rid_names, RID_HAS_TRIVIAL_ASSIGN);
    NAME(rid_names, RID_HAS_TRIVIAL_CONSTRUCTOR);
    NAME(rid_names, RID_HAS_TRIVIAL_COPY);
    NAME(rid_names, RID_HAS_TRIVIAL_DESTRUCTOR);
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_HAS_UNIQUE_OBJ_REPRESENTATIONS);
#endif
    NAME(rid_names, RID_HAS_VIRTUAL_DESTRUCTOR);
    NAME(rid_names, RID_IF);
    NAME(rid_names, RID_IMAGINARY);
    NAME(rid_names, RID_IMAGPART);
    NAME(rid_names, RID_IN);
    NAME(rid_names, RID_INLINE);
    NAME(rid_names, RID_INOUT);
    NAME(rid_names, RID_INT);
#if GCCPLUGIN_VERSION >= 4006 && GCCPLUGIN_VERSION < 5000
    NAME(rid_names, RID_INT128);
#endif
#if GCCPLUGIN_VERSION >= 5000
    NAME(rid_names, RID_INT_N_0);
    NAME(rid_names, RID_INT_N_1);
    NAME(rid_names, RID_INT_N_2);
    NAME(rid_names, RID_INT_N_3);
#endif
    NAME(rid_names, RID_IS_ABSTRACT);
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_IS_AGGREGATE);
#endif
#if GCCPLUGIN_VERSION >= 8000
    NAME(rid_names, RID_IS_ASSIGNABLE);
#endif
    NAME(rid_names, RID_IS_BASE_OF);
    NAME(rid_names, RID_IS_CLASS);
#if GCCPLUGIN_VERSION >= 8000
    NAME(rid_names, RID_IS_CONSTRUCTIBLE);
#endif
#if GCCPLUGIN_VERSION < 5000
    NAME(rid_names, RID_IS_CONVERTIBLE_TO);
#endif
    NAME(rid_names, RID_IS_EMPTY);
    NAME(rid_names, RID_IS_ENUM);
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_IS_FINAL);
#endif
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_IS_LITERAL_TYPE);
#endif
    NAME(rid_names, RID_IS_POD);
    NAME(rid_names, RID_IS_POLYMORPHIC);
#if GCCPLUGIN_VERSION >= 6000
    NAME(rid_names, RID_IS_SAME_AS);
#endif
    NAME(rid_names, RID_IS_STD_LAYOUT);
    NAME(rid_names, RID_IS_TRIVIAL);
#if GCCPLUGIN_VERSION >= 5000
    NAME(rid_names, RID_IS_TRIVIALLY_ASSIGNABLE);
    NAME(rid_names, RID_IS_TRIVIALLY_CONSTRUCTIBLE);
    NAME(rid_names, RID_IS_TRIVIALLY_COPYABLE);
#endif
    NAME(rid_names, RID_IS_UNION);
    NAME(rid_names, RID_LABEL);
    NAME(rid_names, RID_LONG);
    NAME(rid_names, RID_MUTABLE);
    NAME(rid_names, RID_NAMESPACE);
    NAME(rid_names, RID_NEW);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_NOEXCEPT);
    NAME(rid_names, RID_NONATOMIC);
#endif
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_NORETURN);
#endif
    NAME(rid_names, RID_NULL);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_NULLPTR);
#endif
    NAME(rid_names, RID_OFFSETOF);
    NAME(rid_names, RID_ONEWAY);
    NAME(rid_names, RID_OPERATOR);
    NAME(rid_names, RID_OUT);
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_PHI);
#endif
    NAME(rid_names, RID_PRETTY_FUNCTION_NAME);
    NAME(rid_names, RID_PRIVATE);
    NAME(rid_names, RID_PROTECTED);
    NAME(rid_names, RID_PUBLIC);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_READONLY);
    NAME(rid_names, RID_READWRITE);
#endif
    NAME(rid_names, RID_REALPART);
    NAME(rid_names, RID_REGISTER);
    NAME(rid_names, RID_REINTCAST);
#if GCCPLUGIN_VERSION >= 6000
    NAME(rid_names, RID_REQUIRES);
#endif
    NAME(rid_names, RID_RESTRICT);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_RETAIN);
#endif
    NAME(rid_names, RID_RETURN);
#if GCCPLUGIN_VERSION >= 7000
    NAME(rid_names, RID_RTL);
#endif
    NAME(rid_names, RID_SAT);
#if GCCPLUGIN_VERSION >= 4006
    NAME(rid_names, RID_SETTER);
#endif
    NAME(rid_names, RID_SHORT);
    NAME(rid_names, RID_SIGNED);
    NAME(rid_names, RID_SIZEOF);
    NAME(rid_names, RID_STATCAST);
    NAME(rid_names, RID_STATIC);
    NAME(rid_names, RID_STATIC_ASSERT);
    NAME(rid_names, RID_STRUCT);
    NAME(rid_names, RID_SWITCH);
#if GCCPLUGIN_VERSION >= 6000
    NAME(rid_names, RID_SYNCHRONIZED);
#endif
    NAME(rid_names, RID_TEMPLATE);
    NAME(rid_names, RID_THIS);
    NAME(rid_names, RID_THREAD);
    NAME(rid_names, RID_THROW);
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_TRANSACTION_ATOMIC);
    NAME(rid_names, RID_TRANSACTION_CANCEL);
    NAME(rid_names, RID_TRANSACTION_RELAXED);
#endif
    NAME(rid_names, RID_TRUE);
    NAME(rid_names, RID_TRY);
    NAME(rid_names, RID_TYPEDEF);
    NAME(rid_names, RID_TYPEID);
    NAME(rid_names, RID_TYPENAME);
    NAME(rid_names, RID_TYPEOF);
    NAME(rid_names, RID_TYPES_COMPATIBLE_P);
#if GCCPLUGIN_VERSION >= 4007
    NAME(rid_names, RID_UNDERLYING_TYPE);
#endif
    NAME(rid_names, RID_UNION);
    NAME(rid_names, RID_UNSIGNED);
    NAME(rid_names, RID_USING);
    NAME(rid_names, RID_VA_ARG);
    NAME(rid_names, RID_VIRTUAL);
    NAME(rid_names, RID_VOID);
    NAME(rid_names, RID_VOLATILE);
    NAME(rid_names, RID_WCHAR);
    NAME(rid_names, RID_WHILE);
    CHECK(rid_names);

    // from <c-family/c-common.h>
    NAME(cti_names, CTI_C99_FUNCTION_NAME_DECL);
    NAME(cti_names, CTI_CHAR16_ARRAY_TYPE);
    NAME(cti_names, CTI_CHAR16_TYPE);
    NAME(cti_names, CTI_CHAR32_ARRAY_TYPE);
    NAME(cti_names, CTI_CHAR32_TYPE);
    NAME(cti_names, CTI_CHAR_ARRAY_TYPE);
    NAME(cti_names, CTI_CONST_STRING_TYPE);
    NAME(cti_names, CTI_DEFAULT_FUNCTION_TYPE);
    NAME(cti_names, CTI_FUNCTION_NAME_DECL);
    NAME(cti_names, CTI_INT16_TYPE);
    NAME(cti_names, CTI_INT32_TYPE);
    NAME(cti_names, CTI_INT64_TYPE);
    NAME(cti_names, CTI_INT8_TYPE);
    NAME(cti_names, CTI_INTMAX_TYPE);
    NAME(cti_names, CTI_INTPTR_TYPE);
#if GCCPLUGIN_VERSION < 4009
    NAME(cti_names, CTI_INT_ARRAY_TYPE);
#endif
    NAME(cti_names, CTI_INT_FAST16_TYPE);
    NAME(cti_names, CTI_INT_FAST32_TYPE);
    NAME(cti_names, CTI_INT_FAST64_TYPE);
    NAME(cti_names, CTI_INT_FAST8_TYPE);
    NAME(cti_names, CTI_INT_LEAST16_TYPE);
    NAME(cti_names, CTI_INT_LEAST32_TYPE);
    NAME(cti_names, CTI_INT_LEAST64_TYPE);
    NAME(cti_names, CTI_INT_LEAST8_TYPE);
    NAME(cti_names, CTI_NULL);
    NAME(cti_names, CTI_PRETTY_FUNCTION_NAME_DECL);
    NAME(cti_names, CTI_SAVED_FUNCTION_NAME_DECLS);
    NAME(cti_names, CTI_SIGNED_SIZE_TYPE);
    NAME(cti_names, CTI_SIG_ATOMIC_TYPE);
    NAME(cti_names, CTI_STRING_TYPE);
    NAME(cti_names, CTI_TRUTHVALUE_FALSE);
    NAME(cti_names, CTI_TRUTHVALUE_TRUE);
    NAME(cti_names, CTI_TRUTHVALUE_TYPE);
    NAME(cti_names, CTI_UINT16_TYPE);
    NAME(cti_names, CTI_UINT32_TYPE);
    NAME(cti_names, CTI_UINT64_TYPE);
    NAME(cti_names, CTI_UINT8_TYPE);
    NAME(cti_names, CTI_UINTMAX_TYPE);
    NAME(cti_names, CTI_UINTPTR_TYPE);
    NAME(cti_names, CTI_UINT_FAST16_TYPE);
    NAME(cti_names, CTI_UINT_FAST32_TYPE);
    NAME(cti_names, CTI_UINT_FAST64_TYPE);
    NAME(cti_names, CTI_UINT_FAST8_TYPE);
    NAME(cti_names, CTI_UINT_LEAST16_TYPE);
    NAME(cti_names, CTI_UINT_LEAST32_TYPE);
    NAME(cti_names, CTI_UINT_LEAST64_TYPE);
    NAME(cti_names, CTI_UINT_LEAST8_TYPE);
    NAME(cti_names, CTI_UNDERLYING_WCHAR_TYPE);
    NAME(cti_names, CTI_UNSIGNED_PTRDIFF_TYPE);
#if GCCPLUGIN_VERSION < 5000
    NAME(cti_names, CTI_VOID_ZERO);
#endif
    NAME(cti_names, CTI_WCHAR_ARRAY_TYPE);
    NAME(cti_names, CTI_WCHAR_TYPE);
    NAME(cti_names, CTI_WIDEST_INT_LIT_TYPE);
    NAME(cti_names, CTI_WIDEST_UINT_LIT_TYPE);
    NAME(cti_names, CTI_WINT_TYPE);
    CHECK(cti_names);

    // from <tree-core.h>
    NAME(ti_names, TI_ACCUM_TYPE);
#if GCCPLUGIN_VERSION >= 4009
    NAME(ti_names, TI_ATOMICDI_TYPE);
    NAME(ti_names, TI_ATOMICHI_TYPE);
    NAME(ti_names, TI_ATOMICQI_TYPE);
    NAME(ti_names, TI_ATOMICSI_TYPE);
    NAME(ti_names, TI_ATOMICTI_TYPE);
#endif
    NAME(ti_names, TI_BITSIZE_ONE);
    NAME(ti_names, TI_BITSIZE_UNIT);
    NAME(ti_names, TI_BITSIZE_ZERO);
    NAME(ti_names, TI_BOOLEAN_FALSE);
    NAME(ti_names, TI_BOOLEAN_TRUE);
    NAME(ti_names, TI_BOOLEAN_TYPE);
    NAME(ti_names, TI_COMPLEX_DOUBLE_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT_TYPE);
#if GCCPLUGIN_VERSION >= 7000
    NAME(ti_names, TI_COMPLEX_FLOAT16_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT32_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT64_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT128_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT32X_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT64X_TYPE);
    NAME(ti_names, TI_COMPLEX_FLOAT128X_TYPE);
#endif
    NAME(ti_names, TI_COMPLEX_INTEGER_TYPE);
    NAME(ti_names, TI_COMPLEX_LONG_DOUBLE_TYPE);
#if GCCPLUGIN_VERSION >= 8000
    NAME(ti_names, TI_CONST_FENV_T_PTR_TYPE);
    NAME(ti_names, TI_CONST_FEXCEPT_T_PTR_TYPE);
#endif
    NAME(ti_names, TI_CONST_PTR_TYPE);
#if GCCPLUGIN_VERSION >= 7000
    NAME(ti_names, TI_CONST_TM_PTR_TYPE);
#endif
    NAME(ti_names, TI_CURRENT_OPTIMIZE_PRAGMA);
    NAME(ti_names, TI_CURRENT_TARGET_PRAGMA);
    NAME(ti_names, TI_DA_TYPE);
    NAME(ti_names, TI_DFLOAT128_PTR_TYPE);
    NAME(ti_names, TI_DFLOAT128_TYPE);
    NAME(ti_names, TI_DFLOAT32_PTR_TYPE);
    NAME(ti_names, TI_DFLOAT32_TYPE);
    NAME(ti_names, TI_DFLOAT64_PTR_TYPE);
    NAME(ti_names, TI_DFLOAT64_TYPE);
    NAME(ti_names, TI_DOUBLE_PTR_TYPE);
    NAME(ti_names, TI_DOUBLE_TYPE);
    NAME(ti_names, TI_DQ_TYPE);
    NAME(ti_names, TI_ERROR_MARK);
#if GCCPLUGIN_VERSION >= 8000
    NAME(ti_names, TI_FENV_T_PTR_TYPE);
    NAME(ti_names, TI_FEXCEPT_T_PTR_TYPE);
#endif
    NAME(ti_names, TI_FILEPTR_TYPE);
    NAME(ti_names, TI_FLOAT_PTR_TYPE);
    NAME(ti_names, TI_FLOAT_TYPE);
#if GCCPLUGIN_VERSION >= 7000
    NAME(ti_names, TI_FLOAT16_TYPE);
    NAME(ti_names, TI_FLOAT32_TYPE);
    NAME(ti_names, TI_FLOAT64_TYPE);
    NAME(ti_names, TI_FLOAT128_TYPE);
    NAME(ti_names, TI_FLOAT32X_TYPE);
    NAME(ti_names, TI_FLOAT64X_TYPE);
    NAME(ti_names, TI_FLOAT128X_TYPE);
#endif
    NAME(ti_names, TI_FRACT_TYPE);
    NAME(ti_names, TI_HA_TYPE);
    NAME(ti_names, TI_HQ_TYPE);
    NAME(ti_names, TI_INTDI_TYPE);
    NAME(ti_names, TI_INTEGER_MINUS_ONE);
    NAME(ti_names, TI_INTEGER_ONE);
#if GCCPLUGIN_VERSION >= 4006
    NAME(ti_names, TI_INTEGER_THREE);
#endif
    NAME(ti_names, TI_INTEGER_PTR_TYPE);
    NAME(ti_names, TI_INTEGER_ZERO);
    NAME(ti_names, TI_INTHI_TYPE);
    NAME(ti_names, TI_INTQI_TYPE);
    NAME(ti_names, TI_INTSI_TYPE);
    NAME(ti_names, TI_INTTI_TYPE);
    NAME(ti_names, TI_LACCUM_TYPE);
    NAME(ti_names, TI_LFRACT_TYPE);
    NAME(ti_names, TI_LLACCUM_TYPE);
    NAME(ti_names, TI_LLFRACT_TYPE);
    NAME(ti_names, TI_LONG_DOUBLE_PTR_TYPE);
    NAME(ti_names, TI_LONG_DOUBLE_TYPE);
    NAME(ti_names, TI_MAIN_IDENTIFIER);
    NAME(ti_names, TI_NULL_POINTER);
    NAME(ti_names, TI_OPTIMIZATION_CURRENT);
    NAME(ti_names, TI_OPTIMIZATION_DEFAULT);
    NAME(ti_names, TI_PID_TYPE);
#if GCCPLUGIN_VERSION >= 5000
    NAME(ti_names, TI_POINTER_BOUNDS_TYPE);
#endif
#if GCCPLUGIN_VERSION >= 4009
    NAME(ti_names, TI_POINTER_SIZED_TYPE);
#endif
    NAME(ti_names, TI_PRIVATE);
    NAME(ti_names, TI_PROTECTED);
    NAME(ti_names, TI_PTRDIFF_TYPE);
    NAME(ti_names, TI_PTR_TYPE);
    NAME(ti_names, TI_PUBLIC);
    NAME(ti_names, TI_QQ_TYPE);
    NAME(ti_names, TI_SACCUM_TYPE);
    NAME(ti_names, TI_SAT_ACCUM_TYPE);
    NAME(ti_names, TI_SAT_DA_TYPE);
    NAME(ti_names, TI_SAT_DQ_TYPE);
    NAME(ti_names, TI_SAT_FRACT_TYPE);
    NAME(ti_names, TI_SAT_HA_TYPE);
    NAME(ti_names, TI_SAT_HQ_TYPE);
    NAME(ti_names, TI_SAT_LACCUM_TYPE);
    NAME(ti_names, TI_SAT_LFRACT_TYPE);
    NAME(ti_names, TI_SAT_LLACCUM_TYPE);
    NAME(ti_names, TI_SAT_LLFRACT_TYPE);
    NAME(ti_names, TI_SAT_QQ_TYPE);
    NAME(ti_names, TI_SAT_SACCUM_TYPE);
    NAME(ti_names, TI_SAT_SA_TYPE);
    NAME(ti_names, TI_SAT_SFRACT_TYPE);
    NAME(ti_names, TI_SAT_SQ_TYPE);
    NAME(ti_names, TI_SAT_TA_TYPE);
    NAME(ti_names, TI_SAT_TQ_TYPE);
    NAME(ti_names, TI_SAT_UACCUM_TYPE);
    NAME(ti_names, TI_SAT_UDA_TYPE);
    NAME(ti_names, TI_SAT_UDQ_TYPE);
    NAME(ti_names, TI_SAT_UFRACT_TYPE);
    NAME(ti_names, TI_SAT_UHA_TYPE);
    NAME(ti_names, TI_SAT_UHQ_TYPE);
    NAME(ti_names, TI_SAT_ULACCUM_TYPE);
    NAME(ti_names, TI_SAT_ULFRACT_TYPE);
    NAME(ti_names, TI_SAT_ULLACCUM_TYPE);
    NAME(ti_names, TI_SAT_ULLFRACT_TYPE);
    NAME(ti_names, TI_SAT_UQQ_TYPE);
    NAME(ti_names, TI_SAT_USACCUM_TYPE);
    NAME(ti_names, TI_SAT_USA_TYPE);
    NAME(ti_names, TI_SAT_USFRACT_TYPE);
    NAME(ti_names, TI_SAT_USQ_TYPE);
    NAME(ti_names, TI_SAT_UTA_TYPE);
    NAME(ti_names, TI_SAT_UTQ_TYPE);
    NAME(ti_names, TI_SA_TYPE);
    NAME(ti_names, TI_SFRACT_TYPE);
    NAME(ti_names, TI_SIZE_ONE);
    NAME(ti_names, TI_SIZE_TYPE);
    NAME(ti_names, TI_SIZE_ZERO);
    NAME(ti_names, TI_SQ_TYPE);
    NAME(ti_names, TI_TARGET_OPTION_CURRENT);
    NAME(ti_names, TI_TARGET_OPTION_DEFAULT);
    NAME(ti_names, TI_TA_TYPE);
    NAME(ti_names, TI_TQ_TYPE);
    NAME(ti_names, TI_UACCUM_TYPE);
    NAME(ti_names, TI_UDA_TYPE);
    NAME(ti_names, TI_UDQ_TYPE);
    NAME(ti_names, TI_UFRACT_TYPE);
    NAME(ti_names, TI_UHA_TYPE);
    NAME(ti_names, TI_UHQ_TYPE);
#if GCCPLUGIN_VERSION >= 4008
    NAME(ti_names, TI_UINT16_TYPE);
#endif
    NAME(ti_names, TI_UINT32_TYPE);
    NAME(ti_names, TI_UINT64_TYPE);
    NAME(ti_names, TI_UINTDI_TYPE);
    NAME(ti_names, TI_UINTHI_TYPE);
    NAME(ti_names, TI_UINTQI_TYPE);
    NAME(ti_names, TI_UINTSI_TYPE);
    NAME(ti_names, TI_UINTTI_TYPE);
    NAME(ti_names, TI_ULACCUM_TYPE);
    NAME(ti_names, TI_ULFRACT_TYPE);
    NAME(ti_names, TI_ULLACCUM_TYPE);
    NAME(ti_names, TI_ULLFRACT_TYPE);
    NAME(ti_names, TI_UQQ_TYPE);
    NAME(ti_names, TI_USACCUM_TYPE);
    NAME(ti_names, TI_USA_TYPE);
    NAME(ti_names, TI_USFRACT_TYPE);
    NAME(ti_names, TI_USQ_TYPE);
    NAME(ti_names, TI_UTA_TYPE);
    NAME(ti_names, TI_UTQ_TYPE);
    NAME(ti_names, TI_VA_LIST_FPR_COUNTER_FIELD);
    NAME(ti_names, TI_VA_LIST_GPR_COUNTER_FIELD);
    NAME(ti_names, TI_VA_LIST_TYPE);
    NAME(ti_names, TI_VOID_LIST_NODE);
#if GCCPLUGIN_VERSION >= 5000
    NAME(ti_names, TI_VOID);
#endif
    NAME(ti_names, TI_VOID_TYPE);
    CHECK(ti_names);

    // from <tree.h>
    NAME(itk_names, itk_char);
    NAME(itk_names, itk_signed_char);
    NAME(itk_names, itk_unsigned_char);
    NAME(itk_names, itk_short);
    NAME(itk_names, itk_unsigned_short);
    NAME(itk_names, itk_int);
    NAME(itk_names, itk_unsigned_int);
    NAME(itk_names, itk_long);
    NAME(itk_names, itk_unsigned_long);
    NAME(itk_names, itk_long_long);
    NAME(itk_names, itk_unsigned_long_long);
#if GCCPLUGIN_VERSION >= 4006 && GCCPLUGIN_VERSION < 5000
    NAME(itk_names, itk_int128);
    NAME(itk_names, itk_unsigned_int128);
#endif
#if GCCPLUGIN_VERSION >= 5000
    NAME(itk_names, itk_intN_0);
    NAME(itk_names, itk_unsigned_intN_0);
    NAME(itk_names, itk_intN_1);
    NAME(itk_names, itk_unsigned_intN_1);
    NAME(itk_names, itk_intN_2);
    NAME(itk_names, itk_unsigned_intN_2);
    NAME(itk_names, itk_intN_3);
    NAME(itk_names, itk_unsigned_intN_3);
#endif
    CHECK(itk_names);

    // from <tree.h>
#if GCCPLUGIN_VERSION < 4008
    NAME(stk_names, BITSIZETYPE);
    NAME(stk_names, SBITSIZETYPE);
    NAME(stk_names, SIZETYPE);
    NAME(stk_names, SSIZETYPE);
#else
    NAME(stk_names, stk_bitsizetype);
    NAME(stk_names, stk_sbitsizetype);
    NAME(stk_names, stk_sizetype);
    NAME(stk_names, stk_ssizetype);
#endif
    CHECK(stk_names);

    // from <cp/cp-tree.h>
    NAME(cpti_names, CPTI_ABI);
    NAME(cpti_names, CPTI_ABORT_FNDECL);
    NAME(cpti_names, CPTI_AGGR_TAG);
#if GCCPLUGIN_VERSION >= 7000
    NAME(cpti_names, CPTI_ALIGN_TYPE);
#endif
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_ALLOCATE_EXCEPTION_FN);
    NAME(cpti_names, CPTI_ANON_IDENTIFIER);
#endif
#if GCCPLUGIN_VERSION >= 7000
    NAME(cpti_names, CPTI_ANY_TARG);
#endif
    NAME(cpti_names, CPTI_ATEXIT);
    NAME(cpti_names, CPTI_ATEXIT_FN_PTR_TYPE);
#if GCCPLUGIN_VERSION >= 7000
    NAME(cpti_names, CPTI_AUTO_IDENTIFIER);
#endif
    NAME(cpti_names, CPTI_BASE_CTOR_IDENTIFIER);
    NAME(cpti_names, CPTI_BASE_DTOR_IDENTIFIER);
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_BEGIN_CATCH_FN);
#endif
#if GCCPLUGIN_VERSION < 8000
    NAME(cpti_names, CPTI_CALL_UNEXPECTED);
#endif
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_CALL_UNEXPECTED_FN);
#endif
    NAME(cpti_names, CPTI_CLASS_TYPE);
    NAME(cpti_names, CPTI_CLEANUP_TYPE);
    NAME(cpti_names, CPTI_COMPLETE_CTOR_IDENTIFIER);
    NAME(cpti_names, CPTI_COMPLETE_DTOR_IDENTIFIER);
    NAME(cpti_names, CPTI_CONST_TYPE_INFO_TYPE);
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_CONV_OP_IDENTIFIER);
    NAME(cpti_names, CPTI_CONV_OP_MARKER);
#endif
    NAME(cpti_names, CPTI_CTOR_IDENTIFIER);
    NAME(cpti_names, CPTI_DCAST);
#if GCCPLUGIN_VERSION >= 7000
    NAME(cpti_names, CPTI_DECLTYPE_AUTO_IDENTIFIER);
#endif
    NAME(cpti_names, CPTI_DELETING_DTOR_IDENTIFIER);
    NAME(cpti_names, CPTI_DELTA_IDENTIFIER);
    NAME(cpti_names, CPTI_DELTA_TYPE);
#if GCCPLUGIN_VERSION >= 4007 && GCCPLUGIN_VERSION < 4008
    NAME(cpti_names, CPTI_DEPENDENT_LAMBDA_RETURN_TYPE);
#endif
    NAME(cpti_names, CPTI_DSO_HANDLE);
    NAME(cpti_names, CPTI_DTOR_IDENTIFIER);
    NAME(cpti_names, CPTI_EMPTY_EXCEPT_SPEC);
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_END_CATCH_FN);
    NAME(cpti_names, CPTI_FREE_EXCEPTION_FN);
    NAME(cpti_names, CPTI_GET_EXCEPTION_PTR_FN);
    NAME(cpti_names, CPTI_GLOBAL);
    NAME(cpti_names, CPTI_GLOBAL_IDENTIFIER);
    NAME(cpti_names, CPTI_GLOBAL_TYPE);
#endif
#if GCCPLUGIN_VERSION < 5000
    NAME(cpti_names, CPTI_GLOBAL_DELETE_FNDECL);
#endif
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_INIT_LIST_IDENTIFIER);
#endif
    NAME(cpti_names, CPTI_INIT_LIST_TYPE);
    NAME(cpti_names, CPTI_IN_CHARGE_IDENTIFIER);
#if GCCPLUGIN_VERSION < 7000
    NAME(cpti_names, CPTI_JAVA_BOOLEAN_TYPE);
    NAME(cpti_names, CPTI_JAVA_BYTE_TYPE);
    NAME(cpti_names, CPTI_JAVA_CHAR_TYPE);
    NAME(cpti_names, CPTI_JAVA_DOUBLE_TYPE);
    NAME(cpti_names, CPTI_JAVA_FLOAT_TYPE);
    NAME(cpti_names, CPTI_JAVA_INT_TYPE);
    NAME(cpti_names, CPTI_JAVA_LONG_TYPE);
    NAME(cpti_names, CPTI_JAVA_SHORT_TYPE);
    NAME(cpti_names, CPTI_JCLASS);
#endif
#if GCCPLUGIN_VERSION < 8000
    NAME(cpti_names, CPTI_KEYED_CLASSES);
#endif
    NAME(cpti_names, CPTI_LANG_NAME_C);
    NAME(cpti_names, CPTI_LANG_NAME_CPLUSPLUS);
#if GCCPLUGIN_VERSION < 7000
    NAME(cpti_names, CPTI_LANG_NAME_JAVA);
#endif
#if GCCPLUGIN_VERSION < 8000
    NAME(cpti_names, CPTI_NELTS_IDENTIFIER);
#endif
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_NOEXCEPT_DEFERRED_SPEC);
#endif
#if GCCPLUGIN_VERSION >= 4006
    NAME(cpti_names, CPTI_NOEXCEPT_FALSE_SPEC);
    NAME(cpti_names, CPTI_NOEXCEPT_TRUE_SPEC);
    NAME(cpti_names, CPTI_NULLPTR);
    NAME(cpti_names, CPTI_NULLPTR_TYPE);
#endif
    NAME(cpti_names, CPTI_PFN_IDENTIFIER);
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_RETHROW_FN);
#endif
    NAME(cpti_names, CPTI_STD);
    NAME(cpti_names, CPTI_STD_IDENTIFIER);
#if GCCPLUGIN_VERSION < 8000
    NAME(cpti_names, CPTI_TERMINATE);
#endif
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_TERMINATE_FN);
#endif
    NAME(cpti_names, CPTI_THIS_IDENTIFIER);
#if GCCPLUGIN_VERSION >= 8000
    NAME(cpti_names, CPTI_THROW_FN);
#endif
    NAME(cpti_names, CPTI_TYPE_INFO_PTR_TYPE);
    NAME(cpti_names, CPTI_UNKNOWN_TYPE);
    NAME(cpti_names, CPTI_VPTR_IDENTIFIER);
    NAME(cpti_names, CPTI_VTABLE_ENTRY_TYPE);
    NAME(cpti_names, CPTI_VTABLE_INDEX_TYPE);
    NAME(cpti_names, CPTI_VTBL_PTR_TYPE);
    NAME(cpti_names, CPTI_VTBL_TYPE);
    NAME(cpti_names, CPTI_VTT_PARM_IDENTIFIER);
    NAME(cpti_names, CPTI_VTT_PARM_TYPE);
    NAME(cpti_names, CPTI_WCHAR_DECL);
    CHECK(cpti_names);

#if GCCPLUGIN_VERSION >= 5000
    NAME(cilk_ti_names, CILK_TI_FRAME_CONTEXT);
    NAME(cilk_ti_names, CILK_TI_FRAME_EXCEPTION);
    NAME(cilk_ti_names, CILK_TI_FRAME_FLAGS);
    NAME(cilk_ti_names, CILK_TI_FRAME_PARENT);
    NAME(cilk_ti_names, CILK_TI_FRAME_PEDIGREE);
    NAME(cilk_ti_names, CILK_TI_FRAME_PTR);
    NAME(cilk_ti_names, CILK_TI_FRAME_TYPE);
    NAME(cilk_ti_names, CILK_TI_FRAME_WORKER);
    NAME(cilk_ti_names, CILK_TI_F_DETACH);
    NAME(cilk_ti_names, CILK_TI_F_ENTER);
    NAME(cilk_ti_names, CILK_TI_F_ENTER_FAST);
    NAME(cilk_ti_names, CILK_TI_F_LEAVE);
    NAME(cilk_ti_names, CILK_TI_F_LOOP_32);
    NAME(cilk_ti_names, CILK_TI_F_LOOP_64);
    NAME(cilk_ti_names, CILK_TI_F_POP);
    NAME(cilk_ti_names, CILK_TI_F_RETHROW);
    NAME(cilk_ti_names, CILK_TI_F_SAVE_FP);
    NAME(cilk_ti_names, CILK_TI_F_SYNC);
    NAME(cilk_ti_names, CILK_TI_F_WORKER);
    NAME(cilk_ti_names, CILK_TI_PEDIGREE_PARENT);
    NAME(cilk_ti_names, CILK_TI_PEDIGREE_RANK);
    NAME(cilk_ti_names, CILK_TI_PEDIGREE_TYPE);
    NAME(cilk_ti_names, CILK_TI_WORKER_CUR);
    NAME(cilk_ti_names, CILK_TI_WORKER_PEDIGREE);
    NAME(cilk_ti_names, CILK_TI_WORKER_TAIL);
    NAME(cilk_ti_names, CILK_TI_WORKER_TYPE);
    CHECK(cilk_ti_names);
#endif

    NAME(omp_clause_schedule_names, OMP_CLAUSE_SCHEDULE_STATIC);
    NAME(omp_clause_schedule_names, OMP_CLAUSE_SCHEDULE_DYNAMIC);
    NAME(omp_clause_schedule_names, OMP_CLAUSE_SCHEDULE_GUIDED);
    NAME(omp_clause_schedule_names, OMP_CLAUSE_SCHEDULE_AUTO);
    NAME(omp_clause_schedule_names, OMP_CLAUSE_SCHEDULE_RUNTIME);
    CHECK(omp_clause_schedule_names);

    NAME(omp_clause_default_names, OMP_CLAUSE_DEFAULT_UNSPECIFIED);
    NAME(omp_clause_default_names, OMP_CLAUSE_DEFAULT_SHARED);
    NAME(omp_clause_default_names, OMP_CLAUSE_DEFAULT_NONE);
    NAME(omp_clause_default_names, OMP_CLAUSE_DEFAULT_PRIVATE);
    NAME(omp_clause_default_names, OMP_CLAUSE_DEFAULT_FIRSTPRIVATE);
    CHECK(omp_clause_default_names);

    assert (strcmp(tls_model_names[TLS_MODEL_NONE], "none") == 0);
    assert (strcmp(tls_model_names[TLS_MODEL_EMULATED], "emulated") == 0);
    assert (strcmp(tls_model_names[TLS_MODEL_GLOBAL_DYNAMIC], "global_dynamic") == 0);
    assert (strcmp(tls_model_names[TLS_MODEL_REAL], "global_dynamic") == 0);
    assert (strcmp(tls_model_names[TLS_MODEL_LOCAL_DYNAMIC], "local_dynamic") == 0);
    assert (strcmp(tls_model_names[TLS_MODEL_INITIAL_EXEC], "initial_exec") == 0);
    assert (strcmp(tls_model_names[TLS_MODEL_LOCAL_EXEC], "local_exec") == 0);

    NAME(symbol_visibility_names, VISIBILITY_DEFAULT);
    NAME(symbol_visibility_names, VISIBILITY_PROTECTED);
    NAME(symbol_visibility_names, VISIBILITY_HIDDEN);
    NAME(symbol_visibility_names, VISIBILITY_INTERNAL);
    CHECK(symbol_visibility_names);

    NAME(cpp_node_type_names, NT_VOID);
    NAME(cpp_node_type_names, NT_MACRO);
    NAME(cpp_node_type_names, NT_ASSERTION);
    CHECK(cpp_node_type_names);

    NAME(cpp_builtin_type_names, BT_SPECLINE);
    NAME(cpp_builtin_type_names, BT_DATE);
    NAME(cpp_builtin_type_names, BT_FILE);
    NAME(cpp_builtin_type_names, BT_BASE_FILE);
    NAME(cpp_builtin_type_names, BT_INCLUDE_LEVEL);
    NAME(cpp_builtin_type_names, BT_TIME);
    NAME(cpp_builtin_type_names, BT_STDC);
    NAME(cpp_builtin_type_names, BT_PRAGMA);
    NAME(cpp_builtin_type_names, BT_TIMESTAMP);
    NAME(cpp_builtin_type_names, BT_COUNTER);
#if GCCPLUGIN_VERSION >= 5000
    NAME(cpp_builtin_type_names, BT_HAS_ATTRIBUTE);
#endif
#if GCCPLUGIN_VERSION >= 4006
    NAME(cpp_builtin_type_names, BT_FIRST_USER+0);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+1);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+2);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+3);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+4);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+5);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+6);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+7);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+8);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+9);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+10);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+11);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+12);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+13);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+14);
    NAME(cpp_builtin_type_names, BT_FIRST_USER+15);
    NAME(cpp_builtin_type_names, BT_LAST_USER-15);
    NAME(cpp_builtin_type_names, BT_LAST_USER-14);
    NAME(cpp_builtin_type_names, BT_LAST_USER-13);
    NAME(cpp_builtin_type_names, BT_LAST_USER-12);
    NAME(cpp_builtin_type_names, BT_LAST_USER-11);
    NAME(cpp_builtin_type_names, BT_LAST_USER-10);
    NAME(cpp_builtin_type_names, BT_LAST_USER-9);
    NAME(cpp_builtin_type_names, BT_LAST_USER-8);
    NAME(cpp_builtin_type_names, BT_LAST_USER-7);
    NAME(cpp_builtin_type_names, BT_LAST_USER-6);
    NAME(cpp_builtin_type_names, BT_LAST_USER-5);
    NAME(cpp_builtin_type_names, BT_LAST_USER-4);
    NAME(cpp_builtin_type_names, BT_LAST_USER-3);
    NAME(cpp_builtin_type_names, BT_LAST_USER-2);
    NAME(cpp_builtin_type_names, BT_LAST_USER-1);
    NAME(cpp_builtin_type_names, BT_LAST_USER-0);
#endif
    CHECK(cpp_builtin_type_names);
}
