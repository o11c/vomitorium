#include "internal.hpp"

#include <cassert>


void vomitorium_visit(vomitorium_visitor *visitor, tree object)
{
    (void)visitor;
    (void)object;
    assert (!"NYI");
}

void vomitorium_visit_all(vomitorium_visitor *visitor)
{
    assert (!"NYI");
    (void)visitor;
}
