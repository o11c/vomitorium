#include "vomitorium.h"


int plugin_is_GPL_compatible;

int plugin_init (struct plugin_name_args *plugin_info,
                 struct plugin_gcc_version *version)
{
    (void)plugin_info;
    (void)version;

    vomitorium_hello();
    return 0;
}
