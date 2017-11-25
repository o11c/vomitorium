#include "internal.hpp"

#include <cassert>

#include <map>
#include <vector>

#include "intern.hpp"
#include "iter.hpp"
#include "names.hpp"
#include "traits.hpp"
#include "xml.hpp"

#include "vgcc/c-family/c-common.h"
#include "vgcc/c-family/c-pragma.h"
#include "vgcc/c-tree.h"
#include "vgcc/cilk.h"
#include "vgcc/debug.h"
#include "vgcc/expr.h"
#include "vgcc/fixed-value.h"
#include "vgcc/langhooks.h"
#include "vgcc/omp-low.h"
#include "vgcc/omp-offload.h"
#include "vgcc/real.h"
#include "vgcc/rtl.h"
#include "vgcc/ssa-iterators.h"
#include "vgcc/tree-chrec.h"
#include "vgcc/tree-flow.h"
#include "vgcc/tree.h"
#include "vgcc/vec.h"
#include "vgcc/vtable-verify.h"


static size_t incomplete_dumps = 0;

// FIXME: this currently needs to be a lazy global.
static XmlOutput& get_xml_output()
{
    static XmlOutput global_output(vomitorium_output, false);
    return global_output;
}

// forbid implicit conversions
template<class T, typename=void>
struct XmlEmitter;
/*
{
    static void do_xemit(T obj) = delete; // readded by specializations
};
*/

template<class T>
static void xemit(const T& obj)
{
    // this transforms `char [16]` into `char *`
    typedef typename std::decay<T>::type Decay;
    // this transforms `char *` into `const char *`
    typedef typename add_pointer_const<Decay>::type ConstDecay;
    // don't care whether do_xemit is implemented by-reference or by-value.
    XmlEmitter<ConstDecay>::do_xemit(obj);
}
static void xml0(const char *tag)
{
    get_xml_output().tag(tag);
}
template<class V>
static void xml0(const char *tag, const char *k, V v)
{
    auto xml_tag = get_xml_output().tag(tag);
    auto attr = get_xml_output().attr(k);
    xemit(v);
}

class Xml
{
    XmlTag xml_tag;
public:
    Xml(const char *t) : xml_tag(get_xml_output().tag(t))
    {
    }
    template<class V>
    Xml(const char *t, const char *k, V v) : xml_tag(get_xml_output().tag(t))
    {
        auto attr = get_xml_output().attr(k);
        xemit(v);
    }
};

template<class T>
static void xml1(const char *tag, T v)
{
    Xml xml(tag);
    xemit(v);
}

template<class T>
struct XmlEmitter<const T *>
{
    static void do_xemit(const T *obj)
    {
        {
            auto attr = get_xml_output().attr("null");
            xemit(obj ? "false" : "true");
        }
        if (obj)
            xemit(*obj);
    }
};

template<>
struct XmlEmitter<const char *>
{
    static void do_xemit(const char *obj)
    {
        get_xml_output().emit_string(obj);
    }
};

template<class T>
struct XmlEmitter<T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type>
{
    static void do_xemit(T obj)
    {
        get_xml_output().with_output_file(
            [=](FILE *out)
            {
                fprintf(out, "%ju", (uintmax_t)obj);
            }
        );
    }
};
template<class T>
struct XmlEmitter<T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
{
    static void do_xemit(T obj)
    {
        get_xml_output().with_output_file(
            [=](FILE *out)
            {
                fprintf(out, "%jd", (uintmax_t)obj);
            }
        );
    }
};


template<>
struct XmlEmitter<enum tree_code>
{
    static void do_xemit(enum tree_code obj)
    {
        xemit(get_tree_code_name(obj));
    }
};

template<>
struct XmlEmitter<enum rid>
{
    static void do_xemit(enum rid obj)
    {
        xemit(rid_names[obj]);
    }
};

template<>
struct XmlEmitter<enum tree_index>
{
    static void do_xemit(enum tree_index obj)
    {
        xemit(ti_names[obj]);
    }
};

template<>
struct XmlEmitter<enum c_tree_index>
{
    static void do_xemit(enum c_tree_index obj)
    {
        xemit(cti_names[obj]);
    }
};

template<>
struct XmlEmitter<enum cp_tree_index>
{
    static void do_xemit(enum cp_tree_index obj)
    {
        xemit(cpti_names[obj]);
    }
};

#if V(5)
template<>
struct XmlEmitter<enum cilk_tree_index>
{
    static void do_xemit(enum cilk_tree_index obj)
    {
        xemit(cilk_ti_names[obj]);
    }
};
#endif

template<>
struct XmlEmitter<enum built_in_function>
{
    static void do_xemit(enum built_in_function obj)
    {
        xemit(built_in_names[obj]);
    }
};

template<>
struct XmlEmitter<enum integer_type_kind>
{
    static void do_xemit(enum integer_type_kind obj)
    {
        xemit(itk_names[obj]);
    }
};

template<>
struct XmlEmitter<enum size_type_kind>
{
    static void do_xemit(enum size_type_kind obj)
    {
        xemit(stk_names[obj]);
    }
};
// register names are not an enum


template<>
struct XmlEmitter<enum omp_clause_code>
{
    static void do_xemit(enum omp_clause_code obj)
    {
        xemit(omp_clause_code_name[obj]);
    }
};

template<>
struct XmlEmitter<enum omp_clause_schedule_kind>
{
    static void do_xemit(enum omp_clause_schedule_kind obj)
    {
#if !V(6)
        xemit(omp_clause_schedule_names[obj]);
#else
        xemit(omp_clause_schedule_names[obj & OMP_CLAUSE_SCHEDULE_MASK]);
        if (obj & OMP_CLAUSE_SCHEDULE_MONOTONIC)
            xemit("|OMP_CLAUSE_SCHEDULE_MONOTONIC");
        if (obj & OMP_CLAUSE_SCHEDULE_NONMONOTONIC)
            xemit("|OMP_CLAUSE_SCHEDULE_NONMONOTONIC");
        static_assert(OMP_CLAUSE_SCHEDULE_LAST == (1<<5) - 1, "more flags?");
#endif
    }
};

template<>
struct XmlEmitter<enum omp_clause_default_kind>
{
    static void do_xemit(enum omp_clause_default_kind obj)
    {
        xemit(omp_clause_default_names[obj]);
    }
};

template<>
struct XmlEmitter<enum machine_mode>
{
    static void do_xemit(enum machine_mode obj)
    {
        xemit(GET_MODE_NAME(obj));
    }
};

template<>
struct XmlEmitter<enum built_in_class>
{
    static void do_xemit(enum built_in_class obj)
    {
        xemit(built_in_class_names[obj]);
    }
};

template<>
struct XmlEmitter<enum tls_model>
{
    static void do_xemit(enum tls_model obj)
    {
        xemit(tls_model_names[obj]);
    }
};

template<>
struct XmlEmitter<enum symbol_visibility>
{
    static void do_xemit(enum symbol_visibility obj)
    {
        xemit(symbol_visibility_names[obj]);
    }
};

template<>
struct XmlEmitter<enum node_type>
{
    static void do_xemit(enum node_type obj)
    {
        xemit(cpp_node_type_names[obj]);
    }
};

template<>
struct XmlEmitter<enum cpp_builtin_type>
{
    static void do_xemit(enum cpp_builtin_type obj)
    {
        xemit(cpp_builtin_type_names[obj]);
    }
};

#if V(4, 9)
template<>
struct XmlEmitter<enum internal_fn>
{
    static void do_xemit(enum internal_fn obj)
    {
        xemit(internal_fn_name_array[obj]);
    }
};

template<>
struct XmlEmitter<enum omp_clause_depend_kind>
{
    static void do_xemit(enum omp_clause_depend_kind obj)
    {
        xemit(omp_clause_depend_names[obj]);
    }
};

#if !V(5)
template<>
struct XmlEmitter<enum omp_clause_map_kind>
{
    static void do_xemit(enum omp_clause_map_kind obj)
    {
        xemit(omp_clause_map_names[obj]);
    }
};
#else
template<>
struct XmlEmitter<enum gomp_map_kind>
{
    static void do_xemit(enum gomp_map_kind obj)
    {
        assert (obj < GOMP_MAP__ARRAY_LAST_ && "invalid gomp_map_kind");
        const char *name = gomp_map_names[obj];
        if (name)
            xemit(name);
        else
        {
            xemit("NYI: gomp_map_kind other bits (decimal ");
            xemit(obj);
            xemit(")");
        }
    }
};
#endif

template<>
struct XmlEmitter<enum omp_clause_proc_bind_kind>
{
    static void do_xemit(enum omp_clause_proc_bind_kind obj)
    {
        xemit(omp_clause_proc_bind_names[obj]);
    }
};
#endif

#if V(6)
template<>
struct XmlEmitter<enum omp_clause_linear_kind>
{
    static void do_xemit(enum omp_clause_linear_kind obj)
    {
        xemit(omp_clause_linear_names[obj]);
    }
};
#endif
// it turns out we don't need to convert to this.

template<>
struct XmlEmitter<typename std::decay<const mpz_t>::type>
{
    static void do_xemit(const mpz_t obj)
    {
        gmp_fprintf(vomitorium_output, "%Zd", obj);
    }
};
// Note: this always outputs as signed, since that is shorter.

template<>
struct XmlEmitter<double_int>
{
    static void do_xemit(double_int obj)
    {
        // Avoid allocating an mpz if possible (it is rarely needed).
        // The fast path can handle 65-bit integers (actually, only 33-bit
        // integers on some configurations), by manually flipping the sign.
        if (double_int_negative_p(obj))
        {
            xemit("-");
            obj = double_int_neg(obj);
        }
        if (double_int_fits_in_uhwi_p(obj))
        {
            xemit(double_int_to_uhwi(obj));
        }
        else
        {
            dump_double_int(vomitorium_output, obj, false);
            if (0)
            {
                mpz_t val;
                mpz_init(val);
                mpz_set_double_int(val, obj, false);
                xemit(val);
                mpz_clear(val);
            }
        }
    }
};

#if V(5)
template<>
struct XmlEmitter<wide_int_ref>
{
    static void do_xemit(wide_int_ref obj)
    {
        // unlike double_int, unary minus is not cheap/easy here.
        if (wi::fits_shwi_p(obj))
        {
            xemit(obj.to_shwi());
        }
        else
        {
            print_decs(obj, vomitorium_output);
            if (0)
            {
                mpz_t val;
                mpz_init(val);
                wi::to_mpz(obj, val, SIGNED);
                xemit(val);
                mpz_clear(val);
            }
        }
    }
};
#endif

template<>
struct XmlEmitter<struct real_value>
{
    static void do_xemit(const struct real_value& obj)
    {
        static const size_t max_digits = SIGNIFICAND_BITS / 4;
        static const size_t exp_bytes = 16;
        static const size_t buf_size = max_digits + exp_bytes + 1 + 4 + 1;
        char buf[buf_size + 16]; // just in case - will be checked
        memset(buf, 0, sizeof(buf));
        real_to_hexadecimal(buf, &obj, sizeof(buf), 0, false);
        assert (buf[buf_size - 1] == '\0');
        xemit(buf);
    }
};

template<>
struct XmlEmitter<struct fixed_value>
{
    static void do_xemit(const struct fixed_value& obj)
    {
        static const size_t buf_size = sizeof(obj) * 8 + 2;
        char buf[buf_size + 16]; // just in case - will be checked
        fixed_to_decimal(buf, &obj, sizeof(buf));
        assert (buf[buf_size - 1] == '\0');
        xemit(buf);
    }
};

template<>
struct XmlEmitter<expanded_location>
{
    static void do_xemit(expanded_location obj)
    {
        xemit(obj.sysp ? "<" : "\"");
        xemit(obj.file ?: "(null)");
        xemit(obj.sysp ? ">" : "\"");
        xemit(":");
        xemit(obj.line);
        xemit(":");
        xemit(obj.column);
    }
};
#if V(6)
template<>
struct XmlEmitter<source_range>
{
    static void do_xemit(source_range obj)
    {
        xml1("start", expand_location(obj.m_start));
        xml1("finish", expand_location(obj.m_finish));
    }
};
#endif

// TODO implement these dumpers
template<>
struct XmlEmitter<const_rtx>
{
    static void do_xemit(const_rtx obj)
    {
        (void)obj;
        xemit("NYI: rtx");
    }
};

template<>
struct XmlEmitter<const_gimple_ptr>
{
    static void do_xemit(const_gimple_ptr obj)
    {
        (void)obj;
        xemit("NYI: gimple");
    }
};
#if !V(4, 8)
template<>
struct XmlEmitter<const_gimple_seq>
{
    static void do_xemit(const_gimple_seq obj)
    {
        (void)obj;
        xemit("NYI: gimple_seq");
    }
};
// otherwise, the same as gimple
#endif

template<>
struct XmlEmitter<struct ptr_info_def>
{
    static void do_xemit(const struct ptr_info_def& obj)
    {
        (void)obj;
        xemit("NYI: ptr_info_def");
    }
};

template<>
struct XmlEmitter<struct die_struct>
{
    static void do_xemit(const struct die_struct& obj)
    {
        (void)obj;
        xemit("NYI: die_struct");
    }
};

template<>
struct XmlEmitter<struct lang_type>
{
    static void do_xemit(const struct lang_type& obj)
    {
        (void)obj;
        xemit("NYI: lang_type");
    }
};

template<>
struct XmlEmitter<struct lang_decl>
{
    static void do_xemit(const struct lang_decl& obj)
    {
        (void)obj;
        xemit("NYI: lang_decl");
    }
};

template<>
struct XmlEmitter<struct function>
{
    static void do_xemit(const struct function& obj)
    {
        (void)obj;
        xemit("NYI: function");
    }
};

#if !V(4, 8)
template<>
struct XmlEmitter<typename add_pointer_const<var_ann_t>::type>
{
    static void do_xemit(var_ann_t obj)
    {
        (void)obj;
        xemit("NYI: var_ann_t");
    }
};
#endif

template<>
struct XmlEmitter<struct cl_optimization>
{
    static void do_xemit(struct cl_optimization obj)
    {
        (void)obj;
        xemit("NYI: cl_optimization");
    }
};

template<>
struct XmlEmitter<struct cl_target_option>
{
    static void do_xemit(struct cl_target_option obj)
    {
        (void)obj;
        xemit("NYI: cl_target_option");
    }
};

template<>
struct XmlEmitter<struct cxx_binding>
{
    static void do_xemit(const struct cxx_binding& obj)
    {
        (void)obj;
        xemit("NYI: cxx_bindings");
    }
};

template<>
struct XmlEmitter<cpp_macro>
{
    static void do_xemit(const cpp_macro& obj)
    {
        (void)obj;
        xemit("NYI: cpp_macro");
    }
};

template<>
struct XmlEmitter<answer>
{
    static void do_xemit(const answer& obj)
    {
        (void)obj;
        xemit("NYI: answer");
    }
};

template<>
struct XmlEmitter<struct c_binding>
{
    static void do_xemit(const struct c_binding& obj)
    {
        (void)obj;
        xemit("NYI: c_binding");
    }
};

#if V(4, 8)
template<>
struct XmlEmitter<struct target_optabs>
{
    static void do_xemit(const struct target_optabs& obj)
    {
        (void)obj;
        xemit("NYI: target_optabs");
    }
};
#endif

#if V(4, 9)
template<>
struct XmlEmitter<struct range_info_def>
{
    static void do_xemit(const struct range_info_def& obj)
    {
        (void)obj;
        xemit("NYI: range_info_def");
    }
};

template<>
struct XmlEmitter<struct target_globals>
{
    static void do_xemit(const struct target_globals& obj)
    {
        (void)obj;
        xemit("NYI: target_globals");
    }
};
#endif



template<>
struct XmlEmitter<const_tree>
{
    static void do_xemit(const_tree obj)
    {
        xemit("@");
        xemit(intern(obj));
    }
};

template<class T>
struct index_pair
{
    const char *array;
    T index;
};
template<class T>
struct XmlEmitter<index_pair<T>>
{
    static void do_xemit(index_pair<T> obj)
    {
        xemit(obj.array);
        xemit("[");
        xemit(obj.index);
        xemit("]");
    }
};
template<class T>
struct field_pair
{
    T lhs;
    const char *field;
};
template<class T>
struct XmlEmitter<field_pair<T>>
{
    static void do_xemit(field_pair<T> obj)
    {
        xemit(obj.lhs);
        xemit(".");
        xemit(obj.field);
    }
};

class Global : public Xml
{
public:
    Global(const char *name) : Xml("global", "name", name) {}
    template<class T>
    Global(const char *array, T index) : Xml("global", "name", index_pair<T>{array, index}) {}
};

template<class T>
static void global1(const char *name, T v)
{
    Global xml(name);
    xemit(v);
}

static void globalv(const char *name, VEC(tree, gc) *v)
{
    size_t j = 0;
    foreach (tree t, v)
    {
        Global xml(name, j);
        xemit(t);
        j++;
    }
}


// return a mutable pointer from a const one, using base.
static char *jiggle(void *base, const void *other)
{
    assert (other > base);
    char *base_c = (char *)base;
    const char *other_c = (const char *)other;
    ptrdiff_t diff = other_c - base_c;
    return base_c + diff;
}

static bool is_all_zero(const void *data, size_t size)
{
    auto p = (const char *)data;
    for (size_t i = 0; i < size; ++i)
        if (p[i])
            return false;
    return true;
}

static void xemit_hex(const void *data, size_t size)
{
    if (size > 16)
        get_xml_output().emit_newline();
    auto p = (const unsigned char *)data;
    char buf[] = "00 11 22 33  44 55 66 77  88 99 aa bb  cc dd ee ff";
    static const char hex[] = "0123456789abcdef";

    while (size > 16)
    {
        int i, bi;

        i = 0;
        bi = 0;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 1;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 2;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 3;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        bi++;

        i = 4;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 5;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 6;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 7;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        bi++;

        i = 8;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 9;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 10;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 11;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        bi++;

        i = 12;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 13;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 14;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        i = 15;
        bi += 3;
        buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];

        bi += 3;
        assert (sizeof(buf)-1 == bi-1);

        get_xml_output().emit_raw(buf, sizeof(buf)-1);
        get_xml_output().emit_newline();
        p += 16;
        size -= 16;
    }
    if (size)
    {
        int i, bi;

        if (size > 0)
        {
            i = 0;
            bi = 0;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 1)
        {
            i = 1;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 2)
        {
            i = 2;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 3)
        {
            i = 3;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 4)
        {
            bi++;

            i = 4;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 5)
        {
            i = 5;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 6)
        {
            i = 6;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 7)
        {
            i = 7;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 8)
        {
            bi++;

            i = 8;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 9)
        {
            i = 9;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 10)
        {
            i = 10;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 11)
        {
            i = 11;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 12)
        {
            bi++;

            i = 12;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 13)
        {
            i = 13;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 14)
        {
            i = 14;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }
        if (size > 15)
        {
            i = 15;
            bi += 3;
            buf[bi] = hex[p[i] >> 4], buf[bi+1] = hex[p[i] & 0xf];
        }

        bi += 3;

        get_xml_output().emit_raw(buf, bi-1);
    }
    if (p != data)
        get_xml_output().emit_newline();
}


// When debugging, set a breakpoint here, then go up a frame
// to access the typed version of the structure.
__attribute__((noinline))
static const void *dump_remaining(const void *orig, void *mask, size_t size)
{
    Xml xml("remaining");
    xemit_hex(mask, size);

    return orig;
}

// Get a value, then zero it.
#define TAKE0(lval)                     \
({                                      \
    auto _lval = (lval);                \
    decltype(_lval) _zero;              \
    memset(&_zero, 0, sizeof(_zero));   \
    (lval) = _zero;                     \
    _lval;                              \
})
#define TAKE1(LVALUE)                   \
({                                      \
    auto _rv = LVALUE(orig_tree);       \
    decltype(_rv) _zero;                \
    memset(&_zero, 0, sizeof(_zero));   \
    LVALUE(bitmask_tree) = _zero;       \
    _rv;                                \
})
#define TAKE_I(LVALUE, I)               \
({                                      \
    auto _rv = LVALUE(orig_tree, (I));  \
    decltype(_rv) _zero;                \
    memset(&_zero, 0, sizeof(_zero));   \
    LVALUE(bitmask_tree, (I)) = _zero;  \
    _rv;                                \
})
#define TAKE2(GETTER, SETTER)           \
({                                      \
    auto _rv = GETTER(orig_tree);       \
    decltype(_rv) _zero;                \
    memset(&_zero, 0, sizeof(_zero));   \
    SETTER(bitmask_tree, _zero);        \
    _rv;                                \
})

#define DO_LVAL(name, lval)                                             \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE0(lval);                                        \
        static_assert(!is_string<decltype(_val)>::value, "no strings"); \
        xml1(name, _val);                                               \
    }                                                                   \
    while (0)
#define DO_VAL(name, LVALUE)                                            \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE1(LVALUE);                                      \
        static_assert(!is_string<decltype(_val)>::value, "no strings"); \
        xml1(name, _val);                                               \
    }                                                                   \
    while (0)
#define DO_VAL_I(name, LVALUE, I)                                       \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE_I(LVALUE, (I));                                \
        static_assert(!is_string<decltype(_val)>::value, "no strings"); \
        xml1(name, _val);                                               \
    }                                                                   \
    while (0)
#define DO_VAL2(name, GETTER, SETTER)                                   \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE2(GETTER, SETTER);                              \
        static_assert(!is_string<decltype(_val)>::value, "no strings"); \
        xml1(name, _val);                                               \
    }                                                                   \
    while (0)

// TODO use is_all_zero
#define CDO_LVAL(name, lval)        \
    do                              \
    {                               \
        if (auto _v = TAKE0(lval))  \
            xml1(name, _v);         \
    }                               \
    while (0)
#define CDO_VAL(name, LVALUE)           \
    do                                  \
    {                                   \
        if (auto _v = TAKE1(LVALUE))    \
            xml1(name, _v);             \
    }                                   \
    while (0)
#define CDO_VAL_I(name, LVALUE, I)          \
    do                                      \
    {                                       \
        if (auto _v = TAKE_I(LVALUE, (I)))  \
            xml1(name, _v);                 \
    }                                       \
    while (0)
#define CDO_VAL2(name, GETTER, SETTER)          \
    do                                          \
    {                                           \
        if (auto _v = TAKE2(GETTER, SETTER))    \
            xml1(name, _v);                     \
    }                                           \
    while (0)

#define BIT(expr)                       \
({                                      \
    auto _expr = +(expr);               \
    assert (_expr == 0 || _expr == 1);  \
    _expr;                              \
})
#define DO_BIT(name, LVALUE)        \
    do                              \
    {                               \
        if (BIT(TAKE1(LVALUE)))     \
            xml0(name);             \
    }                               \
    while (0)
#define DO_BIT2(name, GETTER, SETTER)       \
    do                                      \
    {                                       \
        if (BIT(TAKE2(GETTER, SETTER)))     \
            xml0(name);                     \
    }                                       \
    while (0)
#define DO_FLAG(name, lvalue, mask)     \
        do                              \
        {                               \
            if ((lvalue) & (mask))      \
            {                           \
                xml0((name));           \
                (lvalue) &= ~(mask);    \
            }                           \
        }                               \
        while (0)

static void dump_cci(const struct c_common_identifier *orig_tree, struct c_common_identifier *bitmask_tree)
{
    cpp_hashnode *node = &bitmask_tree->node;
    const cpp_hashnode *orig_node = &orig_tree->node;
    // already part of tree_identifier
    (void)node->ident;

    if (node->is_directive)
    {
        node->is_directive = 0;
        xml0("is-directive");
        DO_LVAL("directive-index", node->directive_index);
    }
    else
    {
        DO_LVAL("node-operator", node->directive_index);
    }
    DO_VAL2("rid-code", C_RID_CODE, C_SET_RID_CODE);
    DO_LVAL("node-type", node->type);
    {
        Xml xml("flags");
        DO_FLAG("named-operator", node->flags, NODE_OPERATOR);
        DO_FLAG("poisoned", node->flags, NODE_POISONED);
        DO_FLAG("builtin", node->flags, NODE_BUILTIN);
        DO_FLAG("diagnostic", node->flags, NODE_DIAGNOSTIC);
        DO_FLAG("warn", node->flags, NODE_WARN);
        DO_FLAG("disabled", node->flags, NODE_DISABLED);
        DO_FLAG("macro-argument", node->flags, NODE_MACRO_ARG);
        DO_FLAG("used", node->flags, NODE_USED);
        DO_FLAG("conditional", node->flags, NODE_CONDITIONAL);
        DO_FLAG("warn-operator", node->flags, NODE_WARN_OPERATOR);
    }
    // Unhygienic macro! But we're using the copy so we can do it
    // at the end, but still access type/flags.
    switch (CPP_HASHNODE_VALUE_IDX((*orig_node)))
    {
    case NTV_MACRO:
        DO_LVAL("macro", node->value.macro);
        break;
    case NTV_ANSWER:
        DO_LVAL("answers", node->value.answers);
        break;
    case NTV_BUILTIN:
        DO_LVAL("builtin-macro", node->value.builtin);
        break;
    case NTV_ARGUMENT:
        DO_LVAL("argument-index", node->value.arg_index);
        node->value.arg_index = 0;
        break;
    case NTV_NONE:
        break;
    }
}
#if G(4, 6)
# pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wshadow"
static void dump_tree(const_tree orig_tree)
{
    // TODO split into:
    //  * dump_tree_by_struct()
    //  * dump_tree_by_code_class() but only for a few things
    //  * dump_tree_by_code()
    // each of which should use the .def files to switch() to a template.
    // TODO write `fake_tree` function for things that don't want a `code`.
    if (!orig_tree)
    {
        xml0("tree", "id", orig_tree);
        return;
    }
    Xml tree_xml("tree", "id", orig_tree);

    size_t tree_sizeof = tree_size(orig_tree);
    __attribute__((aligned(alignof(*orig_tree))))
    char bitmask_vla[tree_sizeof];
    tree bitmask_tree = (tree)&bitmask_vla[0];
    memcpy(bitmask_tree, orig_tree, tree_sizeof);

    enum tree_code code = TREE_CODE(orig_tree);
    enum tree_node_structure_enum structure = tree_node_structure(orig_tree);
    // For other trees, assign a bogus value to avoid extra if's.
    enum omp_clause_code omp_code = code == OMP_CLAUSE ? OMP_CLAUSE_CODE(orig_tree) : (enum omp_clause_code)-1;
    //enum tree_code_class code_class = TREE_CODE_CLASS(code);

    switch (structure)
    {
    case TS_BASE:
        assert (tree_sizeof == sizeof(orig_tree->base));
        assert (0 && "TS_BASE is abstract ... right?");
        // but note that BLOCK inherits it directly
        break;
#if V(4, 7)
    case TS_TYPED:
        assert (tree_sizeof == sizeof(orig_tree->typed));
#if !V(5)
        assert (0 && "TS_TYPED is abstract ... right?");
#endif
        // most things inherit TS_COMMON, but not e.g. expressions
        break;
#endif
    case TS_COMMON:
        assert (tree_sizeof == sizeof(orig_tree->common));
        // has a few instances, even though it's *mostly* abstract.
        break;
    case TS_INT_CST:
#if !V(5)
        assert (tree_sizeof == sizeof(orig_tree->int_cst));
#else
        assert (tree_sizeof == sizeof(orig_tree->int_cst) + (TREE_INT_CST_EXT_NUNITS(orig_tree) - 1) * sizeof(HOST_WIDE_INT));
#endif
        break;
    case TS_REAL_CST:
        assert (tree_sizeof == sizeof(orig_tree->real_cst));
        break;
    case TS_FIXED_CST:
        assert (tree_sizeof == sizeof(orig_tree->fixed_cst));
        break;
    case TS_VECTOR:
        assert (tree_sizeof == sizeof(orig_tree->vector));
        break;
    case TS_STRING:
        // variable-size
        assert (tree_sizeof == offsetof(decltype(orig_tree->string), str) + orig_tree->string.length + 1);
        break;
    case TS_COMPLEX:
        assert (tree_sizeof == sizeof(orig_tree->complex));
        break;
    case TS_IDENTIFIER:
        // languages subclass this
        assert (tree_sizeof == lang_hooks.identifier_size);
        assert (tree_sizeof >= sizeof(orig_tree->identifier));
        break;
    case TS_DECL_MINIMAL:
        assert (tree_sizeof == sizeof(orig_tree->decl_minimal));
        break;
    case TS_DECL_COMMON:
        assert (tree_sizeof == sizeof(orig_tree->decl_common));
        break;
    case TS_DECL_WRTL:
        assert (tree_sizeof == sizeof(orig_tree->decl_with_rtl));
        break;
    case TS_DECL_NON_COMMON:
        assert (tree_sizeof == sizeof(orig_tree->decl_non_common));
        break;
    case TS_DECL_WITH_VIS:
        assert (tree_sizeof == sizeof(orig_tree->decl_with_vis));
        break;
    case TS_FIELD_DECL:
        assert (tree_sizeof == sizeof(orig_tree->field_decl));
        break;
    case TS_VAR_DECL:
        assert (tree_sizeof == sizeof(orig_tree->var_decl));
        break;
    case TS_PARM_DECL:
        assert (tree_sizeof == sizeof(orig_tree->parm_decl));
        break;
    case TS_LABEL_DECL:
        assert (tree_sizeof == sizeof(orig_tree->label_decl));
        break;
    case TS_RESULT_DECL:
        assert (tree_sizeof == sizeof(orig_tree->result_decl));
        break;
    case TS_CONST_DECL:
        assert (tree_sizeof == sizeof(orig_tree->const_decl));
        break;
    case TS_TYPE_DECL:
        assert (tree_sizeof == sizeof(orig_tree->type_decl));
        break;
    case TS_FUNCTION_DECL:
        assert (tree_sizeof == sizeof(orig_tree->function_decl));
        break;
#if V(4, 6)
    case TS_TRANSLATION_UNIT_DECL:
        assert (tree_sizeof == sizeof(orig_tree->translation_unit_decl));
        break;
#endif
#if !V(4, 7)
    case TS_TYPE:
        assert (tree_sizeof == sizeof(orig_tree->type));
        break;
#else
    case TS_TYPE_COMMON:
        assert (tree_sizeof == sizeof(orig_tree->type_common));
        break;
    case TS_TYPE_WITH_LANG_SPECIFIC:
        assert (tree_sizeof == sizeof(orig_tree->type_with_lang_specific));
        break;
    case TS_TYPE_NON_COMMON:
        assert (tree_sizeof == sizeof(orig_tree->type_non_common));
        break;
#endif
    case TS_LIST:
        assert (tree_sizeof == sizeof(orig_tree->list));
        break;
    case TS_VEC:
        // variable-size
        assert (tree_sizeof >= sizeof(orig_tree->vec));
        break;
    case TS_EXP:
        // variable-size
        assert (tree_sizeof >= sizeof(orig_tree->exp));
        break;
    case TS_SSA_NAME:
        assert (tree_sizeof == sizeof(orig_tree->ssa_name));
        break;
    case TS_BLOCK:
        assert (tree_sizeof == sizeof(orig_tree->block));
        break;
    case TS_BINFO:
        assert (tree_sizeof == sizeof(orig_tree->binfo));
        break;
    case TS_STATEMENT_LIST:
        assert (tree_sizeof == sizeof(orig_tree->stmt_list));
        break;
    case TS_CONSTRUCTOR:
        assert (tree_sizeof == sizeof(orig_tree->constructor));
        break;
    case TS_OMP_CLAUSE:
        assert (tree_sizeof == sizeof(orig_tree->omp_clause));
        break;
    case TS_OPTIMIZATION:
        assert (tree_sizeof == sizeof(orig_tree->optimization));
        break;
    case TS_TARGET_OPTION:
        assert (tree_sizeof == sizeof(orig_tree->target_option));
        break;
    case LAST_TS_ENUM:
        assert (0 && "(not a real type)");
        break;
        // in lieu of default, let -Werror=switch-enum catch this
    }

    xml1("code", code);

    // later GCC shares bits, which we don't full check.
    if (code == TREE_VEC)
    {
        int length = TREE_VEC_LENGTH(orig_tree);
        for (int i = 0; i < length; ++i)
        {
            Xml xml("e", "i", i);
            xemit(TREE_VEC_ELT(orig_tree, i));
            TREE_VEC_ELT(bitmask_tree, i) = nullptr;
        }
        TREE_VEC_LENGTH(bitmask_tree) = 0;
    }

#if !V(4, 7)
    assert (CODE_CONTAINS_STRUCT(code, TS_COMMON));
# define TS_TYPED TS_COMMON
#endif

    // has numerous different meanings
    if (CODE_CONTAINS_STRUCT(code, TS_COMMON))
        DO_VAL("chain", TREE_CHAIN);
    // note that BLOCK_CHAIN is separate as of 4.7

    if (CODE_CONTAINS_STRUCT(code, TS_TYPED))
        DO_VAL((code == POINTER_TYPE || code == ARRAY_TYPE || code == VECTOR_TYPE) ? "element_type" : "type", TREE_TYPE);
    if (EXPR_P(orig_tree))
    {
#define orig_tree CONST_CAST_TREE(orig_tree)
        DO_VAL2("block", TREE_BLOCK, TREE_SET_BLOCK);
#undef orig_tree
    }

    // TODO: emit all flags unconditionally?
    // TODO: instead of doing "if not cleared" for the final flag, emit the
    // *value* unconditionally, but only if MACRO(t) == MACRO(orig_tree)
    // (but will this work for the "opaque object" implementation?)

    if (1
#if 0
            && code != SSA_NAME
#endif
    )
    {
        // addressable_flag
        if (code == CALL_EXPR)
            DO_BIT("tailcall", CALL_EXPR_TAILCALL);
        if (code == CASE_LABEL_EXPR)
            DO_BIT("case-low-seen", CASE_LOW_SEEN);
        if (code == PREDICT_EXPR)
            DO_BIT2("predict-expr-outcome", PREDICT_EXPR_OUTCOME, SET_PREDICT_EXPR_OUTCOME);
        // only if not cleared by the above
        DO_BIT("addressable", TREE_ADDRESSABLE);

        // static_flag
        if (code == ADDR_EXPR)
            DO_BIT("no-trampoline", TREE_NO_TRAMPOLINE);
        if (code == TARGET_EXPR || code == WITH_CLEANUP_EXPR)
            DO_BIT("cleanup-eh-only", CLEANUP_EH_ONLY);
        if (code == TRY_CATCH_EXPR)
            DO_BIT("try-catch-is-cleanup", TRY_CATCH_IS_CLEANUP);
        if (code == CASE_LABEL_EXPR)
            DO_BIT("case-high-seen", CASE_HIGH_SEEN);
        if (code == CALL_EXPR)
        {
#if !V(4, 7)
            DO_BIT("cannot-inline", CALL_CANNOT_INLINE_P);
#endif
#if V(7)
            DO_BIT("must-tail-call", CALL_EXPR_MUST_TAIL_CALL);
#endif
        }
        if (code == IDENTIFIER_NODE)
            DO_BIT("symbol-referenced", TREE_SYMBOL_REFERENCED);
        if (code == POINTER_TYPE || code == REFERENCE_TYPE)
            DO_BIT("ref-can-alias-all", TYPE_REF_CAN_ALIAS_ALL);
#if !V(4, 8)
        if (code == MODIFY_EXPR)
            DO_BIT("nontemporal", MOVE_NONTEMPORAL);
#endif
        if (code == ASM_EXPR)
            DO_BIT("asm-input", ASM_INPUT_P);
        if (code == TREE_BINFO)
            DO_BIT("virtual-base", BINFO_VIRTUAL_P);
#if V(4, 6)
        if (code == ENUMERAL_TYPE)
            DO_BIT("is-scoped", ENUM_IS_SCOPED);
#endif
#if V(4, 7)
        if (code == TRANSACTION_EXPR)
            DO_BIT("outer", TRANSACTION_EXPR_OUTER);
#endif
#if V(4, 9)
        if (code == SSA_NAME)
            DO_BIT("anti-range-p", SSA_NAME_ANTI_RANGE_P);
#endif
        // only if not cleared by the above
        DO_BIT("static", TREE_STATIC);

        // nowarning_flag
#if V(4, 7)
        if (TYPE_P(orig_tree))
            DO_BIT("artificial", TYPE_ARTIFICIAL);
#endif
        DO_BIT("no-warning", TREE_NO_WARNING);

        // public_flag
        if (CONSTANT_CLASS_P(orig_tree))
            DO_BIT("overflow", TREE_OVERFLOW);
        if (TYPE_P(orig_tree))
            DO_BIT("cached-values", TYPE_CACHED_VALUES_P);
        if (code == SAVE_EXPR)
            DO_BIT("resolved", SAVE_EXPR_RESOLVED_P);
        if (code == CALL_EXPR)
            DO_BIT("va-arg-pack", CALL_EXPR_VA_ARG_PACK);
        if (code == ASM_EXPR)
            DO_BIT("asm-volatile", ASM_VOLATILE_P);
#if V(4, 7)
        if (code == TRANSACTION_EXPR)
            DO_BIT("relaxed", TRANSACTION_EXPR_RELAXED);
#endif
#if V(4, 9)
        if (code == CONSTRUCTOR)
            DO_BIT("no-clearing", CONSTRUCTOR_NO_CLEARING);
#endif
#if V(7)
        if (code == SSA_NAME)
            DO_BIT("is-virtual-operand", SSA_NAME_IS_VIRTUAL_OPERAND);
#endif
        // public_flag (omp_code checks)
        if (omp_code == OMP_CLAUSE_PRIVATE)
            DO_BIT("omp-private-debug", OMP_CLAUSE_PRIVATE_DEBUG);
        if (omp_code == OMP_CLAUSE_LASTPRIVATE)
            DO_BIT("omp-lastprivate-firstprivate", OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE);
#if V(4, 9)
        if (omp_code == OMP_CLAUSE_MAP)
            DO_BIT("zero_bias_array_section", OMP_CLAUSE_MAP_ZERO_BIAS_ARRAY_SECTION);
        if (omp_code == OMP_CLAUSE_REDUCTION)
            DO_BIT("omp-orig-ref", OMP_CLAUSE_REDUCTION_OMP_ORIG_REF);
        if (omp_code == OMP_CLAUSE_LINEAR)
            DO_BIT("no-copyin", OMP_CLAUSE_LINEAR_NO_COPYIN);
#endif
#if V(6)
        if (omp_code == OMP_CLAUSE_FIRSTPRIVATE)
            DO_BIT("implicit", OMP_CLAUSE_FIRSTPRIVATE_IMPLICIT);
        if (omp_code == OMP_CLAUSE_SHARED)
            DO_BIT("readonly", OMP_CLAUSE_SHARED_READONLY);
        if (omp_code == OMP_CLAUSE_DEPEND)
            DO_BIT("sink-negative", OMP_CLAUSE_DEPEND_SINK_NEGATIVE);
        if (omp_code == OMP_CLAUSE_SCHEDULE)
            DO_BIT("simd", OMP_CLAUSE_SCHEDULE_SIMD);
#endif
        // only if not cleared by the above
        DO_BIT("public", TREE_PUBLIC);

        // side_effects_flag
        if (code == LABEL_DECL)
            DO_BIT("forced", FORCED_LABEL);
        if (!TYPE_P(orig_tree))
            DO_BIT("side-effects", TREE_SIDE_EFFECTS);

        // volatile_flag
        if (!TYPE_P(orig_tree))
            DO_BIT("volatile", TREE_THIS_VOLATILE);
        else
            DO_BIT("volatile", TYPE_VOLATILE);

        // nothrow_flag
        if (INDIRECT_REF_P(orig_tree)
#if V(4, 6)
            || code == MEM_REF
#endif
        )
            DO_BIT("no-trap", TREE_THIS_NOTRAP);
        if (code == ARRAY_REF || code == ARRAY_RANGE_REF)
            DO_BIT("in-bounds", TREE_THIS_NOTRAP);
#if !V(7)
        if (TYPE_P(orig_tree))
            DO_BIT("align-ok", TYPE_ALIGN_OK);
#endif
        if (code == CALL_EXPR || code == FUNCTION_DECL)
            DO_BIT("nothrow", TREE_NOTHROW);
        if (code == SSA_NAME)
            DO_BIT("freelist", SSA_NAME_IN_FREE_LIST);
#if V(4, 9)
        if (code == VAR_DECL)
            DO_BIT("nonaliased", DECL_NONALIASED);
#endif

        // readonly_flag
        if (code == FUNCTION_DECL)
            DO_BIT("const-fn", TREE_READONLY);
        if (!TYPE_P(orig_tree))
            DO_BIT("rvalue", TREE_READONLY);
        else
            DO_BIT("const", TYPE_READONLY);

        // const_flag
        if (!TYPE_P(orig_tree))
            DO_BIT("constant", TREE_CONSTANT);
        if (TYPE_P(orig_tree))
            DO_BIT("sizes-gimplified", TYPE_SIZES_GIMPLIFIED);

        // unsigned_flag
        if (CODE_CONTAINS_STRUCT(code, TS_DECL_COMMON))
            DO_BIT("unsigned", DECL_UNSIGNED);
        if (TYPE_P(orig_tree))
            DO_BIT("unsigned", TYPE_UNSIGNED);

        // asm_written_flag
        if (code == SSA_NAME)
            DO_BIT("in-abnormal-phi", SSA_NAME_OCCURS_IN_ABNORMAL_PHI);
        DO_BIT("asm-written", TREE_ASM_WRITTEN);

        // used_flag
        DO_BIT("used", TREE_USED);

        // private_flag
        if (code == CALL_EXPR)
            DO_BIT("return-slot-opt", CALL_EXPR_RETURN_SLOT_OPT);
        if (code == OMP_SECTION)
            DO_BIT("section-last", OMP_SECTION_LAST);
        if (code == OMP_PARALLEL)
            DO_BIT("combined", OMP_PARALLEL_COMBINED);
#if V(4, 6)
        if (code == ENUMERAL_TYPE)
            DO_BIT("is-opaque", ENUM_IS_OPAQUE);
        if (code == REFERENCE_TYPE)
            DO_BIT("is-rvalue", TYPE_REF_IS_RVALUE);
#endif
#if V(4, 9)
        if (OMP_ATOMIC <= code && code <= OMP_ATOMIC_CAPTURE_NEW)
            DO_BIT("sequentially-consistent", OMP_ATOMIC_SEQ_CST);
#endif
#if V(5)
#if !V(6)
        if (code == OACC_KERNELS)
            DO_BIT("combined", OACC_KERNELS_COMBINED);
        if (code == OACC_PARALLEL)
            DO_BIT("combined", OACC_PARALLEL_COMBINED);
#endif
        if (code == OMP_TEAMS)
            DO_BIT("combined", OMP_TEAMS_COMBINED);
#endif
#if V(6)
        if (code == OMP_TARGET)
            DO_BIT("combined", OMP_TARGET_COMBINED);
#endif
#if V(7)
        if (code == LABEL_DECL)
            DO_BIT("fallthrough", FALLTHROUGH_LABEL_P);
#endif
        // private_flag (omp_code checks)
        if (omp_code == OMP_CLAUSE_PRIVATE)
            DO_BIT("omp-private-outer-ref", OMP_CLAUSE_PRIVATE_OUTER_REF);
#if V(4, 9)
        if (omp_code == OMP_CLAUSE_LINEAR)
            DO_BIT("no-copyout", OMP_CLAUSE_LINEAR_NO_COPYOUT);
#endif
#if V(6)
        if (omp_code == OMP_CLAUSE_MAP)
            DO_BIT("in-reduction", OMP_CLAUSE_MAP_IN_REDUCTION);
#endif
        DO_BIT("private", TREE_PRIVATE);

        // decl_by_reference_flag
        if (code == VAR_DECL || code == PARM_DECL || code == RESULT_DECL)
        {
            DO_BIT("by-ref", DECL_BY_REFERENCE);
#if !V(4, 8)
            DO_BIT("restricted", DECL_RESTRICTED_P);
#endif
        }

        // protected_flag
        if (code == CALL_EXPR)
        {
#if V(4, 6)
            if (tree fn_p = CALL_EXPR_FN(orig_tree))
            {
                if (TREE_CODE(fn_p) == ADDR_EXPR)
                {
                    tree fn = TREE_OPERAND(fn_p, 0);
                    if (DECL_FUNCTION_CODE(fn) == BUILT_IN_ALLOCA)
                        DO_BIT("alloca-for-variable-sized", CALL_ALLOCA_FOR_VAR_P);
                }
            }
#endif
            DO_BIT("from-thunk", CALL_FROM_THUNK_P);
        }
        // protected_flag (omp_code checks)
#if V(4, 9)
        if (omp_code == OMP_CLAUSE_LINEAR)
            DO_BIT("variable-stride", OMP_CLAUSE_LINEAR_VARIABLE_STRIDE);
#endif
#if V(6)
        if (omp_code == OMP_CLAUSE_PRIVATE)
            DO_BIT("taskloop-iv", OMP_CLAUSE_PRIVATE_TASKLOOP_IV);
        if (omp_code == OMP_CLAUSE_LASTPRIVATE)
            DO_BIT("taskloop-iv", OMP_CLAUSE_LASTPRIVATE_TASKLOOP_IV);
        if (omp_code == OMP_CLAUSE_MAP)
            DO_BIT("maybe-zero-length-array-section", OMP_CLAUSE_MAP_MAYBE_ZERO_LENGTH_ARRAY_SECTION);
#endif
        DO_BIT("protected", TREE_PROTECTED);

        // deprecated_flag
        if (code == IDENTIFIER_NODE)
            DO_BIT("transparent-alias", IDENTIFIER_TRANSPARENT_ALIAS);
#if V(5)
        if (code == CALL_EXPR)
            DO_BIT("with-bounds", CALL_WITH_BOUNDS_P);
#endif
        // deprecated_flag (omp_code checks)
#if V(4, 9)
        if (omp_code == OMP_CLAUSE_LINEAR)
            DO_BIT("array", OMP_CLAUSE_LINEAR_ARRAY);
#endif
        DO_BIT("deprecated", TREE_DEPRECATED);

        // saturating_flag
#if V(6)
        if (code == RECORD_TYPE || code == UNION_TYPE || code == QUAL_UNION_TYPE || code == ARRAY_TYPE)
            DO_BIT("reverse-storage-order", TYPE_REVERSE_STORAGE_ORDER);
        else
#endif
        {
            if (
#if !V(4, 8)
                    1 ||
#endif
                    TYPE_P(orig_tree))
                DO_BIT("saturating", TYPE_SATURATING);
        }

        // default_def_flag
        if (code == SSA_NAME)
            DO_BIT("is-default-definition", SSA_NAME_IS_DEFAULT_DEF);
        if (code == VECTOR_TYPE)
            DO_BIT("opaque", TYPE_VECTOR_OPAQUE);
#if V(4, 9)
        if (RECORD_OR_UNION_TYPE_P(orig_tree))
            DO_BIT("final", TYPE_FINAL_P);
#endif
#if V(6)
        if (code == BIT_FIELD_REF || code == MEM_REF)
            DO_BIT("reverse-storage-order", REF_REVERSE_STORAGE_ORDER);
#endif
#if V(7)
        if (code == ADDR_EXPR)
            DO_BIT("by-descriptor", FUNC_ADDR_BY_DESCRIPTOR);
        if (code == CALL_EXPR)
            DO_BIT("by-descriptor", CALL_EXPR_BY_DESCRIPTOR);
#endif

        // restrict_flag
        if (TYPE_P(orig_tree))
            DO_BIT("restrict", TYPE_RESTRICT);

        // visited_flag
        DO_BIT("visited", TREE_VISITED);

        // packed_flag
        if (TYPE_P(orig_tree))
            DO_BIT("packed", TYPE_PACKED);
        if (code == FIELD_DECL)
            DO_BIT("packed", DECL_PACKED);

        // nameless_flag
#if V(4, 6)
        if (TYPE_P(orig_tree))
            DO_BIT("nameless", TYPE_NAMELESS);
        if (CODE_CONTAINS_STRUCT(code, TS_DECL_MINIMAL))
            DO_BIT("nameless", DECL_NAMELESS);
#endif
#if V(4, 8)
        if (code == BLOCK)
            DO_BIT("same-range", BLOCK_SAME_RANGE);
#endif

        // atomic_flag
#if V(4, 9)
        if (TYPE_P(orig_tree))
            DO_BIT("atomic", TYPE_ATOMIC);
#endif
#if V(7)
        if (code == BLOCK)
            DO_BIT("cold", BLOCK_IN_COLD_SECTION_P);
#endif

        if (code == TREE_BINFO)
        {
#if !V(6)
            DO_BIT("binfo-marked", BINFO_MARKED);
#else
            DO_BIT("binfo-flag-0", BINFO_FLAG_0);
#endif
            DO_BIT("binfo-flag-1", BINFO_FLAG_1);
            DO_BIT("binfo-flag-2", BINFO_FLAG_2);
            DO_BIT("binfo-flag-3", BINFO_FLAG_3);
            DO_BIT("binfo-flag-4", BINFO_FLAG_4);
            DO_BIT("binfo-flag-5", BINFO_FLAG_5);
            DO_BIT("binfo-flag-6", BINFO_FLAG_6);
        }
        if (V(4, 8) && (code == TREE_VEC || code == SSA_NAME))
            ;
        else
        {
            switch (vomitorium_current_frontend)
            {
            default:
                DO_BIT("lang-flag-0", TREE_LANG_FLAG_0);
                DO_BIT("lang-flag-1", TREE_LANG_FLAG_1);
                DO_BIT("lang-flag-2", TREE_LANG_FLAG_2);
                DO_BIT("lang-flag-3", TREE_LANG_FLAG_3);
                DO_BIT("lang-flag-4", TREE_LANG_FLAG_4);
                DO_BIT("lang-flag-5", TREE_LANG_FLAG_5);
                DO_BIT("lang-flag-6", TREE_LANG_FLAG_6);
            }
        }
    } // flags

    if (code == INTEGER_CST)
    {
#if !V(5)
        DO_VAL("int", TREE_INT_CST);
#else
#if !V(8)
        xml1("int", wide_int_ref(orig_tree));
#else
        xml1("int", wide_int_ref(wi::to_wide(orig_tree)));
#endif
        DO_VAL("nunits", TREE_INT_CST_NUNITS);
        // this one is important for TREE_INT_CST_ELT checks
        int extended = TREE_INT_CST_EXT_NUNITS(orig_tree);
        xml1("ext-nunits", extended);
        DO_VAL("offset-nunits", TREE_INT_CST_OFFSET_NUNITS);
        for (int i = 0; i < extended; ++i)
            TREE_INT_CST_ELT(bitmask_tree, i) = 0;
        TREE_INT_CST_EXT_NUNITS(bitmask_tree) = 0;
#endif
    }
    if (code == REAL_CST)
    {
        DO_VAL("real", TREE_REAL_CST_PTR);
    }
    if (code == FIXED_CST)
    {
        DO_VAL("fixed", TREE_FIXED_CST_PTR);
    }
    if (code == STRING_CST)
    {
        int length = TAKE1(TREE_STRING_LENGTH);
        char *str = jiggle(bitmask_tree, TREE_STRING_POINTER(bitmask_tree));
        xml1("length", length);
        Xml xml("hex");
        xemit_hex(str, length);
        memset(str, 0, length);
    }
    if (code == COMPLEX_CST)
    {
        DO_VAL("real", TREE_REALPART);
        DO_VAL("imag", TREE_IMAGPART);
    }
    if (code == VECTOR_CST)
    {
#if !V(4, 8)
        DO_VAL("elements", TREE_VECTOR_CST_ELTS);
#else
        size_t len = VECTOR_CST_NELTS(CONST_CAST_TREE(orig_tree));
#if V(8)
        VECTOR_CST_NELTS(bitmask_tree) = 0;
#endif
        Xml xml("vector-elements");
        for (size_t i = 0; i < len; ++i)
        {
            Xml xml("e", "i", i);
            xemit(TAKE_I(VECTOR_CST_ELT, i));
        }
#endif
    }

    if (code == IDENTIFIER_NODE)
    {
        (void)TAKE1(IDENTIFIER_LENGTH);
        // unfortunately, `jiggle` doesn't work here
        const char *ptr = IDENTIFIER_POINTER(orig_tree); IDENTIFIER_NODE_CHECK(bitmask_tree)->identifier.id.str = nullptr;
        unsigned int hash_value = TAKE1(IDENTIFIER_HASH_VALUE);
        {
            Xml xml("name", "hash", hash_value);
            xemit(ptr);
        }
        switch (vomitorium_current_frontend)
        {
        case VOMITORIUM_FRONTEND_C:
            {
                auto orig_c_id = (const struct c_lang_identifier *)orig_tree;
                auto bitmask_c_id = (struct c_lang_identifier *)bitmask_tree;
                dump_cci(&orig_c_id->common_id, &bitmask_c_id->common_id);
                DO_LVAL("symbol-binding", bitmask_c_id->symbol_binding);
                DO_LVAL("tag-binding", bitmask_c_id->tag_binding);
                DO_LVAL("label-binding", bitmask_c_id->label_binding);
            }
            break;
        case VOMITORIUM_FRONTEND_OBJC:
            // maybe identical to C?
            break;
        case VOMITORIUM_FRONTEND_CXX:
            {
                auto orig_cxx_id = (const struct cxx_lang_identifier *)orig_tree;
                auto bitmask_cxx_id = (struct cxx_lang_identifier *)bitmask_tree;
                dump_cci(&orig_cxx_id->c_common, &bitmask_cxx_id->c_common);
                DO_LVAL("bindings", bitmask_cxx_id->bindings);
#if !V(8)
                DO_LVAL("namespace-bindings", bitmask_cxx_id->namespace_bindings);
                DO_LVAL("class-template-info", bitmask_cxx_id->class_template_info);
#endif
                DO_LVAL("label-value", bitmask_cxx_id->label_value);
            }
            break;
        case VOMITORIUM_FRONTEND_OBJCXX:
            // probably identical to CXX?
            break;
        default:
            break;
        }
    }

    if (code == TREE_LIST)
    {
        DO_VAL("purpose", TREE_PURPOSE);
        DO_VAL("value", TREE_VALUE);
    }

    if (code == CONSTRUCTOR)
    {
        VEC(constructor_elt, gc) *elts = TAKE1(CONSTRUCTOR_ELTS);
        foreach (const constructor_elt& e, elts)
        {
            // Don't need to TAKE these since they're indirect.
            xml1("index", e.index);
            xml1("value", e.value);
        }
    }

#if V(5)
    if (code == MEM_REF || code == TARGET_MEM_REF)
    {
        DO_VAL("dependence-clique", MR_DEPENDENCE_CLIQUE);
        DO_VAL("dependence-base", MR_DEPENDENCE_BASE);
    }
#endif

    if (EXPR_P(orig_tree))
    {
        const ptrdiff_t len = TREE_OPERAND_LENGTH(orig_tree);
        const char *operand_names[len];
        memset(operand_names, 0, sizeof(operand_names));
        int offset = 0;
        if (VL_EXP_CLASS_P(orig_tree))
            operand_names[0] = "vl-operand-count";

#define CALC_OPERAND_NAME(name, LVALUE)                         \
        ({                                                      \
            auto t = CONST_CAST_TREE(orig_tree);                \
            ptrdiff_t _i = &LVALUE(t) - &TREE_OPERAND(t, 0);    \
            assert (0 <= _i && _i < len);                       \
            assert (operand_names[_i] == nullptr);              \
            operand_names[_i] = name;                           \
        })
        switch (code)
        {
        case LOOP_EXPR:
            CALC_OPERAND_NAME("body", LOOP_EXPR_BODY);
            break;
        case TARGET_EXPR:
            CALC_OPERAND_NAME("slot", TARGET_EXPR_SLOT);
            CALC_OPERAND_NAME("initial", TARGET_EXPR_INITIAL);
            CALC_OPERAND_NAME("cleanup", TARGET_EXPR_CLEANUP);
            break;
        case DECL_EXPR:
            CALC_OPERAND_NAME("decl", DECL_EXPR_DECL);
            break;
        case EXIT_EXPR:
            CALC_OPERAND_NAME("cond", EXIT_EXPR_COND);
            break;
        case COMPOUND_LITERAL_EXPR:
            CALC_OPERAND_NAME("decl-expr", COMPOUND_LITERAL_EXPR_DECL_EXPR);
            break;
        case SWITCH_EXPR:
            CALC_OPERAND_NAME("cond", SWITCH_COND);
            CALC_OPERAND_NAME("body", SWITCH_BODY);
            CALC_OPERAND_NAME("labels", SWITCH_LABELS);
            break;
        case CASE_LABEL_EXPR:
            CALC_OPERAND_NAME("low", CASE_LOW);
            CALC_OPERAND_NAME("high", CASE_HIGH);
            CALC_OPERAND_NAME("label", CASE_LABEL);
#if V(4, 7)
            CALC_OPERAND_NAME("chain", CASE_CHAIN);
#endif
            break;
        case TARGET_MEM_REF:
#if !V(4, 6)
            CALC_OPERAND_NAME("symbol", TMR_SYMBOL);
#endif
            CALC_OPERAND_NAME("base", TMR_BASE);
            CALC_OPERAND_NAME("index", TMR_INDEX);
            CALC_OPERAND_NAME("step", TMR_STEP);
            CALC_OPERAND_NAME("offset", TMR_OFFSET);
#if !V(4, 6)
            CALC_OPERAND_NAME("original", TMR_ORIGINAL);
#else
            CALC_OPERAND_NAME("index2", TMR_INDEX2);
#endif
            break;
        case BIND_EXPR:
            CALC_OPERAND_NAME("vars", BIND_EXPR_VARS);
            CALC_OPERAND_NAME("body", BIND_EXPR_BODY);
            CALC_OPERAND_NAME("block", BIND_EXPR_BLOCK);
            break;
        case GOTO_EXPR:
            CALC_OPERAND_NAME("destination", GOTO_DESTINATION);
            break;
        case ASM_EXPR:
            CALC_OPERAND_NAME("string", ASM_STRING);
            CALC_OPERAND_NAME("outputs", ASM_OUTPUTS);
            CALC_OPERAND_NAME("inputs", ASM_INPUTS);
            CALC_OPERAND_NAME("clobbers", ASM_CLOBBERS);
            CALC_OPERAND_NAME("labels", ASM_LABELS);
            break;
        case COND_EXPR:
            CALC_OPERAND_NAME("cond", COND_EXPR_COND);
            CALC_OPERAND_NAME("if-true", COND_EXPR_THEN);
            CALC_OPERAND_NAME("if-false", COND_EXPR_ELSE);
            break;
        case POLYNOMIAL_CHREC:
#if !V(8)
            CALC_OPERAND_NAME("var", CHREC_VAR);
#endif
            CALC_OPERAND_NAME("left", CHREC_LEFT);
            CALC_OPERAND_NAME("right", CHREC_RIGHT);
            break;
        case LABEL_EXPR:
            CALC_OPERAND_NAME("label", LABEL_EXPR_LABEL);
            break;
        // VDEF_EXPR does not exist, despite comments citing tree-flow.h
        case CATCH_EXPR:
            CALC_OPERAND_NAME("types", CATCH_TYPES);
            CALC_OPERAND_NAME("body", CATCH_BODY);
            break;
        case EH_FILTER_EXPR:
            CALC_OPERAND_NAME("types", EH_FILTER_TYPES);
            CALC_OPERAND_NAME("failure", EH_FILTER_FAILURE);
            break;
        case OBJ_TYPE_REF:
            CALC_OPERAND_NAME("expr", OBJ_TYPE_REF_EXPR);
            CALC_OPERAND_NAME("object", OBJ_TYPE_REF_OBJECT);
            CALC_OPERAND_NAME("token", OBJ_TYPE_REF_TOKEN);
            break;
        case ASSERT_EXPR:
            CALC_OPERAND_NAME("var", ASSERT_EXPR_VAR);
            CALC_OPERAND_NAME("cond", ASSERT_EXPR_COND);
            break;
        case CALL_EXPR:
            CALC_OPERAND_NAME("fn", CALL_EXPR_FN);
            CALC_OPERAND_NAME("static-chain", CALL_EXPR_STATIC_CHAIN);
            offset = len - call_expr_nargs(orig_tree);
#if V(5)
            DO_VAL("internal-function", CALL_EXPR_IFN);
#endif
            break;
#if V(4, 7)
        case TRANSACTION_EXPR:
            CALC_OPERAND_NAME("body", TRANSACTION_EXPR_BODY);
            break;
#endif
#if V(4, 9)
        case CILK_SPAWN_STMT:
            CALC_OPERAND_NAME("spawn-function", CILK_SPAWN_FN);
            break;
#endif

        // Below this are all the OMP tree codes (*not* clause codes)
#if V(5)
#if !V(6)
        case OACC_PARALLEL:
            CALC_OPERAND_NAME("body", OACC_PARALLEL_BODY);
            CALC_OPERAND_NAME("clauses", OACC_PARALLEL_CLAUSES);
            break;
        case OACC_KERNELS:
            CALC_OPERAND_NAME("body", OACC_KERNELS_BODY);
            CALC_OPERAND_NAME("clauses", OACC_KERNELS_CLAUSES);
            break;
#endif
        case OACC_DATA:
            CALC_OPERAND_NAME("body", OACC_DATA_BODY);
            CALC_OPERAND_NAME("clauses", OACC_DATA_CLAUSES);
            break;
        case OACC_HOST_DATA:
            CALC_OPERAND_NAME("body", OACC_HOST_DATA_BODY);
            CALC_OPERAND_NAME("clauses", OACC_HOST_DATA_CLAUSES);
            break;
        case OACC_CACHE:
            CALC_OPERAND_NAME("clauses", OACC_CACHE_CLAUSES);
            break;
        case OACC_DECLARE:
            CALC_OPERAND_NAME("clauses", OACC_DECLARE_CLAUSES);
            break;
        case OACC_ENTER_DATA:
            CALC_OPERAND_NAME("clauses", OACC_ENTER_DATA_CLAUSES);
            break;
        case OACC_EXIT_DATA:
            CALC_OPERAND_NAME("clauses", OACC_EXIT_DATA_CLAUSES);
            break;
        case OACC_UPDATE:
            CALC_OPERAND_NAME("clauses", OACC_UPDATE_CLAUSES);
            break;
#endif
        case OMP_PARALLEL:
            CALC_OPERAND_NAME("body", OMP_PARALLEL_BODY);
            CALC_OPERAND_NAME("clauses", OMP_PARALLEL_CLAUSES);
            break;
        case OMP_TASK:
            CALC_OPERAND_NAME("body", OMP_TASK_BODY);
            CALC_OPERAND_NAME("clauses", OMP_TASK_CLAUSES);
            break;
        case OMP_FOR:
            CALC_OPERAND_NAME("body", OMP_FOR_BODY);
            CALC_OPERAND_NAME("clauses", OMP_FOR_CLAUSES);
            CALC_OPERAND_NAME("init", OMP_FOR_INIT);
            CALC_OPERAND_NAME("cond", OMP_FOR_COND);
            CALC_OPERAND_NAME("incr", OMP_FOR_INCR);
            CALC_OPERAND_NAME("pre-body", OMP_FOR_PRE_BODY);
#if V(6)
            CALC_OPERAND_NAME("orig-decls", OMP_FOR_ORIG_DECLS);
#endif
            break;
        case OMP_SECTIONS:
            CALC_OPERAND_NAME("body", OMP_SECTIONS_BODY);
            CALC_OPERAND_NAME("clauses", OMP_SECTIONS_CLAUSES);
            break;
        case OMP_SECTION:
            CALC_OPERAND_NAME("body", OMP_SECTION_BODY);
            break;
        case OMP_SINGLE:
            CALC_OPERAND_NAME("body", OMP_SINGLE_BODY);
            CALC_OPERAND_NAME("clauses", OMP_SINGLE_CLAUSES);
            break;
        case OMP_MASTER:
            CALC_OPERAND_NAME("body", OMP_MASTER_BODY);
            break;
#if V(4, 9)
        case OMP_TASKGROUP:
            CALC_OPERAND_NAME("body", OMP_TASKGROUP_BODY);
            break;
#endif
        case OMP_ORDERED:
            CALC_OPERAND_NAME("body", OMP_ORDERED_BODY);
#if V(6)
            CALC_OPERAND_NAME("clauses", OMP_ORDERED_CLAUSES);
#endif
            break;
        case OMP_CRITICAL:
            CALC_OPERAND_NAME("body", OMP_CRITICAL_BODY);
#if V(6)
            CALC_OPERAND_NAME("clauses", OMP_CRITICAL_CLAUSES);
#endif
            CALC_OPERAND_NAME("name", OMP_CRITICAL_NAME);
            break;
#if V(4, 9)
        case OMP_TEAMS:
            CALC_OPERAND_NAME("body", OMP_TEAMS_BODY);
            CALC_OPERAND_NAME("clauses", OMP_TEAMS_CLAUSES);
            break;
        case OMP_TARGET_DATA:
            CALC_OPERAND_NAME("body", OMP_TARGET_DATA_BODY);
            CALC_OPERAND_NAME("clauses", OMP_TARGET_DATA_CLAUSES);
            break;
        case OMP_TARGET:
            CALC_OPERAND_NAME("body", OMP_TARGET_BODY);
            CALC_OPERAND_NAME("clauses", OMP_TARGET_CLAUSES);
            break;
        case OMP_TARGET_UPDATE:
            CALC_OPERAND_NAME("update-clauses", OMP_TARGET_UPDATE_CLAUSES);
            break;
#endif
#if V(6)
        case OMP_TARGET_ENTER_DATA:
            CALC_OPERAND_NAME("clauses", OMP_TARGET_ENTER_DATA_CLAUSES);
            break;
        case OMP_TARGET_EXIT_DATA:
            CALC_OPERAND_NAME("clauses", OMP_TARGET_EXIT_DATA_CLAUSES);
            break;
#endif
        default:
            break;
        }
#undef CALC_OPERAND_NAME

        if (EXPR_HAS_LOCATION(orig_tree))
        {
            xml1("location", expand_location(TAKE2(EXPR_LOCATION, SET_EXPR_LOCATION)));
#if V(6)
            xml1("range", EXPR_LOCATION_RANGE(CONST_CAST_TREE(orig_tree)));
#endif
        }

        if (offset)
            xml1("operand-offset", offset);
        for (int i = 0; i < len; ++i)
        {
            tree t_o = TREE_OPERAND(orig_tree, i);
            Xml xml(operand_names[i] ?: "operand", "op_i", i);
            xemit(t_o);
            if (t_o)
            {
                if (i != 0 || !VL_EXP_CLASS_P(orig_tree))
                    TREE_OPERAND(bitmask_tree, i) = NULL_TREE;
            }
        }
        if (VL_EXP_CLASS_P(orig_tree))
            TREE_OPERAND(bitmask_tree, 0) = NULL_TREE;
    }

    // this is basically a duplicate of the EXPR_P logic above
    if (code == OMP_CLAUSE)
    {
        const ptrdiff_t len = omp_clause_num_ops[code];
        const char *operand_names[len];
        memset(operand_names, 0, sizeof(operand_names));

#define CALC_OMP_OPERAND_NAME(name, LVALUE)                         \
        ({                                                          \
            auto t = orig_tree;                                     \
            ptrdiff_t _i = &LVALUE(t) - &OMP_CLAUSE_OPERAND(t, 0);  \
            assert (0 <= _i && _i < len);                           \
            assert (operand_names[_i] == nullptr);                  \
            operand_names[_i] = name;                               \
        })
        if (OMP_CLAUSE_HAS_LOCATION(orig_tree))
            xml1("location", expand_location(TAKE1(OMP_CLAUSE_LOCATION)));
        xml1("clause-code", omp_code);

        if (omp_code >= OMP_CLAUSE_PRIVATE && omp_code <=
#if !V(4, 9)
                OMP_CLAUSE_COPYPRIVATE
#else
                OMP_CLAUSE__LOOPTEMP_
#endif
        )
        {
            CALC_OMP_OPERAND_NAME("decl", OMP_CLAUSE_DECL);
        }
#if V(4, 9)
        if (omp_code >= OMP_CLAUSE_FROM && omp_code <=
#if !V(5)
                OMP_CLAUSE_MAP
#else
                OMP_CLAUSE__CACHE_
#endif
        )
        {
            CALC_OMP_OPERAND_NAME("size", OMP_CLAUSE_SIZE);
        }
#endif
        switch (omp_code)
        {
        case OMP_CLAUSE_LASTPRIVATE:
            CALC_OMP_OPERAND_NAME("stmt", OMP_CLAUSE_LASTPRIVATE_STMT);
            CDO_VAL("gimple-seq", OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ);
            break;
        case OMP_CLAUSE_IF:
#if V(6)
            DO_VAL("subcode-if-modifier", OMP_CLAUSE_IF_MODIFIER);
#endif
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_IF_EXPR);
            break;
        case OMP_CLAUSE_NUM_THREADS:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_NUM_THREADS_EXPR);
            break;
        case OMP_CLAUSE_SCHEDULE:
            DO_VAL("subcode-schedule", OMP_CLAUSE_SCHEDULE_KIND);
            CALC_OMP_OPERAND_NAME("chunk-expr", OMP_CLAUSE_SCHEDULE_CHUNK_EXPR);
            break;
        case OMP_CLAUSE_COLLAPSE:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_COLLAPSE_EXPR);
            CALC_OMP_OPERAND_NAME("itervar", OMP_CLAUSE_COLLAPSE_ITERVAR);
            CALC_OMP_OPERAND_NAME("count", OMP_CLAUSE_COLLAPSE_COUNT);
            break;
        case OMP_CLAUSE_REDUCTION:
            DO_VAL("subcode-reduction", OMP_CLAUSE_REDUCTION_CODE);
            CALC_OMP_OPERAND_NAME("init", OMP_CLAUSE_REDUCTION_INIT);
            CALC_OMP_OPERAND_NAME("merge", OMP_CLAUSE_REDUCTION_MERGE);
            CDO_VAL("gimple-init", OMP_CLAUSE_REDUCTION_GIMPLE_INIT);
            CDO_VAL("gimple-merge", OMP_CLAUSE_REDUCTION_GIMPLE_MERGE);
            CALC_OMP_OPERAND_NAME("placeholder", OMP_CLAUSE_REDUCTION_PLACEHOLDER);
#if V(6)
            CALC_OMP_OPERAND_NAME("decl-placeholder", OMP_CLAUSE_REDUCTION_DECL_PLACEHOLDER);
#endif
            break;
        case OMP_CLAUSE_DEFAULT:
            DO_VAL("subcode-default", OMP_CLAUSE_DEFAULT_KIND);
            break;
        case OMP_CLAUSE_ORDERED:
#if V(6)
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_ORDERED_EXPR);
#endif
            break;
        case OMP_CLAUSE_ERROR:
        case OMP_CLAUSE_PRIVATE:
        case OMP_CLAUSE_SHARED:
        case OMP_CLAUSE_FIRSTPRIVATE:
        case OMP_CLAUSE_COPYIN:
        case OMP_CLAUSE_COPYPRIVATE:
        case OMP_CLAUSE_NOWAIT:
        case OMP_CLAUSE_UNTIED:
            break;
#if V(4, 7)
        case OMP_CLAUSE_FINAL:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_FINAL_EXPR);
            break;
        case OMP_CLAUSE_MERGEABLE:
            break;
#endif
#if V(4, 9)
        case OMP_CLAUSE_DEPEND:
            DO_VAL("subcode-depend", OMP_CLAUSE_DEPEND_KIND);
            break;
        case OMP_CLAUSE_MAP:
            DO_VAL2("subcode-map", OMP_CLAUSE_MAP_KIND, OMP_CLAUSE_SET_MAP_KIND);
            break;
        case OMP_CLAUSE_PROC_BIND:
            DO_VAL("subcode-proc-bind", OMP_CLAUSE_PROC_BIND_KIND);
            break;
        case OMP_CLAUSE_LINEAR:
#if V(6)
            DO_VAL("subcode-linear", OMP_CLAUSE_LINEAR_KIND);
#endif
            CALC_OMP_OPERAND_NAME("step", OMP_CLAUSE_LINEAR_STEP);
            CALC_OMP_OPERAND_NAME("statement", OMP_CLAUSE_LINEAR_STMT);
            CDO_VAL("gimple-seq", OMP_CLAUSE_LINEAR_GIMPLE_SEQ);
            break;
        case OMP_CLAUSE_ALIGNED:
            CALC_OMP_OPERAND_NAME("alignment", OMP_CLAUSE_ALIGNED_ALIGNMENT);
            break;
        case OMP_CLAUSE_NUM_TEAMS:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_NUM_TEAMS_EXPR);
            break;
        case OMP_CLAUSE_THREAD_LIMIT:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_THREAD_LIMIT_EXPR);
            break;
        case OMP_CLAUSE_DEVICE:
            CALC_OMP_OPERAND_NAME("id", OMP_CLAUSE_DEVICE_ID);
            break;
        case OMP_CLAUSE_DIST_SCHEDULE:
            CALC_OMP_OPERAND_NAME("chunk-expr", OMP_CLAUSE_DIST_SCHEDULE_CHUNK_EXPR);
            break;
        case OMP_CLAUSE_SAFELEN:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_SAFELEN_EXPR);
            break;
        case OMP_CLAUSE_SIMDLEN:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_SIMDLEN_EXPR);
            break;
        case OMP_CLAUSE__SIMDUID_:
            CALC_OMP_OPERAND_NAME("decl", OMP_CLAUSE__SIMDUID__DECL);
            break;
        case OMP_CLAUSE_UNIFORM:
        case OMP_CLAUSE_FROM:
        case OMP_CLAUSE_TO:
        case OMP_CLAUSE__LOOPTEMP_:
        case OMP_CLAUSE_INBRANCH:
        case OMP_CLAUSE_NOTINBRANCH:
        case OMP_CLAUSE_FOR:
        case OMP_CLAUSE_PARALLEL:
        case OMP_CLAUSE_SECTIONS:
        case OMP_CLAUSE_TASKGROUP:
            break;
#endif
#if V(5)
        case OMP_CLAUSE_GANG:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_GANG_EXPR);
            CALC_OMP_OPERAND_NAME("static-expr", OMP_CLAUSE_GANG_STATIC_EXPR);
            break;
        case OMP_CLAUSE_ASYNC:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_ASYNC_EXPR);
            break;
        case OMP_CLAUSE_WAIT:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_WAIT_EXPR);
            break;
        case OMP_CLAUSE_VECTOR:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_VECTOR_EXPR);
            break;
        case OMP_CLAUSE_WORKER:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_WORKER_EXPR);
            break;
        case OMP_CLAUSE_NUM_GANGS:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_NUM_GANGS_EXPR);
            break;
        case OMP_CLAUSE_NUM_WORKERS:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_NUM_WORKERS_EXPR);
            break;
        case OMP_CLAUSE_VECTOR_LENGTH:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_VECTOR_LENGTH_EXPR);
            break;
#endif
#if V(5)
        case OMP_CLAUSE__CACHE_:
#if !V(7)
        case OMP_CLAUSE_DEVICE_RESIDENT:
#endif
#if !V(6)
        case OMP_CLAUSE_USE_DEVICE:
#endif
        case OMP_CLAUSE_AUTO:
        case OMP_CLAUSE_SEQ:
        case OMP_CLAUSE__CILK_FOR_COUNT_:
        case OMP_CLAUSE_INDEPENDENT:
            break;
#endif
#if V(6)
        case OMP_CLAUSE_NUM_TASKS:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_NUM_TASKS_EXPR);
            break;
        case OMP_CLAUSE_HINT:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_HINT_EXPR);
            break;
        case OMP_CLAUSE_GRAINSIZE:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_GRAINSIZE_EXPR);
            break;
        case OMP_CLAUSE_PRIORITY:
            CALC_OMP_OPERAND_NAME("expr", OMP_CLAUSE_PRIORITY_EXPR);
            break;
        case OMP_CLAUSE_TILE:
            CALC_OMP_OPERAND_NAME("list", OMP_CLAUSE_TILE_LIST);
#if V(7)
            CALC_OMP_OPERAND_NAME("itervar", OMP_CLAUSE_TILE_ITERVAR);
            CALC_OMP_OPERAND_NAME("count", OMP_CLAUSE_TILE_COUNT);
#endif
            break;
        case OMP_CLAUSE__GRIDDIM_:
            DO_VAL("subcode-dimension", OMP_CLAUSE__GRIDDIM__DIMENSION);
            CALC_OMP_OPERAND_NAME("size", OMP_CLAUSE__GRIDDIM__SIZE);
            CALC_OMP_OPERAND_NAME("group", OMP_CLAUSE__GRIDDIM__GROUP);
            break;
        case OMP_CLAUSE_TO_DECLARE:
        case OMP_CLAUSE_LINK:
        case OMP_CLAUSE_USE_DEVICE_PTR:
        case OMP_CLAUSE_IS_DEVICE_PTR:
        case OMP_CLAUSE_NOGROUP:
        case OMP_CLAUSE_THREADS:
        case OMP_CLAUSE_SIMD:
        case OMP_CLAUSE_DEFAULTMAP:
            break;
#endif
#if V(7)
        case OMP_CLAUSE__SIMT_:
            break;
#endif
        }
#undef CALC_OMP_OPERAND_NAME

        // OMP_CLAUSE_CHAIN is just TREE_CHAIN
        for (int i = 0; i < len; ++i)
        {
#define orig_tree CONST_CAST_TREE(orig_tree)
            tree t_o = TAKE_I(OMP_CLAUSE_OPERAND, i);
#undef orig_tree
            if (t_o)
            {
                Xml xml(operand_names[i] ?: "operand", "op_i", i);
                xemit(t_o);
            }
        }
    }

    if (code == SSA_NAME)
    {
        DO_VAL2("var", SSA_NAME_VAR_OR_IDENTIFIER, SET_SSA_NAME_VAR_OR_IDENTIFIER);
        DO_VAL("defining-statement", SSA_NAME_DEF_STMT);
        DO_VAL("version", SSA_NAME_VERSION);
        DO_VAL("pointer-info", SSA_NAME_PTR_INFO);
#if V(4, 9)
        DO_VAL("range-info", SSA_NAME_RANGE_INFO);
#endif
        // We don't need to track uses here; they can be recalculated
        Xml uses_xml("uses");

        imm_use_iterator imm_iter;
        use_operand_p use_p;
        FOR_EACH_IMM_USE_FAST (use_p, imm_iter, CONST_CAST_TREE(orig_tree))
        {
            xml1("use", *use_p->use);
            gimple_ptr use_stmt = USE_STMT(use_p);
            xml1("use-stmt", use_stmt);
        }
        (void)TAKE1(SSA_NAME_IMM_USE_NODE);
    }

    if (code == BLOCK)
    {
        DO_VAL("vars", BLOCK_VARS);
        foreach (tree nlv, TAKE1(BLOCK_NONLOCALIZED_VARS))
        {
            xml1("nonlocalized-var", nlv);
        }
        DO_VAL("subblocks", BLOCK_SUBBLOCKS);
        DO_VAL("supercontext", BLOCK_SUPERCONTEXT);
        // BLOCK_CHAIN exists, but is just TREE_CHAIN through 4.6
#if V(4, 7)
        DO_VAL("chain", BLOCK_CHAIN);
#endif
        DO_VAL("abstract-origin", BLOCK_ABSTRACT_ORIGIN);
        DO_BIT("abstract", BLOCK_ABSTRACT);
        DO_VAL("number", BLOCK_NUMBER);
        DO_VAL("fragment-origin", BLOCK_FRAGMENT_ORIGIN);
        DO_VAL("fragment-chain", BLOCK_FRAGMENT_CHAIN);
        xml1("source-location", expand_location(TAKE1(BLOCK_SOURCE_LOCATION)));
#if V(5)
        xml1("source-end-location", expand_location(TAKE1(BLOCK_SOURCE_LOCATION)));
#endif
#if V(6)
        DO_VAL("die", BLOCK_DIE);
#endif
    }

    if (TYPE_P(orig_tree))
    {
        DO_VAL("uid", TYPE_UID);
        DO_VAL("size", TYPE_SIZE);
        DO_VAL("size-unit", TYPE_SIZE_UNIT);
        if (code == ENUMERAL_TYPE)
            DO_VAL("enum-values", TYPE_VALUES);
        if (code == ARRAY_TYPE)
            DO_VAL("domain", TYPE_DOMAIN);
        if (RECORD_OR_UNION_TYPE_P(orig_tree))
        {
            DO_VAL("fields", TYPE_FIELDS);
#if !V(8)
            DO_VAL("methods", TYPE_METHODS);
#endif
            DO_VAL("vfield", TYPE_VFIELD);
        }
        if (code == FUNCTION_TYPE || code == METHOD_TYPE)
        {
            DO_VAL("arg-types", TYPE_ARG_TYPES);
            DO_VAL("method-basetype", TYPE_METHOD_BASETYPE);
        }
        if (code == OFFSET_TYPE)
            DO_VAL("offset-basetype", TYPE_OFFSET_BASETYPE);
        DO_VAL("pointer-to", TYPE_POINTER_TO);
        DO_VAL("reference-to", TYPE_REFERENCE_TO);
        if (code == POINTER_TYPE)
            DO_VAL("next-ptr-to", TYPE_NEXT_PTR_TO);
        if (code == REFERENCE_TYPE)
            DO_VAL("next-ref-to", TYPE_NEXT_REF_TO);
        if (code == INTEGER_TYPE || code == ENUMERAL_TYPE || code == BOOLEAN_TYPE || code == REAL_TYPE || code == FIXED_POINT_TYPE)
        {
            DO_VAL("min", TYPE_MIN_VALUE);
            DO_VAL("max", TYPE_MAX_VALUE);
        }
        if (code == VECTOR_TYPE)
        {
            xml1("vector-subparts", TYPE_VECTOR_SUBPARTS(orig_tree));
            SET_TYPE_VECTOR_SUBPARTS(bitmask_tree, 1);
        }
        CDO_VAL("precision", TYPE_PRECISION);
        auto symtab_field = DEBUG_HOOKS_TREE_TYPE_SYMTAB_FIELD;
        switch (symtab_field)
        {
        case TYPE_SYMTAB_IS_ADDRESS:
            DO_VAL("symtab-address", TYPE_SYMTAB_ADDRESS);
            break;
        case TYPE_SYMTAB_IS_POINTER:
            CDO_VAL("symtab-pointer", TYPE_SYMTAB_POINTER);
            break;
        case TYPE_SYMTAB_IS_DIE:
            DO_VAL("symtab-die", TYPE_SYMTAB_DIE);
            break;
        }
        DO_VAL("name", TYPE_NAME);
        DO_VAL("next-variant", TYPE_NEXT_VARIANT);
        DO_VAL("main-variant", TYPE_MAIN_VARIANT);
        DO_VAL("context", TYPE_CONTEXT);
        {
            // vector types need to look at parts of the real tree
            xml1("mode", TYPE_MODE(orig_tree));
            DO_VAL("mode-raw", TYPE_MODE_RAW);
        }
        DO_VAL("canonical", TYPE_CANONICAL);
        DO_VAL("lang-specific", TYPE_LANG_SPECIFIC);
#if !V(4, 6)
        if (code == VECTOR_TYPE)
            DO_VAL("debug-representation", TYPE_DEBUG_REPRESENTATION_TYPE);
#endif
        if (RECORD_OR_UNION_TYPE_P(orig_tree))
            DO_VAL("binfo", TYPE_BINFO);
        if (V(8) || !RECORD_OR_UNION_TYPE_P(orig_tree))
            DO_VAL("lang-slot-1", TYPE_LANG_SLOT_1);
        DO_VAL("alias-set", TYPE_ALIAS_SET);
        DO_VAL("attributes", TYPE_ATTRIBUTES);
        DO_VAL2("align", TYPE_ALIGN, SET_TYPE_ALIGN);
        DO_BIT("user-align", TYPE_USER_ALIGN);
#if V(8)
        DO_VAL2("warn-if-not-align", TYPE_WARN_IF_NOT_ALIGN, SET_TYPE_WARN_IF_NOT_ALIGN);
#endif
#if !V(4, 8)
        if (code == INTEGER_TYPE)
            DO_BIT("is-sizetype", TYPE_IS_SIZETYPE);
#endif
        DO_BIT("no-force-blk", TYPE_NO_FORCE_BLK);
        DO_VAL("address-space", TYPE_ADDR_SPACE);

        switch (vomitorium_current_frontend)
        {
        default:
            DO_BIT("type-lang-flag-0", TYPE_LANG_FLAG_0);
            DO_BIT("type-lang-flag-1", TYPE_LANG_FLAG_1);
            DO_BIT("type-lang-flag-2", TYPE_LANG_FLAG_2);
            DO_BIT("type-lang-flag-3", TYPE_LANG_FLAG_3);
            DO_BIT("type-lang-flag-4", TYPE_LANG_FLAG_4);
            DO_BIT("type-lang-flag-5", TYPE_LANG_FLAG_5);
            DO_BIT("type-lang-flag-6", TYPE_LANG_FLAG_6);
#if V(7)
            DO_BIT("type-lang-flag-7", TYPE_LANG_FLAG_7);
#endif
        }
        DO_BIT("is-string", TYPE_STRING_FLAG);
        if (code == ARRAY_TYPE)
            DO_VAL("array-max-size", TYPE_ARRAY_MAX_SIZE);
        DO_BIT("needs-constructing", TYPE_NEEDS_CONSTRUCTING);
        if (RECORD_OR_UNION_TYPE_P(orig_tree))
            DO_BIT("transparent-aggregate", TYPE_TRANSPARENT_AGGR);
        if (code == ARRAY_TYPE)
            DO_BIT("nonaliased-component", TYPE_NONALIASED_COMPONENT);
        DO_VAL("placeholder-internal", TYPE_CONTAINS_PLACEHOLDER_INTERNAL);
#if V(7)
        if (code == ARRAY_TYPE || code == RECORD_TYPE || code == UNION_TYPE || code == QUAL_UNION_TYPE)
            DO_BIT("typeless-storage", TYPE_TYPELESS_STORAGE);
#endif

        // generic fields used for multiple reasons
        CDO_VAL("cached-values", TYPE_CACHED_VALUES);
        CDO_VAL("maxval", TYPE_MAXVAL);
        CDO_VAL("minval", TYPE_MINVAL);
    }

    if (code == TREE_BINFO)
    {
        DO_VAL("offset", BINFO_OFFSET);
        DO_VAL("vtable", BINFO_VTABLE);
        DO_VAL("virtual-functions", BINFO_VIRTUALS);
        DO_VAL("vptr-field", BINFO_VPTR_FIELD);
        VEC(tree, gc) *base_accesses = TAKE1(BINFO_BASE_ACCESSES);
        if (!base_accesses)
            xml0("base-accesses-all-public");
        else
        {
            Xml base_accesses_xml("base-accesses");
            int i = 0;
            foreach (tree ba, base_accesses)
            {
                Xml xml("e", "i", i);
                xemit(ba);
                i += 1;
            }
        }
        DO_VAL("subvtt-index", BINFO_SUBVTT_INDEX);
        DO_VAL("vptr-index", BINFO_VPTR_INDEX);
        DO_VAL("inheritance-chain", BINFO_INHERITANCE_CHAIN);

        // This is an embedded VEC(tree, none)
        Xml base_binfos_xml("base-binfos");
        int length = BINFO_N_BASE_BINFOS(orig_tree);
        for (int i = 0; i < length; ++i)
        {
            Xml xml("e", "i", i);
            xemit(BINFO_BASE_BINFO(orig_tree, i));
        }
        void *z_base = BINFO_BASE_BINFOS(bitmask_tree);
        memset(z_base, 0, tree_sizeof - ((char *)z_base - (char *)bitmask_tree));
    }

    if (code == FUNCTION_DECL)
    {
        switch (DECL_BUILT_IN_CLASS(orig_tree))
        {
        case NOT_BUILT_IN:
            assert (DECL_FUNCTION_CODE(orig_tree) == 0);
            break;
        case BUILT_IN_FRONTEND:
        case BUILT_IN_MD:
            // TODO is this right?
            if (int fn_code = (int)DECL_FUNCTION_CODE(orig_tree))
                xml1("function-code", fn_code);
            DECL_FUNCTION_CODE(bitmask_tree) = (enum built_in_function)0;
            break;
        case BUILT_IN_NORMAL:
            DO_VAL("function-code", DECL_FUNCTION_CODE);
            break;
        }
        DO_VAL("personality", DECL_FUNCTION_PERSONALITY);
        DO_VAL("result", DECL_RESULT);
        DO_BIT("uninlinable", DECL_UNINLINABLE);
        DO_VAL("saved-tree", DECL_SAVED_TREE);
        DO_BIT("is-malloc", DECL_IS_MALLOC);
        DO_BIT("is-operator-new", DECL_IS_OPERATOR_NEW);
        DO_BIT("returns-twice", DECL_IS_RETURNS_TWICE);
        DO_BIT("pure", DECL_PURE_P);
        DO_BIT("looping-const-or-pure", DECL_LOOPING_CONST_OR_PURE_P);
        DO_BIT("novops", DECL_IS_NOVOPS);
        DO_BIT("static-constructor", DECL_STATIC_CONSTRUCTOR);
        DO_BIT("static-destructor", DECL_STATIC_DESTRUCTOR);
        DO_BIT("no-instrument-entry-exit", DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT);
        DO_BIT("no-limit-stack", DECL_NO_LIMIT_STACK);
        DO_BIT("static-chain", DECL_STATIC_CHAIN);
        DO_BIT("possibly-inlined", DECL_POSSIBLY_INLINED);
        DO_BIT("declared-inline", DECL_DECLARED_INLINE_P);
        DO_BIT("no-inline-warning", DECL_NO_INLINE_WARNING_P);
        DO_BIT("disregard-inline-limits", DECL_DISREGARD_INLINE_LIMITS);
        DO_VAL("struct-function", DECL_STRUCT_FUNCTION);
        DO_VAL("builtin-class", DECL_BUILT_IN_CLASS);
        DO_VAL("arguments", DECL_ARGUMENTS);
        DO_VAL("function-specific-target", DECL_FUNCTION_SPECIFIC_TARGET);
        DO_VAL("function-specific-optimization", DECL_FUNCTION_SPECIFIC_OPTIMIZATION);
#if V(4, 7)
        DO_BIT("has-debug-args", DECL_HAS_DEBUG_ARGS_P);
        auto vec_p = decl_debug_args_lookup(CONST_CAST_TREE(orig_tree));
        if (vec_p)
        {
            Xml debug_args_xml("debug-args");
            int i = 0;
            foreach (tree da, *vec_p)
            {
                Xml xml("e", "i", i);
                xemit(da);
                i += 1;
            }
        }
#endif
#if V(4, 8)
        DO_BIT("function-versioned", DECL_FUNCTION_VERSIONED);
#endif
#if V(4, 9)
        DO_BIT("is-cxx-constructor", DECL_CXX_CONSTRUCTOR_P);
        DO_BIT("is-cxx-destructor", DECL_CXX_DESTRUCTOR_P);
        DO_BIT("final", DECL_FINAL_P);
#endif
    }
    if (code == FUNCTION_DECL || code == VAR_DECL)
        DO_BIT("extern", DECL_EXTERNAL);
    if (code == VAR_DECL || code == PARM_DECL
#if V(4, 6)
        || code == RESULT_DECL
#endif
    )
    {
        DO_BIT("debug-has-value-expr", DECL_HAS_VALUE_EXPR_P);
        xml1("debug-value-expr", DECL_VALUE_EXPR(CONST_CAST_TREE(orig_tree)));
    }
    if (code == VAR_DECL || code == PARM_DECL)
    {
        DO_BIT("register", DECL_REGISTER);
#if V(4, 6)
        DO_BIT("read-p", DECL_READ_P);
#endif
    }
    if (HAS_RTL_P(orig_tree))
    {
#define GET_RTL_DAMMIT(NODE) ((NODE)->decl_with_rtl.rtl)
        DO_VAL("rtl", GET_RTL_DAMMIT);
    }
    if (code == FIELD_DECL)
    {
        DO_VAL("offset", DECL_FIELD_OFFSET);
        DO_VAL("bit-offset", DECL_FIELD_BIT_OFFSET);
        DO_VAL("bitfield-type", DECL_BIT_FIELD_TYPE);
        DO_VAL("qualifier", DECL_QUALIFIER);
        xml1("offset-align", DECL_OFFSET_ALIGN(orig_tree));
        SET_DECL_OFFSET_ALIGN(bitmask_tree, 1);
        DO_VAL("fcontext", DECL_FCONTEXT);
        DO_BIT("bitfield", DECL_BIT_FIELD);
        DO_BIT("nonaddressable", DECL_NONADDRESSABLE_P);
#if V(4, 7)
        DO_VAL("bitfield-representative", DECL_BIT_FIELD_REPRESENTATIVE);
#endif
    }
    if (code == LABEL_DECL)
    {
        DO_VAL("uid", LABEL_DECL_UID);
        DO_VAL("eh-landing-pad-number", EH_LANDING_PAD_NR);
#if !V(4, 9)
        DO_BIT("error-issued", DECL_ERROR_ISSUED);
#endif
    }
    if (code == PARM_DECL)
    {
        DO_VAL("incoming-rtl", DECL_INCOMING_RTL);
    }
    if (code == VAR_DECL)
    {
        DO_BIT("in-text-section", DECL_IN_TEXT_SECTION);
#if V(4, 6)
        DO_BIT("in-constant-pool", DECL_IN_CONSTANT_POOL);
#endif
        DO_BIT("hard-register", DECL_HARD_REGISTER);
#if V(4, 9)
        DO_VAL("has-debug-expr-p", DECL_HAS_DEBUG_EXPR_P);
#endif
        xml1("debug-split-expr", DECL_DEBUG_EXPR(CONST_CAST_TREE(orig_tree)));
        DO_BIT("has-init-priority", DECL_HAS_INIT_PRIORITY_P);
        xml1("init-priority", DECL_INIT_PRIORITY(CONST_CAST_TREE(orig_tree)));
        xml1("fini-priority", DECL_FINI_PRIORITY(CONST_CAST_TREE(orig_tree)));
#if !V(5)
        DO_VAL("tls-model", DECL_TLS_MODEL);
        // later part of the varpool_node
#endif
    }
    if (code == VAR_DECL || code == PARM_DECL || code == RESULT_DECL)
    {
#if !V(4, 8)
# define DECL_VAR_ANN(NODE) (*DECL_VAR_ANN_PTR(NODE))
        DO_VAL("var-ann", DECL_VAR_ANN);
        // doesn't exist in any form since 4.8
#endif
    }
    if (code == TYPE_DECL)
    {
        DO_VAL("original-type", DECL_ORIGINAL_TYPE);
        DO_BIT("suppress-debug", TYPE_DECL_SUPPRESS_DEBUG);
    }
    if (code == VAR_DECL || code == RESULT_DECL)
    {
#if V(4, 6)
        DO_BIT("nonshareable", DECL_NONSHAREABLE);
#endif
    }
#if V(4, 6)
    if (code == TRANSLATION_UNIT_DECL)
    {
        CDO_VAL("language", TRANSLATION_UNIT_LANGUAGE);
    }
#endif

    if (CODE_CONTAINS_STRUCT(code, TS_DECL_MINIMAL))
    {
        DO_VAL("uid", DECL_UID);
        DO_VAL("name", DECL_NAME);
        if (DECL_IS_BUILTIN(orig_tree))
            xml0("builtin");
        xml1("location", expand_location(TAKE1(DECL_SOURCE_LOCATION)));
#if V(6)
        xml1("location-range", DECL_LOCATION_RANGE(CONST_CAST_TREE(orig_tree)));
#endif
        if (code == FIELD_DECL)
            DO_VAL("field-context", DECL_FIELD_CONTEXT);
        CDO_VAL("context", DECL_CONTEXT);
    }
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_COMMON))
    {
#if V(4, 6)
        if (DECL_PT_UID_SET_P(orig_tree))
            DO_VAL2("pt-uid", DECL_PT_UID, SET_DECL_PT_UID);
        else
            SET_DECL_PT_UID(bitmask_tree, 0);
#endif
        DO_VAL("abstract-origin", DECL_ABSTRACT_ORIGIN);
        DO_VAL("attributes", DECL_ATTRIBUTES);
        if (code == IMPORTED_DECL)
            DO_VAL("associated-decl", IMPORTED_DECL_ASSOCIATED_DECL);
        if (code == PARM_DECL)
            DO_VAL("arg-type", DECL_ARG_TYPE);
#if V(4, 9)
        if (code == NAMELIST_DECL)
            DO_VAL("associated-decl", NAMELIST_DECL_ASSOCIATED_DECL);
#endif
        CDO_VAL("initial", DECL_INITIAL);
        DO_VAL("size", DECL_SIZE);
        DO_VAL("size-bytes", DECL_SIZE_UNIT);
        DO_VAL2("align", DECL_ALIGN, SET_DECL_ALIGN);
#if V(8)
        DO_VAL2("warn-if-not-align", DECL_WARN_IF_NOT_ALIGN, SET_DECL_WARN_IF_NOT_ALIGN);
#endif
        DO_BIT("user-align", DECL_USER_ALIGN);
        DO_VAL("mode", DECL_MODE);
#if !V(4, 9)
        // DECL_HAS_DEBUG_EXPR_P since 4.9, and VAR_DECL-specific
        DO_VAL("debug-expr-is-from", DECL_DEBUG_EXPR_IS_FROM);
#endif
        DO_BIT("debug-ignored", DECL_IGNORED_P);
        DO_BIT("debug-abstract", DECL_ABSTRACT_P);
        DO_VAL("lang-specific", DECL_LANG_SPECIFIC);
        DO_BIT("nonlocal", DECL_NONLOCAL);
        DO_BIT("virtual", DECL_VIRTUAL_P);
        DO_BIT("artificial", DECL_ARTIFICIAL);
        switch (vomitorium_current_frontend)
        {
        default:
            DO_BIT("decl-lang-flag-0", DECL_LANG_FLAG_0);
            DO_BIT("decl-lang-flag-1", DECL_LANG_FLAG_1);
            DO_BIT("decl-lang-flag-2", DECL_LANG_FLAG_2);
            DO_BIT("decl-lang-flag-3", DECL_LANG_FLAG_3);
            DO_BIT("decl-lang-flag-4", DECL_LANG_FLAG_4);
            DO_BIT("decl-lang-flag-5", DECL_LANG_FLAG_5);
            DO_BIT("decl-lang-flag-6", DECL_LANG_FLAG_6);
            DO_BIT("decl-lang-flag-7", DECL_LANG_FLAG_7);
            DO_BIT("decl-lang-flag-8", DECL_LANG_FLAG_8);
        }
        DO_BIT("attribute-used", DECL_PRESERVE_P);
        DO_BIT("gimple-register", DECL_GIMPLE_REG_P);
    }
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_WITH_VIS))
    {
        DO_BIT("seen-in-bind-expr", DECL_SEEN_IN_BIND_EXPR_P);
        DO_BIT("defer-output", DECL_DEFER_OUTPUT);
        DO_BIT("weak", DECL_WEAK);
        DO_BIT("dllimport", DECL_DLLIMPORT_P);
        DO_BIT("comdat", DECL_COMDAT);
#if !V(5)
        DO_VAL("comdat-group", DECL_COMDAT_GROUP);
        // later, just part of the symtab_node
#endif
        if (DECL_ASSEMBLER_NAME_SET_P(orig_tree))
        {
#define orig_tree CONST_CAST_TREE(orig_tree)
            DO_VAL2("assembler-name", DECL_ASSEMBLER_NAME, SET_DECL_ASSEMBLER_NAME);
#undef orig_tree
        }
#if V(8)
        DO_VAL("assembler-name-raw", DECL_ASSEMBLER_NAME_RAW);
#endif
#if V(4, 6) && !V(5)
        DO_BIT("has-implicit-section-name", DECL_HAS_IMPLICIT_SECTION_NAME_P);
#endif
#if !V(5)
        DO_VAL("section-name", DECL_SECTION_NAME);
        // later, just part of the symtab_node
#endif
        DO_VAL("visibility", DECL_VISIBILITY);
        DO_BIT("visibility-specified", DECL_VISIBILITY_SPECIFIED);
        DO_BIT("common", DECL_COMMON);
    }
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_NON_COMMON))
    {
        DO_VAL("result-field", DECL_RESULT_FLD);
        if (!V(5) || code == FUNCTION_DECL)
            DO_VAL("vindex", DECL_VINDEX);
#if !V(5)
        DO_VAL("argument-field", DECL_ARGUMENT_FLD);
#endif
    }

    if (code == STATEMENT_LIST)
    {
        Xml statement_list_xml("statement-list");
        int i = 0;
        for (tree_statement_list_node *node = STATEMENT_LIST_HEAD(orig_tree); node; node = node->next)
        {
            Xml xml("e", "i", i);
            xemit(node->stmt);
            ++i;
        }
        STATEMENT_LIST_HEAD(bitmask_tree) = nullptr;
        STATEMENT_LIST_TAIL(bitmask_tree) = nullptr;
    }

    if (code == OPTIMIZATION_NODE)
    {
        DO_VAL("optimization-option", TREE_OPTIMIZATION_REALLY);
#if V(4, 8)
        auto optabs = (struct target_optabs *)TAKE1(TREE_OPTIMIZATION_OPTABS);
        xml1("optabs", optabs);
        DO_VAL("base-optabs", TREE_OPTIMIZATION_BASE_OPTABS);
#endif
    }
    if (code == TARGET_OPTION_NODE)
    {
        DO_VAL("target-option", TREE_TARGET_OPTION_REALLY);
#if V(4, 9)
        DO_VAL("target-globals", TREE_TARGET_GLOBALS);
#endif
    }

    if (false)
        (void)0;
    if (true)
        TREE_SET_CODE(bitmask_tree, (enum tree_code)0);
    if (!is_all_zero(bitmask_tree, tree_sizeof))
    {
        dump_remaining(orig_tree, bitmask_tree, tree_sizeof);
        incomplete_dumps++;
    }
}
#if G(4, 6)
# pragma GCC diagnostic pop
#endif

template<class E, typename=typename std::is_enum<E>::type>
E& operator ++(E& e)
{
    return e = (E)(e + 1);
}

static void dump_all()
{
    Xml root("vomitorium-dump", "version", 1);

    {
        Xml all_globals("globals");

        for (enum rid i = (enum rid)0; i < RID_MAX; ++i)
        {
            Global xml("ridpointers", i);
            xemit(ridpointers[i]);
        }

        for (enum c_tree_index i = (enum c_tree_index)0; i < CTI_MAX; ++i)
        {
            Global xml("c_global_trees", i);
            xemit(c_global_trees[i]);
        }

        // TODO: named_section has a `tree decl`

        // TODO: rtx*/function has several trees

        // TODO: types_used_by_vars_entry consists of trees

#if !V(4, 6)
        global1("types_used_by_cur_var_decl", types_used_by_cur_var_decl);
#else
        globalv("types_used_by_cur_var_decl", types_used_by_cur_var_decl);
#endif
        foreach (auto& p, alias_pairs)
        {
            Global xml("alias_pairs", p.decl);
            xemit(p.target);
        }

        for (enum built_in_function i = (enum built_in_function)0; i < END_BUILTINS; ++i)
        {
#if V(5)
            // Ugh, some names are NULL!
            // But the values are NULL too, so we aren't missing anything.
            if (!built_in_names[i] && !builtin_decl_explicit(i))
                continue;
#endif
            {
                Global xml("explicit_built_in_decls", i);
                xemit(builtin_decl_explicit(i));
            }
            {
                Global xml("implicit_built_in_decls", i);
                xemit(builtin_decl_implicit(i));
            }
        }

        for (enum tree_index i = (enum tree_index)0; i < TI_MAX; ++i)
        {
            Global xml("global_trees", i);
            xemit(global_trees[i]);
        }

        for (enum integer_type_kind i = (enum integer_type_kind)0; i < itk_none; ++i)
        {
            Global xml("integer_types", i);
            xemit(integer_types[i]);
        }

#if V(4, 8)
# define TYPE_KIND_LAST stk_type_kind_last
#endif
        for (enum size_type_kind i = (enum size_type_kind)0; i < TYPE_KIND_LAST; ++i)
        {
            Global xml("sizetype_tab", i);
            xemit(sizetype_tab[i]);
        }

        global1("current_function_decl", current_function_decl);

#if !V(4, 6)
        global1("memory_identifier_string", memory_identifier_string);
#endif

        // TODO: cgraph_node wraps a tree

        // TODO: cgraph_asm_node wraps a tree

        // TODO: varpool_node wraps a tree

        if (vomitorium_current_frontend == VOMITORIUM_FRONTEND_CXX)
        {
            for (enum cp_tree_index i = (enum cp_tree_index)0; i < (int)CPTI_MAX; ++i)
            {
                Global xml("cp_global_trees", i);
                xemit(cp_global_trees[i]);
            }

            // TODO: saved_scope has several trees

            global1("integer_two_node", integer_two_node);
            global1("integer_three_node", integer_three_node);

            globalv("local_classes", local_classes);

            global1("static_aggregates", static_aggregates);
#if V(4, 8)
            global1("tls_aggregates", tls_aggregates);
#endif

            // TODO operator_name_info and assignment_operator_name_info

#if !V(6)
            globalv("deferred_mark_used_calls", deferred_mark_used_calls);
#endif

            globalv("unemitted_tinfo_decls", unemitted_tinfo_decls);

            global1("global_namespace", global_namespace);
#if !V(8)
            global1("global_scope_name", global_scope_name);
#endif
            global1("global_type_node", global_type_node);
        } // C++ only

#if V(4, 6)
    // TODO default_target_rtl has trees

    globalv("all_translation_units", all_translation_units);
#endif

#if V(4, 7)
        for (int i = 0; i < FIRST_PSEUDO_REGISTER; ++i)
        {
            Global xml("global_regs_decl", this_target_hard_regs->x_reg_names[i]);
            xemit(global_regs_decl[i]);
        }

        global1("pragma_extern_prefix", pragma_extern_prefix);

        // TODO symtab_node has trees
        // TODO asm_node has trees
#endif

        // TODO ipa_edge_args_vector has trees
        // TODO ipa_node_agg_replacements has a tree

#if V(4, 9)
        for (enum internal_fn i = (enum internal_fn)0; i < IFN_LAST + 1; ++i)
        {
            Global xml("internal_fn_fnspec_array", i);
            xemit(internal_fn_fnspec_array[i]);
        }
#endif

#if V(5) && !V(7)
        global1("block_clear_fn", block_clear_fn);
#endif

#if V(5)
        for (enum cilk_tree_index i = (enum cilk_tree_index)0; i < CILK_TI_MAX; ++i)
        {
            Global xml("cilk_trees", i);
            xemit(cilk_trees[i]);
        }

        global1("registered_builtin_types", registered_builtin_types);

        for (int i = 0; i < NUM_INT_N_ENTS; ++i)
        {
            {
                Xml("global", "name", field_pair<index_pair<int>>{{"int_n_trees", i}, "signed_type"});
                xemit(int_n_trees[i].signed_type);
            }
            {
                Xml("global", "name", field_pair<index_pair<int>>{{"int_n_trees", i}, "unsigned_type"});
                xemit(int_n_trees[i].unsigned_type);
            }
        }

        globalv("offload_funcs", offload_funcs);
        globalv("offload_vars", offload_vars);

        // TODO symtab eventually has trees
        // TODO cfi_vec eventually has trees

        global1("chrec_not_analyzed_yet", chrec_not_analyzed_yet);
        global1("chrec_dont_know", chrec_dont_know);
        global1("chrec_known", chrec_known);

        // TODO ipcp_transformations has trees
        // TODO inline_summaries eventually has trees
#endif
#if V(6)
        globalv("vtbl_mangled_name_types", vtbl_mangled_name_types);
        globalv("vtbl_mangled_name_ids", vtbl_mangled_name_ids);
#endif
#if V(8)
        if (vomitorium_current_frontend == VOMITORIUM_FRONTEND_CXX)
        {
            globalv("static_decls", static_decls);
            globalv("keyed_classes", keyed_classes);
        }
#endif
    } // </globals>


    // Finally, dump all the top-level trees we've encountered.
    {
        Xml all_trees("trees");
        // This will add more trees as it walks them, so we can't use for-each.
        for (size_t i = 0; i < interned_tree_list.size(); ++i)
        {
            dump_tree(interned_tree_list[i]);
        }
    } // </trees>

    if (incomplete_dumps)
    {
        printf("warning: %zu/%zu incomplete dumps\n", incomplete_dumps, interned_tree_list.size());
        printf("note: set a breakpoint on `dump_remaining` to help fix this\n");
    }
}
static void do_dump(void *, void *)
{
    dump_all();
}

void enable_dump_v1()
{
    register_callback("vomitorium", PLUGIN_PRE_GENERICIZE, do_dump, nullptr);
}
