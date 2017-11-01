#include "internal.hpp"
#include "names.hpp"
#include "iter.hpp"

#include <cassert>

#include <map>
#include <vector>


// For XML this is generally a good choice,
// although since we aren't doing much nesting, we *could* use 4.
#define INDENT_SPACES 2
bool start_of_line = true;
size_t indent;
std::vector<const_tree> interned_tree_list = {NULL_TREE};
std::map<const_tree, size_t> interned_tree_ids = {{NULL_TREE, 0}};

static size_t intern(const_tree t)
{
    auto pair = interned_tree_ids.insert(std::make_pair(t, interned_tree_ids.size()));
    if (pair.second)
        interned_tree_list.push_back(t);
    return pair.first->second;
}


static const char many_spaces[] = "   ";
static void emit_spaces(size_t n)
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
static void emit_raw(const char *s, size_t len)
{
    if (start_of_line)
    {
        start_of_line = false;
        emit_spaces(indent * INDENT_SPACES);
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
    emit_raw("\n", 1);
    start_of_line = true;
}

static void emit(const char *s)
{
    assert (s && "Can't emit a NULL string!");
    for (size_t i = 0; s[i]; ++i)
    {
        assert (' ' <= s[i] && s[i] <= '~');
    }

    while (true)
    {
        size_t len = strcspn(s, "<>&\"");
        if (len)
        {
            emit_raw(s, len);
            s += len;
        }

        while (true)
        {
            switch (*s)
            {
            case '<':
                emit_raw("&lt;", 4);
                ++s;
                continue;
            case '>':
                emit_raw("&gt;", 4);
                ++s;
                continue;
            case '&':
                emit_raw("&amp;", 5);
                ++s;
                continue;
            case '"':
                emit_raw("&quot;", 6);
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
static void emit(T i)
{
    fprintf(vomitorium_output, "%ju", (uintmax_t)i);
}

template<class T, typename=typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type, typename=void>
static void emit(T i)
{
    fprintf(vomitorium_output, "%jd", (intmax_t)i);
}

static void emit(const_tree t)
{
    emit("@");
    emit(intern(t));
}

class Xml
{
    const char *tag;
public:
    Xml(Xml&&) = delete;
    Xml& operator = (Xml&&) = delete;

    Xml(const char *t) : tag(t)
    {
        emit_raw("<", 1);
        emit(this->tag);
        emit_raw(">", 1);
        indent += 1;
    }
    template<class V>
    Xml(const char *t, const char *k, V v) : tag(t)
    {
        emit_raw("<", 1);
        emit(this->tag);
        emit_raw(" ", 1);
        emit(k);
        emit_raw("=\"", 2);
        emit(v);
        emit_raw("\">", 2);
        indent += 1;
    }
    ~Xml()
    {
        indent -= 1;
        emit_raw("</", 2);
        emit(this->tag);
        emit_raw(">", 1);
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
static void emit(index_pair<T> p)
{
    emit(p.array);
    emit("[");
    emit(p.index);
    emit("]");
}
template<class T>
struct field_pair
{
    T lhs;
    const char *field;
};
template<class T>
static void emit(field_pair<T> p)
{
    emit(p.lhs);
    emit(".");
    emit(p.field);
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
    emit(v);
}

template<class T>
static void global1(const char *name, T v)
{
    Global xml(name);
    emit(v);
}

static void globalv(const char *name, VEC(tree, gc) *v)
{
    size_t j = 0;
    foreach (tree t, v)
    {
        Global xml(name, j);
        emit(t);
        j++;
    }
}

static void dump_tree(const_tree t)
{
    // This will be used to detect any unimplemented tree details
    // TODO only set this based on the struct
    union tree_node tree_bitmask;
    // or just deref? We probably don't want a bunch of =0s?
    memset(&tree_bitmask, -1, sizeof(tree_bitmask));

    Xml xml("tree", "id", t);
    if (!t)
        return;
    nl();
    {
        int code = (int)TREE_CODE(t);
        tree_bitmask.base.code = (enum tree_code)0;
        assert (code < (int)MAX_TREE_CODES);
#if GCCPLUGIN_VERSION < 4009
#define get_tree_code_name(code) tree_code_name[code]
#endif
        Xml code_xml("code", "name", get_tree_code_name((enum tree_code)code));
        nl();

        xml1("code-class", TREE_CODE_CLASS_STRING(TREE_CODE_CLASS(code)));
        xml1("code-length", TREE_CODE_LENGTH(code));
        xml1("structure", ts_enum_names[tree_node_structure(t)]);
    }

    (void)t;
}

static void dump_all()
{
    emit_raw("<?xml version=\"1.0\" encoding=\"ascii\"?>", 38);
    nl();
    Xml root("vomitorium-dump", "version", 1);
    nl();

    {
        Xml all_globals("globals");
        nl();

        for (int i = 0; i < (int)RID_MAX; ++i)
        {
            Global xml("ridpointers", rid_names[i]);
            emit(ridpointers[i]);
        }

        for (int i = 0; i < (int)CTI_MAX; ++i)
        {
            Global xml("c_global_trees", cti_names[i]);
            emit(c_global_trees[i]);
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
            emit(p.target);
        }

        for (int i = 0; i < (int)END_BUILTINS; ++i)
        {
#if GCCPLUGIN_VERSION >= 5000
            // Ugh, some names are NULL!
            // But the values are NULL too, so we aren't missing anything.
            if (!built_in_names[i] && !builtin_decl_explicit((enum built_in_function)i))
                continue;
#endif
            {
                Global xml("explicit_built_in_decls", built_in_names[i]);
#if GCCPLUGIN_VERSION < 4007
                emit(built_in_decls[i]);
#else
                emit(builtin_decl_explicit((enum built_in_function)i));
#endif
            }
            {
                Global xml("implicit_built_in_decls", built_in_names[i]);
#if GCCPLUGIN_VERSION < 4007
                emit(implicit_built_in_decls[i]);
#else
                emit(builtin_decl_implicit((enum built_in_function)i));
#endif
            }
        }

        for (int i = 0; i < (int)TI_MAX; ++i)
        {
            Global xml("global_trees", ti_names[i]);
            emit(global_trees[i]);
        }

        for (int i = 0; i < (int)itk_none; ++i)
        {
            Global xml("integer_types", itk_names[i]);
            emit(integer_types[i]);
        }

#if GCCPLUGIN_VERSION >= 4008
# define TYPE_KIND_LAST stk_type_kind_last
#endif
        for (int i = 0; i < (int)TYPE_KIND_LAST; ++i)
        {
            Global xml("sizetype_tab", stk_names[i]);
            emit(sizetype_tab[i]);
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
            for (int i = 0; i < (int)CPTI_MAX; ++i)
            {
                Global xml("cp_global_trees", cpti_names[i]);
                emit(cp_global_trees[i]);
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
            emit(global_regs_decl[i]);
        }

        global1("pragma_extern_prefix", pragma_extern_prefix);

        // TODO symtab_node has trees
        // TODO asm_node has trees
#endif

        // TODO ipa_edge_args_vector has trees
        // TODO ipa_node_agg_replacements has a tree

#if GCCPLUGIN_VERSION >= 4009
        for (int i = 0; i < IFN_LAST + 1; ++i)
        {
            Global xml("internal_fn_fnspec_array", internal_fn_name_array[i]);
            emit(internal_fn_fnspec_array[i]);
        }
#endif

#if GCCPLUGIN_VERSION >= 5000 && GCCPLUGIN_VERSION < 7000
        global1("block_clear_fn", block_clear_fn);
#endif

#if GCCPLUGIN_VERSION >= 5000
        for (int i = 0; i < CILK_TI_MAX; ++i)
        {
            Global xml("cilk_trees", cilk_ti_names[i]);
            emit(cilk_trees[i]);
        }

        global1("registered_builtin_types", registered_builtin_types);

        for (int i = 0; i < NUM_INT_N_ENTS; ++i)
        {
            {
                Xml("global", "name", field_pair<index_pair<int>>{{"int_n_trees", i}, "signed_type"});
                emit(int_n_trees[i].signed_type);
            }
            {
                Xml("global", "name", field_pair<index_pair<int>>{{"int_n_trees", i}, "unsigned_type"});
                emit(int_n_trees[i].unsigned_type);
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
}
static void do_dump(void *, void *)
{
    dump_all();
}

void enable_dump_v1()
{
    register_callback("vomitorium", PLUGIN_PRE_GENERICIZE, do_dump, nullptr);
}
