#include "xml.hpp"


static void test_root_only()
{
    XmlOutput out(stdout, false);
    out.tag("root-only");
}

static void test_root_attr()
{
    XmlOutput out(stdout, false);
    auto root = out.tag("root-attr");
    out.attr("attr-empty");
}

static void test_root_content()
{
    XmlOutput out(stdout, false);
    auto root = out.tag("root-content");
    out.emit_string("content");
}

static void test_children()
{
    XmlOutput out(stdout, false);
    auto root = out.tag("root-children");

    out.tag("child1");
    out.tag("child2");
    out.tag("child3");
}

static void test_fancy()
{
    XmlOutput out(stdout, false);
    auto root = out.tag("root");
    {
        auto attr = out.attr("attr1");
        out.emit_string("val1");
    }
    out.attr("attr-empty");
    {
        auto attr = out.attr("attr2");
        out.emit_string("val2");
    }
    auto tag = out.tag("tag");
    {
        auto child = out.tag("child");
        out.emit_string("foo");
    }
    {
        auto child = out.tag("child");
        out.emit_string("bar");
    }
}


int main()
{
    test_root_only();
    puts("");
    test_root_attr();
    puts("");
    test_root_content();
    puts("");
    test_children();
    puts("");
    test_fancy();
}
