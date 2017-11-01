vomitorium (n): A passage located behind a tier of seats in an amphitheatre used as an exit for the crowds

This is a plugin for GCC that dumps trees into an external format. It is
applicable to all frontends and at all stages during compilation, but the
main focus is on C/C++ prioer to genericization.

Note that, due to the way the Runtime Exception works,
all code that manipulates this format must still be GPL3-compatible.
This is a feature, not a bug.

##Usage as a tool:

    CFLAGS += -fplugin=vomitorium -fplugin-arg-vomitorium-dump -fplugin-arg-vomitorium-output=./foo.xml

(if you have not installed it, pass -fplugin=/path/to/vomitorium.so instead)

##Usage as a library:

Compile the following as a shared library.
Link to vomitorium.so, probably with rpath.

// Note that this does not include GCC headers,
// this *your* plugin is version-independent, even if mine isn't.

    #include "vomitorium.h"

    int plugin_is_GPL_compatible;

    // it's okay for these types to be opaque
    int plugin_init (struct plugin_name_args *plugin_info,
                     struct plugin_gcc_version *version)
    {
        // call functions from vomitorium.h
        return 0;
    }

Then use (order is not significant):

    CFLAGS += -fplugin=vomitorium -fplugin=YOUR_SHARED_LIBRARY

Of course, nothing is stopping you from using parts of GCC's plugin API
yourself - for example, to call vomitorium from specific callbacks - but
that makes your plugin much hairier.

See demo.c for an example.
