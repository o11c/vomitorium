#include "intern.hpp"

#include "vgcc/tree.h"


std::vector<const_tree> interned_tree_list = {NULL_TREE};
std::map<const_tree, size_t> interned_tree_ids = {{NULL_TREE, 0}};

size_t intern(const_tree t)
{
    auto pair = interned_tree_ids.insert(std::make_pair(t, interned_tree_ids.size()));
    if (pair.second)
        interned_tree_list.push_back(t);
    return pair.first->second;
}
