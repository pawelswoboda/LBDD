#pragma once

#include "bdd_node.h"
#include <unordered_map>

namespace BDD {

    // store depth information of bdd nodes here for deciding, whether recursive calls on stack are possible.
    // For each node address, store the depth of the tree rooted at node

    class bdd_node_depth {
        public:
            size_t add_node(node* p);
            void add_botsink(node* p);
            void add_topsink(node* p);

            // TODO: add garbage collection

        private:
            // TODO: use better hash table
            std::unordered_map<node*, size_t> depth;
    };
}
