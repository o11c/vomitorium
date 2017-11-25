#pragma once

#include "internal.hpp"

#include <map>
#include <vector>

#include "vgcc/coretypes.h"

// TODO genericize this.


// TODO: in order to support multiple dumps properly, this needs to be
// registered as a GC root. Possibly switch to VEC(tree, gc)?
// (the map is fine since it uses the same trees)
extern std::vector<const_tree> interned_tree_list;
extern std::map<const_tree, size_t> interned_tree_ids;

extern size_t intern(const_tree t);
