// mumble, avoid poison
#include <cstdlib>

#include "internal.hpp"

#include <cassert>

#include <string>
#include <map>
#include <memory>


static plugin_info vomitorium_info =
{
    "v0.0.0",
    "See vomitorium's README.md",
};


struct Options
{
    bool debug_events;
    bool dump;
    bool hello;
    bool info;
    const char *output;

    Options()
    {
        memset(this, 0, sizeof(*this));
    }
};

class AbstractOptionSetter
{
public:
    virtual bool set_option(Options *opts, const char *val) = 0;
    // Not actually *necessary*, since I ended up using shared_ptr,
    // but it's still polite.
    virtual ~AbstractOptionSetter() {}
};

template<class T>
class PmdOptionSetter : public AbstractOptionSetter
{
    T Options::*member_pointer;
public:
    PmdOptionSetter(T Options::*mp) : member_pointer(mp) {}

    virtual bool parse_option(T *out, const char *val) = 0;

    virtual bool set_option(Options *opts, const char *val) override
    {
        T *ptr = &(opts->*(this->member_pointer));
        return parse_option(ptr, val);
    }
};

class BoolOptionSetter : public PmdOptionSetter<bool>
{
public:
    template<class... A>
    BoolOptionSetter(A&&... a) : PmdOptionSetter<bool>(std::forward<A>(a)...) {}

    virtual bool parse_option(bool *out, const char *val) override
    {
        if (val)
            return false;
        *out = true;
        return true;
    }
};
static std::shared_ptr<BoolOptionSetter> make_option_setter(bool Options::*mp)
{
    return std::make_shared<BoolOptionSetter>(mp);
}

class StringOptionSetter : public PmdOptionSetter<const char *>
{
public:
    template<class... A>
    StringOptionSetter(A&&... a) : PmdOptionSetter<const char *>(std::forward<A>(a)...) {}

    virtual bool parse_option(const char **out, const char *val) override
    {
        if (!val)
            return false;
        *out = val;
        return true;
    }
};
static std::shared_ptr<StringOptionSetter> make_option_setter(const char *Options::*mp)
{
    return std::make_shared<StringOptionSetter>(mp);
}

class OptionSetter
{
    std::shared_ptr<AbstractOptionSetter> impl;
public:
    template<class T>
    OptionSetter(T v) : impl(make_option_setter(v)) {}

    bool set_option(Options *opts, const char *val)
    {
        return impl->set_option(opts, val);
    }
};


static std::map<std::string, OptionSetter> option_map =
{
    {"debug_events", &Options::debug_events},
    {"dump", &Options::dump},
    {"hello", &Options::hello},
    {"info", &Options::info},
    {"output", &Options::output},
};


static void check_initialized(void *, void *)
{
    if (vomitorium_current_frontend == VOMITORIUM_FRONTEND_NONE)
    {
        fprintf(stderr, "Warning: vomitorium plugin was never initialized!\n");
        fprintf(stderr, "         If you are using it as a library, you must\n");
        fprintf(stderr, "         pass both it and your own plugin to -fplugin=\n");
        exit(1);
    }
}

__attribute__((constructor))
static void do_check_initialized()
{
    register_callback("vomitorium", PLUGIN_ATTRIBUTES, check_initialized, nullptr);
}


static std::map<std::string, vomitorium_frontend> frontend_map =
{
    {"GNU Ada", VOMITORIUM_FRONTEND_ADA},
    {"GNU Brig", VOMITORIUM_FRONTEND_BRIG},
    {"GNU C", VOMITORIUM_FRONTEND_C},
    {"GNU C++", VOMITORIUM_FRONTEND_CXX},   // or OBJCXX - ick
    {"GNU Objective-C", VOMITORIUM_FRONTEND_OBJC},
    //{"GNU C++", VOMITORIUM_FRONTEND_OBJCXX},
    {"GNU D", VOMITORIUM_FRONTEND_D},
    {"GNU Fortran", VOMITORIUM_FRONTEND_FORTRAN},
    {"GNU Go", VOMITORIUM_FRONTEND_GO},
    {"GNU Java", VOMITORIUM_FRONTEND_JAVA},
};

vomitorium_frontend vomitorium_current_frontend;
vomitorium_frontend vomitorium_calc_frontend()
{
    if (vomitorium_current_frontend == VOMITORIUM_FRONTEND_NONE)
    {
        std::string name = lang_hooks.name;
        // Recent versions include the year of the standard at the end
        while (!name.empty() && isdigit(name[name.size()-1]))
            name.resize(name.size()-1);
        auto it = frontend_map.find(name);
        if (it == frontend_map.end())
        {
            printf("Warning: unknown frontend '%s'\n", lang_hooks.name);
            vomitorium_current_frontend = VOMITORIUM_FRONTEND_UNKNOWN;
        }
        else
        {
            vomitorium_current_frontend = it->second;
            switch (vomitorium_current_frontend)
            {
            case VOMITORIUM_FRONTEND_C:
                assert (c_language == clk_c);
                break;
            case VOMITORIUM_FRONTEND_OBJC:
                assert (c_language == clk_objc);
                break;
            case VOMITORIUM_FRONTEND_CXX:
                if (c_language == clk_objcxx)
                {
                    vomitorium_current_frontend = VOMITORIUM_FRONTEND_OBJCXX;
                    break;
                }
                assert (c_language == clk_cxx);
                break;
            case VOMITORIUM_FRONTEND_OBJCXX:
                assert(0 && "ObjC++ always lied in the past! Did it stop?");
                break;
            default:
                break;
            }
        }
    }
    return vomitorium_current_frontend;
}


int plugin_init (struct plugin_name_args *plugin_info,
                 struct plugin_gcc_version *version)
{
    vomitorium_calc_frontend();

    if (!plugin_default_version_check (version, &gcc_version))
    {
        printf("Version mismatch! Aborting ...\n");
        return 1;
    }

    // one of 3 pseudo-callbacks
    // we don't need to register any passes or GGC roots.
    register_callback("vomitorium", PLUGIN_INFO, nullptr, (void *)&vomitorium_info);

    assert (plugin_info->version == vomitorium_info.version);
    assert (plugin_info->help == vomitorium_info.help);

    Options options;

    for (int i = 0; i < plugin_info->argc; ++i)
    {
        plugin_argument *arg = &plugin_info->argv[i];
        auto it = option_map.find(arg->key);
        if (it == option_map.end())
        {
            fprintf(stderr, "Warning: unknown vomitorium option '%s'\n", arg->key);
            continue;
        }
        bool ok = it->second.set_option(&options, arg->value);
        if (!ok)
        {
            fprintf(stderr, "Warning: unknown vomitorium option value '%s'='%s'\n", arg->key, arg->value);
        }
    }

    if (options.output)
    {
        if (!(vomitorium_output = fopen(options.output, "w")))
        {
            fprintf(stderr, "Error: vomitorium unable to open output '%s'\n", options.output);
            exit(1);
        }
    }
    else
    {
        vomitorium_output = stdout;
    }

    if (options.hello)
        vomitorium_hello();

    if (options.info)
    {
        printf("compiled with %d, for %s [code '%c'] %d\n", GCCPLUGIN_VERSION, lang_hooks.name, vomitorium_current_frontend, GCC_VERSION);

        printf("compile-time GCC basever: %s\n", gcc_version.basever);
        printf("compile-time GCC datestamp: %s\n", gcc_version.datestamp);
        printf("compile-time GCC devphase: %s\n", gcc_version.devphase);
        printf("compile-time GCC revision: %s\n", gcc_version.revision);
        printf("compile-time GCC configuration_arguments: %s\n", gcc_version.configuration_arguments);

        printf("runtime GCC basever: %s\n", version->basever);
        printf("runtime GCC datestamp: %s\n", version->datestamp);
        printf("runtime GCC devphase: %s\n", version->devphase);
        printf("runtime GCC revision: %s\n", version->revision);
        printf("runtime GCC configuration_arguments: %s\n", version->configuration_arguments);

        printf("PLUGIN base_name: %s\n", plugin_info->base_name);
        printf("PLUGIN full_name: %s\n", plugin_info->full_name);
        printf("PLUGIN argc: %d\n", plugin_info->argc);
        printf("PLUGIN argv: %p\n", plugin_info->argv);
        for (int i = 0; i < plugin_info->argc; ++i)
        {
            plugin_argument *arg = &plugin_info->argv[i];
            printf("    %s=%s\n", arg->key, arg->value);
        }
        printf("PLUGIN version: %s\n", plugin_info->version);
        printf("PLUGIN help: %s\n", plugin_info->help);
    }

    if (options.debug_events)
    {
        debug_events();
    }

    if (options.dump)
    {
        enable_dump_v1();
    }

    return 0;
}
