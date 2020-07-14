#include "bdd_node_depth.h"
#include <cassert>
#include <algorithm>

namespace BDD {

    size_t bdd_node_depth::add_node(node* p)
    {
        assert(!p->is_terminal());
        assert(depth.count(p->lo) > 0);
        assert(depth.count(p->hi) > 0);
        assert(depth.count(p) == 0);
        const size_t max_subtree_depth = std::max(depth.find(p->lo)->second, depth.find(p->hi)->second);
        depth.insert({p,max_subtree_depth + 1});
        return max_subtree_depth + 1;
    }

    void bdd_node_depth::add_botsink(node* p)
    {
        assert(p->is_botsink());
        assert(depth.count(p) == 0);
        depth.insert({p, 0});
    }

    void bdd_node_depth::add_topsink(node* p)
    {
        assert(p->is_topsink());
        assert(depth.count(p) == 0);
        depth.insert({p, 0});
    }

}
