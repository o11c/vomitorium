#pragma once

#include "vomitorium-fwd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#pragma GCC visibility push(default)

// Even though vomitorium uses C++ internally, the interface is C.
#ifdef __cplusplus
extern "C"
{
#endif

#define VOMITORIUM_SKIP ((vomitorium_cookie)-1)

    extern int plugin_is_GPL_compatible;
    extern FILE *vomitorium_output;


    // since frontends may be added, explicitly assign values
    enum vomitorium_frontend
    {
        VOMITORIUM_FRONTEND_NONE = '\0',    // prior to initialization
        VOMITORIUM_FRONTEND_UNKNOWN = '?',  // initialized, but unknown

        VOMITORIUM_FRONTEND_ADA = 'a',      // GNU Ada              gnat?   -> gnat1
        VOMITORIUM_FRONTEND_BRIG = 'b',     // GNU Brig             gccbrig -> brig1
        VOMITORIUM_FRONTEND_C = 'c',        // GNU C:               gcc     -> cc1
        VOMITORIUM_FRONTEND_CXX = 'C',      // GNU C++:             g++     -> cc1plus
        VOMITORIUM_FRONTEND_D = 'D',        // GNU D:               gdc     -> cc1d        ; what about the old D?
        VOMITORIUM_FRONTEND_FORTRAN = 'F',  // GNU Fortran:         gfortran-> f951        ; what about the old f77?
        VOMITORIUM_FRONTEND_GO = 'g',       // GNU Go:              gccgo   -> go1
        VOMITORIUM_FRONTEND_JAVA = 'j',     // GNU Java:            gcj     -> jc1         ; only until GCC 6
        VOMITORIUM_FRONTEND_JIT = 'J',      // ? libgccjit.so
        VOMITORIUM_FRONTEND_LTO = 'l',      // ? lto1?
        VOMITORIUM_FRONTEND_OBJC = 'o',     // GNU Objective-C:     N/A     -> cc1obj
        VOMITORIUM_FRONTEND_OBJCXX = 'O',   // N/A:                 N/A     -> cc1objplus
    };
    typedef enum vomitorium_frontend vomitorium_frontend;

    // TODO: actually implement this
    struct vomitorium_visitor
    {
        // The size of this struct that *you* were compiled against.
        size_t _size;

        // Arbitrary user data. You could also put some past the end of
        // the struct (C-style subclassing), vomitorium won't touch it.
        void *user_data;

        // Flags, if ever needed.
        struct
        {
            uint64_t _unused0 : 1;
            uint64_t _unused : 63;
        } flags;

        /// Visitor functions

        // Visit a tree. The returned cookie will be used next time.
        vomitorium_cookie (*visit_tree)(vomitorium_visitor *self, const char *name, tree t);
        // Visit a tree again. Pass the cookie returned from the first time.
        void (*visit_again)(vomitorium_visitor *self, const char *name, vomitorium_cookie t);
        // Raw string data, of any of 3 widths.
        // To distinguish wchar_t, see the tree that this was contained in.
        void (*visit_string8)(vomitorium_visitor *self, const char *name, const uint8_t *str, size_t len);
        void (*visit_string16)(vomitorium_visitor *self, const char *name, const uint16_t *str, size_t len);
        void (*visit_string32)(vomitorium_visitor *self, const char *name, const uint32_t *str, size_t len);
    };


#define VOMITORIUM_GET_FIELD(t, s, ptr, NAME, d) (offsetof(t, NAME) + sizeof((ptr)->NAME) <= s ? (ptr)->NAME : d)
#define VOMITORIUM_VISITOR_GET_FIELD(ptr, NAME) VOMITORIUM_GET_FIELD(vomitorium_visitor, (ptr)->_size, ptr, NAME, NULL)

    __attribute__((unused))
    static void vomitorium_visitor_init(vomitorium_visitor *visitor)
    {
        memset(visitor, 0, sizeof(*visitor));
        visitor->_size = sizeof(*visitor);
    }


    extern vomitorium_frontend vomitorium_current_frontend;
    vomitorium_frontend vomitorium_calc_frontend(void);

    void vomitorium_hello(void);

    void vomitorium_visit(vomitorium_visitor *visitor, tree object);
    void vomitorium_visit_all(vomitorium_visitor *visitor);


    int plugin_init (struct plugin_name_args *plugin_info,
                     struct plugin_gcc_version *version);

#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop
