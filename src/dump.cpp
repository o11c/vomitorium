#include "internal.hpp"


// Usually stdout, but only initialized in plugin_init() to catch abusers.
FILE *vomitorium_output;

// Initialized by a constructor below.
static vomitorium_visitor dump_visitor;


static void do_dump(void *, void *visitor)
{
    vomitorium_visit_all((vomitorium_visitor *)visitor);
}

void enable_dump()
{
    register_callback("vomitorium", PLUGIN_PRE_GENERICIZE, do_dump, (void *)&dump_visitor);
}

static vomitorium_cookie dump_visit_tree(vomitorium_visitor *self, const char *name, tree t)
{
    (void)self;
    (void)name;
    (void)t;

    printf("visit_tree: %s %p\n", name, t);

    return nullptr;
}
static void dump_visit_again(vomitorium_visitor *self, const char *name, vomitorium_cookie t)
{
    (void)self;
    (void)name;
    (void)t;

    printf("visit_again: %s %p\n", name, t);
}
static void dump_visit_string8(vomitorium_visitor *self, const char *name, const uint8_t *str, size_t len)
{
    (void)self;
    (void)name;
    (void)str;
    (void)len;

    printf("visit_string8: %s, %p[%zu]\n", name, str, len);
}
static void dump_visit_string16(vomitorium_visitor *self, const char *name, const uint16_t *str, size_t len)
{
    (void)self;
    (void)name;
    (void)str;
    (void)len;

    printf("visit_string16: %s, %p[%zu]\n", name, str, len);
}
static void dump_visit_string32(vomitorium_visitor *self, const char *name, const uint32_t *str, size_t len)
{
    (void)self;
    (void)name;
    (void)str;
    (void)len;

    printf("visit_string32: %s, %p[%zu]\n", name, str, len);
}

__attribute__((constructor))
static void init_dump_visitor()
{
    vomitorium_visitor_init(&dump_visitor);
    dump_visitor.visit_tree = dump_visit_tree;
    dump_visitor.visit_again = dump_visit_again;
    dump_visitor.visit_string8 = dump_visit_string8;
    dump_visitor.visit_string16 = dump_visit_string16;
    dump_visitor.visit_string32 = dump_visit_string32;
}

