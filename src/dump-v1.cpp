#include "internal.hpp"

#include <cassert>

#include <map>
#include <vector>

#include "iter.hpp"
#include "names.hpp"


static size_t incomplete_dumps = 0;

// For XML this is generally a good choice,
// although since we aren't doing much nesting, we *could* use 4.
#define INDENT_SPACES 2
bool start_of_line = true;
size_t indent;

// TODO: in order to support multiple dumps properly, this needs to be
// registered as a GC root. Possibly switch to VEC(tree, gc)?
// (the map is fine since it uses the same trees)
std::vector<const_tree> interned_tree_list = {NULL_TREE};
std::map<const_tree, size_t> interned_tree_ids = {{NULL_TREE, 0}};

static size_t intern(const_tree t)
{
    auto pair = interned_tree_ids.insert(std::make_pair(t, interned_tree_ids.size()));
    if (pair.second)
        interned_tree_list.push_back(t);
    return pair.first->second;
}


// TODO increase this once it's well-tested.
static const char many_spaces[] = "   ";
static void xemit_spaces(size_t n)
{
    while (n)
    {
        size_t l = std::min(n, strlen(many_spaces));
        size_t rv = fwrite(many_spaces, 1, l, vomitorium_output);
        if (rv == 0)
            abort();
        n -= rv;
    }
}
static void xemit_raw(const char *s, size_t len)
{
    if (start_of_line)
    {
        start_of_line = false;
        xemit_spaces(indent * INDENT_SPACES);
    }
    while (len)
    {
        size_t rv = fwrite(s, 1, len, vomitorium_output);
        if (rv == 0)
            abort();
        s += rv;
        len -= rv;
    }
}
static void nl()
{
    xemit_raw("\n", 1);
    start_of_line = true;
}

static void xemit(const char *s)
{
    assert (s && "Can't xemit a NULL string!");
    for (size_t i = 0; s[i]; ++i)
    {
        assert (' ' <= s[i] && s[i] <= '~');
    }

    while (true)
    {
        size_t len = strcspn(s, "<>&\"");
        if (len)
        {
            xemit_raw(s, len);
            s += len;
        }

        while (true)
        {
            switch (*s)
            {
            case '<':
                xemit_raw("&lt;", 4);
                ++s;
                continue;
            case '>':
                xemit_raw("&gt;", 4);
                ++s;
                continue;
            case '&':
                xemit_raw("&amp;", 5);
                ++s;
                continue;
            case '"':
                xemit_raw("&quot;", 6);
                ++s;
                continue;
            case '\0':
                return;
            }
            break;
        }
    }
};

template<class T, typename=typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type>
static void xemit(T i)
{
    fprintf(vomitorium_output, "%ju", (uintmax_t)i);
}
template<class T, typename=typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type, typename=void>
static void xemit(T i)
{
    fprintf(vomitorium_output, "%jd", (intmax_t)i);
}

static void xemit(enum tree_code e)
{
#if GCCPLUGIN_VERSION < 4009
# define get_tree_code_name(code) tree_code_name[code]
#endif
    xemit(get_tree_code_name(e));
}
static void xemit(enum rid e)
{
    xemit(rid_names[e]);
}
static void xemit(enum tree_index e)
{
    xemit(ti_names[e]);
}
static void xemit(enum c_tree_index e)
{
    xemit(cti_names[e]);
}
static void xemit(enum cp_tree_index e)
{
    xemit(cpti_names[e]);
}
#if GCCPLUGIN_VERSION >= 5000
static void xemit(enum cilk_tree_index e)
{
    xemit(cilk_ti_names[e]);
}
#endif
static void xemit(enum built_in_function e)
{
    xemit(built_in_names[e]);
}
static void xemit(enum integer_type_kind e)
{
    xemit(itk_names[e]);
}
static void xemit(enum size_type_kind e)
{
    xemit(stk_names[e]);
}
// register names are not an enum
#if GCCPLUGIN_VERSION >= 4009
static void xemit(enum internal_fn e)
{
    xemit(internal_fn_name_array[e]);
}
#endif

static void xemit(enum omp_clause_code e)
{
    xemit(omp_clause_code_name[e]);
}
static void xemit(enum omp_clause_schedule_kind e)
{
    xemit(omp_clause_schedule_names[e]);
}
static void xemit(enum omp_clause_default_kind e)
{
    xemit(omp_clause_default_names[e]);
}
static void xemit(enum machine_mode e)
{
    xemit(GET_MODE_NAME(e));
}
static void xemit(enum built_in_class e)
{
    xemit(built_in_class_names[e]);
}
static void xemit(enum tls_model e)
{
    xemit(tls_model_names[e]);
}
static void xemit(enum symbol_visibility e)
{
    xemit(symbol_visibility_names[e]);
}
static void xemit(enum node_type e)
{
    xemit(cpp_node_type_names[e]);
}
static void xemit(enum cpp_builtin_type e)
{
    xemit(cpp_builtin_type_names[e]);
}

static void xemit(mpz_t i)
{
    gmp_fprintf(vomitorium_output, "%Zd", i);
}
static void xemit(double_int i)
{
    // Avoid allocating an mpz if possible (it is rarely needed).
    // The fast path can handle 65-bit integers (actually, only 33-bit
    // integers on some configurations), by manually flipping the sign.
    if (double_int_negative_p(i))
    {
        xemit("-");
        i = double_int_neg(i);
    }
    if (double_int_fits_in_uhwi_p(i))
    {
        xemit(double_int_to_uhwi(i));
    }
    else
    {
        mpz_t val;
        mpz_init(val);
        mpz_set_double_int(val, i, true);
        xemit(val);
    }
}
static void xemit(struct real_value *f)
{
    static const size_t max_digits = SIGNIFICAND_BITS / 4;
    static const size_t exp_bytes = 16;
    static const size_t buf_size = max_digits + exp_bytes + 1 + 4 + 1;
    char buf[buf_size + 16]; // just in case - will be checked
    memset(buf, 0, sizeof(buf));
    real_to_hexadecimal(buf, f, sizeof(buf), 0, false);
    assert (buf[buf_size - 1] == '\0');
    xemit(buf);
}
static void xemit(struct fixed_value *f)
{
    static const size_t buf_size = sizeof(*f) * 8 + 2;
    char buf[buf_size + 16]; // just in case - will be checked
    fixed_to_decimal(buf, f, sizeof(buf));
    assert (buf[buf_size - 1] == '\0');
    xemit(buf);
}
static void xemit(expanded_location loc)
{
    xemit("\"<"[loc.sysp]);
    xemit(loc.file);
    xemit("\">"[loc.sysp]);
    xemit(":");
    xemit(loc.line);
    xemit(":");
    xemit(loc.column);
}
// TODO implement these dumpers
static void xemit(rtx r)
{
    (void)r;
    xemit("NYI: rtx");
}
static void xemit(gimple g)
{
    (void)g;
    xemit("NYI: gimple");
}
static void xemit(gimple_seq g)
{
    (void)g;
    xemit("NYI: gimple_seq");
}
static void xemit(struct ptr_info_def *alias_info)
{
    (void)alias_info;
    xemit("NYI: ptr_info_def");
}
static void xemit(struct die_struct *die)
{
    (void)die;
    xemit("NYI: die_struct");
}
static void xemit(struct lang_type *lt)
{
    (void)lt;
    xemit("NYI: lang_type");
}
static void xemit(struct lang_decl *ld)
{
    (void)ld;
    xemit("NYI: lang_decl");
}
static void xemit(struct function *f)
{
    (void)f;
    xemit("NYI: function");
}
static void xemit(var_ann_t a)
{
    (void)a;
    xemit("NYI: var_ann_t");
}
static void xemit(struct cl_optimization oo)
{
    (void)oo;
    xemit("NYI: cl_optimization");
}
static void xemit(struct cl_target_option to)
{
    (void)to;
    xemit("NYI: cl_target_option");
}
static void xemit(struct cxx_binding *bindings)
{
    (void)bindings;
    xemit("NYI: cxx_bindings");
}
static void xemit(cpp_macro *m)
{
    (void)m;
    xemit("NYI: cpp_macro");
}
static void xemit(answer *a)
{
    (void)a;
    xemit("NYI: answer");
}
static void xemit(struct c_binding *b)
{
    (void)b;
    xemit("NYI: c_binding");
}


static void xemit(const_tree t)
{
    xemit("@");
    xemit(intern(t));
}

static void xml0(const char *tag)
{
    xemit_raw("<", 1);
    xemit(tag);
    xemit_raw("/>", 2);
    nl();
}
template<class V>
static void xml0(const char *tag, const char *k, V v)
{
    xemit_raw("<", 1);
    xemit(tag);
    xemit_raw(" ", 1);
    xemit(k);
    xemit_raw("=\"", 2);
    xemit(v);
    xemit_raw("\"/>", 3);
    nl();
}

class Xml
{
    const char *tag;
public:
    Xml(Xml&&) = delete;
    Xml& operator = (Xml&&) = delete;

    Xml(const char *t) : tag(t)
    {
        xemit_raw("<", 1);
        xemit(this->tag);
        xemit_raw(">", 1);
        indent += 1;
    }
    template<class V>
    Xml(const char *t, const char *k, V v) : tag(t)
    {
        xemit_raw("<", 1);
        xemit(this->tag);
        xemit_raw(" ", 1);
        xemit(k);
        xemit_raw("=\"", 2);
        xemit(v);
        xemit_raw("\">", 2);
        indent += 1;
    }
    ~Xml()
    {
        indent -= 1;
        xemit_raw("</", 2);
        xemit(this->tag);
        xemit_raw(">", 1);
        nl();
    }
};

template<class T>
struct index_pair
{
    const char *array;
    T index;
};
template<class T>
static void xemit(index_pair<T> p)
{
    xemit(p.array);
    xemit("[");
    xemit(p.index);
    xemit("]");
}
template<class T>
struct field_pair
{
    T lhs;
    const char *field;
};
template<class T>
static void xemit(field_pair<T> p)
{
    xemit(p.lhs);
    xemit(".");
    xemit(p.field);
}

class Global : public Xml
{
public:
    Global(const char *name) : Xml("global", "name", name) {}
    template<class T>
    Global(const char *array, T index) : Xml("global", "name", index_pair<T>{array, index}) {}
};

template<class T>
static void xml1(const char *tag, T v)
{
    Xml xml(tag);
    xemit(v);
}

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
        nl();
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

        xemit_raw(buf, sizeof(buf)-1);
        nl();
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

        xemit_raw(buf, bi-1);
    }
    if (p != data)
        nl();
}


// When debugging, set a breakpoint here, then go up a frame
// to access the typed version of the structure.
static void dump_remaining(void *p, size_t s)
{
    Xml xml("remaining");
    xemit_hex(p, s);
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
#define TAKE1(node, LVALUE)             \
({                                      \
    auto _node = (node);                \
    auto _rv = LVALUE(_node);           \
    decltype(_rv) _zero;                \
    memset(&_zero, 0, sizeof(_zero));   \
    LVALUE(_node) = _zero;              \
    _rv;                                \
})
#define TAKE2(node, GETTER, SETTER)     \
({                                      \
    auto _node = (node);                \
    auto _rv = GETTER(_node);           \
    decltype(_rv) _zero;                \
    memset(&_zero, 0, sizeof(_zero));   \
    SETTER(_node, _zero);               \
    _rv;                                \
})

template<class T>
struct is_string : std::integral_constant<bool, std::is_same<T, char *>::value || std::is_same<T, const char *>::value>
{
};

#define DO_LVAL(name, lval)                                             \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE0(lval);                                        \
        static_assert(!is_string<decltype(_val)>::value, "no strings"); \
        xml1(name, _val);                                               \
    }                                                                   \
    while (0)
#define DO_VAL(t, name, LVALUE)                                         \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE1(t, LVALUE);                                   \
        static_assert(!is_string<decltype(_val)>::value, "no strings"); \
        xml1(name, _val);                                               \
    }                                                                   \
    while (0)
#define DO_VAL2(t, name, GETTER, SETTER)                                \
    do                                                                  \
    {                                                                   \
        auto _val = TAKE2(t, GETTER, SETTER);                           \
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
#define CDO_VAL(t, name, LVALUE)        \
    do                                  \
    {                                   \
        if (auto _v = TAKE1(t, LVALUE)) \
            xml1(name, _v);             \
    }                                   \
    while (0)
#define CDO_VAL2(t, name, GETTER, SETTER)       \
    do                                          \
    {                                           \
        if (auto _v = TAKE2(t, GETTER, SETTER)) \
            xml1(name, _v);                     \
    }                                           \
    while (0)

#define BIT(expr)                       \
({                                      \
    auto _expr = +(expr);               \
    assert (_expr == 0 || _expr == 1);  \
    _expr;                              \
})
#define DO_BIT(t, name, LVALUE)    \
    do                              \
    {                               \
        if (BIT(TAKE1(t, LVALUE)))  \
            xml0(name);             \
    }                               \
    while (0)
#define DO_BIT2(t, name, GETTER, SETTER)    \
    do                                      \
    {                                       \
        if (BIT(TAKE2(t, GETTER, SETTER)))  \
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

static void dump_cci(struct c_common_identifier *cci)
{
    cpp_hashnode *node = &cci->node;
    auto node_copy = *node;
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
    DO_VAL2(cci, "rid-code", C_RID_CODE, C_SET_RID_CODE);
    DO_LVAL("node-type", node->type);
    {
        Xml xml("flags");
        nl();
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
    switch (CPP_HASHNODE_VALUE_IDX(node_copy))
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
#if GCC_VERSION >= 4006
# pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wshadow"
static void dump_tree(const_tree orig_tree)
{
    if (!orig_tree)
    {
        xml0("tree", "id", orig_tree);
        return;
    }
    Xml tree_xml("tree", "id", orig_tree);
    nl();

    size_t tree_sizeof = tree_size(orig_tree);
    __attribute__((aligned(alignof(*orig_tree))))
    char bitmask_vla[tree_sizeof];
    tree t = (tree)&bitmask_vla[0];
    memcpy(t, orig_tree, tree_sizeof);

    enum tree_code code = TREE_CODE(t);
    enum tree_node_structure_enum structure = tree_node_structure(t);
    // For other trees, assign a bogus value to avoid extra if's.
    enum omp_clause_code omp_code = code == OMP_CLAUSE ? OMP_CLAUSE_CODE(t) : (enum omp_clause_code)-1;
    //enum tree_code_class code_class = TREE_CODE_CLASS(code);

    switch (structure)
    {
    case TS_BASE:
        assert (tree_sizeof == sizeof(t->base));
        assert (0 && "everything is at least TS_COMMON");
        break;
    case TS_COMMON:
        assert (tree_sizeof == sizeof(t->common));
        break;
    case TS_INT_CST:
        assert (tree_sizeof == sizeof(t->int_cst));
        break;
    case TS_REAL_CST:
        assert (tree_sizeof == sizeof(t->real_cst));
        break;
    case TS_FIXED_CST:
        assert (tree_sizeof == sizeof(t->fixed_cst));
        break;
    case TS_VECTOR:
        assert (tree_sizeof == sizeof(t->vector));
        break;
    case TS_STRING:
        // variable-size
        assert (tree_sizeof >= sizeof(t->string));
        break;
    case TS_COMPLEX:
        assert (tree_sizeof == sizeof(t->complex));
        break;
    case TS_IDENTIFIER:
        // languages subclass this
        assert (tree_sizeof == lang_hooks.identifier_size);
        assert (tree_sizeof >= sizeof(t->identifier));
        break;
    case TS_DECL_MINIMAL:
        assert (tree_sizeof == sizeof(t->decl_minimal));
        break;
    case TS_DECL_COMMON:
        assert (tree_sizeof == sizeof(t->decl_common));
        break;
    case TS_DECL_WRTL:
        assert (tree_sizeof == sizeof(t->decl_with_rtl));
        break;
    case TS_DECL_NON_COMMON:
        assert (tree_sizeof == sizeof(t->decl_non_common));
        break;
    case TS_DECL_WITH_VIS:
        assert (tree_sizeof == sizeof(t->decl_with_vis));
        break;
    case TS_FIELD_DECL:
        assert (tree_sizeof == sizeof(t->field_decl));
        break;
    case TS_VAR_DECL:
        assert (tree_sizeof == sizeof(t->var_decl));
        break;
    case TS_PARM_DECL:
        assert (tree_sizeof == sizeof(t->parm_decl));
        break;
    case TS_LABEL_DECL:
        assert (tree_sizeof == sizeof(t->label_decl));
        break;
    case TS_RESULT_DECL:
        assert (tree_sizeof == sizeof(t->result_decl));
        break;
    case TS_CONST_DECL:
        assert (tree_sizeof == sizeof(t->const_decl));
        break;
    case TS_TYPE_DECL:
        assert (tree_sizeof == sizeof(t->type_decl));
        break;
    case TS_FUNCTION_DECL:
        assert (tree_sizeof == sizeof(t->function_decl));
        break;
    case TS_TYPE:
        assert (tree_sizeof == sizeof(t->type));
        break;
    case TS_LIST:
        assert (tree_sizeof == sizeof(t->list));
        break;
    case TS_VEC:
        // variable-size
        assert (tree_sizeof >= sizeof(t->vec));
        break;
    case TS_EXP:
        // variable-size
        assert (tree_sizeof >= sizeof(t->exp));
        break;
    case TS_SSA_NAME:
        assert (tree_sizeof == sizeof(t->ssa_name));
        break;
    case TS_BLOCK:
        assert (tree_sizeof == sizeof(t->block));
        break;
    case TS_BINFO:
        assert (tree_sizeof == sizeof(t->binfo));
        break;
    case TS_STATEMENT_LIST:
        assert (tree_sizeof == sizeof(t->stmt_list));
        break;
    case TS_CONSTRUCTOR:
        assert (tree_sizeof == sizeof(t->constructor));
        break;
    case TS_OMP_CLAUSE:
        assert (tree_sizeof == sizeof(t->omp_clause));
        break;
    case TS_OPTIMIZATION:
        assert (tree_sizeof == sizeof(t->optimization));
        break;
    case TS_TARGET_OPTION:
        assert (tree_sizeof == sizeof(t->target_option));
        break;
    default:
        assert (0 && "new value in treestruct.def");
        break;
    }

    xml1("code", code);

    // later GCC shares bits, which we don't full check.
    if (code == TREE_VEC)
    {
        int length = TREE_VEC_LENGTH(t);
        for (int i = 0; i < length; ++i)
        {
            Xml xml("e", "i", i);
            xemit(TREE_VEC_ELT(t, i));
            TREE_VEC_ELT(t, i) = nullptr;
        }
        TREE_VEC_LENGTH(t) = 0;
    }

    // has numerous different meanings
    DO_VAL(t, "chain", TREE_CHAIN);

    DO_VAL(t, (code == POINTER_TYPE || code == ARRAY_TYPE || code == VECTOR_TYPE) ? "element_type" : "type", TREE_TYPE);
    if (EXPR_P(t))
        DO_VAL(t, "block", TREE_BLOCK);

    // addressable_flag
    if (code == CALL_EXPR)
        DO_BIT(t, "tailcall", CALL_EXPR_TAILCALL);
    if (code == CASE_LABEL_EXPR)
        DO_BIT(t, "case-low-seen", CASE_LOW_SEEN);
    if (code == PREDICT_EXPR)
        DO_BIT2(t, "predict-expr-outcome", PREDICT_EXPR_OUTCOME, SET_PREDICT_EXPR_OUTCOME);
    // only if not cleared by the above
    DO_BIT(t, "addressable", TREE_ADDRESSABLE);

    // static_flag
    if (code == ADDR_EXPR)
        DO_BIT(t, "no-trampoline", TREE_NO_TRAMPOLINE);
    if (code == TARGET_EXPR || code == WITH_CLEANUP_EXPR)
        DO_BIT(t, "cleanup-eh-only", CLEANUP_EH_ONLY);
    if (code == TRY_CATCH_EXPR)
        DO_BIT(t, "try-catch-is-cleanup", TRY_CATCH_IS_CLEANUP);
    if (code == CASE_LABEL_EXPR)
        DO_BIT(t, "case-high-seen", CASE_HIGH_SEEN);
    if (code == CALL_EXPR)
        DO_BIT(t, "cannot-inline", CALL_CANNOT_INLINE_P);
    if (code == IDENTIFIER_NODE)
        DO_BIT(t, "symbol-referenced", TREE_SYMBOL_REFERENCED);
    if (code == POINTER_TYPE || code == REFERENCE_TYPE)
        DO_BIT(t, "ref-can-alias-all", TYPE_REF_CAN_ALIAS_ALL);
    if (code == MODIFY_EXPR)
        DO_BIT(t, "nontemporal", MOVE_NONTEMPORAL);
    if (code == ASM_EXPR)
        DO_BIT(t, "asm-input", ASM_INPUT_P);
    if (code == TREE_BINFO)
        DO_BIT(t, "virtual-base", BINFO_VIRTUAL_P);
    // only if not cleared by the above
    DO_BIT(t, "static", TREE_STATIC);

    // nowarning_flag
    DO_BIT(t, "no-warning", TREE_NO_WARNING);

    // public_flag
    if (CONSTANT_CLASS_P(t))
        DO_BIT(t, "overflow", TREE_OVERFLOW);
    if (TYPE_P(t))
        DO_BIT(t, "cached-values", TYPE_CACHED_VALUES_P);
    if (code == SAVE_EXPR)
        DO_BIT(t, "resolved", SAVE_EXPR_RESOLVED_P);
    if (code == CALL_EXPR)
        DO_BIT(t, "va-arg-pack", CALL_EXPR_VA_ARG_PACK);
    if (code == ASM_EXPR)
        DO_BIT(t, "asm-volatile", ASM_VOLATILE_P);
    if (omp_code == OMP_CLAUSE_PRIVATE)
        DO_BIT(t, "omp-private-debug", OMP_CLAUSE_PRIVATE_DEBUG);
    if (omp_code == OMP_CLAUSE_LASTPRIVATE)
        DO_BIT(t, "omp-lastprivate-firstprivate", OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE);
    // only if not cleared by the above
    DO_BIT(t, "public", TREE_PUBLIC);

    // side_effects_flag
    if (code == LABEL_DECL)
        DO_BIT(t, "forced", FORCED_LABEL);
    if (!TYPE_P(t))
        DO_BIT(t, "side-effects", TREE_SIDE_EFFECTS);

    // volatile_flag
    if (!TYPE_P(t))
        DO_BIT(t, "volatile", TREE_THIS_VOLATILE);
    else
        DO_BIT(t, "volatile", TYPE_VOLATILE);

    // nothrow_flag
    if (INDIRECT_REF_P(t))
        DO_BIT(t, "no-trap", TREE_THIS_NOTRAP);
    if (code == ARRAY_REF || code == ARRAY_RANGE_REF)
        DO_BIT(t, "in-bounds", TREE_THIS_NOTRAP);
    if (TYPE_P(t))
        DO_BIT(t, "align-ok", TYPE_ALIGN_OK);
    if (code == CALL_EXPR || code == FUNCTION_DECL)
        DO_BIT(t, "nothrow", TREE_NOTHROW);
    if (code == SSA_NAME)
        DO_BIT(t, "freelist", SSA_NAME_IN_FREE_LIST);

    // readonly_flag
    if (code == FUNCTION_DECL)
        DO_BIT(t, "const-fn", TREE_READONLY);
    if (!TYPE_P(t))
        DO_BIT(t, "rvalue", TREE_READONLY);
    else
        DO_BIT(t, "const", TYPE_READONLY);

    // const_flag
    if (!TYPE_P(t))
        DO_BIT(t, "constant", TREE_CONSTANT);
    if (TYPE_P(t))
        DO_BIT(t, "sizes-gimplified", TYPE_SIZES_GIMPLIFIED);

    // unsigned_flag
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_COMMON))
        DO_BIT(t, "unsigned", DECL_UNSIGNED);
    if (TYPE_P(t))
        DO_BIT(t, "unsigned", TYPE_UNSIGNED);

    // asm_written_flag
    if (code == SSA_NAME)
        DO_BIT(t, "in-abnormal-phi", SSA_NAME_OCCURS_IN_ABNORMAL_PHI);
    DO_BIT(t, "asm-written", TREE_ASM_WRITTEN);

    // used_flag
    DO_BIT(t, "used", TREE_USED);

    // private_flag
    if (code == CALL_EXPR)
        DO_BIT(t, "return-slot-opt", CALL_EXPR_RETURN_SLOT_OPT);
    if (code == OMP_SECTION)
        DO_BIT(t, "section-last", OMP_SECTION_LAST);
    if (code == OMP_PARALLEL)
        DO_BIT(t, "parallel-combined", OMP_PARALLEL_COMBINED);
    if (omp_code == OMP_CLAUSE_PRIVATE)
        DO_BIT(t, "omp-private-outer-ref", OMP_CLAUSE_PRIVATE_OUTER_REF);
    DO_BIT(t, "private", TREE_PRIVATE);

    // decl_by_reference_flag
    if (code == VAR_DECL || code == PARM_DECL || code == RESULT_DECL)
    {
        DO_BIT(t, "by-ref", DECL_BY_REFERENCE);
        DO_BIT(t, "restricted", DECL_RESTRICTED_P);
    }

    // protected_flag
    if (code == CALL_EXPR)
        DO_BIT(t, "from-thunk", CALL_FROM_THUNK_P);
    DO_BIT(t, "protected", TREE_PROTECTED);

    // deprecated_flag
    if (code == IDENTIFIER_NODE)
        DO_BIT(t, "transparent-alias", IDENTIFIER_TRANSPARENT_ALIAS);
    DO_BIT(t, "deprecated", TREE_DEPRECATED);

    // saturating_flag
    if (1)
        DO_BIT(t, "saturating", TYPE_SATURATING);

    // default_def_flag
    if (code == SSA_NAME)
        DO_BIT(t, "is-default-definition", SSA_NAME_IS_DEFAULT_DEF);
    if (code == VECTOR_TYPE)
        DO_BIT(t, "opaque", TYPE_VECTOR_OPAQUE);

    // restrict_flag
    if (TYPE_P(t))
        DO_BIT(t, "restrict", TYPE_RESTRICT);

    // visited_flag
    DO_BIT(t, "visited", TREE_VISITED);

    // packed_flag
    if (TYPE_P(t))
        DO_BIT(t, "packed", TYPE_PACKED);
    if (code == FIELD_DECL)
        DO_BIT(t, "packed", DECL_PACKED);

    if (code == TREE_BINFO)
    {
        DO_BIT(t, "binfo-marked", BINFO_MARKED);
        DO_BIT(t, "binfo-flag-1", BINFO_FLAG_1);
        DO_BIT(t, "binfo-flag-2", BINFO_FLAG_2);
        DO_BIT(t, "binfo-flag-3", BINFO_FLAG_3);
        DO_BIT(t, "binfo-flag-4", BINFO_FLAG_4);
        DO_BIT(t, "binfo-flag-5", BINFO_FLAG_5);
        DO_BIT(t, "binfo-flag-6", BINFO_FLAG_6);
    }
    switch (vomitorium_current_frontend)
    {
    default:
        DO_BIT(t, "lang-flag-0", TREE_LANG_FLAG_0);
        DO_BIT(t, "lang-flag-1", TREE_LANG_FLAG_1);
        DO_BIT(t, "lang-flag-2", TREE_LANG_FLAG_2);
        DO_BIT(t, "lang-flag-3", TREE_LANG_FLAG_3);
        DO_BIT(t, "lang-flag-4", TREE_LANG_FLAG_4);
        DO_BIT(t, "lang-flag-5", TREE_LANG_FLAG_5);
        DO_BIT(t, "lang-flag-6", TREE_LANG_FLAG_6);
    }

    if (code == INTEGER_CST)
    {
        DO_VAL(t, "int", TREE_INT_CST);
    }
    if (code == REAL_CST)
    {
        DO_VAL(t, "real", TREE_REAL_CST_PTR);
    }
    if (code == FIXED_CST)
    {
        DO_VAL(t, "fixed", TREE_FIXED_CST_PTR);
    }
    if (code == STRING_CST)
    {
        int length = TAKE1(t, TREE_STRING_LENGTH);
        char *str = jiggle(t, TREE_STRING_POINTER(t));
        xml1("length", length);
        Xml xml("hex");
        xemit_hex(str, length);
        memset(str, 0, length);
    }
    if (code == COMPLEX_CST)
    {
        DO_VAL(t, "real", TREE_REALPART);
        DO_VAL(t, "imag", TREE_IMAGPART);
    }
    if (code == VECTOR_CST)
        DO_VAL(t, "elements", TREE_VECTOR_CST_ELTS);

    if (code == IDENTIFIER_NODE)
    {
        unsigned int len = TAKE1(t, IDENTIFIER_LENGTH);
        // unfortunately, `jiggle` doesn't work here
        const char *ptr = IDENTIFIER_POINTER(t); IDENTIFIER_NODE_CHECK(t)->identifier.id.str = nullptr;
        unsigned int hash_value = TAKE1(t, IDENTIFIER_HASH_VALUE);
        {
            Xml xml("name", "hash", hash_value);
            xemit_raw(ptr, len);
        }
        switch (vomitorium_current_frontend)
        {
        case VOMITORIUM_FRONTEND_C:
            {
                // TODO the hidden (c_)lang_identifier
                auto c_id = (struct c_lang_identifier *)t;
                dump_cci(&c_id->common_id);
                DO_LVAL("symbol-binding", c_id->symbol_binding);
                DO_LVAL("tag-binding", c_id->tag_binding);
                DO_LVAL("label-binding", c_id->label_binding);
            }
            break;
        case VOMITORIUM_FRONTEND_OBJC:
            // maybe identical to C?
            break;
        case VOMITORIUM_FRONTEND_CXX:
            {
                auto cxx_id = (cxx_lang_identifier *)t;
                dump_cci(&cxx_id->c_common);
                DO_LVAL("namespace-bindings", cxx_id->namespace_bindings);
                DO_LVAL("bindings", cxx_id->bindings);
                DO_LVAL("class-template-info", cxx_id->class_template_info);
                DO_LVAL("label-value", cxx_id->label_value);
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
        DO_VAL(t, "purpose", TREE_PURPOSE);
        DO_VAL(t, "value", TREE_VALUE);
    }

    if (code == CONSTRUCTOR)
    {
        VEC(constructor_elt, gc) *elts = TAKE1(t, CONSTRUCTOR_ELTS);
        foreach (const constructor_elt& e, elts)
        {
            // Don't need to TAKE these since they're indirect.
            xml1("index", e.index);
            xml1("value", e.value);
        }
    }

    if (EXPR_P(t))
    {
        const ptrdiff_t len = TREE_OPERAND_LENGTH(t);
        const char *operand_names[len];
        memset(operand_names, 0, sizeof(operand_names));
        int offset = 0;
        if (VL_EXP_CLASS_P(t))
            operand_names[0] = "vl-operand-count";

#define CALC_OPERAND_NAME(t, name, LVALUE)                      \
        ({                                                      \
            ptrdiff_t _i = &LVALUE(t) - &TREE_OPERAND(t, 0);    \
            assert (0 <= _i && _i < len);                       \
            assert (operand_names[_i] == nullptr);              \
            operand_names[_i] = name;                           \
        })
        switch (code)
        {
        case LOOP_EXPR:
            CALC_OPERAND_NAME(t, "body", LOOP_EXPR_BODY);
            break;
        case TARGET_EXPR:
            CALC_OPERAND_NAME(t, "slot", TARGET_EXPR_SLOT);
            CALC_OPERAND_NAME(t, "initial", TARGET_EXPR_INITIAL);
            CALC_OPERAND_NAME(t, "cleanup", TARGET_EXPR_CLEANUP);
            break;
        case DECL_EXPR:
            CALC_OPERAND_NAME(t, "decl", DECL_EXPR_DECL);
            break;
        case EXIT_EXPR:
            CALC_OPERAND_NAME(t, "cond", EXIT_EXPR_COND);
            break;
        case COMPOUND_LITERAL_EXPR:
            CALC_OPERAND_NAME(t, "decl-expr", COMPOUND_LITERAL_EXPR_DECL_EXPR);
            break;
        case SWITCH_EXPR:
            CALC_OPERAND_NAME(t, "cond", SWITCH_COND);
            CALC_OPERAND_NAME(t, "body", SWITCH_BODY);
            CALC_OPERAND_NAME(t, "labels", SWITCH_LABELS);
            break;
        case CASE_LABEL_EXPR:
            CALC_OPERAND_NAME(t, "low", CASE_LOW);
            CALC_OPERAND_NAME(t, "high", CASE_HIGH);
            CALC_OPERAND_NAME(t, "label", CASE_LABEL);
            break;
        case TARGET_MEM_REF:
            CALC_OPERAND_NAME(t, "symbol", TMR_SYMBOL);
            CALC_OPERAND_NAME(t, "base", TMR_BASE);
            CALC_OPERAND_NAME(t, "index", TMR_INDEX);
            CALC_OPERAND_NAME(t, "step", TMR_STEP);
            CALC_OPERAND_NAME(t, "offset", TMR_OFFSET);
            CALC_OPERAND_NAME(t, "original", TMR_ORIGINAL);
            break;
        case BIND_EXPR:
            CALC_OPERAND_NAME(t, "vars", BIND_EXPR_VARS);
            CALC_OPERAND_NAME(t, "body", BIND_EXPR_BODY);
            CALC_OPERAND_NAME(t, "block", BIND_EXPR_BLOCK);
            break;
        case GOTO_EXPR:
            CALC_OPERAND_NAME(t, "destination", GOTO_DESTINATION);
            break;
        case ASM_EXPR:
            CALC_OPERAND_NAME(t, "string", ASM_STRING);
            CALC_OPERAND_NAME(t, "outputs", ASM_OUTPUTS);
            CALC_OPERAND_NAME(t, "inputs", ASM_INPUTS);
            CALC_OPERAND_NAME(t, "clobbers", ASM_CLOBBERS);
            CALC_OPERAND_NAME(t, "labels", ASM_LABELS);
            break;
        case COND_EXPR:
            CALC_OPERAND_NAME(t, "cond", COND_EXPR_COND);
            CALC_OPERAND_NAME(t, "if-true", COND_EXPR_THEN);
            CALC_OPERAND_NAME(t, "if-false", COND_EXPR_ELSE);
            break;
        case POLYNOMIAL_CHREC:
            CALC_OPERAND_NAME(t, "var", CHREC_VAR);
            CALC_OPERAND_NAME(t, "left", CHREC_LEFT);
            CALC_OPERAND_NAME(t, "right", CHREC_RIGHT);
            break;
        case LABEL_EXPR:
            CALC_OPERAND_NAME(t, "label", LABEL_EXPR_LABEL);
            break;
        // VDEF_EXPR does not exist, despite comments citing tree-flow.h
        case CATCH_EXPR:
            CALC_OPERAND_NAME(t, "types", CATCH_TYPES);
            CALC_OPERAND_NAME(t, "body", CATCH_BODY);
            break;
        case EH_FILTER_EXPR:
            CALC_OPERAND_NAME(t, "types", EH_FILTER_TYPES);
            CALC_OPERAND_NAME(t, "failure", EH_FILTER_FAILURE);
            break;
        case OBJ_TYPE_REF:
            CALC_OPERAND_NAME(t, "expr", OBJ_TYPE_REF_EXPR);
            CALC_OPERAND_NAME(t, "object", OBJ_TYPE_REF_OBJECT);
            CALC_OPERAND_NAME(t, "token", OBJ_TYPE_REF_TOKEN);
            break;
        case ASSERT_EXPR:
            CALC_OPERAND_NAME(t, "var", ASSERT_EXPR_VAR);
            CALC_OPERAND_NAME(t, "cond", ASSERT_EXPR_COND);
            break;
        case CALL_EXPR:
            CALC_OPERAND_NAME(t, "fn", CALL_EXPR_FN);
            CALC_OPERAND_NAME(t, "static-chain", CALL_EXPR_STATIC_CHAIN);
            offset = len - call_expr_nargs(t);
            break;
        case OMP_PARALLEL:
            CALC_OPERAND_NAME(t, "body", OMP_PARALLEL_BODY);
            CALC_OPERAND_NAME(t, "clauses", OMP_PARALLEL_CLAUSES);
            break;
        case OMP_TASK:
            CALC_OPERAND_NAME(t, "body", OMP_TASK_BODY);
            CALC_OPERAND_NAME(t, "clauses", OMP_TASK_CLAUSES);
            break;
        case OMP_FOR:
            CALC_OPERAND_NAME(t, "body", OMP_FOR_BODY);
            CALC_OPERAND_NAME(t, "clauses", OMP_FOR_CLAUSES);
            CALC_OPERAND_NAME(t, "init", OMP_FOR_INIT);
            CALC_OPERAND_NAME(t, "cond", OMP_FOR_COND);
            CALC_OPERAND_NAME(t, "incr", OMP_FOR_INCR);
            CALC_OPERAND_NAME(t, "pre-body", OMP_FOR_PRE_BODY);
            break;
        case OMP_SECTIONS:
            CALC_OPERAND_NAME(t, "body", OMP_SECTIONS_BODY);
            CALC_OPERAND_NAME(t, "clauses", OMP_SECTIONS_CLAUSES);
            break;
        case OMP_SECTION:
            CALC_OPERAND_NAME(t, "body", OMP_SECTION_BODY);
            break;
        case OMP_SINGLE:
            CALC_OPERAND_NAME(t, "body", OMP_SINGLE_BODY);
            CALC_OPERAND_NAME(t, "clauses", OMP_SINGLE_CLAUSES);
            break;
        case OMP_MASTER:
            CALC_OPERAND_NAME(t, "body", OMP_MASTER_BODY);
            break;
        case OMP_ORDERED:
            CALC_OPERAND_NAME(t, "body", OMP_ORDERED_BODY);
            break;
        case OMP_CRITICAL:
            CALC_OPERAND_NAME(t, "body", OMP_CRITICAL_BODY);
            CALC_OPERAND_NAME(t, "name", OMP_CRITICAL_NAME);
            break;
        default:
            break;
        }

        if (EXPR_HAS_LOCATION(t))
            xml1("location", expand_location(TAKE2(t, EXPR_LOCATION, SET_EXPR_LOCATION)));

        if (offset)
            xml1("operand-offset", offset);
        for (int i = 0; i < len; ++i)
        {
            tree& t_o = TREE_OPERAND(t, i);
            if (t_o)
            {
                Xml xml(operand_names[i] ?: "operand", "op_i", i);
                xemit(t_o);
                if (i != 0 || !VL_EXP_CLASS_P(t))
                    t_o = NULL_TREE;
            }
        }
        if (VL_EXP_CLASS_P(t))
            TREE_OPERAND(t, 0) = NULL_TREE;
    }

    // this is basically a duplicate of the EXPR_P logic above
    if (code == OMP_CLAUSE)
    {
        const ptrdiff_t len = omp_clause_num_ops[code];
        const char *operand_names[len];
        memset(operand_names, 0, sizeof(operand_names));

#define CALC_OMP_OPERAND_NAME(t, name, LVALUE)                      \
        ({                                                          \
            ptrdiff_t _i = &LVALUE(t) - &OMP_CLAUSE_OPERAND(t, 0);  \
            assert (0 <= _i && _i < len);                           \
            assert (operand_names[_i] == nullptr);              \
            operand_names[_i] = name;                           \
        })
        if (OMP_CLAUSE_HAS_LOCATION(t))
            xml1("location", expand_location(TAKE1(t, OMP_CLAUSE_LOCATION)));
        xml1("clause-code", omp_code);

        if (omp_code >= OMP_CLAUSE_PRIVATE && omp_code <= OMP_CLAUSE_COPYPRIVATE)
        {
            CALC_OMP_OPERAND_NAME(t, "decl", OMP_CLAUSE_DECL);
        }
        switch (code)
        {
        case OMP_CLAUSE_LASTPRIVATE:
            CALC_OMP_OPERAND_NAME(t, "stmt", OMP_CLAUSE_LASTPRIVATE_STMT);
            if (auto v = TAKE1(t, OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ))
                xml1("gimple-seq", v);
            break;
        case OMP_CLAUSE_IF:
            CALC_OMP_OPERAND_NAME(t, "expr", OMP_CLAUSE_IF_EXPR);
            break;
        case OMP_CLAUSE_NUM_THREADS:
            CALC_OMP_OPERAND_NAME(t, "expr", OMP_CLAUSE_NUM_THREADS_EXPR);
            break;
        case OMP_CLAUSE_SCHEDULE:
            DO_VAL(t, "subcode-schedule", OMP_CLAUSE_SCHEDULE_KIND);
            CALC_OMP_OPERAND_NAME(t, "chunk-expr", OMP_CLAUSE_SCHEDULE_CHUNK_EXPR);
            break;
        case OMP_CLAUSE_COLLAPSE:
            CALC_OPERAND_NAME(t, "expr", OMP_CLAUSE_COLLAPSE_EXPR);
            CALC_OPERAND_NAME(t, "itervar", OMP_CLAUSE_COLLAPSE_ITERVAR);
            CALC_OPERAND_NAME(t, "count", OMP_CLAUSE_COLLAPSE_COUNT);
            break;
        case OMP_CLAUSE_REDUCTION:
            DO_VAL(t, "subcode-reduction", OMP_CLAUSE_REDUCTION_CODE);
            CALC_OPERAND_NAME(t, "init", OMP_CLAUSE_REDUCTION_INIT);
            CALC_OPERAND_NAME(t, "merge", OMP_CLAUSE_REDUCTION_MERGE);
            CDO_VAL(t, "gimple-init", OMP_CLAUSE_REDUCTION_GIMPLE_INIT);
            CDO_VAL(t, "gimple-merge", OMP_CLAUSE_REDUCTION_GIMPLE_MERGE);
            CALC_OPERAND_NAME(t, "placeholder", OMP_CLAUSE_REDUCTION_PLACEHOLDER);
            break;
        case OMP_CLAUSE_DEFAULT:
            DO_VAL(t, "subcode-default", OMP_CLAUSE_DEFAULT_KIND);
            break;
        default:
            break;
        }

        // OMP_CLAUSE_CHAIN is just TREE_CHAIN
        for (int i = 0; i < len; ++i)
        {
            tree& t_o = OMP_CLAUSE_OPERAND(t, i);
            if (t_o)
            {
                Xml xml(operand_names[i] ?: "operand", "op_i", i);
                xemit(t_o);
                t_o = NULL_TREE;
            }
        }
    }

    if (code == SSA_NAME)
    {
        DO_VAL(t, "var", SSA_NAME_VAR);
        DO_VAL(t, "defining-statement", SSA_NAME_DEF_STMT);
        DO_VAL(t, "version", SSA_NAME_VERSION);
        DO_VAL(t, "pointer-info", SSA_NAME_PTR_INFO);
        // We don't need to track uses here; they can be recalculated
        Xml uses_xml("uses");

        imm_use_iterator imm_iter;
        use_operand_p use_p;
        FOR_EACH_IMM_USE_FAST (use_p, imm_iter, t)
        {
            xml1("use", *use_p->use);
            gimple use_stmt = USE_STMT(use_p);
            xml1("use-stmt", use_stmt);
        }
        (void)TAKE1(t, SSA_NAME_IMM_USE_NODE);
    }

    if (code == BLOCK)
    {
        DO_VAL(t, "vars", BLOCK_VARS);
        foreach (tree nlv, TAKE1(t, BLOCK_NONLOCALIZED_VARS))
        {
            xml1("nonlocalized-var", nlv);
        }
        DO_VAL(t, "subblocks", BLOCK_SUBBLOCKS);
        DO_VAL(t, "supercontext", BLOCK_SUPERCONTEXT);
        // BLOCK_CHAIN is just TREE_CHAIN
        DO_VAL(t, "abstract-origin", BLOCK_ABSTRACT_ORIGIN);
        DO_BIT(t, "abstract", BLOCK_ABSTRACT);
        DO_VAL(t, "number", BLOCK_NUMBER);
        DO_VAL(t, "fragment-origin", BLOCK_FRAGMENT_ORIGIN);
        DO_VAL(t, "fragment-chain", BLOCK_FRAGMENT_CHAIN);
        DO_VAL(t, "source-location", BLOCK_SOURCE_LOCATION);
    }

    if (TYPE_P(t))
    {
        DO_VAL(t, "uid", TYPE_UID);
        DO_VAL(t, "size", TYPE_SIZE);
        DO_VAL(t, "size-unit", TYPE_SIZE_UNIT);
        if (code == ENUMERAL_TYPE)
            DO_VAL(t, "enum-values", TYPE_VALUES);
        if (code == ARRAY_TYPE)
            DO_VAL(t, "domain", TYPE_DOMAIN);
        if (RECORD_OR_UNION_TYPE_P(t))
        {
            DO_VAL(t, "fields", TYPE_FIELDS);
            DO_VAL(t, "methods", TYPE_METHODS);
            DO_VAL(t, "vfield", TYPE_VFIELD);
        }
        if (code == FUNCTION_TYPE || code == METHOD_TYPE)
        {
            DO_VAL(t, "arg-types", TYPE_ARG_TYPES);
            DO_VAL(t, "method-basetype", TYPE_METHOD_BASETYPE);
        }
        if (code == OFFSET_TYPE)
            DO_VAL(t, "offset-basetype", TYPE_OFFSET_BASETYPE);
        DO_VAL(t, "pointer-to", TYPE_POINTER_TO);
        DO_VAL(t, "reference-to", TYPE_REFERENCE_TO);
        if (code == POINTER_TYPE)
            DO_VAL(t, "next-ptr-to", TYPE_NEXT_PTR_TO);
        if (code == REFERENCE_TYPE)
            DO_VAL(t, "next-ref-to", TYPE_NEXT_REF_TO);
        if (code == INTEGER_TYPE || code == ENUMERAL_TYPE || code == BOOLEAN_TYPE || code == REAL_TYPE || code == FIXED_POINT_TYPE)
        {
            DO_VAL(t, "min", TYPE_MIN_VALUE);
            DO_VAL(t, "max", TYPE_MAX_VALUE);
        }
        if (code == VECTOR_TYPE)
        {
            xml1("vector-subparts", TYPE_VECTOR_SUBPARTS(t));
            SET_TYPE_VECTOR_SUBPARTS(t, 1);
        }
        CDO_VAL(t, "precision", TYPE_PRECISION);
        DO_VAL(t, "symtab-address", TYPE_SYMTAB_ADDRESS);
        CDO_VAL(t, "symtab-pointer", TYPE_SYMTAB_POINTER);
        DO_VAL(t, "symtab-die", TYPE_SYMTAB_DIE);
        DO_VAL(t, "name", TYPE_NAME);
        DO_VAL(t, "next-variant", TYPE_NEXT_VARIANT);
        DO_VAL(t, "main-variant", TYPE_MAIN_VARIANT);
        DO_VAL(t, "context", TYPE_CONTEXT);
        {
            // vector types need to look at parts of the real tree
            xml1("mode", TYPE_MODE(orig_tree));
            SET_TYPE_MODE(t, (enum machine_mode)0);
        }
        DO_VAL(t, "canonical", TYPE_CANONICAL);
        DO_VAL(t, "lang-specific", TYPE_LANG_SPECIFIC);
        if (code == VECTOR_TYPE)
            DO_VAL(t, "debug-representation", TYPE_DEBUG_REPRESENTATION_TYPE);
        if (RECORD_OR_UNION_TYPE_P(t))
            DO_VAL(t, "binfo", TYPE_BINFO);
        else
            DO_VAL(t, "lang-slot-1", TYPE_LANG_SLOT_1);
        DO_VAL(t, "alias-set", TYPE_ALIAS_SET);
        DO_VAL(t, "attributes", TYPE_ATTRIBUTES);
        DO_VAL(t, "align", TYPE_ALIGN);
        DO_BIT(t, "user-align", TYPE_USER_ALIGN);
        if (code == INTEGER_TYPE)
            DO_BIT(t, "is-sizetype", TYPE_IS_SIZETYPE);
        DO_BIT(t, "no-force-blk", TYPE_NO_FORCE_BLK);
        DO_VAL(t, "address-space", TYPE_ADDR_SPACE);

        switch (vomitorium_current_frontend)
        {
        default:
            DO_BIT(t, "type-lang-flag-0", TYPE_LANG_FLAG_0);
            DO_BIT(t, "type-lang-flag-1", TYPE_LANG_FLAG_1);
            DO_BIT(t, "type-lang-flag-2", TYPE_LANG_FLAG_2);
            DO_BIT(t, "type-lang-flag-3", TYPE_LANG_FLAG_3);
            DO_BIT(t, "type-lang-flag-4", TYPE_LANG_FLAG_4);
            DO_BIT(t, "type-lang-flag-5", TYPE_LANG_FLAG_5);
            DO_BIT(t, "type-lang-flag-6", TYPE_LANG_FLAG_6);
        }
        DO_BIT(t, "is-string", TYPE_STRING_FLAG);
        if (code == ARRAY_TYPE)
            DO_VAL(t, "array-max-size", TYPE_ARRAY_MAX_SIZE);
        DO_BIT(t, "needs-constructing", TYPE_NEEDS_CONSTRUCTING);
        if (RECORD_OR_UNION_TYPE_P(t))
            DO_BIT(t, "transparent-aggregate", TYPE_TRANSPARENT_AGGR);
        if (code == ARRAY_TYPE)
            DO_BIT(t, "nonaliased-component", TYPE_NONALIASED_COMPONENT);
        DO_VAL(t, "placeholder-internal", TYPE_CONTAINS_PLACEHOLDER_INTERNAL);

        // generic fields used for multiple reasons
        CDO_VAL(t, "cached-values", TYPE_CACHED_VALUES);
        CDO_VAL(t, "maxval", TYPE_MAXVAL);
        CDO_VAL(t, "minval", TYPE_MINVAL);
    }

    if (code == TREE_BINFO)
    {
        DO_VAL(t, "offset", BINFO_OFFSET);
        DO_VAL(t, "vtable", BINFO_VTABLE);
        DO_VAL(t, "virtual-functions", BINFO_VIRTUALS);
        DO_VAL(t, "vptr-field", BINFO_VPTR_FIELD);
        VEC(tree, gc) *base_accesses = TAKE1(t, BINFO_BASE_ACCESSES);
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
        DO_VAL(t, "subvtt-index", BINFO_SUBVTT_INDEX);
        DO_VAL(t, "vptr-index", BINFO_VPTR_INDEX);
        DO_VAL(t, "inheritance-chain", BINFO_INHERITANCE_CHAIN);

        // This is an embedded VEC(tree, none)
        Xml base_binfos_xml("base-binfos");
        int length = BINFO_N_BASE_BINFOS(t);
        for (int i = 0; i < length; ++i)
        {
            Xml xml("e", "i", i);
            xemit(BINFO_BASE_BINFO(t, i));
        }
        void *z_base = BINFO_BASE_BINFOS(t);
        memset(z_base, 0, tree_sizeof - ((char *)z_base - (char *)t));
    }

    if (code == FUNCTION_DECL)
    {
        if (DECL_BUILT_IN(t))
            DO_VAL(t, "function-code", DECL_FUNCTION_CODE);
        else
            assert (DECL_FUNCTION_CODE(t) == 0);
        DO_VAL(t, "personality", DECL_FUNCTION_PERSONALITY);
        DO_VAL(t, "result", DECL_RESULT);
        DO_BIT(t, "uninlinable", DECL_UNINLINABLE);
        DO_VAL(t, "saved-tree", DECL_SAVED_TREE);
        DO_BIT(t, "is-malloc", DECL_IS_MALLOC);
        DO_BIT(t, "is-operator-new", DECL_IS_OPERATOR_NEW);
        DO_BIT(t, "returns-twice", DECL_IS_RETURNS_TWICE);
        DO_BIT(t, "pure", DECL_PURE_P);
        DO_BIT(t, "looping-const-or-pure", DECL_LOOPING_CONST_OR_PURE_P);
        DO_BIT(t, "novops", DECL_IS_NOVOPS);
        DO_BIT(t, "static-constructor", DECL_STATIC_CONSTRUCTOR);
        DO_BIT(t, "static-destructor", DECL_STATIC_DESTRUCTOR);
        DO_BIT(t, "no-instrument-entry-exit", DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT);
        DO_BIT(t, "no-limit-stack", DECL_NO_LIMIT_STACK);
        DO_BIT(t, "static-chain", DECL_STATIC_CHAIN);
        DO_BIT(t, "possibly-inlined", DECL_POSSIBLY_INLINED);
        DO_BIT(t, "declared-inline", DECL_DECLARED_INLINE_P);
        DO_BIT(t, "no-inline-warning", DECL_NO_INLINE_WARNING_P);
        DO_BIT(t, "disregard-inline-limits", DECL_DISREGARD_INLINE_LIMITS);
        DO_VAL(t, "struct-function", DECL_STRUCT_FUNCTION);
        DO_VAL(t, "builtin-class", DECL_BUILT_IN_CLASS);
        DO_VAL(t, "arguments", DECL_ARGUMENTS);
        DO_VAL(t, "function-specific-target", DECL_FUNCTION_SPECIFIC_TARGET);
        DO_VAL(t, "function-specific-optimization", DECL_FUNCTION_SPECIFIC_OPTIMIZATION);
    }
    if (code == FUNCTION_DECL || code == VAR_DECL)
        DO_BIT(t, "extern", DECL_EXTERNAL);
    if (code == VAR_DECL || code == PARM_DECL)
    {
        DO_BIT(t, "debug-has-value-expr", DECL_HAS_VALUE_EXPR_P);
        xml1("debug-value-expr", DECL_VALUE_EXPR(CONST_CAST_TREE(orig_tree)));
        DO_BIT(t, "register", DECL_REGISTER);
    }
    if (HAS_RTL_P(t))
    {
#define GET_RTL_DAMMIT(NODE) ((NODE)->decl_with_rtl.rtl)
        DO_VAL(t, "rtl", GET_RTL_DAMMIT);
    }
    if (code == FIELD_DECL)
    {
        DO_VAL(t, "offset", DECL_FIELD_OFFSET);
        DO_VAL(t, "bit-offset", DECL_FIELD_BIT_OFFSET);
        DO_VAL(t, "bitfield-type", DECL_BIT_FIELD_TYPE);
        DO_VAL(t, "qualifier", DECL_QUALIFIER);
        xml1("offset-align", DECL_OFFSET_ALIGN(t));
        SET_DECL_OFFSET_ALIGN(t, 1);
        DO_VAL(t, "fcontext", DECL_FCONTEXT);
        DO_BIT(t, "bitfield", DECL_BIT_FIELD);
        DO_BIT(t, "nonaddressable", DECL_NONADDRESSABLE_P);
    }
    if (code == LABEL_DECL)
    {
        DO_VAL(t, "uid", LABEL_DECL_UID);
        DO_VAL(t, "eh-landing-pad-number", EH_LANDING_PAD_NR);
        DO_BIT(t, "error-issued", DECL_ERROR_ISSUED);
    }
    if (code == PARM_DECL)
    {
        DO_VAL(t, "incoming-rtl", DECL_INCOMING_RTL);
    }
    if (code == VAR_DECL)
    {
        DO_BIT(t, "in-text-section", DECL_IN_TEXT_SECTION);
        DO_BIT(t, "hard-register", DECL_HARD_REGISTER);
        xml1("debug-split-expr", DECL_DEBUG_EXPR(CONST_CAST_TREE(orig_tree)));
        DO_BIT(t, "has-init-priority", DECL_HAS_INIT_PRIORITY_P);
        xml1("init-priority", DECL_INIT_PRIORITY(CONST_CAST_TREE(orig_tree)));
        xml1("fini-priority", DECL_FINI_PRIORITY(CONST_CAST_TREE(orig_tree)));
        DO_VAL(t, "tls-model", DECL_TLS_MODEL);
    }
    if (code == VAR_DECL || code == PARM_DECL || code == RESULT_DECL)
    {
#if GCCPLUGIN_VERSION < 4008
# define DECL_VAR_ANN(NODE) (*DECL_VAR_ANN_PTR(NODE))
        DO_VAL(t, "var-ann", DECL_VAR_ANN);
#endif
    }
    if (code == TYPE_DECL)
    {
        DO_VAL(t, "original-type", DECL_ORIGINAL_TYPE);
        DO_BIT(t, "suppress-debug", TYPE_DECL_SUPPRESS_DEBUG);
    }

    if (CODE_CONTAINS_STRUCT(code, TS_DECL_MINIMAL))
    {
        DO_VAL(t, "uid", DECL_UID);
        DO_VAL(t, "name", DECL_NAME);
        if (DECL_IS_BUILTIN(t))
            xml0("builtin");
        DO_VAL(t, "location", DECL_SOURCE_LOCATION);
        if (code == FIELD_DECL)
            DO_VAL(t, "field-context", DECL_FIELD_CONTEXT);
        CDO_VAL(t, "context", DECL_CONTEXT);
    }
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_COMMON))
    {
        DO_VAL(t, "abstract-origin", DECL_ABSTRACT_ORIGIN);
        DO_VAL(t, "attributes", DECL_ATTRIBUTES);
        if (code == IMPORTED_DECL)
            DO_VAL(t, "associated-decl", IMPORTED_DECL_ASSOCIATED_DECL);
        if (code == PARM_DECL)
            DO_VAL(t, "arg-type", DECL_ARG_TYPE);
        CDO_VAL(t, "initial", DECL_INITIAL);
        DO_VAL(t, "size", DECL_SIZE);
        DO_VAL(t, "size-bytes", DECL_SIZE_UNIT);
        DO_VAL(t, "align", DECL_ALIGN);
        DO_BIT(t, "user-align", DECL_USER_ALIGN);
        DO_VAL(t, "mode", DECL_MODE);
        DO_VAL(t, "debug-expr-is-from", DECL_DEBUG_EXPR_IS_FROM);
        DO_BIT(t, "debug-ignored", DECL_IGNORED_P);
        DO_BIT(t, "debug-abstract", DECL_ABSTRACT);
        DO_VAL(t, "lang-specific", DECL_LANG_SPECIFIC);
        DO_BIT(t, "nonlocal", DECL_NONLOCAL);
        DO_BIT(t, "virtual", DECL_VIRTUAL_P);
        DO_BIT(t, "artificial", DECL_ARTIFICIAL);
        switch (vomitorium_current_frontend)
        {
        default:
            DO_BIT(t, "decl-lang-flag-0", DECL_LANG_FLAG_0);
            DO_BIT(t, "decl-lang-flag-1", DECL_LANG_FLAG_1);
            DO_BIT(t, "decl-lang-flag-2", DECL_LANG_FLAG_2);
            DO_BIT(t, "decl-lang-flag-3", DECL_LANG_FLAG_3);
            DO_BIT(t, "decl-lang-flag-4", DECL_LANG_FLAG_4);
            DO_BIT(t, "decl-lang-flag-5", DECL_LANG_FLAG_5);
            DO_BIT(t, "decl-lang-flag-6", DECL_LANG_FLAG_6);
            DO_BIT(t, "decl-lang-flag-7", DECL_LANG_FLAG_7);
            DO_BIT(t, "decl-lang-flag-8", DECL_LANG_FLAG_8);
        }
        DO_BIT(t, "attribute-used", DECL_PRESERVE_P);
        DO_BIT(t, "gimple-register", DECL_GIMPLE_REG_P);
    }
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_WITH_VIS))
    {
        DO_BIT(t, "seen-in-bind-expr", DECL_SEEN_IN_BIND_EXPR_P);
        DO_BIT(t, "defer-output", DECL_DEFER_OUTPUT);
        DO_BIT(t, "weak", DECL_WEAK);
        DO_BIT(t, "dllimport", DECL_DLLIMPORT_P);
        DO_BIT(t, "comdat", DECL_COMDAT);
        DO_VAL(t, "comdat-group", DECL_COMDAT_GROUP);
        if (DECL_ASSEMBLER_NAME_SET_P(t))
        {
            DO_VAL2(t, "assembler-name", DECL_ASSEMBLER_NAME, SET_DECL_ASSEMBLER_NAME);
        }
        DO_VAL(t, "section-name", DECL_SECTION_NAME);
        DO_VAL(t, "visibility", DECL_VISIBILITY);
        DO_BIT(t, "visibility-specified", DECL_VISIBILITY_SPECIFIED);
        DO_BIT(t, "common", DECL_COMMON);
    }
    if (CODE_CONTAINS_STRUCT(code, TS_DECL_NON_COMMON))
    {
        DO_VAL(t, "result-field", DECL_RESULT_FLD);
        DO_VAL(t, "vindex", DECL_VINDEX);
        DO_VAL(t, "argument-field", DECL_ARGUMENT_FLD);
    }

    if (code == STATEMENT_LIST)
    {
        Xml statement_list_xml("statement-list");
        int i = 0;
        for (tree_statement_list_node *node = STATEMENT_LIST_HEAD(t); node; node = node->next)
        {
            Xml xml("e", "i", i);
            xemit(node->stmt);
            ++i;
        }
        STATEMENT_LIST_HEAD(t) = nullptr;
        STATEMENT_LIST_TAIL(t) = nullptr;
    }

    if (code == OPTIMIZATION_NODE)
    {
#define TREE_OPTIMIZATION_REALLY(NODE) (*(TREE_OPTIMIZATION(NODE)))
        DO_VAL(t, "optimization-option", TREE_OPTIMIZATION_REALLY);
    }
    if (code == TARGET_OPTION_NODE)
    {
#define TREE_TARGET_OPTION_REALLY(NODE) (*(TREE_TARGET_OPTION(NODE)))
        DO_VAL(t, "target-option", TREE_TARGET_OPTION_REALLY);
    }

    // TODO: should we do one last pass over named bits?  NO!
    if (false)
        (void)0;
    if (true)
        TREE_SET_CODE(t, (enum tree_code)0);
    if (!is_all_zero(t, tree_sizeof))
    {
        dump_remaining(t, tree_sizeof);
        incomplete_dumps++;
    }
}
#if GCC_VERSION >= 4006
# pragma GCC diagnostic pop
#endif

template<class E, typename=typename std::is_enum<E>::type>
E& operator ++(E& e)
{
    return e = (E)(e + 1);
}

static void dump_all()
{
    xemit_raw("<?xml version=\"1.0\" encoding=\"ascii\"?>", 38);
    nl();
    Xml root("vomitorium-dump", "version", 1);
    nl();

    {
        Xml all_globals("globals");
        nl();

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

#if GCCPLUGIN_VERSION < 4006
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
#if GCCPLUGIN_VERSION >= 5000
            // Ugh, some names are NULL!
            // But the values are NULL too, so we aren't missing anything.
            if (!built_in_names[i] && !builtin_decl_explicit(i))
                continue;
#endif
            {
                Global xml("explicit_built_in_decls", i);
#if GCCPLUGIN_VERSION < 4007
                xemit(built_in_decls[i]);
#else
                xemit(builtin_decl_explicit(i));
#endif
            }
            {
                Global xml("implicit_built_in_decls", i);
#if GCCPLUGIN_VERSION < 4007
                xemit(implicit_built_in_decls[i]);
#else
                xemit(builtin_decl_implicit((enum built_in_function)i));
#endif
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

#if GCCPLUGIN_VERSION >= 4008
# define TYPE_KIND_LAST stk_type_kind_last
#endif
        for (enum size_type_kind i = (enum size_type_kind)0; i < TYPE_KIND_LAST; ++i)
        {
            Global xml("sizetype_tab", i);
            xemit(sizetype_tab[i]);
        }

        global1("current_function_decl", current_function_decl);

#if GCCPLUGIN_VERSION < 4006
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
#if GCCPLUGIN_VERSION >= 4008
            global1("tls_aggregates", tls_aggregates);
#endif

            // TODO operator_name_info and assignment_operator_name_info

#if GCCPLUGIN_VERSION < 6000
            globalv("deferred_mark_used_calls", deferred_mark_used_calls);
#endif

            globalv("unemitted_tinfo_decls", unemitted_tinfo_decls);

            global1("global_namespace", global_namespace);
#if GCCPLUGIN_VERSION < 8000
            global1("global_scope_name", global_scope_name);
#endif
            global1("global_type_node", global_type_node);
        } // C++ only

#if GCCPLUGIN_VERSION >= 4006
    // TODO default_target_rtl has trees

    globalv("all_translation_units", all_translation_units);
#endif

#if GCCPLUGIN_VERSION >= 4007
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

#if GCCPLUGIN_VERSION >= 4009
        for (enum internal_fn i = (enum internal_function)0; i < IFN_LAST + 1; ++i)
        {
            Global xml("internal_fn_fnspec_array", i);
            xemit(internal_fn_fnspec_array[i]);
        }
#endif

#if GCCPLUGIN_VERSION >= 5000 && GCCPLUGIN_VERSION < 7000
        global1("block_clear_fn", block_clear_fn);
#endif

#if GCCPLUGIN_VERSION >= 5000
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
#if GCCPLUGIN_VERSION >= 6000
        globalv("vtbl_mangled_name_types", vtbl_mangled_name_types);
        globalv("vtbl_mangled_name_ids", vtbl_mangled_name_ids);
#endif
#if GCCPLUGIN_VERSION >= 8000
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
        nl();
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
