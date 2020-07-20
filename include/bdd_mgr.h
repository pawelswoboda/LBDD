#pragma once

#include "bdd_node.h"
#include "bdd_node_cache.h"
#include "bdd_var.h"
#include "bdd_memo_cache.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace BDD {

    class bdd_mgr {
        public:
            bdd_mgr();
            ~bdd_mgr();
            size_t add_variable();
            size_t nr_variables() const { return vars.size(); }
            size_t nr_nodes() const { return node_cache_.nr_nodes(); }
            node_ref projection(const size_t var);
            node_ref neg_projection(const size_t var);
            node_ref negate(node_ref p);

            template<class... NODES>
                node_ref and_rec(node_ref p, NODES...);
            template<class ITERATOR>
                node_ref and_rec(ITERATOR nodes_begin, ITERATOR nodes_end);
            node_ref and_rec(node_ref f, node_ref g);

            template<class... NODES>
                node_ref or_rec(node_ref p, NODES...);
            template<class ITERATOR>
                node_ref or_rec(ITERATOR nodes_begin, ITERATOR nodes_end);
            node_ref or_rec(node_ref f, node_ref g);

            template<class... NODES>
                node_ref xor_rec(node_ref p, NODES... tail);
            template<class ITERATOR>
                node_ref xor_rec(ITERATOR nodes_begin, ITERATOR nodes_end);
            node_ref xor_rec(node_ref f, node_ref g);

            node_ref ite_rec(node_ref f, node_ref g, node_ref h);

            // make a copy of bdd rooted at node to variables given
            template<typename VAR_MAP>
            node_ref rebase(node_ref p, const VAR_MAP& var_map);

            // make private and add friend classes
            bdd_node_cache& get_node_cache() { return node_cache_; }
            unique_table_page_cache& get_unique_table_page_cache() { return page_cache_; }

            void collect_garbage();

        private:

            bdd_node_cache node_cache_;
            unique_table_page_cache page_cache_;
            memo_cache memo_;
            std::vector<var_struct> vars; // vars must be after node cache und page cache for correct destructor calling order

    }; 

    template<typename VAR_MAP>
    node_ref bdd_mgr::rebase(node_ref p, const VAR_MAP& var_map)
    {
        size_t last_var = 0;
        for(const auto [x,y] : var_map)
            last_var = std::max(y, last_var);
        for(size_t i=nr_variables(); i<=last_var; ++i)
            add_variable();

        // copy nodes one by one in postordering
        const auto postorder = p.address()->nodes_postorder();
        std::unordered_map<node*, node*> node_map;
        node_map.insert({node_cache_.botsink(), node_cache_.botsink()});
        node_map.insert({node_cache_.topsink(), node_cache_.topsink()});
        for(node* p : postorder) {
            const size_t v_orig = p->index;
            assert(var_map.count(v_orig) > 0);
            const size_t v_new = var_map.find(v_orig)->second;

            assert(node_map.count(p->lo) > 0);
            assert(node_map.count(p->hi) > 0);
            node* lo_mapped = node_map.find(p->lo)->second;
            node* hi_mapped = node_map.find(p->hi)->second;
            node_map.insert({p, vars[v_new].unique_find(lo_mapped, hi_mapped)});
        }

        return node_ref(node_map.find(p.address())->second);
    }

    //remplate<class... NODES, class = std::conjunction<std::is_same<node*, NODES>...>
    template<class... NODES>
        node_ref bdd_mgr::and_rec(node_ref p, NODES... tail)
        {
            node_ref and_tail(and_rec(tail...));
            return and_rec(p, and_tail); 
        }

    template<class ITERATOR>
        node_ref bdd_mgr::and_rec(ITERATOR nodes_begin, ITERATOR nodes_end)
        {
            const size_t n = std::distance(nodes_begin, nodes_end);
            assert(n >= 2);
            if(n == 2)
                return and_rec(*nodes_begin, *(nodes_begin+1));
            else if(n == 3)
                return and_rec(*nodes_begin, and_rec(*(nodes_begin+1), *(nodes_begin+2))); 

            node_ref a1(and_rec(nodes_begin, nodes_begin+n/2));
            node_ref a2(and_rec(nodes_begin+n/2, nodes_end));
            return and_rec(a1,a2);
        }

    template<class... NODES>
        node_ref bdd_mgr::or_rec(node_ref p, NODES... tail)
        {
            node_ref or_tail(or_rec(tail...));
            return or_rec(p, or_tail); 
        }
    template<class ITERATOR>
        node_ref bdd_mgr::or_rec(ITERATOR nodes_begin, ITERATOR nodes_end)
        {
            const size_t n = std::distance(nodes_begin, nodes_end);
            assert(n >= 2);
            if(n == 2)
                return or_rec(*nodes_begin, *(nodes_begin+1));
            else if(n == 3)
                return or_rec(*nodes_begin, or_rec(*(nodes_begin+1), *(nodes_begin+2))); 

            node_ref o1(or_rec(nodes_begin, nodes_begin+n/2));
            node_ref o2(or_rec(nodes_begin+n/2, nodes_end));
            return or_rec(o1,o2);
        }

    template<class... NODES>
        node_ref bdd_mgr::xor_rec(node_ref p, NODES... tail)
        {
            node_ref xor_tail(xor_rec(tail...));
            return xor_rec(p, xor_tail); 
        }
    template<class ITERATOR>
        node_ref bdd_mgr::xor_rec(ITERATOR nodes_begin, ITERATOR nodes_end)
        {
            const size_t n = std::distance(nodes_begin, nodes_end);
            assert(n >= 2);
            if(n == 2)
                return xor_rec(*nodes_begin, *(nodes_begin+1));
            else if(n == 3)
                return xor_rec(*nodes_begin, xor_rec(*(nodes_begin+1), *(nodes_begin+2))); 

            node_ref o1(xor_rec(nodes_begin, nodes_begin+n/2));
            node_ref o2(xor_rec(nodes_begin+n/2, nodes_end));
            return xor_rec(o1,o2);
        } 

    inline node_ref operator&(node_ref a, node_ref b)
    {
        bdd_mgr* mgr = a.find_bdd_mgr();
        return mgr->and_rec(a, b);

    }

    inline node_ref operator||(node_ref a, node_ref b)
    {
        bdd_mgr* mgr = a.find_bdd_mgr();
        return mgr->or_rec(a, b);

    }

    inline node_ref operator^(node_ref a, node_ref b)
    {
        bdd_mgr* mgr = a.find_bdd_mgr();
        return mgr->xor_rec(a, b); 
    }
}
