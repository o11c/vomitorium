#include "internal.hpp"


template<int i>
static void simple_event_printer(void *gcc_data, void *user_data);

void debug_events()
{
#if GCCPLUGIN_VERSION < 4009
# define is_pseudo_event(evt) ((evt) == PLUGIN_PASS_MANAGER_SETUP || (evt) == PLUGIN_INFO || (evt) == PLUGIN_REGISTER_GGC_ROOTS || (evt) == PLUGIN_REGISTER_GGC_CACHES)
#else
# define is_pseudo_event(evt) ((evt) == PLUGIN_PASS_MANAGER_SETUP || (evt) == PLUGIN_INFO || (evt) == PLUGIN_REGISTER_GGC_ROOTS)
#endif
#define DEFEVENT(evt)                                                                       \
        if (!is_pseudo_event(evt))                                                          \
            register_callback("vomitorium", evt, simple_event_printer<evt>, (void *)#evt);
#include <plugin.def>
#undef DEFEVENT
}

template<int i>
static void simple_event_printer(void *gcc_data, void *user_data)
{
    switch (i)
    {
#if GCCPLUGIN_VERSION >= 6000
    case PLUGIN_START_PARSE_FUNCTION:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_FINISH_PARSE_FUNCTION:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
#endif
    case PLUGIN_FINISH_TYPE:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
#if GCCPLUGIN_VERSION >= 4007
    case PLUGIN_FINISH_DECL:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
#endif
    case PLUGIN_FINISH_UNIT:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_PRE_GENERICIZE:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_FINISH:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_GGC_START:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_GGC_MARKING:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_GGC_END:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_ATTRIBUTES:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_START_UNIT:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_PRAGMAS:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_ALL_PASSES_START:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_ALL_PASSES_END:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_ALL_IPA_PASSES_START:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_ALL_IPA_PASSES_END:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_OVERRIDE_GATE:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_PASS_EXECUTION:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_EARLY_GIMPLE_PASSES_START:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_EARLY_GIMPLE_PASSES_END:
        printf("%s\n", (const char *)user_data);
        break;
    case PLUGIN_NEW_PASS:
        printf("%s: %p\n", (const char *)user_data, gcc_data);
        break;
#if GCCPLUGIN_VERSION >= 4009
    case PLUGIN_INCLUDE_FILE:
        printf("%s: %s\n", (const char *)user_data, (const char *)gcc_data);
        break;
#endif
    default:
        printf("? %s: %p\n", (const char *)user_data, gcc_data);
        break;
    case PLUGIN_PASS_MANAGER_SETUP:
    case PLUGIN_INFO:
    case PLUGIN_REGISTER_GGC_ROOTS:
#if GCCPLUGIN_VERSION < 4009
    case PLUGIN_REGISTER_GGC_CACHES:
#endif
        // these are not real events
        abort();
    }
}
